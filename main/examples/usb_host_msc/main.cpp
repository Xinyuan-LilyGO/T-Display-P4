/*
 * @Description: USB Host MSC flash drive test
 * @Author: LILYGO_L
 * @Date: 2026-05-20
 * @License: GPL 3.0
 */
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_check.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lilygo_device_driver_library.h"
#include "sdkconfig.h"
#include "usb/msc_host.h"
#include "usb/msc_host_vfs.h"
#include "usb/usb_host.h"

#ifndef USB_HOST_MSC_ENABLE_RW_TEST
#define USB_HOST_MSC_ENABLE_RW_TEST 1
#endif

namespace {

static const char* TAG = "usb_host_msc";

static constexpr char MSC_MOUNT_ROOT[] = "/usb";
static constexpr int MAX_MSC_DEVICES = CONFIG_FATFS_VOLUME_COUNT;
#if USB_HOST_MSC_ENABLE_RW_TEST
static constexpr size_t MSC_RW_TEST_FILE_SIZE = 4096;
static constexpr char MSC_RW_TEST_FILE_NAME[] = "usb_host_msc_test.txt";
#endif

struct MscDeviceEntry {
  bool mounted;
  uint8_t usb_addr;
  msc_host_device_handle_t msc_device;
  msc_host_vfs_handle_t vfs_handle;
#if USB_HOST_MSC_ENABLE_RW_TEST
  uint32_t rw_test_counter;
#endif
};

struct AppMessage {
  enum {
    DEVICE_CONNECTED,
    DEVICE_DISCONNECTED,
  } id;

  union {
    uint8_t new_dev_address;
    msc_host_device_handle_t device_handle;
  } data;
};

static QueueHandle_t s_app_queue = nullptr;
static MscDeviceEntry s_msc_devices[MAX_MSC_DEVICES] = {};

#if USB_HOST_MSC_ENABLE_RW_TEST
static uint8_t s_write_buffer[MSC_RW_TEST_FILE_SIZE] = {};
static uint8_t s_read_buffer[MSC_RW_TEST_FILE_SIZE] = {};
#endif

static bool usb_host_enum_filter_cb(
    const usb_device_desc_t* dev_desc, uint8_t* bConfigurationValue) {
  *bConfigurationValue = 1;
  ESP_LOGI(TAG, "USB device VID:%04X PID:%04X use config %u",
      dev_desc->idVendor, dev_desc->idProduct, *bConfigurationValue);
  return true;
}

static int find_free_msc_slot(void) {
  for (int i = 0; i < MAX_MSC_DEVICES; ++i) {
    if (!s_msc_devices[i].mounted) {
      return i;
    }
  }
  return -1;
}

static int find_msc_slot_by_handle(msc_host_device_handle_t handle) {
  for (int i = 0; i < MAX_MSC_DEVICES; ++i) {
    if (s_msc_devices[i].mounted && s_msc_devices[i].msc_device == handle) {
      return i;
    }
  }
  return -1;
}

static void make_mount_path(int slot, char* buffer, size_t buffer_size) {
  snprintf(buffer, buffer_size, "%s%d", MSC_MOUNT_ROOT, slot);
}

static void scan_msc_files(int slot) {
  char mount_path[16] = {};
  make_mount_path(slot, mount_path, sizeof(mount_path));

  DIR* dir = opendir(mount_path);
  if (dir == nullptr) {
    ESP_LOGW(TAG, "Open %s failed: errno=%d", mount_path, errno);
    return;
  }

  ESP_LOGI(TAG, "Listing %s", mount_path);
  int entry_count = 0;
  struct dirent* entry = nullptr;

  while ((entry = readdir(dir)) != nullptr) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char path[256] = {};
    if (snprintf(path, sizeof(path), "%s/%s", mount_path, entry->d_name) >=
        static_cast<int>(sizeof(path))) {
      ESP_LOGW(TAG, "Skip long path: %s/%s", mount_path, entry->d_name);
      continue;
    }

    struct stat st = {};
    if (stat(path, &st) != 0) {
      ESP_LOGW(TAG, "stat %s failed: errno=%d", path, errno);
      continue;
    }

    ++entry_count;
    if (S_ISDIR(st.st_mode)) {
      ESP_LOGI(TAG, "[DIR ] %s", path);
    } else {
      ESP_LOGI(TAG, "[FILE] %s (%lld bytes)", path,
          static_cast<long long>(st.st_size));
    }
  }

  closedir(dir);

  if (entry_count == 0) {
    ESP_LOGI(TAG, "%s is empty", mount_path);
  }
}

#if USB_HOST_MSC_ENABLE_RW_TEST
static void fill_text_test_buffer(
    uint8_t* buffer, size_t size, int slot, uint32_t counter) {
  if (size == 0) {
    return;
  }

  const int header_len = snprintf(reinterpret_cast<char*>(buffer), size,
      "usb_host_msc_test slot=%d counter=%" PRIu32 "\r\n", slot, counter);
  size_t start = header_len > 0 ? static_cast<size_t>(header_len) : 0;
  if (start > size) {
    start = size;
  }

  for (size_t i = start; i < size; ++i) {
    buffer[i] = static_cast<uint8_t>('A' + ((counter + slot + i) % 26));
  }

  if (size >= 2) {
    buffer[size - 2] = '\r';
    buffer[size - 1] = '\n';
  }
}

static FILE* open_rw_test_file_for_write(const char* path) {
  FILE* file = fopen(path, "r+b");
  if (file == nullptr && errno == ENOENT) {
    file = fopen(path, "w+b");
  }
  return file;
}

static esp_err_t close_file_checked(
    FILE** file, const char* path, const char* operation) {
  if (*file == nullptr) {
    return ESP_OK;
  }

  if (fclose(*file) != 0) {
    ESP_LOGW(TAG, "Close %s after %s failed: errno=%d", path, operation, errno);
    *file = nullptr;
    return ESP_FAIL;
  }

  *file = nullptr;
  return ESP_OK;
}

static esp_err_t run_msc_rw_test_once(int slot) {
  MscDeviceEntry* entry = &s_msc_devices[slot];
  if (!entry->mounted) {
    return ESP_ERR_INVALID_STATE;
  }

  char mount_path[16] = {};
  make_mount_path(slot, mount_path, sizeof(mount_path));

  char file_path[64] = {};
  snprintf(file_path, sizeof(file_path), "%s/%s", mount_path,
      MSC_RW_TEST_FILE_NAME);

  const uint32_t counter = ++entry->rw_test_counter;
  fill_text_test_buffer(s_write_buffer, MSC_RW_TEST_FILE_SIZE, slot, counter);
  memset(s_read_buffer, 0, MSC_RW_TEST_FILE_SIZE);

  esp_err_t ret = ESP_OK;
  FILE* file = open_rw_test_file_for_write(file_path);
  if (file == nullptr) {
    ESP_LOGW(TAG, "Open %s for write failed: errno=%d", file_path, errno);
    return ESP_FAIL;
  }

  if (fseek(file, 0, SEEK_SET) != 0) {
    ESP_LOGW(TAG, "Seek %s for write failed: errno=%d", file_path, errno);
    ret = ESP_FAIL;
    close_file_checked(&file, file_path, "seek-write");
    goto cleanup;
  }

  if (fwrite(s_write_buffer, 1, MSC_RW_TEST_FILE_SIZE, file) !=
      MSC_RW_TEST_FILE_SIZE) {
    ESP_LOGW(TAG, "Write %s failed: errno=%d", file_path, errno);
    ret = ESP_FAIL;
    close_file_checked(&file, file_path, "write");
    goto cleanup;
  }

  if (fflush(file) != 0) {
    ESP_LOGW(TAG, "Flush %s failed: errno=%d", file_path, errno);
    ret = ESP_FAIL;
    close_file_checked(&file, file_path, "flush");
    goto cleanup;
  }

  ret = close_file_checked(&file, file_path, "write");
  if (ret != ESP_OK) {
    goto cleanup;
  }

  file = fopen(file_path, "rb");
  if (file == nullptr) {
    ESP_LOGW(TAG, "Open %s for read failed: errno=%d", file_path, errno);
    ret = ESP_FAIL;
    goto cleanup;
  }

  if (fread(s_read_buffer, 1, MSC_RW_TEST_FILE_SIZE, file) !=
      MSC_RW_TEST_FILE_SIZE) {
    ESP_LOGW(TAG, "Read %s failed: errno=%d", file_path, errno);
    ret = ESP_FAIL;
    close_file_checked(&file, file_path, "read");
    goto cleanup;
  }

  ret = close_file_checked(&file, file_path, "read");
  if (ret != ESP_OK) {
    goto cleanup;
  }

  if (memcmp(s_write_buffer, s_read_buffer, MSC_RW_TEST_FILE_SIZE) != 0) {
    ESP_LOGE(TAG, "MSC RW verify failed: %s", file_path);
    ret = ESP_FAIL;
    goto cleanup;
  }

  ESP_LOGI(TAG, "MSC RW verify OK: %s (%u bytes)", file_path,
      static_cast<unsigned>(MSC_RW_TEST_FILE_SIZE));

cleanup:
  if (file != nullptr) {
    close_file_checked(&file, file_path, "cleanup");
  }
  return ret;
}

static void run_msc_rw_tests(void) {
  for (int slot = 0; slot < MAX_MSC_DEVICES; ++slot) {
    if (!s_msc_devices[slot].mounted) {
      continue;
    }

    const esp_err_t ret = run_msc_rw_test_once(slot);
    if (ret != ESP_OK) {
      ESP_LOGW(TAG, "MSC RW test slot %d failed: %s", slot, esp_err_to_name(ret));
    }
  }
}
#endif

static esp_err_t allocate_new_msc_device(uint8_t usb_addr) {
  const int slot = find_free_msc_slot();
  if (slot < 0) {
    ESP_LOGW(TAG, "No free MSC slots, max=%d", MAX_MSC_DEVICES);
    return ESP_ERR_NOT_FOUND;
  }

  MscDeviceEntry* entry = &s_msc_devices[slot];
  *entry = {};

  esp_err_t ret = msc_host_install_device(usb_addr, &entry->msc_device);
  if (ret != ESP_OK) {
    return ret;
  }

  entry->usb_addr = usb_addr;

  const esp_vfs_fat_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 8192,
      .disk_status_check_enable = false,
      .use_one_fat = false,
  };

  char mount_path[16] = {};
  make_mount_path(slot, mount_path, sizeof(mount_path));

  ret = msc_host_vfs_register(
      entry->msc_device, mount_path, &mount_config, &entry->vfs_handle);
  if (ret != ESP_OK) {
    const esp_err_t mount_ret = ret;
    ESP_LOGE(TAG, "msc_host_vfs_register %s failed: %s", mount_path,
        esp_err_to_name(ret));
    const esp_err_t uninstall_ret = msc_host_uninstall_device(entry->msc_device);
    if (uninstall_ret != ESP_OK) {
      ESP_LOGW(TAG, "msc_host_uninstall_device after mount failure failed: %s",
          esp_err_to_name(uninstall_ret));
    }
    *entry = {};
    return mount_ret;
  }

  entry->mounted = true;

  msc_host_device_info_t info = {};
  ret = msc_host_get_device_info(entry->msc_device, &info);
  if (ret == ESP_OK) {
    const uint64_t capacity_mb =
        (static_cast<uint64_t>(info.sector_size) * info.sector_count) /
        (1024 * 1024);
    ESP_LOGI(TAG,
        "MSC mounted at %s: VID=0x%04X PID=0x%04X capacity=%" PRIu64
        "MB sector=%" PRIu32 " count=%" PRIu32,
        mount_path, info.idVendor, info.idProduct, capacity_mb,
        info.sector_size, info.sector_count);
  }

  scan_msc_files(slot);
  return ESP_OK;
}

static void free_msc_device(int slot) {
  if (slot < 0 || slot >= MAX_MSC_DEVICES || !s_msc_devices[slot].mounted) {
    return;
  }

  MscDeviceEntry* entry = &s_msc_devices[slot];

  if (entry->vfs_handle != nullptr) {
    const esp_err_t ret = msc_host_vfs_unregister(entry->vfs_handle);
    if (ret != ESP_OK) {
      ESP_LOGW(TAG, "msc_host_vfs_unregister slot %d failed: %s", slot,
          esp_err_to_name(ret));
    }
  }
  if (entry->msc_device != nullptr) {
    const esp_err_t ret = msc_host_uninstall_device(entry->msc_device);
    if (ret != ESP_OK) {
      ESP_LOGW(TAG, "msc_host_uninstall_device slot %d failed: %s", slot,
          esp_err_to_name(ret));
    }
  }

  *entry = {};
  ESP_LOGI(TAG, "MSC slot %d unmounted", slot);
}

static void msc_event_cb(const msc_host_event_t* event, void* arg) {
  if (s_app_queue == nullptr) {
    return;
  }

  AppMessage message = {};
  if (event->event == msc_host_event_t::MSC_DEVICE_CONNECTED) {
    ESP_LOGI(TAG, "MSC device connected, usb_addr=%u", event->device.address);
    message.id = AppMessage::DEVICE_CONNECTED;
    message.data.new_dev_address = event->device.address;
    xQueueSend(s_app_queue, &message, portMAX_DELAY);
  } else if (event->event == msc_host_event_t::MSC_DEVICE_DISCONNECTED) {
    ESP_LOGI(TAG, "MSC device disconnected");
    message.id = AppMessage::DEVICE_DISCONNECTED;
    message.data.device_handle = event->device.handle;
    xQueueSend(s_app_queue, &message, portMAX_DELAY);
  } else {
    ESP_LOGW(TAG, "Unsupported MSC event: %d", event->event);
  }
}

static void msc_app_task(void* arg) {
  while (true) {
    AppMessage msg = {};
    const TickType_t wait_ticks =
#if USB_HOST_MSC_ENABLE_RW_TEST
        pdMS_TO_TICKS(1000);
#else
        portMAX_DELAY;
#endif

    if (xQueueReceive(s_app_queue, &msg, wait_ticks) == pdTRUE) {
      if (msg.id == AppMessage::DEVICE_CONNECTED) {
        esp_err_t ret = allocate_new_msc_device(msg.data.new_dev_address);
        if (ret != ESP_OK) {
          ESP_LOGE(TAG, "Install MSC device failed: %s", esp_err_to_name(ret));
        }
      } else if (msg.id == AppMessage::DEVICE_DISCONNECTED) {
        const int slot = find_msc_slot_by_handle(msg.data.device_handle);
        free_msc_device(slot);
      }
    }

#if USB_HOST_MSC_ENABLE_RW_TEST
    run_msc_rw_tests();
#endif
  }
}

static void usb_lib_task(void* arg) {
  const usb_host_config_t host_config = {
      .skip_phy_setup = false,
      .root_port_unpowered = false,
      .intr_flags = ESP_INTR_FLAG_LEVEL1,
      .enum_filter_cb = usb_host_enum_filter_cb,
      .fifo_settings_custom = {},
      .peripheral_map = 0,
  };
  ESP_ERROR_CHECK(usb_host_install(&host_config));

  const msc_host_driver_config_t msc_config = {
      .create_backround_task = true,
      .task_priority = 5,
      .stack_size = 4096,
      .core_id = 0,
      .callback = msc_event_cb,
      .callback_arg = nullptr,
  };
  ESP_ERROR_CHECK(msc_host_install(&msc_config));

  xTaskNotifyGive(static_cast<TaskHandle_t>(arg));

  bool has_clients = true;
  bool has_devices = false;
  while (has_clients) {
    uint32_t event_flags = 0;
    ESP_ERROR_CHECK(usb_host_lib_handle_events(portMAX_DELAY, &event_flags));

    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
      ESP_LOGI(TAG, "USB host has no clients");
      if (usb_host_device_free_all() == ESP_OK) {
        ESP_LOGI(TAG, "All USB devices are free");
        has_clients = false;
      } else {
        has_devices = true;
      }
    }

    if (has_devices && (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE)) {
      ESP_LOGI(TAG, "All USB devices are free");
      has_clients = false;
    }
  }

  ESP_LOGI(TAG, "Uninstall USB Host library");
  vTaskDelay(pdMS_TO_TICKS(100));
  msc_host_uninstall();
  usb_host_uninstall();
  vTaskDelete(nullptr);
}

}  // namespace

extern "C" void app_main(void) {
  printf("Ciallo\n");

  lilygo_device_driver::TDisplayP4Driver::GetInstance().Init();

#if USB_HOST_MSC_ENABLE_RW_TEST
  ESP_LOGI(TAG, "MSC RW test enabled: %u bytes every 1000 ms",
      static_cast<unsigned>(MSC_RW_TEST_FILE_SIZE));
#else
  ESP_LOGI(TAG, "MSC RW test disabled");
#endif

  s_app_queue = xQueueCreate(8, sizeof(AppMessage));
  ESP_ERROR_CHECK(s_app_queue != nullptr ? ESP_OK : ESP_ERR_NO_MEM);

  BaseType_t task_created =
      xTaskCreate(msc_app_task, "msc_app", 4096, nullptr, 4, nullptr);
  ESP_ERROR_CHECK(task_created == pdPASS ? ESP_OK : ESP_FAIL);

  task_created = xTaskCreatePinnedToCore(usb_lib_task, "usb_lib", 4096,
      xTaskGetCurrentTaskHandle(), 5, nullptr, 0);
  ESP_ERROR_CHECK(task_created == pdPASS ? ESP_OK : ESP_FAIL);

  uint32_t notify_value = ulTaskNotifyTake(pdFALSE, pdMS_TO_TICKS(1000));
  ESP_ERROR_CHECK(notify_value > 0 ? ESP_OK : ESP_ERR_TIMEOUT);

  ESP_LOGI(TAG, "Connect a USB flash drive");
  vTaskSuspend(nullptr);
}

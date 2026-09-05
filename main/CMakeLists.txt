#这里选择你需要编译的项目
if(CONFIG_EXAMPLE_BUILD_ES8311_SD_WAV)
    set(project_path "examples/es8311_sd_wav/")
elseif(CONFIG_EXAMPLE_BUILD_ESP32C6_AT_HOST_SDIO_UART)
    set(project_path "examples/esp32c6_at_host_sdio_uart/")
elseif(CONFIG_EXAMPLE_BUILD_ESP32C6_AT_HOST_SDIO_WIFI)
    set(project_path "examples/esp32c6_at_host_sdio_wifi/")
elseif(CONFIG_EXAMPLE_BUILD_SX1262_GFSK_SEND_RECEIVE)
    set(project_path "examples/sx1262_gfsk_send_receive/")
elseif(CONFIG_EXAMPLE_BUILD_SX1262_LORA_SEND_RECEIVE)
    set(project_path "examples/sx1262_lora_send_receive/")
elseif(CONFIG_EXAMPLE_BUILD_SX1262_TX_CONTINUOUS_WAVE)
    set(project_path "examples/sx1262_tx_continuous_wave/")
elseif(CONFIG_EXAMPLE_BUILD_AFE)
    set(project_path "examples/afe/")
elseif(CONFIG_EXAMPLE_BUILD_RADIOLIB_SX1262_SEND_RECEIVE)
    set(project_path "examples/radiolib_sx1262_send_receive/")
elseif(CONFIG_EXAMPLE_BUILD_CC1101_SEND_RECEIVE)
    set(project_path "keyboard_examples/radiolib_cc1101_send_receive/")
elseif(CONFIG_EXAMPLE_BUILD_RADIOLIB_CC1101_SEND_RECEIVE)
    set(project_path "keyboard_examples/radiolib_cc1101_send_receive/")
elseif(CONFIG_EXAMPLE_BUILD_RADIOLIB_NRF24L01_SEND_RECEIVE)
    set(project_path "keyboard_examples/radiolib_nrf24l01_send_receive/")
elseif(CONFIG_EXAMPLE_BUILD_NFC_RFAL_ST25R3916)
    set(project_path "keyboard_examples/nfc-rfal_st25r3916/")
elseif(CONFIG_EXAMPLE_BUILD_SCREEN_TCA8418_LVGL_TOUCH_DRAW)
    set(project_path "keyboard_examples/screen_tca8418_lvgl_touch_draw/")
elseif(CONFIG_EXAMPLE_BUILD_ETHERNET_IPERF)
    set(project_path "examples/ethernet_iperf/")
elseif(CONFIG_EXAMPLE_BUILD_USB_ETHERNET_IPERF)
    set(project_path "examples/usb_ethernet_iperf/")
elseif(CONFIG_EXAMPLE_BUILD_USB_ETHERNET_IPERF_HOST_MSC)
    set(project_path "examples/usb_ethernet_iperf_host_msc/")
elseif(CONFIG_EXAMPLE_BUILD_USB_HOST_MSC)
    set(project_path "examples/usb_host_msc/")
endif()

idf_component_register(SRC_DIRS ${project_path} 
                        INCLUDE_DIRS ${project_path})

if(CONFIG_EXAMPLE_BUILD_NFC_RFAL_ST25R3916)
    idf_component_get_property(arduino_rfal_dir NFC-RFAL COMPONENT_DIR)
    idf_component_get_property(arduino_st25r3916_dir ST25R3916 COMPONENT_DIR)
    idf_component_get_property(native_rfal_dir stsw-st25rfal002_cpp_bus_driver COMPONENT_DIR)

    # The Arduino and native RFAL headers share names but define different APIs.
    set_property(SOURCE "keyboard_examples/nfc-rfal_st25r3916/st25r3916_driver.cpp"
        APPEND PROPERTY INCLUDE_DIRECTORIES
        "${arduino_rfal_dir}/src"
        "${arduino_st25r3916_dir}/src")
    set_property(SOURCE "keyboard_examples/nfc-rfal_st25r3916/main.cpp"
        APPEND PROPERTY INCLUDE_DIRECTORIES
        "${native_rfal_dir}/src"
        "${native_rfal_dir}/src/platform"
        "${native_rfal_dir}/third_party/STSW-ST25RFAL002/include"
        "${native_rfal_dir}/third_party/STSW-ST25RFAL002/source"
        "${native_rfal_dir}/third_party/STSW-ST25RFAL002/source/st25r3916")
endif()

#这里选择你需要编译的项目
set(project_path 
    # "examples/aw86224/"
    # "examples/bq27220/"
    # "examples/deep_sleep/"
    # "examples/es8311/"
    # "examples/es8311_sd_wav/"
    # "examples/esp32c6_at_host_sdio_uart/"
    # "examples/esp32c6_at_host_sdio_wifi/"
    # "examples/hi8561_camera/"
    # "examples/hi8561_lvgl/"
    # "examples/hi8561_lvgl_touch_draw/"
    # "examples/icm20948/"
    # "examples/iic_scan/"
    # "examples/l76k/"
    # "examples/lvgl_9_ui/"
    # "examples/pcf8563/"
    # "examples/sgm38121/"
    # "examples/sx1262_gfsk_send_receive/"
    # "examples/sx1262_lora_send_receive/"
    # "examples/sx1262_tx_continuous_wave/"
    "examples/tusb_serial_device/"
    )
        
idf_component_register(SRC_DIRS ${project_path} 
                        INCLUDE_DIRS ${project_path})

target_compile_options(
    ${COMPONENT_LIB}
    PRIVATE -Wno-error=unused-const-variable -Wno-error=delete-non-virtual-dtor
            -Wno-error=implicit-function-declaration -fpermissive)
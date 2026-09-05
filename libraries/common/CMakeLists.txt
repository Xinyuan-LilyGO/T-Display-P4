if(CONFIG_EXAMPLE_BUILD_SCREEN_TCA8418_LVGL_TOUCH_DRAW)
    set(src_dirs "./")
endif()

if(CONFIG_EXAMPLE_BUILD_ESP32C6_AT_HOST_SDIO_UART OR
   CONFIG_EXAMPLE_BUILD_ESP32C6_AT_HOST_SDIO_WIFI)
    set(requires "lilygo_device_driver")
endif()

idf_component_register(
        SRC_DIRS
        ${src_dirs}
        INCLUDE_DIRS
        "."
        REQUIRES
        ${requires}
    )

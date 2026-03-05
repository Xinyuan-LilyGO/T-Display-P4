if(CONFIG_EXAMPLE_BUILD_RADIOLIB_LR1121_SEND_RECEIVE)
    set(src_dirs "./radiolib_bridge_driver")
    set(requires "cpp_bus_driver" "RadioLib")
elseif(CONFIG_EXAMPLE_BUILD_ES8311)
    set(src_dirs "./")
elseif(CONFIG_EXAMPLE_BUILD_SCREEN_TCA8418_LVGL_TOUCH_DRAW)
    set(src_dirs "./")
endif()

idf_component_register(
        SRC_DIRS 
        ${src_dirs} 
        INCLUDE_DIRS 
        ${src_dirs}
        REQUIRES
        ${requires}
    )
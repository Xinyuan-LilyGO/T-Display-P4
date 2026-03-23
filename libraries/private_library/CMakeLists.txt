if(CONFIG_EXAMPLE_BUILD_ES8311)
    set(src_dirs "./")
elseif(CONFIG_EXAMPLE_BUILD_SCREEN_TCA8418_LVGL_TOUCH_DRAW)
    set(src_dirs "./")
elseif(CONFIG_EXAMPLE_BUILD_LVGL_9_UI)
    set(src_dirs "./")
    set(requires "lvgl")
endif()

idf_component_register(
        SRC_DIRS 
        ${src_dirs} 
        INCLUDE_DIRS 
        ${src_dirs}
        REQUIRES
        ${requires}
    )
if(CONFIG_EXAMPLE_BUILD_SCREEN_TCA8418_LVGL_TOUCH_DRAW)
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

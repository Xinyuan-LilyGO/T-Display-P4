set(src_dirs ./)
set(include_dirs ./)
idf_component_register(SRC_DIRS ${src_dirs}  
                        INCLUDE_DIRS ${include_dirs}
                        # PRIV_REQUIRES fatfs esp_netif esp_eth
                        )

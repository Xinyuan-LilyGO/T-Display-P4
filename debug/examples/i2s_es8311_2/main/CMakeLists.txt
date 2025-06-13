set(src_dirs ./)
set(inc_dirs ./)
idf_component_register(SRC_DIRS ${src_dirs} 
                        INCLUDE_DIRS ${inc_dirs}
                        EMBED_FILES "canon.pcm")

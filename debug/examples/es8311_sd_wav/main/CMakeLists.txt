set(src_dirs ./)
set(inc_dirs ./)
idf_component_register(SRC_DIRS ${src_dirs} 
                        INCLUDE_DIRS ${inc_dirs})

target_compile_options(
  ${COMPONENT_LIB}
  PRIVATE -Wno-error=unused-const-variable -Wno-error=delete-non-virtual-dtor
          -Wno-error=implicit-function-declaration -fpermissive)
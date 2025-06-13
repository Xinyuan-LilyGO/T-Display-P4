set(
        src_dirs 
        ./src 

        ./src/bus
        ./src/bus/iic
        ./src/bus/spi
        ./src/bus/uart

        ./src/chip
        ./src/chip/iic
        ./src/chip/spi
        ./src/chip/uart

        ./src/resources
    )
    
idf_component_register(
        SRC_DIRS 
        ${src_dirs} 
        INCLUDE_DIRS 
        ${src_dirs} 
        REQUIRES
        driver
    )
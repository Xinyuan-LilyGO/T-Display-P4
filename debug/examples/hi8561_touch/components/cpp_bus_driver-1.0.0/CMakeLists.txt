set(
        src_dirs 
        ./src 

        ./src/bus
        ./src/bus/iic
        ./src/bus/spi

        ./src/chip
        ./src/chip/iic
        ./src/chip/spi

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
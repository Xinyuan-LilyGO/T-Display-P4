set(
        src_dirs 
        ./src 

        ./src/bus
        ./src/bus/iic
        ./src/bus/spi
        ./src/bus/uart
        ./src/bus/iis
        ./src/bus/sdio

        ./src/chip
        ./src/chip/iic
        ./src/chip/spi
        ./src/chip/uart
        ./src/chip/iic_iis
        ./src/chip/sdio

        ./src/resources
    )
    
idf_component_register(
        SRC_DIRS 
        ${src_dirs} 
        INCLUDE_DIRS 
        ${src_dirs} 
        REQUIRES
        "driver"
    )
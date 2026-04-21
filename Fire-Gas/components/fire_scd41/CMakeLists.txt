idf_component_register(
    SRCS "SCD41.c"
    INCLUDE_DIRS "include"
    REQUIRES fire_common driver fire_gpio
)
idf_component_register(
    SRCS "mq5.c"
    INCLUDE_DIRS "include"
    REQUIRES fire_gpio fire_common freertos fire_ads1115
)
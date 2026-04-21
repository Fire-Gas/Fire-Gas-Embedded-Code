idf_component_register(
    SRCS "led.c"
    INCLUDE_DIRS "include"
    REQUIRES fire_gpio fire_common driver
)
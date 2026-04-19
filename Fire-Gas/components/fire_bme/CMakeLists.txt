idf_component_register(
    SRCS "bme.c"
    INCLUDE_DIRS "include"
    REQUIRES fire_gpio driver freertos fire_common
)
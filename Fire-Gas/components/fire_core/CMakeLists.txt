idf_component_register(
    SRCS "core.c"
    INCLUDE_DIRS "include"
    REQUIRES fire_gpio driver freertos fire_mics fire_bme fire_scd41 fire_mq5 fire_ai
)
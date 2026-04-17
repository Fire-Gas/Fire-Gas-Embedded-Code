idf_component_register(
    SRCS "core.c"
    INCLUDE_DIRS "include"
    REQUIRES fire_gpio driver freertos
)
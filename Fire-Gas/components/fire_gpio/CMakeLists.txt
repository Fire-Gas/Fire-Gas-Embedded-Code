idf_component_register(
    SRCS "gpio.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_adc fire_common fire_scd41
)
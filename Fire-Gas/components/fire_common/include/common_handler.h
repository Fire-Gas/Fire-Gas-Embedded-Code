#ifndef COMMON_GPIO_PIN
#define COMMON_GPIO_PIN

#include "esp_adc/adc_oneshot.h"

#define BME680_I2C_ADDR 0x77   

extern adc_oneshot_unit_handle_t adc1_handle;
extern adc_cali_handle_t adc1_cali_handle;

#endif
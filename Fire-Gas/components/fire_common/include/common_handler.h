#ifndef COMMON_HANDLER
#define COMMON_HANDLER

#include "esp_adc/adc_oneshot.h"

#ifndef BME680_I2C_ADDr
#define BME680_I2C_ADDR 0x77   
#endif 

extern adc_oneshot_unit_handle_t adc1_handle;
extern adc_cali_handle_t adc1_cali_handle;

#endif
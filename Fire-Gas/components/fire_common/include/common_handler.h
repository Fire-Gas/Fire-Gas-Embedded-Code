<<<<<<< Updated upstream
#ifndef COMMON_HANDLER
#define COMMON_HANDLER

#include "esp_adc/adc_oneshot.h"

#define BME680_I2C_ADDR 0x77   

extern adc_oneshot_unit_handle_t adc1_handle;
extern adc_cali_handle_t adc1_cali_handle;

=======
#ifndef COMMON_GPIO_PIN
#define COMMON_GPIO_PIN

#include "esp_adc/adc_oneshot.h"
#include "driver/i2c_master.h"

// adc 관련 handle
extern adc_oneshot_unit_handle_t adc1_handle;
extern adc_cali_handle_t adc1_cali_handle;

// SCD41 측정 값 구조체
typedef struct {
    uint16_t co2;
    float temperature;
    float humidity;
} scd41_data_t;

extern scd41_data_t g_scd41_info;

>>>>>>> Stashed changes
#endif
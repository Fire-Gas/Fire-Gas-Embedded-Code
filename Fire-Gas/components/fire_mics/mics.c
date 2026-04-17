#include "mics.h"
#include "common_handler.h"
#include "pin.h"

#include <stdint.h>

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h" 
#include "freertos/task.h"

static const char* TAG = "MICS";

void mics_sensor_get_value(void* pvParameters) {
    int co_raw, nh3_raw, no2_raw;

    while(1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, CO_CHANNEL, &co_raw));
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, NH3_CHANNEL, &nh3_raw));
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, NO2_CHANNEL, &no2_raw));

        ESP_LOGI(TAG, "[MICS-6814] CO: %4d | NH3: %4d | NO2: %4d", co_raw, nh3_raw, no2_raw);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
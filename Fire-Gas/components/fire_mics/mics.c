#include "mics.h"
#include "common_handler.h"
#include "common_struct.h"
#include "pin.h"

#include <stdint.h>

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h" 
#include "freertos/task.h"
#include "freertos/queue.h"

static const char* TAG = "MICS";

#include "mics.h"
#include "driver/gpio.h"
#include "esp_log.h"

float base_volt_co = 0;
float base_volt_nh3 = 0;
float base_volt_no2 = 0;

esp_err_t mics_init(void) {
    if (adc1_handle == NULL) {
        ESP_LOGE(TAG, "ADC1이 설정되지 않았습니다. gpio_init을 먼저 호출하세요.");
        return ESP_FAIL;
    }

    /* 
    MICS의 경우 화학 반응을 감지하기에, 히터 예열 시간 필요
    실제로 30s ~ 60s 정도 필요
    */
    vTaskDelay(pdMS_TO_TICKS(5000));

    int raw;
    int mv;
    long sum_co = 0, sum_nh3 = 0, sum_no2 = 0;
    int samples = 20;

    for (int i = 0; i < samples; i++) {
        adc_oneshot_read(adc1_handle, CO_CHANNEL, &raw);
        adc_cali_raw_to_voltage(adc1_cali_handle, raw, &mv);
        sum_co += mv;

        adc_oneshot_read(adc1_handle, NH3_CHANNEL, &raw);
        adc_cali_raw_to_voltage(adc1_cali_handle, raw, &mv);
        sum_nh3 += mv;

        adc_oneshot_read(adc1_handle, NO2_CHANNEL, &raw);
        adc_cali_raw_to_voltage(adc1_cali_handle, raw, &mv);
        sum_no2 += mv;
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    base_volt_co = (float)sum_co / samples;
    base_volt_nh3 = (float)sum_nh3 / samples;
    base_volt_no2 = (float)sum_no2 / samples;

    ESP_LOGI(TAG, "영점 조절 완료 - CO: %.2fmV, NH3: %.2fmV, NO2: %.2fmV", 
             base_volt_co, base_volt_nh3, base_volt_no2);

    return ESP_OK;
}

void mics_sensor_get_value(void* pvParameters) {
    QueueHandle_t mics_queue_handler = (QueueHandle_t)pvParameters;
    mics_data_t data;

    adc_channel_t channels[3] = {CO_CHANNEL, NH3_CHANNEL, NO2_CHANNEL};
    int raw_vals[3];
    int mv_vals[3];

    ESP_ERROR_CHECK(mics_init());
    while(1) {
        for (int i = 0; i < 3; i++) {
            adc_oneshot_read(adc1_handle, channels[i], &raw_vals[i]);
            
            if (adc1_cali_handle) {
                adc_cali_raw_to_voltage(adc1_cali_handle, raw_vals[i], &mv_vals[i]);
            } else {
                mv_vals[i] = (raw_vals[i] * 3300) / 4095;
            }
        }

        data.co = mv_vals[0];
        data.nh = mv_vals[1];
        data.no = mv_vals[2];

        if (xQueueSend(mics_queue_handler, &data, pdMS_TO_TICKS(100)) != pdPASS) {
            ESP_LOGE(TAG, "mics data전송 실패, 재전송 시도");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
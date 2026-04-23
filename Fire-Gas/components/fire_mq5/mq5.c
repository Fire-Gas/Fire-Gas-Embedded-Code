#include "mq5.h"
#include "pin.h"
#include "ads1115.h"
#include "common_struct.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MQ5";

static esp_err_t mq5_init(void) {
    esp_err_t ret = ads1115_init(); 
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MQ5용 ADS1115 초기화 실패!");
        return ret;
    }

    ESP_LOGI(TAG, "MQ-5 하드웨어 준비 완료");
    return ESP_OK;
}

void mq5_sensor_task(void* pvParameters) {
    QueueHandle_t mq5_queue = (QueueHandle_t)pvParameters;
    mq5_data_t sensor_data;

    esp_err_t init_ret = mq5_init();
    if (init_ret != ESP_OK) {
        ESP_LOGE(TAG, "MQ-5 하드웨어 초기화 실패 (에러 코드: %s)", esp_err_to_name(init_ret));
        // 초기화 실패 시 태스크 종료 또는 재시도 로직
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "MQ-5 예열 시작 (20초)");
    vTaskDelay(pdMS_TO_TICKS(20000));

    while (1) {
        int16_t raw_val = 0;
        esp_err_t ret = ads1115_read_raw(ADS1115_CH_3, &raw_val);

        // 전압 값을 기반으로 농도 계산
        if (ret == ESP_OK) {
            float voltage_mv = ads1115_raw_to_voltage(raw_val);
            
            sensor_data.voltage_mv = (uint32_t)voltage_mv;
            sensor_data.gas_detected = (voltage_mv > WARNING_VALUE) ? 1 : 0;

            if (xQueueSend(mq5_queue, &sensor_data, pdMS_TO_TICKS(10)) != pdPASS) {
                ESP_LOGW(TAG, "MQ5 Queue 꽉 참");
            }
        }
        else {
            ESP_LOGE(TAG, "ADS1115 통신 오류 발생!");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
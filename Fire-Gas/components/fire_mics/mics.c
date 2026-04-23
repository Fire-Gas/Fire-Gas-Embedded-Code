#include "mics.h"
#include "ads1115.h"
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

void mics_sensor_get_value(void* pvParameters) {
    QueueHandle_t mics_queue_handler = (QueueHandle_t)pvParameters;
    mics_data_t data;

    if (mics_queue_handler == NULL) {
        ESP_LOGE(TAG, "Queue가 비어있습니다! 태스크를 종료합니다.");
        vTaskDelete(NULL);
        return;
    }

    if (ads1115_init() != ESP_OK) {
        ESP_LOGE(TAG, "ADS1115 초기화 실패! MiCS 수집을 시작할 수 없습니다.");
        vTaskDelete(NULL);
        return;
    }

    while(1) {
        int16_t co_raw = 0, nh3_raw = 0, no2_raw = 0;
        esp_err_t ret = ESP_OK;

        ret |= ads1115_read_raw(ADS1115_CH_4, &co_raw);
        ret |= ads1115_read_raw(ADS1115_CH_5, &nh3_raw);
        ret |= ads1115_read_raw(ADS1115_CH_6, &no2_raw);

        // 보정된 전압(mV)으로 변환
        if (ret == ESP_OK) {
            data.co = (uint32_t)ads1115_raw_to_voltage(co_raw);
            data.nh = (uint32_t)ads1115_raw_to_voltage(nh3_raw);
            data.no = (uint32_t)ads1115_raw_to_voltage(no2_raw);

            if (xQueueSend(mics_queue_handler, &data, pdMS_TO_TICKS(100)) != pdPASS) {
                ESP_LOGE(TAG, "MiCS 데이터 전송 실패");
            }
        } 
        else {
            ESP_LOGE(TAG, "ADS1115 통신 오류 발생");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
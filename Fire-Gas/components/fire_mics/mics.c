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

#include "mics.h"
#include "driver/gpio.h"
#include "esp_log.h"

adc_channel_t channels[3] = {CO_CHANNEL, NH3_CHANNEL, NO2_CHANNEL};

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
   
    int32_t raw;
    int32_t mv;
    uint8_t samples = 20; // 20번의 샘플링
    uint32_t values[3] = {0};

    if (adc1_cali_handle) {
        for (uint8_t i = 0; i < samples; i++) {
            for(uint8_t j = 0; j < 3; j++) {
                adc_oneshot_read(adc1_handle, channels[j], &raw);
                adc_cali_raw_to_voltage(adc1_cali_handle, raw, &mv);
                values[j] += mv;
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
    else {
        ESP_LOGE(TAG, "adc1_cali_handle핸들러 관련 오류 발생");
    }

    ESP_LOGI(TAG, "영점 조절 완료 - CO: %lumV, NH3: %lumV, NO2: %.lumV", 
             values[0] / samples, values[1] / samples, values[2] / samples);

    return ESP_OK;
}

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
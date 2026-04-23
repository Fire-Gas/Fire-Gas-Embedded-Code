#include "mq5.h"
#include "pin.h"
#include "common_struct.h"

#include "driver/adc.h" 
#include "esp_adc_cal.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MQ5";

// ADC 설정을 위한 변수
static esp_adc_cal_characteristics_t adc_chars_static;

static esp_err_t mq5_adc_init(void) {
    esp_err_t ret;

    // 12비트 해상도 (0~4095)
    ret = adc1_config_width(ADC_WIDTH_BIT_12);
    if (ret != ESP_OK) {
        return ret;
    }

    // 0~3.3V 측정 범위
    ret = adc1_config_channel_atten(MQ5_ADC_CHANNEL, ADC_ATTEN_DB_11);
    if (ret != ESP_OK) {
        return ret;
    }

    // ADC 특성 곡선 보정
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(MQ5_ADC_UNIT, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, MQ5_DEFAULT_VREF, &adc_chars_static);

    // 보정 타입 확인
    if (val_type > 0) {
        ESP_LOGI(TAG, "ADC 보정 완료 (타입: %d)", val_type);
    } else {
        ESP_LOGW(TAG, "ADC 보정 데이터 없음 (기본값 사용)");
    }

    return ESP_OK;
}

static uint32_t mq5_read_voltage(void) {
    uint32_t adc_reading = 0;

    // 노이즈 감소를 위해 10번 샘플링하여 평균값 사용
    const int sampling_count = 10;
    for (int i = 0; i < sampling_count; i++) {
        adc_reading += adc1_get_raw(MQ5_ADC_CHANNEL);
    }
    adc_reading /= sampling_count;

    return esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars_static);
}

void mq5_sensor_task(void* pvParameters) {
    QueueHandle_t mq5_queue = (QueueHandle_t)pvParameters;
    mq5_data_t sensor_data;

    esp_err_t init_ret = mq5_adc_init();
    if (init_ret != ESP_OK) {
        ESP_LOGE(TAG, "MQ-5 하드웨어 초기화 실패 (에러 코드: %s)", esp_err_to_name(init_ret));
        // 초기화 실패 시 태스크 종료 또는 재시도 로직
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "MQ-5 예열 시작 (20초)");
    vTaskDelay(pdMS_TO_TICKS(20000));

    while (1) {
        uint32_t voltage = mq5_read_voltage();
        
        // 전압 값을 기반으로 농도 계산
        sensor_data.voltage_mv = voltage;
        sensor_data.gas_detected = (voltage > WARNING_VALUE) ? 1 : 0;

        if (xQueueSend(mq5_queue, &sensor_data, pdMS_TO_TICKS(10)) != pdPASS) {
            ESP_LOGW(TAG, "Queue 꽉참");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
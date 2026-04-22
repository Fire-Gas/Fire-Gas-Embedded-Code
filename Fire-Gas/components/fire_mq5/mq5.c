#include "mq5.h"
#include "pin.h"
#include "common_struct.h"

#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MQ5";

// 가스 실험 후 어느정도가 가스인지 확인 후 변경
#define WARNING_VALUE 1500

// ADC 설정을 위한 변수
static esp_adc_cal_characteristics_t *adc_chars;
#define MQ5_ADC_CHANNEL  ADC_CHANNEL_6
#define MQ5_ADC_UNIT     ADC_UNIT_1
#define MQ5_DEFAULT_VREF 1100

static void mq5_adc_init(void) {
    // 12비트 해상도 (0~4095)
    adc1_config_width(ADC_WIDTH_BIT_12);
    // 0~3.3V 측정 범위
    adc1_config_channel_atten(MQ5_ADC_CHANNEL, ADC_ATTEN_DB_11);

    // ADC 특성 곡선 보정
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(MQ5_ADC_UNIT, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, MQ5_DEFAULT_VREF, adc_chars);
}

static uint32_t mq5_read_voltage(void) {
    uint32_t adc_reading = 0;
    // 노이즈 감소를 위해 10번 샘플링하여 평균값 사용
    for (int i = 0; i < 10; i++) {
        adc_reading += adc1_get_raw(MQ5_ADC_CHANNEL);
    }
    adc_reading /= 10;

    return esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
}

void mq5_sensor_task(void* pvParameters) {
    QueueHandle_t mq5_queue = (QueueHandle_t)pvParameters;
    mq5_data_t sensor_data;

    mq5_adc_init();
    
    // 센서 구조상 초기 대기 시간이 필요
    ESP_LOGI(TAG, "MQ-5 초기 대기 (약 20초)");
    vTaskDelay(pdMS_TO_TICKS(20000)); 

    while (1) {
        uint32_t voltage = mq5_read_voltage();
        
        // 전압 값을 기반으로 농도 계산
        sensor_data.voltage_mv = voltage;
        sensor_data.gas_detected = (voltage > WARNING_VALUE);

        if (xQueueSend(mq5_queue, &sensor_data, pdMS_TO_TICKS(10)) != pdPASS) {
            ESP_LOGW(TAG, "Queue Full");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#include "mics.h"
#include "common_handler.h"
<<<<<<< Updated upstream
#include "common_struct.h"
=======
>>>>>>> Stashed changes
#include "pin.h"

#include <stdint.h>

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h" 
#include "freertos/task.h"
<<<<<<< Updated upstream
#include "freertos/queue.h"
=======
>>>>>>> Stashed changes

static const char* TAG = "MICS";

void mics_sensor_get_value(void* pvParameters) {
<<<<<<< Updated upstream
    QueueHandle_t mics_queue_hanlder = (QueueHandle_t)pvParameters;
    mics_data_t data;

    static uint16_t co_raw, nh3_raw, no2_raw;
    static uint16_t co_mv, nh3_mv, no2_mv; // 위 변수 3개와 너무 동일함 -> 개선 필요
=======
    int co_raw, nh3_raw, no2_raw;
    int co_mv, nh3_mv, no2_mv; // 위 변수 3개와 너무 동일함 -> 개선 필요
>>>>>>> Stashed changes

    while(1) {
        adc_oneshot_read(adc1_handle, CO_CHANNEL, &co_raw);
        adc_oneshot_read(adc1_handle, NH3_CHANNEL, &nh3_raw);
        adc_oneshot_read(adc1_handle, NO2_CHANNEL, &no2_raw);

        // 2. 보정된 전압(mV)으로 변환
        if (adc1_cali_handle) {
            adc_cali_raw_to_voltage(adc1_cali_handle, co_raw, &co_mv);
            adc_cali_raw_to_voltage(adc1_cali_handle, nh3_raw, &nh3_mv);
            adc_cali_raw_to_voltage(adc1_cali_handle, no2_raw, &no2_mv);
        } else {
            co_mv = (co_raw * 3300) / 4095;
        }

<<<<<<< Updated upstream
        data.co = co_mv;
        data.nh = nh3_mv;
        data.no = no2_mv;

        if (xQueueSend(mics_queue_hanlder, &data, pdMS_TO_TICKS(100)) != pdPASS) {
            ESP_LOGE(TAG, "mics data전송 실패, 재전송 시도");
        }
=======
        // 3. 출력 (이제 전압 값으로 확인!)
        ESP_LOGI(TAG, "[MICS-6814 mV] CO: %d mV | NH3: %d mV | NO2: %d mV", co_mv, nh3_mv, no2_mv);
>>>>>>> Stashed changes

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
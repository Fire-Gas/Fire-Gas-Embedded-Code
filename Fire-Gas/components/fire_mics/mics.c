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

void mics_sensor_get_value(void* pvParameters) {
    QueueHandle_t mics_queue_handler = (QueueHandle_t)pvParameters;
    mics_data_t data;

    adc_channel_t channels[3] = {CO_CHANNEL, NH3_CHANNEL, NO2_CHANNEL};
    int raw_vals[3];
    int mv_vals[3];

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
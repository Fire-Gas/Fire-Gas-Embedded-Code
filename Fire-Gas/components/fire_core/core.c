#include "core.h"
#include "gpio.h"
#include "mics.h"
#include "bme.h"
#include "scd41.h"
#include "mq5.h"

#include "esp_log.h"
#include "driver/i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "common_struct.h"

static const char* TAG = "Core";

void MainCore(void* pvParameters) {

    QueueHandle_t mics_queue_handler = xQueueCreate(10, sizeof(mics_data_t)); 
    QueueHandle_t bme_queue_hadler = xQueueCreate(10, sizeof(bme_data_t));
    QueueHandle_t scd_queue_handler = xQueueCreate(5, sizeof(scd41_data_t));
    QueueHandle_t mq5_queue_handler = xQueueCreate(5, sizeof(mq5_data_t));

    mics_data_t mics_data;
    bme_data_t bme_data;
    scd41_data_t scd_data;
    mq5_data_t mq5_data;

    static uint8_t status = 1;
    static uint8_t retry_cnt;

    sensor_target_t sensor_list[] = {
        { mics_queue_handler, &mics_data, sizeof(mics_data_t), "MICS" },
        { bme_queue_hadler,  &bme_data,  sizeof(bme_data_t),  "BME"  },
        { scd_queue_handler,  &scd_data,  sizeof(scd41_data_t), "SCD"  },
        { mq5_queue_handler,  &mq5_data,  sizeof(mq5_data_t),   "MQ5"  }
    };
    const int sensor_count = sizeof(sensor_list) / sizeof(sensor_list[0]);

    xTaskCreate(mics_sensor_get_value, "mics_sensor_get_value", 6144, (void*)mics_queue_handler, 5, NULL);
    xTaskCreate(bme_raw_sensor, "bme_raw_sensor", 6144, (void*)bme_queue_hadler, 5, NULL);
    xTaskCreate(scd41_sensor_task, "scd41_task", 4096, (void*)scd_queue_handler, 5, NULL);
    xTaskCreate(mq5_sensor_task, "mq5_task",   4096, (void*)mq5_queue_handler, 5, NULL);
    
    while (1) {
        retry_cnt = 0;

        for (int i = 0; i < sensor_count; i++) {
            if (xQueueReceive(sensor_list[i].handle, sensor_list[i].data_ptr, pdMS_TO_TICKS(50)) != pdPASS) {
                ESP_LOGE(TAG, "%s 데이터 수신 실패", sensor_list[i].name);
                retry_cnt++;
            }
        }

        if (retry_cnt == 0) {
            ESP_LOGI(TAG, "모든 데이터 수신 성공 (%d개)", sensor_count);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }


    // 결과가 나오면 led_configure 호출
}
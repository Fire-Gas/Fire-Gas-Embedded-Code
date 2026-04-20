#include "core.h"
#include "gpio.h"
#include "mics.h"
#include "bme.h"

#include "esp_log.h"
#include "driver/i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

void MainCore(void* pvParameters) {
    QueueHandle_t mics_queue_handler = xQueueCreate(10, sizeof(int)); 
    QueueHandle_t bme_queue_hadler = xQueueCreate(10, sizeof(int));

    xTaskCreate(sensor_check, "sensor_check", 6144, NULL, 5, NULL);
    xTaskCreate(mics_sensor_get_value, "mics_sensor_get_value", 6144, (void*)mics_queue_handler, 5, NULL);
    xTaskCreate(bme_raw_sensor, "bme_raw_sensor", 6144, (void*)bme_queue_hadler, 5, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#include "core.h"
#include "gpio.h"
#include "mics.h"

#include "esp_log.h"
#include "driver/i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

void MainCore(void* pvParameters) {
    xTaskCreate(sensor_check, "sensor_check", 6144, NULL, 5, NULL);
    xTaskCreate(mics_sensor_get_value, "mics_sensor_get_value", 6144, NULL, 5, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
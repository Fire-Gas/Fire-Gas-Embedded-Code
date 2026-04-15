#include <stdio.h>

#include "core.h"
#include "gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

static const char *TAG = "Main APP";

int app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(300));

    ESP_LOGI(TAG, "[Steps 0] Starting program");

    ESP_LOGI(TAG, "[Steps 1] Initializing GPIO");
    ESP_ERROR_CHECK(gpio_init);

    ESP_LOGI(TAG, "[Steps 2] Launching main Logic Task");

    // 1024 x 6 = 6144 ( =  6KB)
    xTaskCreate(MainCore, "MainCore", 6144, NULL, 5, NULL);

    return 0;
}

#include "core.h"
#include "gpio.h"

#include "esp_log.h"
#include "driver/i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char* TAG = "Core";

void MainCore(void* pvParameters) {
    xTaskCreate(sensor_check, "sensor_check", 6144, NULL, 5, NULL);

    while (1) {

    }
}  

/* 센서 체킹 함수 queue에 이상 값 있으면?

    - 다른 센서 하나 더 개발
    
    - core 개발
    - main flow 정링

    - 테스트

*/
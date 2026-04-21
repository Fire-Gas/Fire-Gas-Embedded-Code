#include "core.h"
#include "gpio.h"
<<<<<<< Updated upstream
=======
#include "mics.h"
<<<<<<< Updated upstream
#include "bme.h"
=======
#include "SCD41.h"
>>>>>>> Stashed changes
>>>>>>> Stashed changes

#include "esp_log.h"
#include "driver/i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

<<<<<<< Updated upstream
=======
<<<<<<< Updated upstream
#include "common_struct.h"

>>>>>>> Stashed changes
static const char* TAG = "Core";

=======
>>>>>>> Stashed changes
void MainCore(void* pvParameters) {
    xTaskCreate(sensor_check, "sensor_check", 6144, NULL, 5, NULL);
<<<<<<< Updated upstream

=======
<<<<<<< Updated upstream
    xTaskCreate(mics_sensor_get_value, "mics_sensor_get_value", 6144, (void*)mics_queue_handler, 5, NULL);
    xTaskCreate(bme_raw_sensor, "bme_raw_sensor", 6144, (void*)bme_queue_hadler, 5, NULL);
    
>>>>>>> Stashed changes
    while (1) {

<<<<<<< Updated upstream
=======
        for (int i = 0; i < sensor_count; i++) {
            if (xQueueReceive(sensor_list[i].handle, sensor_list[i].data_ptr, pdMS_TO_TICKS(50)) != pdPASS) {
                ESP_LOGE(TAG, "%s 데이터 수신 실패", sensor_list[i].name);
                retry_cnt++;
            }
        }

        if (retry_cnt == 0) {
            ESP_LOGI(TAG, "모든 데이터 수신 성공 (%d개)", sensor_count);
        }
=======
    xTaskCreate(mics_sensor_get_value, "mics_sensor_get_value", 6144, NULL, 5, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
>>>>>>> Stashed changes
>>>>>>> Stashed changes
    }
}  

/* 센서 체킹 함수 queue에 이상 값 있으면?

    - 다른 센서 하나 더 개발
    
    - core 개발
    - main flow 정링

    - 테스트

*/
#include "bme.h"
#include "common_handler.h"

#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BME";

void bme_read_task(void* pvParameters) {
    esp_err_t ret;
    uint8_t raw_temp[3]; 
    uint32_t temp_adc;

    while(1) {
        uint8_t cmd[2] = {0x74, 0x25};
        // 0x74: register     (제어 레지스터 위치) 
        // 0x25: control mode (한번 읽고 전송 모드로 지정)
        ret = i2c_master_write_to_device(I2C_NUM_0, BME680_I2C_ADDR, cmd, 2, pdMS_TO_TICKS(50));

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "명령 전달 실패");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(50));

        ret = i2c_master_write_read_device(I2C_NUM_0, BME680_I2C_ADDR, 
                                           (uint8_t[]){0x22}, 1, 
                                           raw_temp, 3,          
                                           pdMS_TO_TICKS(50));
                                           // 0x22 부터 읽어오기 (데이터 시작)

        if (ret == ESP_OK) {
            temp_adc = (raw_temp[0] << 12) | (raw_temp[1] << 4) | (raw_temp[2] >> 4);
            
            ESP_LOGI(TAG, "온도 Raw 데이터: %lu", temp_adc);
        } else {
            ESP_LOGE(TAG, "데이터 읽기 실패");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#include "bme.h"
#include "common_handler.h"
#include "common_struct.h"
#include "gpio.h"

#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdint.h>

static const char *TAG = "BME";

uint16_t par_t1 = 0;
int16_t  par_t2 = 0;
int8_t   par_t3 = 0;

int32_t bme_sensor(uint32_t temp_adc) {
    int64_t var1;
    int64_t var2;
    int32_t calc_temp;

    // 데이터시트 공식 정확히 적용
    var1 = ((int32_t)temp_adc >> 3) - ((int32_t)par_t1 << 1);
    var2 = (var1 * (int32_t)par_t2) >> 11;
    var1 = ((var1 >> 1) * (var1 >> 1)) >> 12;
    var1 = ((var1 * ((int32_t)par_t3 << 4)) >> 14);

    int32_t t_fine = (int32_t)(var2 + var1);
    calc_temp = (t_fine * 5 + 128) >> 8;

    return calc_temp;
}

esp_err_t bme_init(void) {
    esp_err_t ret = ESP_OK;
    uint8_t reg_addrs[3] = {0xE9, 0x8A, 0x8C};
    uint8_t read_buf[2];

    ESP_ERROR_CHECK(sensor_check());
    for (uint8_t i = 0; i < 3; i++) {
        uint8_t read_len = (i == 2) ? 1 : 2; 

        ret |= i2c_master_write_read_device(I2C_NUM_0, BME680_I2C_ADDR, &reg_addrs[i], 1, read_buf, read_len, pdMS_TO_TICKS(100));
        if (i == 0)      par_t1 = (uint16_t)((read_buf[1] << 8) | read_buf[0]);
        else if (i == 1) par_t2 = (int16_t)((read_buf[1] << 8) | read_buf[0]);
        else if (i == 2) par_t3 = (int8_t)read_buf[0];
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "캘리브레이션 데이터 읽기 실패");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "BME680 준비 완료 (T1:%u, T2:%d, T3:%d)", par_t1, par_t2, par_t3);
    return ESP_OK;
}

void bme_raw_sensor(void* pvParameters) {
    QueueHandle_t bme_queue_handler = (QueueHandle_t)pvParameters;
    bme_data_t data;

    esp_err_t ret;
    uint8_t raw_temp[3]; 
    uint32_t temp_adc;

    ESP_ERROR_CHECK(bme_init());
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
            int32_t bme_real_sensor = (int32_t)bme_sensor(temp_adc);
            float real_temp_float = (float)bme_real_sensor / 100.0;

            data.raw_adc = temp_adc;
            data.real_adc = real_temp_float;

            if (xQueueSend(bme_queue_handler, &data, pdMS_TO_TICKS(100)) != pdPASS) {
                ESP_LOGE(TAG, "bme data 전송 실패");
            }
        } 
        else {
            ESP_LOGE(TAG, "데이터 읽기 실패");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
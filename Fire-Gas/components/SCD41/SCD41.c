#include "SCD41.h"
#include "pin.h"
#include "common_struct.h"

#include "driver/i2c.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SCD41";

static scd41_data_t g_scd41_info;

// 명령 보내기
static esp_err_t scd41_send_command(uint16_t command) {
    uint8_t cmd_buffer[2] = {(uint8_t)(command >> 8), (uint8_t)(command & 0xFF)};
    return i2c_master_write_to_device(I2C_NUM_0, SCD41_I2C_ADDR, cmd_buffer, 2, pdMS_TO_TICKS(100));
}

static esp_err_t scd41_init(void) {
    // 측정 중지
    scd41_send_command(SCD41_CMD_STOP_MEASURE);
    vTaskDelay(pdMS_TO_TICKS(500));

    // 연속 측정 시작 명령 전송
    esp_err_t ret = scd41_send_command(SCD41_CMD_START_MEASURE);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SCD41 Measurement Started Successfully");
    } else {
        ESP_LOGE(TAG, "Failed to start SCD41");
    }
    return ret;
}

static esp_err_t scd41_read_data(void) {
    uint8_t cmd[2] = {(uint8_t)(SCD41_CMD_READ_MEASURE >> 8), (uint8_t)(SCD41_CMD_READ_MEASURE & 0xFF)};
    uint8_t raw_data[9]; // CO2(3) + T(3) + H(3) (Data 2 bytes + CRC 1 byte씩)

    esp_err_t ret = i2c_master_write_read_device(I2C_NUM_0, SCD41_I2C_ADDR, 
                                                cmd, 2, 
                                                raw_data, 9, 
                                                pdMS_TO_TICKS(100));

    if (ret == ESP_OK) {
        // 데이터 변환
        g_scd41_info.co2 = (uint16_t)((raw_data[0] << 8) | raw_data[1]);
        
        // 온도
        uint16_t raw_temp = (uint16_t)((raw_data[3] << 8) | raw_data[4]);
        g_scd41_info.temperature = -45.0f + 175.0f * (float)raw_temp / 65536.0f;
        
        // 습도
        uint16_t raw_humi = (uint16_t)((raw_data[6] << 8) | raw_data[7]);
        g_scd41_info.humidity = 100.0f * (float)raw_humi / 65536.0f;
    }

    return ret;
}

void scd41_sensor_task(void* pvParameters) {
    QueueHandle_t scd_queue = (QueueHandle_t)pvParameters;
    scd41_data_t sensor_data;

    if (scd41_init() != ESP_OK) {
        ESP_LOGE(TAG, "SCD41 초기화 실패");
        vTaskDelete(NULL);
    }

    ESP_LOGI("SCD41_TASK", "SCD41 수집 태스크 시작");

    while (1) {
        if (scd41_read_data() == ESP_OK) {
            sensor_data = g_scd41_info;

            if (xQueueSend(scd_queue, &sensor_data, pdMS_TO_TICKS(100)) != pdPASS) {
                ESP_LOGW("SCD41_TASK", "Queue 전송 실패 (Full)");
            }
        } else {
            ESP_LOGE("SCD41_TASK", "데이터 읽기 실패");
        }
        // 센서 구조상 측정 시간이 좀 오래걸림
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
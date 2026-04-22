#include "scd41.h"
#include "pin.h"
#include "common_struct.h"

#include "driver/i2c.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SCD41";

static scd41_data_t g_scd41_info;

// CRC8 계산 함수
static uint8_t scd41_generate_crc(const uint8_t* data, uint16_t count) {
    uint8_t crc = 0xFF;
    for (uint16_t i = 0; i < count; i++) {
        crc ^= data[i];
        for (uint8_t bit = 8; bit > 0; --bit) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x31;
            else crc = (crc << 1);
        }
    }
    return crc;
}

// 명령 보내기
static esp_err_t scd41_send_command(uint16_t command) {
    uint8_t cmd_buffer[2] = {(uint8_t)(command >> 8), (uint8_t)(command & 0xFF)};
    return i2c_master_write_to_device(I2C_NUM_0, SCD41_I2C_ADDR, cmd_buffer, 2, pdMS_TO_TICKS(100));
}

// 초기화
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

// 데이터 full 확인
static bool scd41_get_data_ready_status(void) {
    uint8_t cmd[2] = {0xe4, 0xb8};
    uint8_t res[3];
    if (i2c_master_write_read_device(I2C_NUM_0, SCD41_I2C_ADDR, cmd, 2, res, 3, pdMS_TO_TICKS(100)) == ESP_OK) {
        uint16_t status = (res[0] << 8) | res[1];
        return (status & 0x07FF) != 0; // 하위 11비트가 0이 아니면 준비 완료
    }
    return false;
}

static esp_err_t scd41_read_data(void) {
    uint8_t cmd[2] = {(uint8_t)(SCD41_CMD_READ_MEASURE >> 8), (uint8_t)(SCD41_CMD_READ_MEASURE & 0xFF)};
    uint8_t raw_data[9]; // CO2(3) + T(3) + H(3) (Data 2 bytes + CRC 1 byte씩)

    esp_err_t ret = i2c_master_write_read_device(I2C_NUM_0, SCD41_I2C_ADDR, cmd, 2, raw_data, 9, pdMS_TO_TICKS(100));

    if (ret != ESP_OK) return ret;

    if (scd41_generate_crc(&raw_data[0], 2) != raw_data[2] || 
        scd41_generate_crc(&raw_data[3], 2) != raw_data[5] || 
        scd41_generate_crc(&raw_data[6], 2) != raw_data[8]) {
        ESP_LOGE(TAG, "CRC Checksum Error!");
        return ESP_ERR_INVALID_CRC;
    }

    // co2
    g_scd41_info.co2 = (uint16_t)((raw_data[0] << 8) | raw_data[1]);
    // 온도
    g_scd41_info.temperature = -45.0f + 175.0f * (float)((raw_data[3] << 8) | raw_data[4]) / 65536.0f;
    // 습도
    g_scd41_info.humidity = 100.0f * (float)((raw_data[6] << 8) | raw_data[7]) / 65536.0f;

    return ESP_OK;
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
        if (scd41_get_data_ready_status()) {
            if (scd41_read_data() == ESP_OK) {
                sensor_data = g_scd41_info;
                xQueueSend(scd_queue, &sensor_data, pdMS_TO_TICKS(10));
            }
            // 측정 대기 시간 (SCD41 센서 구조상 오래걸림)
            vTaskDelay(pdMS_TO_TICKS(5000));
        } else {
            // 대기 시간이 끝난 후에도 다 차지 않으면 조금 대기
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}
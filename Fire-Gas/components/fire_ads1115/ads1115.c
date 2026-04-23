#include "ads1115.h"
#include "ads1115_setting.h"

#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "ADS1115";

esp_err_t ads1115_init(void) {
    ESP_LOGI(TAG, "ADS1115 드라이버 준비 완료");
    return ESP_OK;
}

esp_err_t ads1115_read_raw(ads1115_channel_t channel, int16_t *out_raw) {
    if (out_raw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t config = ADS1115_OS_START | ADS1115_PGA_4_096V | ADS1115_MODE_SINGLE_SHOT | ADS1115_DR_128SPS | ADS1115_COMP_QUE_DISABLE;

    config &= ~(0x7 << ADS1115_MUX_SHIFT);
    
    config |= ((0x4 + channel) << ADS1115_MUX_SHIFT);

    uint8_t tx_buf[3] = {
        ADS1115_REG_CONFIG, 
        (uint8_t)(config >> 8),   // 상위 8비트
        (uint8_t)(config & 0xFF)  // 하위 8비트
    };

    // 설정 전송
    esp_err_t ret = i2c_master_write_to_device(I2C_NUM_0, 0x48, tx_buf, 3, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        return ret;
    }

    // 변환 시간 대기 (최소 8ms)
    vTaskDelay(pdMS_TO_TICKS(20));

    // Conversion Register에서 데이터 읽기
    uint8_t reg_ptr = 0x00;
    uint8_t rx_buf[2];
    ret = i2c_master_write_read_device(I2C_NUM_0, 0x48, &reg_ptr, 1, rx_buf, 2, pdMS_TO_TICKS(100));

    if (ret == ESP_OK) {
        // 결과값을 포인터에 저장
        *out_raw = (int16_t)((rx_buf[0] << 8) | rx_buf[1]);
    }

    return ret;
}

float ads1115_raw_to_voltage(int16_t raw) {
    return raw * 0.1875f; 
}
#include "gpio.h"

#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"

// #define BME680_I2C_ADDR    0x77   // 슬라이드 스위치 기본값
// #define BME680_CHIP_ID_REG 0xD0   // ID가 저장된 레지스터 주소
// #define BME680_CHIP_ID_VAL 0x61   // 읽었을 때 나와야 하는 값

static const char *TAG = "GPIO";

esp_err_t gpio_init(void) {
    esp_err_t ret;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 21,           
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = 22,          
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000, 
    };

    ret = i2c_param_config(I2C_NUM_0, &conf);
    ret |= i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fail to install I2C driver");
        return ret;
    }
    return ret;
}
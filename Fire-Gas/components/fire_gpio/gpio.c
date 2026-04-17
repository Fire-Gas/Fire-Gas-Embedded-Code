#include "gpio.h"

#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"

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

void sensor_check(void) {
    uint8_t chip_id = 0;
    esp_err_t ret = ESP_OK;

    ret |= i2c_master_write_read_device(I2C_NUM_0, BME680_I2C_ADDR, 
                                        (uint8_t[]){BME680_CHIP_ID_REG}, 1, 
                                        &chip_id, 1, pdMS_TO_TICKS(50));


    while(ret != ESP_OK || chip_id != BME680_CHIP_ID_VAL) {
        ESP_LOGW(TAG, "failed to sensor conneted");
        vTaskDelay(pdMS_TO_TICKS(1000));

        ret = i2c_master_write_read_device(I2C_NUM_0, BME680_I2C_ADDR, 
                                           (uint8_t[]){BME680_CHIP_ID_REG}, 1, 
                                           &chip_id, 1, pdMS_TO_TICKS(50));
    }

    ESP_LOGI(TAG, "Successed to connect sensor");
    vTaskDelay(pdMS_TO_TICKS(1000));
}
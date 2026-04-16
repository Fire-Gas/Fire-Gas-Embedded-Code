#include "core.h"
#include "gpio.h"

#include "esp_log.h"
#include "driver/i2c.h"

static const char* TAG = "Core";

void MainCore(void* pvParameters) {
    uint8_t chip_id = 0;
    esp_err_t ret = ESP_OK;

    ret |= i2c_master_write_read_device(I2C_NUM_0, BME680_I2C_ADDR, (uint8_t[]){BME680_CHIP_ID_REG}, 1, &chip_id, 1, pdMS_TO_TICKS(50));


    while(ret != ESP_OK || chip_id != BME680_CHIP_ID_VAL) {
        ESP_LOGW(TAG, "failed to sensor conneted");
    }
    return;
}  


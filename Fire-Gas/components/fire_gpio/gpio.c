#include "gpio.h"
#include "pin.h"
#include "SCD41.h"
#include "common_handler.h"

#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "GPIO";

adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t adc1_cali_handle = NULL;  

static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle);

esp_err_t gpio_init(void) {
    // i2c init
    bool cali_enabled = true;
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

    // adc init
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, 
        .atten = ADC_ATTEN_DB_12,         
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, CO_CHANNEL, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, NH3_CHANNEL, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, NO2_CHANNEL, &config));

    cali_enabled = adc_calibration_init(ADC_UNIT_1, ADC_ATTEN_DB_12, &adc1_cali_handle);

    // 센서 init
    scd41_init();

    if (adc1_handle == NULL || !cali_enabled) {
        ESP_LOGE(TAG, "ADC1 핸들러 관련 오류");
    }
    return ret;
}

void sensor_check(void* pvParameters) {
    uint8_t chip_id = 0;

    esp_err_t ret_BME = ESP_OK;

    ret_BME |= i2c_master_write_read_device(I2C_NUM_0, BME680_I2C_ADDR, 
                                        (uint8_t[]){BME680_CHIP_ID_REG}, 1, 
                                        &chip_id, 1, pdMS_TO_TICKS(50));

    while(ret_BME != ESP_OK || chip_id != BME680_CHIP_ID_VAL) {
        ESP_LOGW(TAG, "failed to sensor conneted");
        vTaskDelay(pdMS_TO_TICKS(1000));

        ret_BME = i2c_master_write_read_device(I2C_NUM_0, BME680_I2C_ADDR, 
                                           (uint8_t[]){BME680_CHIP_ID_REG}, 1, 
                                           &chip_id, 1, pdMS_TO_TICKS(50));
    }

    ESP_LOGI(TAG, "Successed to connect sensor");
    vTaskDelay(pdMS_TO_TICKS(4000));
}

// 오차 보정 함수
static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;


    // 다른 ADC 사용 센서는 define 사용하여 추가
    #if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit, // adc1, 2 구분
            .atten = atten, // 감쇠 정도
            .bitwidth = ADC_BITWIDTH_DEFAULT, // 분해능
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) calibrated = true;
    #endif

    *out_handle = handle;
    return calibrated;
}
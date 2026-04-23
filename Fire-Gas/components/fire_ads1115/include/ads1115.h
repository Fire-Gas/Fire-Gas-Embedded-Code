#ifndef ADS1115_H
#define ADS1115_H

#include <stdint.h>
#include "esp_err.h"

#define ADS1115_I2C_ADDR         0x48
#define ADS1115_REG_CONVERSION   0x00
#define ADS1115_REG_CONFIG       0x01

typedef enum {
    ADS1115_CH_0 = 0,
    ADS1115_CH_1 = 1,
    ADS1115_CH_2 = 2,
    ADS1115_CH_3 = 3
} ads1115_channel_t;

esp_err_t ads1115_init(void);

/**
 * @brief 특정 채널의 ADC raw 값을 읽어옴
 * @param channel 읽고자 하는 채널 (ADS1115_CH_0 ~ 3)
 * @param out_raw 데이터가 저장될 포인터
 * @return esp_err_t ESP_OK 성공, 그 외 에러
 */
esp_err_t ads1115_get_raw_value(ads1115_channel_t channel, int16_t *out_raw);

/**
 * @brief raw 값을 전압(mV)으로 변환
 * @param raw_val ADC 원본 값
 * @return float 계산된 전압 (mV)
 */
float ads1115_to_voltage(int16_t raw_val);

#endif
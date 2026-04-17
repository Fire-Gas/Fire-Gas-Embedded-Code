#ifndef GPIO_SETTING
#define GPIO_SETTING

#include "esp_err.h"

#define BME680_I2C_ADDR    0x77   // 슬라이드 스위치 기본값
#define BME680_CHIP_ID_REG 0xD0   // ID가 저장된 레지스터 주소
#define BME680_CHIP_ID_VAL 0x61   // 읽었을 때 나와야 하는 값

/**
 * @brief GPIO 초기화 함수
 * @param void
 * @return esp_err_t
 * - ESP_OK   : 성공
 * - ESP_FAIL : 실패
 */
esp_err_t gpio_init(void);

/**
 * @brief 센서 연결 확인 함수
 * @param void
 * @return esp_err_t
 * - ESP_OK   : 성공
 * - ESP_FAIL : 실패
 */
void sensor_check(void);

#endif
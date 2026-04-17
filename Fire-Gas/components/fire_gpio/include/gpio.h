#ifndef GPIO_SETTING
#define GPIO_SETTING

#include "esp_err.h"

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
 * @param void* pvParameters
 * @return esp_err_t
 * - ESP_OK   : 성공
 * - ESP_FAIL : 실패
 */
void sensor_check(void* pvParameters);

#endif
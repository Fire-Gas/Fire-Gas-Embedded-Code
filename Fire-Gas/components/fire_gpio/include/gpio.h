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

#endif
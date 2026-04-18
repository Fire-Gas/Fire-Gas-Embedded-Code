#ifndef BME_SENSOR
#define BME_SENSOR

/**
 * @brief BME 센서값 읽기 함수
 * @param void* pvParameters
 * @return None
 */
void bme_read_task(void* pvParameters);

/**
 * @brief BME raw값을 도씨 값으로 바꿔주는 함수
 * @param uint32_t temp_adc
 * @return int32_t: 도씨로 변환한 값
 */
int32_t bme_sensor(uint32_t temp_adc);

#endif
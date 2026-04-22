#ifndef SCD41_H
#define SCD41_H

#include "esp_err.h"

#define SCD41_I2C_ADDR             0x62
#define SCD41_CMD_START_MEASURE    0x21b1
#define SCD41_CMD_READ_MEASURE     0xec05
#define SCD41_CMD_STOP_MEASURE     0x3f86

/**
 * @brief Core에서 호출되는 SCD41 데이터 수집 태스크
 * @details 센서 초기화 후 무한 루프를 돌며 데이터 준비 상태를 확인하고, 성공적으로 읽은 데이터를 지정된 큐로 전송
 * @param pvParameters Core로부터 전달받은 QueueHandle_t
 */
void scd41_sensor_task(void* pvParameters);

#endif
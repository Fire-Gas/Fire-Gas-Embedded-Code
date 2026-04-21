#ifndef SCD41_H
#define SCD41_H

#include "esp_err.h"

// 상태 지정 (pin.h로 옮길까 생각중)
#define SCD41_I2C_ADDR             0x62
#define SCD41_CMD_START_MEASURE    0x21b1
#define SCD41_CMD_READ_MEASURE     0xec05
#define SCD41_CMD_STOP_MEASURE     0x3f86

void scd41_sensor_task(void* pvParameters);

#endif
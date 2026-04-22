#ifndef COMMON_STRUCT
#define COMMON_STRUCT

#include <stdint.h>
#include "freertos/FreeRTOS.h"  
#include "freertos/queue.h"

typedef struct mics_data_t {
    uint32_t co;
    uint32_t nh;
    uint32_t no;

} mics_data_t;

typedef struct bme_data_t {
    uint32_t raw_adc;
    uint32_t real_adc;
} bme_data_t;

typedef struct __attribute__((packed)) sensor_target_t{
    QueueHandle_t handle;
    void* data_ptr;
    size_t size;
    const char* name;
} sensor_target_t;

typedef struct __attribute__((packed)) scd41_data_t{
    uint16_t co2;
    int32_t temperature;
    int32_t humidity;
} scd41_data_t;

typedef enum final_value {
    A = 1,
    B = 2,
    C = 3,
    K = 4,
    battery = 5,
    warn = 6,
} final_value;

#endif
#ifndef SCD41_H
#define SCD41_H

#include "esp_err.h"

esp_err_t scd41_init(void);

esp_err_t scd41_read_data(void);

#endif
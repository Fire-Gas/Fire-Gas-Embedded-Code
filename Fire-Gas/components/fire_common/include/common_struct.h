#ifndef COMMON_STRUCT
#define COMMON_STRUCT

#include <stdint.h>

typedef struct __attribute__((packed)) mics_data_t {
    uint32_t co;
    uint32_t nh;
    uint32_t no;

} mics_data_t;


#endif
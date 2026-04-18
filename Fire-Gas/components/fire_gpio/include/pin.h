#define BME680_CHIP_ID_REG 0xD0   // ID가 저장된 레지스터 주소
#define BME680_CHIP_ID_VAL 0x61   // 읽었을 때 나와야 하는 값

// ESP32 dev board pinmap 기준
#define CO_CHANNEL  ADC_CHANNEL_4 // GPIO 32
#define NH3_CHANNEL ADC_CHANNEL_5 // GPIO 33
#define NO2_CHANNEL ADC_CHANNEL_6 // GPIO 34
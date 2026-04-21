<<<<<<< Updated upstream
#define BME680_CHIP_ID_REG 0xD0   // ID가 저장된 레지스터 주소
#define BME680_CHIP_ID_VAL 0x61   // 읽었을 때 나와야 하는 값

// ESP32 dev board pinmap 기준
#define CO_CHANNEL  ADC_CHANNEL_4 // GPIO 32
#define NH3_CHANNEL ADC_CHANNEL_5 // GPIO 33
#define NO2_CHANNEL ADC_CHANNEL_6 // GPIO 34
=======
// BME 센서 핀
#define BME680_I2C_ADDR    0x77   // 슬라이드 스위치 기본값
#define BME680_CHIP_ID_REG 0xD0   // ID가 저장된 레지스터 주소
#define BME680_CHIP_ID_VAL 0x61   // 읽었을 때 나와야 하는 값

// MICS 센서 핀 ESP32 dev board pinmap 기준
#define CO_CHANNEL  ADC_CHANNEL_4 // GPIO 32
#define NH3_CHANNEL ADC_CHANNEL_5 // GPIO 33
#define NO2_CHANNEL ADC_CHANNEL_6 // GPIO 34

// SCD41 센서 핀
#define I2C_MASTER_SCL_IO    22    // SCL 핀
#define I2C_MASTER_SDA_IO    21    // SDA 핀
#define I2C_MASTER_NUM       0     // I2C 포트 번호

// SCD41 주소
#define SCD41_I2C_ADDR 0x62

// SCD41 명령어 정의
#define SCD41_CMD_START_MEASURE    0x21b1
#define SCD41_CMD_READ_MEASURE     0xec05
#define SCD41_CMD_STOP_MEASURE     0x3f86
>>>>>>> Stashed changes

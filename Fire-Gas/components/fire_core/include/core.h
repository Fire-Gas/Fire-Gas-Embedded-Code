#ifndef MAINTASK
#define MAINTASK

#define BME680_I2C_ADDR    0x77   // 슬라이드 스위치 기본값
#define BME680_CHIP_ID_REG 0xD0   // ID가 저장된 레지스터 주소
#define BME680_CHIP_ID_VAL 0x61   // 읽었을 때 나와야 하는 값

/**
 * @brief 전체 시스템의 Main 함수로 작동한다
 * @param void* pvParameters 태스크 생성 파라미터
 * @return None
 */
void MainCore(void* pvParameters);

#endif
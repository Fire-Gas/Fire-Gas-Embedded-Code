#ifndef MQ5_H
#define MQ5_H

/**
 * @brief MQ5 가스 센서 수집 전용 태스크
 * @details 내부적으로 ADC를 읽어 가스 농도를 계산 후, 결과를 Queue로 전송
 * @param pvParameters Core로부터 전달받은 QueueHandle_t
 */
void mq5_sensor_task(void* pvParameters);

#endif
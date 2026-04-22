#ifndef LED_INDICATE
#define LED_INDICATE

#include "common_struct.h"

/**
 * @brief led 제어 태스크
 * @param final_value data 
 *  - final_value enum을 기반으로 하는 현재 상태 분류용 파라미터
 * @return None
 */
void led_configure(final_value data);

#endif
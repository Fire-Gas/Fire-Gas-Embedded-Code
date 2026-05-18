#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "common_struct.h"

/* 화재 클래스 — config.py CLASS_NAMES 순서와 동일 */
#define FIRE_CLASS_A      0
#define FIRE_CLASS_B      1
#define FIRE_CLASS_C      2
#define FIRE_CLASS_K      3
#define FIRE_CLASS_NORMAL 4

/* LED 상태 코드 — risk.py STATE_* 및 LED 드라이버와 동기화 필수 */
#define LED_STATE_OFF         0
#define LED_STATE_SOLID       1
#define LED_STATE_BLINK_SLOW  2   /* 1Hz */
#define LED_STATE_BLINK_FAST  3   /* 4Hz */
#define LED_STATE_BLINK_EMERG 4   /* 8Hz */

typedef struct {
    float   severity;        /* 0.0~1.0 급성 위험도 (비정상 클래스 최대 확률) */
    float   chronic_ratio;   /* 0.0~1.0+ 만성 누적 도즈 (8h TLV 기준) */
    int     class_id;        /* FIRE_CLASS_* */
    float   class_score;     /* 해당 클래스 softmax 확률 */
    uint8_t led_states[6];   /* final_value 순서: A,B,C,K,batt,warn */
} fire_ai_result_t;

bool fire_ai_init(void);

bool fire_ai_infer(
    const mics_data_t*  mics,
    const bme_data_t*   bme,
    const scd41_data_t* scd,
    const mq5_data_t*   mq5,
    fire_ai_result_t*   out
);

#ifdef __cplusplus
}
#endif

#ifndef FIRE_AI_H
#define FIRE_AI_H

#include <stdint.h>
#include <stdbool.h>
#include "common_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float severity;
    int   class_id;
    float class_score;
} fire_ai_result_t;

/**
 * @brief TFLM 인터프리터/텐서 아레나 초기화. 부팅 시 1회 호출.
 * @return true 성공, false 실패 (모델 버전 불일치 / AllocateTensors 실패)
 */
bool fire_ai_init(void);

/**
 * @brief 한 샘플 추론. 센서 4종 구조체를 받아 sev/cls 결과를 채워준다.
 * @return true 성공, false Invoke 실패 또는 미초기화
 */
bool fire_ai_infer(const mics_data_t* mics,
                   const bme_data_t*  bme,
                   const scd41_data_t* scd,
                   const mq5_data_t*  mq5,
                   fire_ai_result_t*  out);

#ifdef __cplusplus
}
#endif

#endif

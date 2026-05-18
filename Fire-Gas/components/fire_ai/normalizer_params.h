#pragma once
/*
 * !! 이 파일은 빌드 전 gen_normalizer_header.py 를 실행해 덮어써야 합니다 !!
 *    python components/fire_ai/gen_normalizer_header.py
 *
 * 아래 값은 플레이스홀더입니다 — 실제 normalizer.npz 없이 빌드만 확인용.
 * 플레이스홀더 상태에서는 AI 추론 결과가 의미 없습니다.
 */
#define NORM_FEATURE_COUNT 25

static const float NORM_MEAN[NORM_FEATURE_COUNT] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  /* MICS_CO, MICS_NH, MICS_NO, BME_RAW_ADC, BME_REAL_ADC */
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  /* SCD_CO2, SCD_TEMP, SCD_HUM, MQ5_VOLTAGE_MV, MQ5_GAS_DETECTED */
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  /* ROC x5 */
    0.0f, 0.0f, 0.0f, 0.0f,        /* ROC x4 */
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f  /* ratios x6 */
};

static const float NORM_STD[NORM_FEATURE_COUNT] = {
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
};

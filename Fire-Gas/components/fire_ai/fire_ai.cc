#include "fire_ai.h"
#include "firegas_model.h"
#include "normalizer_params.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "esp_log.h"
#include <string.h>

static const char* TAG = "FireAI";

/* --- 상수 (config.py 와 동기화) --- */
#define NUM_CLASSES   5
#define WINDOW_SIZE   15
#define NUM_FEATURES  NORM_FEATURE_COUNT   /* 25 */
#define NUM_RAW       10                   /* raw 센서 값 수 */

/* 위험도 임계 (config.py RISK_THRESHOLDS) */
#define THRESH_SAFE    0.20f
#define THRESH_CAUTION 0.45f
#define THRESH_WARNING 0.70f
#define THRESH_DANGER  0.85f

/* 만성 노출 기준 시간 (8h, Haber 법칙) */
#define CHRONIC_REF_S  (8.0f * 3600.0f)

/* --- TFLite Micro 정적 자원 --- */
#define TENSOR_ARENA_KB 100
static uint8_t tensor_arena[TENSOR_ARENA_KB * 1024] __attribute__((aligned(16)));

/* CNN-LSTM 모델에 필요한 ops: Conv2D, Reshape, LSTM, FullyConnected, Softmax, Tanh, Logistic */
static tflite::MicroMutableOpResolver<8> s_resolver;
static tflite::MicroInterpreter*    s_interp   = nullptr;
static TfLiteTensor*                s_input    = nullptr;
static TfLiteTensor*                s_output   = nullptr;

/* --- 슬라이딩 윈도우 (정규화 완료 피처 저장) --- */
static float window_buf[WINDOW_SIZE][NUM_FEATURES];
static int   window_head  = 0;
static bool  window_ready = false;

/* --- ROC 계산용 이전 raw 값 --- */
static float prev_raw[NUM_RAW];
static bool  first_sample = true;

/* --- 만성 도즈 누적 --- */
static float chronic_accum = 0.0f;

/* ------------------------------------------------------------------ */

static uint8_t sev_to_state(float sev)
{
    if (sev < THRESH_SAFE)    return LED_STATE_OFF;
    if (sev < THRESH_CAUTION) return LED_STATE_BLINK_SLOW;
    if (sev < THRESH_WARNING) return LED_STATE_BLINK_FAST;
    if (sev < THRESH_DANGER)  return LED_STATE_SOLID;
    return LED_STATE_BLINK_EMERG;
}

/* 센서 구조체 → raw float 배열
 * 순서: MICS_CO(0) MICS_NH(1) MICS_NO(2) BME_RAW(3) BME_REAL(4)
 *        SCD_CO2(5) SCD_TEMP(6) SCD_HUM(7) MQ5_MV(8) MQ5_DET(9)
 */
static void extract_raw(
    const mics_data_t* m, const bme_data_t* b,
    const scd41_data_t* s, const mq5_data_t* q,
    float raw[NUM_RAW])
{
    raw[0] = (float)m->co;
    raw[1] = (float)m->nh;
    raw[2] = (float)m->no;
    raw[3] = (float)b->raw_adc;
    raw[4] = (float)b->real_adc;
    raw[5] = (float)s->co2;
    raw[6] = (float)s->temperature;
    raw[7] = (float)s->humidity;
    raw[8] = (float)q->voltage_mv;
    raw[9] = q->gas_detected ? 1.0f : 0.0f;
}

/* raw + prev_raw → 정규화된 25-dim 피처 벡터
 * feature_columns.json 순서와 정확히 일치해야 함:
 *   [0..9]  raw
 *   [10..18] ROC (MQ5_GAS_DETECTED 제외)
 *   [19..24] 센서 간 비율 6종
 */
static void build_features(const float raw[NUM_RAW], float feat[NUM_FEATURES])
{
    /* raw */
    for (int i = 0; i < NUM_RAW; i++) feat[i] = raw[i];

    /* ROC — indices 0..8 (MQ5_GAS_DETECTED=index9 제외) */
    for (int i = 0; i < NUM_RAW - 1; i++) {
        feat[NUM_RAW + i] = first_sample ? 0.0f : (raw[i] - prev_raw[i]);
    }

    /* 비율 (features.py add_ratios 순서, eps=1.0) */
    float eps = 1.0f;
    feat[19] = raw[2] / (raw[0] + eps);   /* R_NO_OVER_CO   */
    feat[20] = raw[1] / (raw[0] + eps);   /* R_NH_OVER_CO   */
    feat[21] = raw[4] / (raw[0] + eps);   /* R_VOC_OVER_CO  */
    feat[22] = raw[4] / (raw[5] + eps);   /* R_VOC_OVER_CO2 */
    feat[23] = raw[8] / (raw[0] + eps);   /* R_MQ5_OVER_CO  */
    feat[24] = raw[2] / (raw[5] + eps);   /* R_NO_OVER_CO2  */

    /* 정규화: (x - mean) / std */
    for (int i = 0; i < NUM_FEATURES; i++) {
        feat[i] = (feat[i] - NORM_MEAN[i]) / NORM_STD[i];
    }
}

/* 윈도우를 입력 텐서 [1, WINDOW_SIZE, NUM_FEATURES] 에 복사.
 * window_head가 다음 쓸 위치이므로 head부터 읽으면 oldest→newest.
 */
static void fill_input_tensor(void)
{
    float* data = s_input->data.f;
    for (int t = 0; t < WINDOW_SIZE; t++) {
        int idx = (window_head + t) % WINDOW_SIZE;
        memcpy(data + t * NUM_FEATURES, window_buf[idx], NUM_FEATURES * sizeof(float));
    }
}

/* ------------------------------------------------------------------ */

bool fire_ai_init(void)
{
    s_resolver.AddConv2D();
    s_resolver.AddReshape();
    s_resolver.AddUnidirectionalSequenceLSTM();
    s_resolver.AddFullyConnected();
    s_resolver.AddSoftmax();
    s_resolver.AddTanh();
    s_resolver.AddLogistic();

    const tflite::Model* model = tflite::GetModel(firegas_model_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGE(TAG, "TFLite schema 버전 불일치 model=%lu runtime=%lu",
                 (unsigned long)model->version(), (unsigned long)TFLITE_SCHEMA_VERSION);
        return false;
    }

    static tflite::MicroInterpreter interp(
        model, s_resolver, tensor_arena, sizeof(tensor_arena));
    s_interp = &interp;

    if (s_interp->AllocateTensors() != kTfLiteOk) {
        ESP_LOGE(TAG, "AllocateTensors 실패");
        return false;
    }

    s_input  = s_interp->input(0);
    s_output = s_interp->output(0);

    if (s_input->dims->size != 3
        || s_input->dims->data[1] != WINDOW_SIZE
        || s_input->dims->data[2] != NUM_FEATURES) {
        ESP_LOGE(TAG, "입력 텐서 형상 불일치 [%d,%d,%d] != [1,%d,%d]",
                 s_input->dims->data[0], s_input->dims->data[1], s_input->dims->data[2],
                 WINDOW_SIZE, NUM_FEATURES);
        return false;
    }

    memset(window_buf, 0, sizeof(window_buf));
    memset(prev_raw, 0, sizeof(prev_raw));
    window_head   = 0;
    window_ready  = false;
    first_sample  = true;
    chronic_accum = 0.0f;

    ESP_LOGI(TAG, "AI 초기화 완료 (arena=%dKB)", TENSOR_ARENA_KB);
    return true;
}

bool fire_ai_infer(
    const mics_data_t* mics, const bme_data_t* bme,
    const scd41_data_t* scd, const mq5_data_t* mq5,
    fire_ai_result_t* out)
{
    if (!s_interp) return false;

    /* 1. raw 추출 */
    float raw[NUM_RAW];
    extract_raw(mics, bme, scd, mq5, raw);

    /* 2. 피처 빌드 + 정규화 */
    float feat[NUM_FEATURES];
    build_features(raw, feat);

    memcpy(prev_raw, raw, sizeof(prev_raw));
    first_sample = false;

    /* 3. 슬라이딩 윈도우 갱신 */
    if (!window_ready) {
        /* 첫 샘플: 전체 윈도우를 동일 피처로 패딩 */
        for (int i = 0; i < WINDOW_SIZE; i++) {
            memcpy(window_buf[i], feat, sizeof(feat));
        }
        window_head  = 1;   /* 다음 쓸 위치 */
        window_ready = true;
    } else {
        memcpy(window_buf[window_head], feat, sizeof(feat));
        window_head = (window_head + 1) % WINDOW_SIZE;
    }

    /* 4. 텐서 채우기 */
    fill_input_tensor();

    /* 5. 추론 */
    if (s_interp->Invoke() != kTfLiteOk) {
        ESP_LOGE(TAG, "Invoke 실패");
        return false;
    }

    /* 6. 출력 파싱 */
    float probs[NUM_CLASSES];
    for (int i = 0; i < NUM_CLASSES; i++) probs[i] = s_output->data.f[i];

    /* 최고 클래스 */
    int top = 0;
    for (int i = 1; i < NUM_CLASSES; i++) {
        if (probs[i] > probs[top]) top = i;
    }

    /* 급성 위험도: 비정상 클래스(0~3) 중 최대 */
    float severity = 0.0f;
    for (int i = 0; i < NUM_CLASSES - 1; i++) {
        if (probs[i] > severity) severity = probs[i];
    }

    /* 만성 도즈 누적 (1s 주기 가정, CHRONIC_REF_S 기준 비율) */
    chronic_accum += severity;
    float chronic_ratio = chronic_accum / CHRONIC_REF_S;

    /* 7. LED 상태 결정 */
    uint8_t states[6] = {0};
    if (top < 4 && probs[top] > 0.5f) {
        states[top] = LED_STATE_SOLID;   /* A/B/C/K 화재 LED */
    }
    float ch = chronic_ratio > 1.0f ? 1.0f : chronic_ratio;
    uint8_t ws = sev_to_state(severity);
    uint8_t wc = sev_to_state(ch);
    states[5] = ws > wc ? ws : wc;      /* warn LED */
    /* states[4] (batt): core.c 에서 ADC 측정 후 덮어쓸 것 */

    /* 8. 결과 채우기 */
    out->severity      = severity;
    out->chronic_ratio = chronic_ratio;
    out->class_id      = top;
    out->class_score   = probs[top];
    for (int i = 0; i < 6; i++) out->led_states[i] = states[i];

    return true;
}

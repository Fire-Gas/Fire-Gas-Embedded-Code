#include "fire_ai.h"
#include "firegas_model.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "esp_log.h"
#include <string.h>

static const char* TAG = "FireAI";

// 학습 시 사용한 시퀀스 길이와 반드시 일치해야 함.
#define FIRE_AI_SEQ_LEN   20
#define FIRE_AI_FEAT_DIM  10

// LSTM은 상태 텐서 때문에 아레나 여유가 더 필요. 부족하면 늘려서 재빌드.
constexpr int kArenaSize = 64 * 1024;
static uint8_t tensor_arena[kArenaSize];

static const tflite::Model*       g_model       = nullptr;
static tflite::MicroInterpreter*  g_interpreter = nullptr;
static bool                       g_ready       = false;

// 링버퍼: 과거 T개 샘플의 10피처. oldest = g_buf_head 위치(다음 쓸 자리).
static float g_seq_buffer[FIRE_AI_SEQ_LEN][FIRE_AI_FEAT_DIM];
static int   g_buf_head  = 0;
static int   g_buf_count = 0;

static int g_out_sev_idx = -1;
static int g_out_cls_idx = -1;

static void write_val(TfLiteTensor* t, int flat_idx, float v) {
    if (t->type == kTfLiteFloat32) { t->data.f[flat_idx] = v; return; }
    int q = (int)(v / t->params.scale + t->params.zero_point + 0.5f);
    if (t->type == kTfLiteInt8) {
        if (q < -128) q = -128; else if (q > 127) q = 127;
        t->data.int8[flat_idx] = (int8_t)q;
    } else if (t->type == kTfLiteUInt8) {
        if (q < 0) q = 0; else if (q > 255) q = 255;
        t->data.uint8[flat_idx] = (uint8_t)q;
    }
}

static float read_val(const TfLiteTensor* t, int flat_idx) {
    if (t->type == kTfLiteFloat32) return t->data.f[flat_idx];
    if (t->type == kTfLiteInt8) {
        return (t->data.int8[flat_idx] - t->params.zero_point) * t->params.scale;
    }
    if (t->type == kTfLiteUInt8) {
        return (t->data.uint8[flat_idx] - t->params.zero_point) * t->params.scale;
    }
    return 0.0f;
}

bool fire_ai_init(void) {
    g_model = tflite::GetModel(firegas_model_tflite);
    if (g_model->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGE(TAG, "schema version mismatch: model=%lu lib=%d",
                 (unsigned long)g_model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }

    // LSTM + 일반 op들. 모델에 없는 op는 그냥 남겨둬도 무방.
    static tflite::MicroMutableOpResolver<20> resolver;
    resolver.AddUnidirectionalSequenceLSTM();
    resolver.AddFullyConnected();
    resolver.AddRelu();
    resolver.AddRelu6();
    resolver.AddSoftmax();
    resolver.AddLogistic();
    resolver.AddTanh();
    resolver.AddReshape();
    resolver.AddQuantize();
    resolver.AddDequantize();
    resolver.AddStridedSlice();
    resolver.AddPack();
    resolver.AddUnpack();
    resolver.AddConcatenation();
    resolver.AddSplit();
    resolver.AddAdd();
    resolver.AddMul();
    resolver.AddMean();
    resolver.AddTranspose();
    resolver.AddShape();

    static tflite::MicroInterpreter static_interp(
        g_model, resolver, tensor_arena, kArenaSize);
    g_interpreter = &static_interp;

    if (g_interpreter->AllocateTensors() != kTfLiteOk) {
        ESP_LOGE(TAG, "AllocateTensors 실패 — kArenaSize 늘리세요");
        return false;
    }

    // 입력 shape 검증: [1, T, F]
    TfLiteTensor* in = g_interpreter->input(0);
    if (in->dims->size != 3 ||
        in->dims->data[1] != FIRE_AI_SEQ_LEN ||
        in->dims->data[2] != FIRE_AI_FEAT_DIM) {
        ESP_LOGE(TAG, "input shape mismatch: got [%d,%d,%d] expected [1,%d,%d]",
                 in->dims->size > 0 ? in->dims->data[0] : 0,
                 in->dims->size > 1 ? in->dims->data[1] : 0,
                 in->dims->size > 2 ? in->dims->data[2] : 0,
                 FIRE_AI_SEQ_LEN, FIRE_AI_FEAT_DIM);
        return false;
    }

    // 출력 2개 (sev, cls) 자동 매칭. 마지막 dim 1 → sev, ≥2 → cls.
    for (size_t i = 0; i < g_interpreter->outputs_size(); ++i) {
        TfLiteTensor* t = g_interpreter->output(i);
        int last = t->dims->data[t->dims->size - 1];
        if (last <= 1) g_out_sev_idx = (int)i;
        else           g_out_cls_idx = (int)i;
    }

    ESP_LOGI(TAG, "init OK. seq=%d feat=%d arena=%d/%d in_type=%d",
             FIRE_AI_SEQ_LEN, FIRE_AI_FEAT_DIM,
             (int)g_interpreter->arena_used_bytes(), kArenaSize,
             (int)in->type);

    g_buf_head  = 0;
    g_buf_count = 0;
    g_ready = true;
    return true;
}

bool fire_ai_infer(const mics_data_t* mics,
                   const bme_data_t*  bme,
                   const scd41_data_t* scd,
                   const mq5_data_t*  mq5,
                   fire_ai_result_t*  out) {
    if (!g_ready || g_interpreter == nullptr) return false;

    // 1) 새 샘플을 링버퍼에 push (raw 값. 정규화는 모델 내부에서 처리한다고 가정)
    float* slot = g_seq_buffer[g_buf_head];
    slot[0] = (float)mics->co;
    slot[1] = (float)mics->nh;
    slot[2] = (float)mics->no;
    slot[3] = (float)bme->raw_adc;
    slot[4] = (float)bme->real_adc;
    slot[5] = (float)scd->co2;
    slot[6] = (float)scd->temperature;
    slot[7] = (float)scd->humidity;
    slot[8] = (float)mq5->voltage_mv;
    slot[9] = mq5->gas_detected ? 1.0f : 0.0f;

    g_buf_head = (g_buf_head + 1) % FIRE_AI_SEQ_LEN;
    if (g_buf_count < FIRE_AI_SEQ_LEN) {
        g_buf_count++;
        ESP_LOGI(TAG, "warmup %d/%d", g_buf_count, FIRE_AI_SEQ_LEN);
        return false;
    }

    // 2) 시퀀스를 oldest→newest 순서로 입력 텐서에 채움
    TfLiteTensor* in = g_interpreter->input(0);
    int oldest = g_buf_head;
    for (int t = 0; t < FIRE_AI_SEQ_LEN; ++t) {
        int src = (oldest + t) % FIRE_AI_SEQ_LEN;
        for (int f = 0; f < FIRE_AI_FEAT_DIM; ++f) {
            int flat = t * FIRE_AI_FEAT_DIM + f;
            write_val(in, flat, g_seq_buffer[src][f]);
        }
    }

    // 3) Invoke
    if (g_interpreter->Invoke() != kTfLiteOk) {
        ESP_LOGE(TAG, "Invoke 실패");
        return false;
    }

    // 4) 결과 읽기
    out->severity    = 0.0f;
    out->class_id    = -1;
    out->class_score = 0.0f;

    if (g_out_sev_idx >= 0) {
        out->severity = read_val(g_interpreter->output(g_out_sev_idx), 0);
    }
    if (g_out_cls_idx >= 0) {
        const TfLiteTensor* cls = g_interpreter->output(g_out_cls_idx);
        int n = cls->dims->data[cls->dims->size - 1];
        int best_i = 0;
        float best_v = read_val(cls, 0);
        for (int i = 1; i < n; ++i) {
            float v = read_val(cls, i);
            if (v > best_v) { best_v = v; best_i = i; }
        }
        out->class_id    = best_i;
        out->class_score = best_v;
    }
    return true;
}

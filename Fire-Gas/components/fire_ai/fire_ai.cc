#include "fire_ai.h"
#include "firegas_model.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "esp_log.h"

static const char* TAG = "FireAI";

// 모델이 작으면(< ~70KB) 20KB로 충분. AllocateTensors 실패하면 늘릴 것.
constexpr int kArenaSize = 24 * 1024;
static uint8_t tensor_arena[kArenaSize];

static const tflite::Model*       g_model       = nullptr;
static tflite::MicroInterpreter*  g_interpreter = nullptr;
static bool                       g_ready       = false;

// 출력 텐서 인덱스. signature의 출력 이름이 "sev"/"cls"이므로 런타임에 매칭.
static int g_out_sev_idx = -1;
static int g_out_cls_idx = -1;

bool fire_ai_init(void) {
    g_model = tflite::GetModel(firegas_model_tflite);
    if (g_model->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGE(TAG, "schema version mismatch: model=%lu lib=%d",
                 (unsigned long)g_model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }

    // Keras Dense + ReLU + Softmax 모델 가정. 모델이 다른 op를 쓰면 추가/제거.
    static tflite::MicroMutableOpResolver<12> resolver;
    resolver.AddFullyConnected();
    resolver.AddRelu();
    resolver.AddRelu6();
    resolver.AddSoftmax();
    resolver.AddLogistic();
    resolver.AddReshape();
    resolver.AddQuantize();
    resolver.AddDequantize();
    resolver.AddAdd();
    resolver.AddMul();
    resolver.AddMean();
    resolver.AddTanh();

    static tflite::MicroInterpreter static_interp(
        g_model, resolver, tensor_arena, kArenaSize);
    g_interpreter = &static_interp;

    if (g_interpreter->AllocateTensors() != kTfLiteOk) {
        ESP_LOGE(TAG, "AllocateTensors 실패. kArenaSize 늘리세요.");
        return false;
    }

    // 출력 2개 (sev, cls). 순서는 변환 시점에 따라 다를 수 있어 dim으로 추정.
    // sev: 스칼라 또는 (1,1), cls: (1,N) N>=2
    for (size_t i = 0; i < g_interpreter->outputs_size(); ++i) {
        TfLiteTensor* t = g_interpreter->output(i);
        int last = t->dims->data[t->dims->size - 1];
        if (last <= 1)        g_out_sev_idx = (int)i;
        else                  g_out_cls_idx = (int)i;
    }

    ESP_LOGI(TAG, "init OK. in=%d out=%d arena_used=%d/%d",
             (int)g_interpreter->inputs_size(),
             (int)g_interpreter->outputs_size(),
             (int)g_interpreter->arena_used_bytes(), kArenaSize);

    g_ready = true;
    return true;
}

static float read_float_or_dequant(const TfLiteTensor* t, int idx) {
    if (t->type == kTfLiteFloat32) return t->data.f[idx];
    if (t->type == kTfLiteInt8) {
        return (t->data.int8[idx] - t->params.zero_point) * t->params.scale;
    }
    if (t->type == kTfLiteUInt8) {
        return (t->data.uint8[idx] - t->params.zero_point) * t->params.scale;
    }
    return 0.0f;
}

static void write_float_or_quant(TfLiteTensor* t, int idx, float v) {
    if (t->type == kTfLiteFloat32) { t->data.f[idx] = v; return; }
    int q = (int)(v / t->params.scale + t->params.zero_point + 0.5f);
    if (t->type == kTfLiteInt8) {
        if (q < -128) q = -128; else if (q > 127) q = 127;
        t->data.int8[idx] = (int8_t)q;
    } else if (t->type == kTfLiteUInt8) {
        if (q < 0) q = 0; else if (q > 255) q = 255;
        t->data.uint8[idx] = (uint8_t)q;
    }
}

bool fire_ai_infer(const mics_data_t* mics,
                   const bme_data_t*  bme,
                   const scd41_data_t* scd,
                   const mq5_data_t*  mq5,
                   fire_ai_result_t*  out) {
    if (!g_ready || g_interpreter == nullptr) return false;

    // CSV 열 순서와 동일하게 10개 피처를 만든다.
    // MICS_CO, MICS_NH, MICS_NO, BME_RAW, BME_REAL,
    // SCD_CO2, SCD_TEMP, SCD_HUM, MQ5_MV, MQ5_DETECTED
    const float feats[10] = {
        (float)mics->co,
        (float)mics->nh,
        (float)mics->no,
        (float)bme->raw_adc,
        (float)bme->real_adc,
        (float)scd->co2,
        (float)scd->temperature,
        (float)scd->humidity,
        (float)mq5->voltage_mv,
        mq5->gas_detected ? 1.0f : 0.0f,
    };

    TfLiteTensor* in = g_interpreter->input(0);
    int in_len = 1;
    for (int d = 0; d < in->dims->size; ++d) in_len *= in->dims->data[d];
    if (in_len != 10) {
        ESP_LOGE(TAG, "input length mismatch: model=%d, expected=10", in_len);
        return false;
    }
    for (int i = 0; i < 10; ++i) write_float_or_quant(in, i, feats[i]);

    if (g_interpreter->Invoke() != kTfLiteOk) {
        ESP_LOGE(TAG, "Invoke 실패");
        return false;
    }

    out->severity    = 0.0f;
    out->class_id    = -1;
    out->class_score = 0.0f;

    if (g_out_sev_idx >= 0) {
        out->severity = read_float_or_dequant(g_interpreter->output(g_out_sev_idx), 0);
    }
    if (g_out_cls_idx >= 0) {
        const TfLiteTensor* cls = g_interpreter->output(g_out_cls_idx);
        int n = cls->dims->data[cls->dims->size - 1];
        int   best_i = 0;
        float best_v = read_float_or_dequant(cls, 0);
        for (int i = 1; i < n; ++i) {
            float v = read_float_or_dequant(cls, i);
            if (v > best_v) { best_v = v; best_i = i; }
        }
        out->class_id    = best_i;
        out->class_score = best_v;
    }

    return true;
}

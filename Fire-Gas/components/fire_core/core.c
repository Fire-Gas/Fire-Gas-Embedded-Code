#include "core.h"
#include "gpio.h"
#include "mics.h"
#include "bme.h"
#include "scd41.h"
#include "fire_ai.h"
#include "mq5.h"

#include "esp_log.h"
#include "driver/i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "common_struct.h"
#include <stdio.h>
#include <sys/stat.h>

// 경로 SD카드 확인이 필요함 
#define CSV_FILE_PATH "/sdcard/Sensor_data.csv"

/*
1: csv 수집 모드
0: ai 추론 모드
*/
#define AI_TEST_MODE 1 

static const char* TAG = "Core";
static void ai_dataset(mics_data_t* mics_data, bme_data_t* bme_data, scd41_data_t* scd_data, mq5_data_t* mq5_data);

void main_core(void* pvParameters) {
    QueueHandle_t mics_queue_handler = xQueueCreate(10, sizeof(mics_data_t)); 
    QueueHandle_t bme_queue_hadler = xQueueCreate(10, sizeof(bme_data_t));
    QueueHandle_t scd_queue_handler = xQueueCreate(5, sizeof(scd41_data_t));
    QueueHandle_t mq5_queue_handler = xQueueCreate(5, sizeof(mq5_data_t));

    mics_data_t mics_data;
    bme_data_t bme_data;
    scd41_data_t scd_data;
    mq5_data_t mq5_data;

    static uint8_t retry_cnt;

    sensor_target_t sensor_list[] = {
        { mics_queue_handler, &mics_data, sizeof(mics_data_t), "MICS" },
        { bme_queue_hadler,  &bme_data,  sizeof(bme_data_t),  "BME"  },
        { scd_queue_handler,  &scd_data,  sizeof(scd41_data_t), "SCD"  },
        { mq5_queue_handler,  &mq5_data,  sizeof(mq5_data_t),   "MQ5"  }
    };
    const int sensor_count = sizeof(sensor_list) / sizeof(sensor_list[0]);

    if(!fire_ai_init()) {
        ESP_LOGE(TAG, "AI 초기화 실패");
    }

    xTaskCreate(mics_sensor_get_value, "mics_sensor_get_value", 6144, (void*)mics_queue_handler, 5, NULL);
    xTaskCreate(bme_raw_sensor, "bme_raw_sensor", 6144, (void*)bme_queue_hadler, 5, NULL);
    xTaskCreate(scd41_sensor_task, "scd41_task", 4096, (void*)scd_queue_handler, 5, NULL);
    xTaskCreate(mq5_sensor_task, "mq5_task",   4096, (void*)mq5_queue_handler, 5, NULL);
    
    while (1) {
        retry_cnt = 0;

        for (int i = 0; i < sensor_count; i++) {
            if (xQueueReceive(sensor_list[i].handle, sensor_list[i].data_ptr, pdMS_TO_TICKS(50)) != pdPASS) {
                ESP_LOGE(TAG, "%s 데이터 수신 실패", sensor_list[i].name);
                retry_cnt++;
            }
        }

        if (retry_cnt == 0) {
            ESP_LOGI(TAG, "모든 데이터 수신 성공 (%d개)", sensor_count);

            #if AI_TEST_MODE
            ai_dataset(&mics_data, &bme_data, &scd_data, &mq5_data);
            #else
            fire_ai_result_t r;
            if (fire_ai_infer(&mics_data, &bme_data, &scd_data, &mq5_data, &r)) {
                ESP_LOGI(TAG, "sev=%.3f cls=%d score=%.3f", r.severity, r.class_id, r.class_score);
            }
            #endif


        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }


    // 결과가 나오면 led_configure 호출
}

// AI dataset을 모으는 함수로, AI 학습 진행 시 필요한 데이터를 모을 때만 사용한다
static void ai_dataset(mics_data_t* mics_data, bme_data_t* bme_data, scd41_data_t* scd_data, mq5_data_t* mq5_data) {
    struct stat st;
    bool file_exists = (stat(CSV_FILE_PATH, &st) == 0);

    FILE* f = fopen(CSV_FILE_PATH, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "CSV파일 생성 실패");
        return;
    }

    if (!file_exists) {
        fprintf(f, "MICS_CO,MICS_NH,MICS_NO,BME_RAW_ADC,BME_REAL_ADC,SCD_CO2,SCD_TEMP,SCD_HUM,MQ5_VOLTAGE_MV,MQ5_GAS_DETECTED\n");
    }

    fprintf(f, "%lu,%lu,%lu,%lu,%lu,%u,%ld,%ld,%lu,%d\n",
            (unsigned long)mics_data->co,
            (unsigned long)mics_data->nh,
            (unsigned long)mics_data->no,
            (unsigned long)bme_data->raw_adc,
            (unsigned long)bme_data->real_adc,
            scd_data->co2,
            (long)scd_data->temperature,
            (long)scd_data->humidity,
            (unsigned long)mq5_data->voltage_mv,
            mq5_data->gas_detected ? 1 : 0); 

    fclose(f);
}
#include "led.h"

#include "driver/gpio.h"
#include "esp_log.h"

static const char* TAG = "LED";
static const uint8_t LED_CNT = 5;
static const gpio_num_t led_pins[5] = {
    GPIO_NUM_2, 
    GPIO_NUM_4,
    GPIO_NUM_5, 
    GPIO_NUM_18,
    GPIO_NUM_19
};

static void led_init(void) {
    uint64_t pin_mask = 0;
    for (int i = 0; i < LED_CNT; i++) {
        pin_mask |= (1ULL << led_pins[i]);
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = pin_mask, 
        // bit mask 등록
        .mode = GPIO_MODE_OUTPUT,       
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE     
    };

    gpio_config(&io_conf);
    ESP_LOGI(TAG, "%d개의 LED 초기화 완료", LED_CNT);
}
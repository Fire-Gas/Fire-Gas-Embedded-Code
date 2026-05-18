#include "led.h"
#include "common_struct.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "LED";

const gpio_num_t led_pins[LED_CNT] = {
    [A]    = GPIO_NUM_2,
    [B]    = GPIO_NUM_4,
    [C]    = GPIO_NUM_7,  
    [K]    = GPIO_NUM_5,
    [batt] = GPIO_NUM_18,
    [warn] = GPIO_NUM_19
};

esp_err_t led_init(void) {
    uint64_t pin_mask = 0;
    esp_err_t ret = 1;

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

    if ((ret = gpio_config(&io_conf)) != ESP_OK) {
        ESP_LOGE(TAG, "LED 초기화 실패");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "%d개의 LED 초기화 완료", LED_CNT);
    return ESP_OK;
}

static void set_led_state(int index, uint32_t state) {
    if (index >= 0 && index < LED_CNT) {
        gpio_set_level(led_pins[index], state);
    }
}

static void all_leds_off() {
    for (int i = 0; i < LED_CNT; i++) {
        gpio_set_level(led_pins[i], 0);
    }
}

void led_configure(final_value data) {
    all_leds_off();
    switch (data) {
    case A:
        ESP_LOGW(TAG, "Use A");
        gpio_set_level(led_pins[A], 1);
        break;
    case B:
        ESP_LOGW(TAG, "Use B");
        gpio_set_level(led_pins[B], 1);
        break;
    case C:
        ESP_LOGW(TAG, "Use C");
        gpio_set_level(led_pins[C], 1);
        break;
    case K:
        ESP_LOGW(TAG, "Use K");
        gpio_set_level(led_pins[K], 1);
        break;
    case batt:
        ESP_LOGW(TAG, "Need to charge");
        gpio_set_level(led_pins[batt], 1);
        break;
    case warn:
        ESP_LOGW(TAG, "Occurred SW/HW error");
        gpio_set_level(led_pins[warn], 1);
        break;
    default:
        ESP_LOGW(TAG, "어느 분류에도 속하지 않음");
        break;
    }
}
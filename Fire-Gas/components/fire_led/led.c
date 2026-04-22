#include "led.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "LED";
static const uint8_t LED_CNT = 5;
static const gpio_num_t led_pins[5] = {
    GPIO_NUM_2, 
    GPIO_NUM_4,
    GPIO_NUM_5, 
    GPIO_NUM_18,
    GPIO_NUM_19
};

static esp_err_t led_init(void) {
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

// 모든 LED 켜기 (사용하지 않음)
static void set_led_state(int index, uint32_t state) {
    if (index >= 0 && index < LED_CNT) {
        gpio_set_level(led_pins[index], state);
    }
}
// 모든 LED 끄기 (사용하지 않음)
static void all_leds_off() {
    for (int i = 0; i < LED_CNT; i++) {
        gpio_set_level(led_pins[i], 0);
    }
}

void led_test_task(void *pvParameters) {
    ESP_ERROR_CHECK(led_init());

    while (1) {
        for (int i = 0; i < LED_CNT; i++) {
            set_led_state(i, 1);
            ESP_LOGI(TAG, "LED %d ON", i);
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        vTaskDelay(pdMS_TO_TICKS(500));

        all_leds_off();
        ESP_LOGI(TAG, "All LEDs OFF");
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
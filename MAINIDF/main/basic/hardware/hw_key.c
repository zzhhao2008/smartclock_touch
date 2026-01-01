#include "basic/hardware/hw_key.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include <string.h>

#define TAG "HW_KEY"
#define MAX_KEYS 4
#define DEBOUNCE_MS 35

typedef struct {
    int gpio;
    int active_level;
    key_callback_t cb;
    void* cb_arg;
    int pressed; // 0 or 1
} key_item_t;

static key_item_t keys[MAX_KEYS];
static QueueHandle_t key_queue = NULL;
static TaskHandle_t key_task_handle = NULL;

static void IRAM_ATTR key_isr(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    if (key_queue) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(key_queue, &gpio_num, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
    }
}

static void key_task(void* param)
{
    uint32_t gpio_num;
    for (;;) {
        if (xQueueReceive(key_queue, &gpio_num, portMAX_DELAY) == pdTRUE) {
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            int level = gpio_get_level((gpio_num_t)gpio_num);
            for (int i = 0; i < MAX_KEYS; i++) {
                if (keys[i].gpio == (int)gpio_num) {
                    int is_pressed = (level == keys[i].active_level);
                    if (is_pressed && !keys[i].pressed) {
                        keys[i].pressed = 1;
                        if (keys[i].cb) keys[i].cb(keys[i].gpio, KEY_EVT_PRESS, keys[i].cb_arg);
                    } else if (!is_pressed && keys[i].pressed) {
                        keys[i].pressed = 0;
                        if (keys[i].cb) keys[i].cb(keys[i].gpio, KEY_EVT_RELEASE, keys[i].cb_arg);
                    }
                    break;
                }
            }
        }
    }
}

bool hw_key_init(void)
{
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "gpio_install_isr_service failed: %s", esp_err_to_name(err));
        return false;
    }
    key_queue = xQueueCreate(10, sizeof(uint32_t));
    if (!key_queue) {
        ESP_LOGE(TAG, "Failed to create key queue");
        return false;
    }
    if (xTaskCreate(key_task, "key_task", 2048, NULL, 10, &key_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create key task");
        vQueueDelete(key_queue);
        key_queue = NULL;
        return false;
    }
    memset(keys, 0, sizeof(keys));
    return true;
}

bool hw_key_add(int gpio, int active_level)
{
    for (int i = 0; i < MAX_KEYS; i++) {
        if (keys[i].gpio == 0) {
            keys[i].gpio = gpio;
            keys[i].active_level = active_level;
            keys[i].pressed = (gpio_get_level((gpio_num_t)gpio) == active_level);
            keys[i].cb = NULL;
            keys[i].cb_arg = NULL;
            gpio_set_intr_type((gpio_num_t)gpio, GPIO_INTR_ANYEDGE);
            gpio_isr_handler_add((gpio_num_t)gpio, key_isr, (void*) (uintptr_t) gpio);
            ESP_LOGI(TAG, "Added key GPIO%d active_level=%d", gpio, active_level);
            return true;
        }
    }
    ESP_LOGE(TAG, "No space to add key GPIO%d", gpio);
    return false;
}

bool hw_key_register_callback(int gpio, key_callback_t cb, void* arg)
{
    for (int i = 0; i < MAX_KEYS; i++) {
        if (keys[i].gpio == gpio) {
            keys[i].cb = cb;
            keys[i].cb_arg = arg;
            return true;
        }
    }
    ESP_LOGW(TAG, "Key GPIO%d not found when registering callback", gpio);
    return false;
}

#include "driver/gpio.h"
#include "esp_log.h"
#define TAG "PM_D"
void ACC(int level){
    gpio_set_level(40, level);
    ESP_LOGI(TAG, "ACC: %d", level);
}
#include <stdio.h>
#include "basic/jlc_lcd.h"
#include "demos/lv_demos.h"
#include "esp_littlefs.h"
#include "driver/gpio.h"

static void init_littlefs(void)
{
    const esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE("littlefs", "Failed to mount or format filesystem.");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE("littlefs", "Failed to find littlefs partition.");
        }
        else
        {
            ESP_LOGE("littlefs", "Failed to initialize littlefs (%s).", esp_err_to_name(ret));
        }
        return;
    }
    size_t total = 0, used = 0;
    esp_littlefs_info("storage", &total, &used);
    ESP_LOGI("littlefs", "Partition size: total: %d, used: %d.", total, used);
}

bool init_gpio(void)
{
    typedef struct {
        int pin;
        gpio_mode_t mode;
        gpio_pullup_t pull_up;
        gpio_pulldown_t pull_down;
        gpio_int_type_t intr_type;
    } gpio_init_t;
    gpio_init_t gpios[] = {
        {40, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE, GPUI_INTR_DISABLE},  // 电源控制
    };

    gpio_config_t io_conf = {0}; // 初始化为0
    for (int i = 0; i < sizeof(gpios) / sizeof(gpios[0]); i++) {
        io_conf.pin_bit_mask = BIT64(gpios[i].pin);
        io_conf.mode = gpios[i].mode;
        io_conf.pull_up_en = gpios[i].pull_up;
        io_conf.pull_down_en = gpios[i].pull_down;
        io_conf.intr_type = gpios[i].intr_type;
        if (gpio_config(&io_conf) != ESP_OK) {
            ESP_LOGE("GPIO", "GPIO%d init failed", gpios[i].pin);
            return false;
        }
        ESP_LOGI("GPIO", "GPIO%d initialized", gpios[i].pin);
    }
    return true;
}

void app_main(void)
{
    init_gpio();
    gpio_set_level(40, (uint32_t)sta); // 打开电源
    bsp_i2c_init();   // I2C初始化
    init_littlefs();  // 初始化文件系统
    bsp_lvgl_start(); // 初始化液晶屏lvgl接口

    /* 下面5个demos 只打开1个运行 */
    //lv_demo_benchmark();
    // lv_demo_keypad_encoder();
    lv_demo_music();
    // lv_demo_stress();
    // lv_demo_widgets();
}

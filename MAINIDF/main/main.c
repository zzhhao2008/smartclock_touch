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
        .format_if_mount_failed = true};

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE("littlefs", "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE("littlefs", "Failed to find littlefs partition");
        }
        else
        {
            ESP_LOGE("littlefs", "Failed to initialize littlefs (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    esp_littlefs_info("storage", &total, &used);
    ESP_LOGI("littlefs", "Partition size: total: %d, used: %d", total, used);
}

#include "driver/gpio.h"

bool power_acc(bool sta)
{
    // 配置 GPIO40 为输出模式（仅首次调用时配置）
    static bool is_initialized = false;
    if (!is_initialized) {
        gpio_config_t io_conf = {
            .pin_bit_mask = BIT64(40),          // 设置GPIO40
            .mode = GPIO_MODE_OUTPUT,           // 输出模式
            .pull_up_en = GPIO_PULLUP_DISABLE,  // 禁用上拉
            .pull_down_en = GPIO_PULLDOWN_DISABLE, // 禁用下拉
            .intr_type = GPIO_INTR_DISABLE      // 禁用中断
        };
        gpio_config(&io_conf);
        is_initialized = true;
    }

    // 设置电平 (sta=true 输出高电平, sta=false 输出低电平)
    gpio_set_level(40, (uint32_t)sta);
    
    return true; // 始终返回成功（实际使用中可根据需求添加错误检查）
}

void app_main(void)
{
    power_acc(true); // 打开电源
    bsp_i2c_init();   // I2C初始化
    init_littlefs();  // <<< 新增：初始化文件系统
    bsp_lvgl_start(); // 初始化液晶屏lvgl接口

    /* 下面5个demos 只打开1个运行 */
    //lv_demo_benchmark();
    // lv_demo_keypad_encoder();
    lv_demo_music();
    // lv_demo_stress();
    // lv_demo_widgets();
}

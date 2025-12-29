#include <stdio.h>
#include "esp32_s3_szp.h"
#include "logo_en_240x240_lcd.h"
#include "yingwu.h"
#include "demos/lv_demos.h"
#include "esp_littlefs.h"

static void init_littlefs(void)
{
    const esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .max_files = 10,
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

void app_main(void)
{
    bsp_i2c_init();   // I2C初始化
    init_littlefs();  // <<< 新增：初始化文件系统
    bsp_lvgl_start(); // 初始化液晶屏lvgl接口

    /* 下面5个demos 只打开1个运行 */
    lv_demo_benchmark();
    // lv_demo_keypad_encoder();
    // lv_demo_music();
    // lv_demo_stress();
    // lv_demo_widgets();
}

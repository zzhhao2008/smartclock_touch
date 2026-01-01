#include <stdio.h>
#include "basic/jlc_lcd.h"
#include "demos/lv_demos.h"
#include "esp_littlefs.h"
#include "driver/gpio.h"
#include "basic/hardware/hw_key.h"

// 按键GPIO定义
#define BOOT_KEY_GPIO     GPIO_NUM_0   // 左上方按键
#define HOME_KEY_GPIO     GPIO_NUM_39  // 右上方按键
#define KEY_PRESS_LEVEL   1            // 按键按下时的电平（高电平）


static void init_littlefs(void)
{
    static const char *TAG = "LittleFS";
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
            ESP_LOGE(TAG, "Failed to mount or format filesystem.");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find littlefs partition.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize littlefs (%s).", esp_err_to_name(ret));
        }
        return;
    }
    size_t total = 0, used = 0;
    esp_littlefs_info("storage", &total, &used);
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d.", total, used);
}

bool init_gpio(void)
{
    static const char *TAG = "GPIO";
    
    // 安装GPIO ISR服务
    gpio_install_isr_service(0);
    
    typedef struct {
        int pin;
        gpio_mode_t mode;
        gpio_pullup_t pull_up;
        gpio_pulldown_t pull_down;
        gpio_int_type_t intr_type;
    } gpio_init_t;
    gpio_init_t gpios[] = {
        {40, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_DISABLE},  // 电源控制
        {0, GPIO_MODE_INPUT, GPIO_PULLUP_ENABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_ANYEDGE},    // BOOT按键 (GPIO0)
        {39, GPIO_MODE_INPUT, GPIO_PULLUP_ENABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_ANYEDGE},   // HOME按键 (GPIO39)
    };

    gpio_config_t io_conf = {0}; // 初始化为0
    for (int i = 0; i < sizeof(gpios) / sizeof(gpios[0]); i++) {
        io_conf.pin_bit_mask = BIT64(gpios[i].pin);
        io_conf.mode = gpios[i].mode;
        io_conf.pull_up_en = gpios[i].pull_up;
        io_conf.pull_down_en = gpios[i].pull_down;
        io_conf.intr_type = gpios[i].intr_type;
        if (gpio_config(&io_conf) != ESP_OK) {
            ESP_LOGE(TAG, "GPIO%d init failed", gpios[i].pin);
            return false;
        }
        ESP_LOGI(TAG, "GPIO%d initialized", gpios[i].pin);
    }
    
    return true;
}

/**
 * 按键事件回调示例：
 * - evt == KEY_EVT_PRESS  表示按下事件
 * - evt == KEY_EVT_RELEASE 表示释放事件
 *
 * 你可以在回调中执行相应处理（例如：更新 UI、发送事件、长按检测等）。
 */
static void key_event_handler(int gpio, key_event_t evt, void* arg)
{
    if (evt == KEY_EVT_PRESS) {
        ESP_LOGI("KEY", "GPIO%d pressed", gpio);
    } else {
        ESP_LOGI("KEY", "GPIO%d released", gpio);
    }
}

/*
 * 使用说明（示例流程）：
 * 1. 在系统启动时调用 hw_key_init()
 * 2. 使用 hw_key_add(GPIO, active_level) 添加需要管理的按键
 * 3. 使用 hw_key_register_callback(GPIO, cb, arg) 注册响应回调
 *
 * 注意：hw_key 使用中断 + 队列 + 后台任务的方式，回调在任务上下文中被调用，
 * 因此可执行相对复杂的操作（但仍应避免长时间阻塞以影响其他任务）。
 */

void app_main(void)
{
    init_gpio();
    gpio_set_level(40, 1); // 打开电源
    bsp_i2c_init();   // I2C初始化

    // 初始化按键模块并注册回调
    if (!hw_key_init()) {
        ESP_LOGE("KEY", "Key module init failed");
    } else {
        hw_key_add(BOOT_KEY_GPIO, KEY_PRESS_LEVEL);
        hw_key_add(HOME_KEY_GPIO, KEY_PRESS_LEVEL);
        hw_key_register_callback(BOOT_KEY_GPIO, key_event_handler, NULL);
        hw_key_register_callback(HOME_KEY_GPIO, key_event_handler, NULL);
    }

    init_littlefs();  // 初始化文件系统
    bsp_lvgl_start(); // 初始化液晶屏lvgl接口

    /* 下面5个demos 只打开1个运行 */
    //lv_demo_benchmark();
    // lv_demo_keypad_encoder();
    lv_demo_music();
    // lv_demo_stress();
    // lv_demo_widgets();
}

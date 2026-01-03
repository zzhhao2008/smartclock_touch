#include <stdio.h>
#include "basic/jlc_lcd.h"
#include "demos/lv_demos.h"
#include "esp_littlefs.h"
#include "driver/gpio.h"
#include "basic/hardware/hw_key.h"
#include "basic/pm.h"
#include "nvs_flash.h"
#include "ui/app_wifi_ui.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "basic/beepdrive.h"

static const char *TAG = "MAPP";
struct key_log
{
    TickType_t presstime;
} HOME_KEY_LOG, PW_KEY_LOG;

struct sysStatus
{
    bool wifi_connected;
    bool charger_connected;
    bool battery_charging;
    float battery_voltage;
    float battery_percentage;
    float usb_voltage;
    bool is_charging;
    int screen_brightness;
    bool screen_on;
} sys_status;
typedef struct sysStatus sysStatus_t;
typedef struct key_log key_log_t;

// 按键GPIO定义
#define HOME_KEY_GPIO GPIO_NUM_0 // 左上方按键
#define PW_KEY_GPIO GPIO_NUM_39  // 右上方按键
#define KEY_PRESS_LEVEL 1        // 按键按下时的电平（高电平）
void init_littlefs(void)
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

void init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}
bool init_gpio(void)
{
    static const char *TAG = "GPIO";

    // 安装GPIO ISR服务
    gpio_install_isr_service(0);

    typedef struct
    {
        int pin;
        gpio_mode_t mode;
        gpio_pullup_t pull_up;
        gpio_pulldown_t pull_down;
        gpio_int_type_t intr_type;
    } gpio_init_t;
    gpio_init_t gpios[] = {
        {40, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_DISABLE}, // 电源控制
        {0, GPIO_MODE_INPUT, GPIO_PULLUP_ENABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_ANYEDGE},    // BOOT按键 (GPIO0)
        {39, GPIO_MODE_INPUT, GPIO_PULLUP_ENABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_ANYEDGE},   // HOME按键 (GPIO39)
        {41, GPIO_MODE_INPUT, GPIO_PULLUP_ENABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_DISABLE},   // 充电检测
    };

    gpio_config_t io_conf = {0}; // 初始化为0
    for (int i = 0; i < sizeof(gpios) / sizeof(gpios[0]); i++)
    {
        io_conf.pin_bit_mask = BIT64(gpios[i].pin);
        io_conf.mode = gpios[i].mode;
        io_conf.pull_up_en = gpios[i].pull_up;
        io_conf.pull_down_en = gpios[i].pull_down;
        io_conf.intr_type = gpios[i].intr_type;
        if (gpio_config(&io_conf) != ESP_OK)
        {
            ESP_LOGE(TAG, "GPIO%d init failed", gpios[i].pin);
            return false;
        }
        ESP_LOGI(TAG, "GPIO%d initialized", gpios[i].pin);
    }

    return true;
}
/**
 * 默认按键事件回调：
 * - evt == KEY_EVT_PRESS  表示按下事件
 * - evt == KEY_EVT_RELEASE 表示释放事件
 */
void key_event_handler(int gpio, key_event_t evt, void *arg)
{
    if (evt == KEY_EVT_PRESS)
    {
        ESP_LOGI("KEY", "GPIO%d pressed", gpio);
    }
    else
    {
        ESP_LOGI("KEY", "GPIO%d released", gpio);
    }

    if (gpio == PW_KEY_GPIO && evt == KEY_EVT_PRESS)
    {
        PW_KEY_LOG.presstime = xTaskGetTickCount();
    }

    if (gpio == PW_KEY_GPIO && evt == KEY_EVT_RELEASE)
    {
        TickType_t releasetime = xTaskGetTickCount();
        TickType_t duration = releasetime - PW_KEY_LOG.presstime;

        if (duration >= pdMS_TO_TICKS(10000)) // 长按3秒以上
        {
            ESP_LOGI("KEY", "Power off triggered by long press");
            ACC(0); // 关闭电源
        }else{
            if(sys_status.screen_on){
                sys_status.screen_on = false;
                ESP_LOGI("KEY", "Screen turned off");
                //bsp_display_backlight_off();
            }else{
                sys_status.screen_on = true;
                ESP_LOGI("KEY", "Screen turned on");
                //bsp_display_backlight_on();
            }
        }
    }
}

void default_key_cbReg()
{
    hw_key_register_callback(PW_KEY_GPIO, key_event_handler, NULL);
    hw_key_register_callback(HOME_KEY_GPIO, key_event_handler, NULL);
}

void app_main(void)
{
    init_gpio();
    ACC(1);         // 使能电源
    bsp_i2c_init(); // I2C初始化
    init_adc();

    // 初始化按键模块并注册回调
    if (!hw_key_init())
    {
        ESP_LOGE("KEY", "Key module init failed");
    }
    else
    {
        hw_key_add(PW_KEY_GPIO, KEY_PRESS_LEVEL);
        hw_key_add(HOME_KEY_GPIO, KEY_PRESS_LEVEL);
        default_key_cbReg();
    }

    ACC(1);

    ESP_LOGI(TAG, "Initializing buzzer...");
    beep_init();                   // 初始化蜂鸣器
    vTaskDelay(pdMS_TO_TICKS(10)); // 稍等一下确保初始化完成

    ESP_LOGI(TAG, "Playing startup sound...");
    esp_err_t result = play_note_async(1500, 200); // 播放启动音，1kHz，持续500ms
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to play startup sound: %s", esp_err_to_name(result));
    }
    else
    {
        ESP_LOGI(TAG, "Startup sound played successfully");
    }

    init_littlefs(); // 初始化文件系统
    init_nvs();
    bsp_lvgl_start(); // 初始化液晶屏lvgl接口

    app_wifi_connect(); // 运行wifi连接程序
}

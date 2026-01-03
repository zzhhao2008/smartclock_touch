#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define TAG "PM_D"

// GPIO定义
#define BAT_ADC_GPIO    2   // 电池检测GPIO
#define USB_ADC_GPIO    1   // USB检测GPIO
#define CHG_PIN         41  // 充电检测引脚
#define ACC_PIN         40  // ACC控制引脚

// ADC配置
#define ADC_UNIT        ADC_UNIT_1
#define ADC_ATTEN       ADC_ATTEN_DB_12
#define ADC_WIDTH       ADC_BITWIDTH_DEFAULT  // 使用默认位宽

static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t adc_cali_handle = NULL;

void ACC(int level)
{
    gpio_set_level(ACC_PIN, level);
    ESP_LOGI(TAG, "ACC: %d", level);
}

static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    // 使用曲线拟合校准方案
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = ADC_CHANNEL_2,  // 任意通道，校准是针对整个ADC单元的
        .atten = atten,
        .bitwidth = ADC_WIDTH,
    };
    
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
        calibrated = true;
        ESP_LOGI(TAG, "ADC calibration initialized using curve fitting");
    }

    *out_handle = handle;
    if (!calibrated) {
        ESP_LOGW(TAG, "ADC calibration failed. Using default values");
    }
    return calibrated;
}

void init_adc(void)
{
    // 配置ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    // 配置电池检测通道 (GPIO2 = ADC1_CH2)
    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_WIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_2, &channel_config));
    
    // 配置USB检测通道 (GPIO1 = ADC1_CH1)
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_1, &channel_config));

    // 初始化ADC校准
    adc_calibration_init(ADC_UNIT, ADC_ATTEN, &adc_cali_handle);

    // 配置GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ACC_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // 配置充电检测引脚为输入模式
    io_conf.pin_bit_mask = (1ULL << CHG_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    // 初始化ACC为低电平
    gpio_set_level(ACC_PIN, 0);
}

float read_bat_voltage(void)
{
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_2, &raw));  // 修正函数名
    
    int voltage = 0;
    if (adc_cali_handle) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage));
    } else {
        // 没有校准时使用默认计算 (3.3V参考电压, 12位精度)
        voltage = (raw * 3300) / 4095;
    }
    
    ESP_LOGD(TAG, "BAT raw: %d, voltage: %d mV", raw, voltage);
    return voltage / 1000.0f;
}

float read_bat_percentage(void)
{
    float voltage = read_bat_voltage();
    float percentage;
    
    // 电池电压范围: 3.0V (0%) to 4.2V (100%)
    if (voltage >= 4.2f) {
        percentage = 100.0f;
    } else if (voltage <= 3.0f) {
        percentage = 0.0f;
    } else {
        percentage = (voltage - 3.0f) / (4.2f - 3.0f) * 100.0f;
    }
    
    ESP_LOGI(TAG, "BAT voltage: %.2f V, percentage: %.1f%%", voltage, percentage);
    return percentage;
}

float read_usb_voltage(void)
{
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_1, &raw));  // 修正函数名
    
    int voltage = 0;
    if (adc_cali_handle) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage));
    } else {
        voltage = (raw * 3300) / 4095;
    }
    
    ESP_LOGD(TAG, "USB raw: %d, voltage: %d mV", raw, voltage);
    return voltage / 1000.0f;
}

bool is_charger_connected(void)
{
    float voltage = read_usb_voltage();
    bool connected = (voltage > 4.5f);
    ESP_LOGD(TAG, "USB voltage: %.2f V, charger connected: %s", voltage, connected ? "yes" : "no");
    return connected;
}

bool is_charging(void)
{
    int level = gpio_get_level(CHG_PIN);
    ESP_LOGD(TAG, "CHG pin level: %d", level);
    return (level == 0); // 低电平表示正在充电
}
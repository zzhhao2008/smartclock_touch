/**
 * 蜂鸣器驱动程序
 * 使用LEDC模块控制蜂鸣器发声
 * 蜂鸣器连接到GPIO 42
 * PWM驱动 最大占空比50%
 * 频率范围 200Hz - 2.7kHz
 * 闲置时置低电平
 */
#include "beepdrive.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define BEEP_GPIO           42
#define BEEP_LEDC_CHANNEL   LEDC_CHANNEL_1
#define BEEP_LEDC_TIMER     LEDC_TIMER_0
#define BEEP_LEDC_MODE      LEDC_LOW_SPEED_MODE
#define BEEP_LEDC_DUTY      2048            // 占空比 50% (4096的50%)
#define BEEP_LEDC_FREQ_MIN  200             // 最小频率 200Hz
#define BEEP_LEDC_FREQ_MAX  2700            // 最大频率 2.7kHz

static const char *TAG = "BEEP";
static bool beep_initialized = false;
static SemaphoreHandle_t beep_mutex = NULL;

esp_err_t beep_init(void)
{
    if (beep_initialized) {
        ESP_LOGW(TAG, "Beep already initialized");
        return ESP_OK;
    }

    // 创建互斥锁
    beep_mutex = xSemaphoreCreateMutex();
    if (beep_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = BEEP_LEDC_MODE,
        .timer_num        = BEEP_LEDC_TIMER,
        .duty_resolution  = LEDC_TIMER_12_BIT,
        .freq_hz          = BEEP_LEDC_FREQ_MAX,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_timer_config failed: %s", esp_err_to_name(ret));
        vSemaphoreDelete(beep_mutex);
        beep_mutex = NULL;
        return ret;
    }

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = BEEP_LEDC_MODE,
        .channel        = BEEP_LEDC_CHANNEL,
        .timer_sel      = BEEP_LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = BEEP_GPIO,
        .duty           = 0, // 初始占空比为0
        .hpoint         = 0,
    };
    ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_channel_config failed: %s", esp_err_to_name(ret));
        vSemaphoreDelete(beep_mutex);
        beep_mutex = NULL;
        return ret;
    }

    // 初始状态为停止
    ret = ledc_set_duty(BEEP_LEDC_MODE, BEEP_LEDC_CHANNEL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_set_duty failed: %s", esp_err_to_name(ret));
        vSemaphoreDelete(beep_mutex);
        beep_mutex = NULL;
        return ret;
    }
    ret = ledc_update_duty(BEEP_LEDC_MODE, BEEP_LEDC_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_update_duty failed: %s", esp_err_to_name(ret));
        vSemaphoreDelete(beep_mutex);
        beep_mutex = NULL;
        return ret;
    }

    beep_initialized = true;
    ESP_LOGI(TAG, "Beep initialized on GPIO %d", BEEP_GPIO);
    return ESP_OK;
}

static esp_err_t beep_acquire_mutex(TickType_t wait_ticks)
{
    if (beep_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(beep_mutex, wait_ticks) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

static void beep_release_mutex(void)
{
    if (beep_mutex != NULL) {
        xSemaphoreGive(beep_mutex);
    }
}

esp_err_t beep_set_freq(uint16_t freq)
{
    if (!beep_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = beep_acquire_mutex(pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        return ret;
    }

    do {
        if (freq < BEEP_LEDC_FREQ_MIN || freq > BEEP_LEDC_FREQ_MAX) {
            ESP_LOGE(TAG, "Invalid frequency: %d Hz (range: %d-%d Hz)", 
                    freq, BEEP_LEDC_FREQ_MIN, BEEP_LEDC_FREQ_MAX);
            ret = ESP_ERR_INVALID_ARG;
            break;
        }
        
        ret = ledc_set_freq(BEEP_LEDC_MODE, BEEP_LEDC_TIMER, freq);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ledc_set_freq failed: %s", esp_err_to_name(ret));
            break;
        }
        
        ret = ledc_set_duty(BEEP_LEDC_MODE, BEEP_LEDC_CHANNEL, BEEP_LEDC_DUTY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ledc_set_duty failed: %s", esp_err_to_name(ret));
            break;
        }
        
        ret = ledc_update_duty(BEEP_LEDC_MODE, BEEP_LEDC_CHANNEL);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ledc_update_duty failed: %s", esp_err_to_name(ret));
            break;
        }
        
        ESP_LOGD(TAG, "Beep frequency set to %d Hz", freq);
    } while (0);

    beep_release_mutex();
    return ret;
}

esp_err_t beep_stop(void)
{
    if (!beep_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = beep_acquire_mutex(pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        return ret;
    }

    do {
        ret = ledc_set_duty(BEEP_LEDC_MODE, BEEP_LEDC_CHANNEL, 0);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ledc_set_duty failed: %s", esp_err_to_name(ret));
            break;
        }
        
        ret = ledc_update_duty(BEEP_LEDC_MODE, BEEP_LEDC_CHANNEL);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ledc_update_duty failed: %s", esp_err_to_name(ret));
            break;
        }
        
        ESP_LOGD(TAG, "Beep stopped");
    } while (0);

    beep_release_mutex();
    return ret;
}

static void beep_stop_async_task(void *arg)
{
    int delay_ms = *((int *)arg);
    free(arg); // 释放分配的内存
    
    // 延时一段时间后停止蜂鸣器
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    
    esp_err_t ret = beep_stop();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "beep_stop failed in async task: %s", esp_err_to_name(ret));
    }
    
    vTaskDelete(NULL);
}

esp_err_t play_note(uint16_t freq, uint32_t duration_ms)
{
    if (!beep_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = beep_acquire_mutex(pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        return ret;
    }

    do {
        ret = beep_set_freq(freq);
        if (ret != ESP_OK) {
            break;
        }
        
        // 释放锁，允许其他任务运行
        beep_release_mutex();
        
        // 播放指定时长
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        
        // 重新获取锁
        ret = beep_acquire_mutex(pdMS_TO_TICKS(100));
        if (ret != ESP_OK) {
            return ret;
        }
        
        ret = beep_stop();
    } while (0);

    beep_release_mutex();
    return ret;
}

esp_err_t play_note_async(uint16_t freq, uint32_t duration_ms)
{
    if (!beep_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = beep_acquire_mutex(pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        return ret;
    }

    do {
        ret = beep_set_freq(freq);
        if (ret != ESP_OK) {
            break;
        }
        
        // 释放锁，避免阻塞异步任务
        beep_release_mutex();
        
        // 创建一个任务在duration_ms后停止蜂鸣器
        int *delay_arg = malloc(sizeof(int));
        if (delay_arg == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for delay_arg");
            return ESP_ERR_NO_MEM;
        }
        *delay_arg = duration_ms;
        
        if (xTaskCreate(beep_stop_async_task, "beep_stop", 1024, delay_arg, 2, NULL) != pdPASS) {
            free(delay_arg);
            ESP_LOGE(TAG, "Failed to create async task");
            return ESP_ERR_NO_MEM;
        }
    } while (0);

    if (ret != ESP_OK) {
        beep_release_mutex();
    }
    return ret;
}

esp_err_t beep_deinit(void)
{
    if (!beep_initialized) {
        return ESP_OK;
    }

    esp_err_t ret = beep_acquire_mutex(pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        return ret;
    }

    // 停止蜂鸣器
    beep_stop();
    
    // 释放资源
    if (beep_mutex != NULL) {
        vSemaphoreDelete(beep_mutex);
        beep_mutex = NULL;
    }
    
    beep_initialized = false;
    ESP_LOGI(TAG, "Beep deinitialized");
    
    beep_release_mutex();
    return ESP_OK;
}
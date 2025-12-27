#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"

#include "lvgl.h"
#include "lib/touch.h"
#include "lib/lvgl_display.h"

#include "st7789.h"
#include "fontx.h"

#define INTERVAL 100
#define WAIT vTaskDelay(INTERVAL)

static const char *TAG = "MAIN";

// You have to set these CONFIG value using menuconfig.
#define CONFIG_WIDTH 320  // Horizontal resolution
#define CONFIG_HEIGHT 240 // Vertical resolution
#define CONFIG_MOSI_GPIO 45
#define CONFIG_SCLK_GPIO 3
#define CONFIG_CS_GPIO 14
#define CONFIG_DC_GPIO 47
#define CONFIG_RESET_GPIO 21
#define CONFIG_BL_GPIO 5
#define CONFIG_OFFSETX 0
#define CONFIG_OFFSETY 0
#define CONFIG_INVERSION 1

void traceHeap()
{
    static uint32_t _free_heap_size = 0;
    if (_free_heap_size == 0)
        _free_heap_size = esp_get_free_heap_size();

    int _diff_free_heap_size = _free_heap_size - esp_get_free_heap_size();
    ESP_LOGI(__FUNCTION__, "_diff_free_heap_size=%d", _diff_free_heap_size);
    ESP_LOGI(__FUNCTION__, "esp_get_free_heap_size() : %6" PRIu32 "\n", esp_get_free_heap_size());
}

static void listSPIFFS(char *path)
{
    DIR *dir = opendir(path);
    assert(dir != NULL);
    while (true)
    {
        struct dirent *pe = readdir(dir);
        if (!pe)
            break;
        ESP_LOGI(__FUNCTION__, "d_name=%s d_ino=%d d_type=%x", pe->d_name, pe->d_ino, pe->d_type);
    }
    closedir(dir);
}

esp_err_t mountSPIFFS(char *path, char *label, int max_files)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = path,
        .partition_label = label,
        .max_files = max_files,
        .format_if_mount_failed = true};

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "Mount %s to %s success", path, label);
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ret;
}

void lv_tick_task(void *pvParameter)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(5));
        lv_tick_inc(5);
    }
}

void lvgl_demo_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting LVGL demo");

    // Create a simple demo screen
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_scr_load(scr);

    // Create a label
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "Hello LVGL 8.3.1!\nHorizontal Mode");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // Create a button
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Touch Me");

    while (1)
    {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");
    ESP_ERROR_CHECK(mountSPIFFS("/fs", "MFS", 7));
    listSPIFFS("/fs/");

    // Initialize LVGL
    lv_init();

    // Initialize display in horizontal mode
    lvgl_display_init();

    // Initialize touch with error handling
    esp_err_t touch_ret = touch_init();
    if (touch_ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Touch initialization failed (0x%x). Continuing without touch.", touch_ret);
    }

    // Initialize touch input device only if touch init was successful
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read;
    lv_indev_t *indev = lv_indev_drv_register(&indev_drv);

    if (indev == NULL && touch_ret == ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register touch input device");
    }

    // Create LVGL tick task
    xTaskCreate(lv_tick_task, "lv_tick", 2048, NULL, 2, NULL);

    // Create LVGL demo task
    xTaskCreate(lvgl_demo_task, "lvgl_demo", 4096, NULL, 3, NULL);

    ESP_LOGI(TAG, "LVGL initialization complete");
}
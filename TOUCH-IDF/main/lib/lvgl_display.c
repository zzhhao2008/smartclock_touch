#include "lvgl.h"
#include "st7789.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "LVGL_DISPLAY";

#define LVGL_HRES 320  // Horizontal resolution for landscape mode
#define LVGL_VRES 240  // Vertical resolution for landscape mode

static TFT_t dev;
static lv_disp_drv_t disp_drv;
static lv_disp_t *disp;

static void st7789_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    // For horizontal orientation, coordinates need to be adjusted
    uint16_t x1 = area->x1;
    uint16_t y1 = area->y1;
    uint16_t x2 = area->x2;
    uint16_t y2 = area->y2;
    
    lcdSetAddrWindow(&dev, x1, y1, x2, y2);
    lcdWriteBitmap(&dev, (uint8_t*)color_map, w, h);
    
    lv_disp_flush_ready(drv);
}

static void st7789_rounder(lv_disp_drv_t *disp_drv, lv_area_t *area)
{
    // Round coordinates to multiples of 2 for better performance
    area->x1 = area->x1 & 0xFFFE;
    area->y1 = area->y1 & 0xFFFE;
    area->x2 = area->x2 | 0x0001;
    area->y2 = area->y2 | 0x0001;
}

static void st7789_set_px(lv_disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa)
{
    // Custom pixel setting function (optional)
    // Not needed for ST7789 as it handles full area writes efficiently
}

void lvgl_display_init(void)
{
    // Initialize ST7789 for horizontal orientation
    spi_clock_speed(40000000);
    spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, 
                   CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
    
    // For horizontal orientation: width=320, height=240
    lcdInit(&dev, LVGL_HRES, LVGL_VRES, 0, 0);
    lcdInversionOn(&dev);

    // Backlight on
    lcdBacklightOn(&dev);

    //显示一个空白屏幕
    lcdFillScreen(&dev, WHITE);

    //等待1s
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Initialize LVGL display driver
    lv_disp_drv_init(&disp_drv);
    
    // Set resolution for horizontal mode
    disp_drv.hor_res = LVGL_HRES;
    disp_drv.ver_res = LVGL_VRES;
    
    // Set functions
    disp_drv.flush_cb = st7789_flush;
    disp_drv.rounder_cb = st7789_rounder;
    // disp_drv.set_px_cb = st7789_set_px; // Optional
    
    // Create the display buffer
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[LVGL_HRES * 40];  // Buffer for 40 lines
    
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, LVGL_HRES * 40);
    disp_drv.draw_buf = &draw_buf;
    
    // Register the display driver
    disp = lv_disp_drv_register(&disp_drv);
    
    if (disp == NULL) {
        ESP_LOGE(TAG, "Failed to register display driver");
        return;
    }
    
    ESP_LOGI(TAG, "LVGL display initialized successfully in horizontal mode");
}

lv_disp_t* lvgl_get_display(void)
{
    return disp;
}
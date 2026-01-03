#pragma once

#include <string.h>
#include "math.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_types.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch_ft5x06.h"
#include "esp_lvgl_port.h"

/******************************************************************************/
/***************************  I2C ↓ *******************************************/
#define BSP_I2C_SDA (GPIO_NUM_11) // SDA引脚
#define BSP_I2C_SCL (GPIO_NUM_9) // SCL引脚

#define BSP_I2C_NUM (0)        // I2C外设
#define BSP_I2C_FREQ_HZ 100000 // 100kHz

esp_err_t bsp_i2c_init(void); // 初始化I2C接口
/***************************  I2C ↑  *******************************************/
/*******************************************************************************/

/***********************************************************/
/****************    LCD显示屏 ↓   *************************/
#define BSP_LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define BSP_LCD_SPI_NUM (SPI3_HOST)
#define LCD_CMD_BITS (8)
#define LCD_PARAM_BITS (8)
#define BSP_LCD_BITS_PER_PIXEL (16)
#define LCD_LEDC_CH LEDC_CHANNEL_0

#define BSP_LCD_H_RES (320)
#define BSP_LCD_V_RES (240)

#define BSP_LCD_SPI_MOSI (GPIO_NUM_17)
#define BSP_LCD_SPI_CLK (GPIO_NUM_18)
#define BSP_LCD_DC (GPIO_NUM_15)
#define BSP_LCD_RST (GPIO_NUM_7)
#define BSP_LCD_BACKLIGHT (GPIO_NUM_8)
#define BSP_LCD_SPI_CS (GPIO_NUM_6)

#define BSP_LCD_DRAW_BUF_HEIGHT (20)

#define LCD_FADE_TIME_MS 500            // 默认渐变时间500ms
#define LCD_FADE_MODE LEDC_FADE_NO_WAIT // 非阻塞模式

esp_err_t bsp_display_brightness_init(void);
esp_err_t bsp_display_brightness_set(int brightness_percent);
esp_err_t bsp_display_brightness_fade(int target_brightness_percent, int fade_time_ms);
esp_err_t bsp_display_backlight_off(void);
esp_err_t bsp_display_backlight_on(void);
esp_err_t bsp_lcd_init(void);
void lcd_set_color(uint16_t color);
void lcd_draw_pictrue(int x_start, int y_start, int x_end, int y_end, const unsigned char *gImage);
void bsp_lvgl_start(void);
/***************    LCD显示屏 ↑   *************************/
/***********************************************************/

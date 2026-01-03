#include <stdio.h>
#include "jlc_lcd.h"


static const char *TAG = "LCDI";

/******************************************************************************/
/***************************  I2C ↓ *******************************************/
esp_err_t bsp_i2c_init(void)
{
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BSP_I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = BSP_I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = BSP_I2C_FREQ_HZ
    };
    i2c_param_config(BSP_I2C_NUM, &i2c_conf);

    return i2c_driver_install(BSP_I2C_NUM, i2c_conf.mode, 0, 0, 0);
}
/***************************  I2C ↑  *******************************************/
/*******************************************************************************/

/***********************************************************/
/****************    LCD显示屏 ↓   *************************/

// 背光PWM初始化
esp_err_t bsp_display_brightness_init(void)
{
    // Setup LEDC peripheral for PWM backlight control
    const ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = BSP_LCD_BACKLIGHT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LCD_LEDC_CH,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = 0,
        .duty = 0,
        .hpoint = 0,
        .flags.output_invert = false
    };
    const ledc_timer_config_t LCD_backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = 0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&LCD_backlight_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&LCD_backlight_channel));

    return ESP_OK;
}

// 背光亮度设置
esp_err_t bsp_display_brightness_set(int brightness_percent)
{
    if (brightness_percent > 100) {
        brightness_percent = 100;
    } else if (brightness_percent < 0) {
        brightness_percent = 0;
    }

    ESP_LOGI(TAG, "Setting LCD backlight: %d%%", brightness_percent);
    // LEDC resolution set to 10bits, thus: 100% = 1023
    uint32_t duty_cycle = (1023 * brightness_percent) / 100;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LCD_LEDC_CH, duty_cycle));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LCD_LEDC_CH));

    return ESP_OK;
}

// 关闭背光
esp_err_t bsp_display_backlight_off(void)
{
    return bsp_display_brightness_set(0);
}

// 打开背光 最亮
esp_err_t bsp_display_backlight_on(void)
{
    return bsp_display_brightness_set(100);
}

// 定义液晶屏句柄
static esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_panel_io_handle_t io_handle = NULL; 
static esp_lcd_touch_handle_t tp;   // 触摸屏句柄
static lv_disp_t *disp;      // 指向液晶屏
static lv_indev_t *disp_indev = NULL; // 指向触摸屏

// 液晶屏初始化
esp_err_t bsp_display_new(void)
{
    esp_err_t ret = ESP_OK;
    // 背光初始化
    ESP_RETURN_ON_ERROR(bsp_display_brightness_init(), TAG, "Brightness init failed");
    // 初始化SPI总线
    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .sclk_io_num = BSP_LCD_SPI_CLK,
        .mosi_io_num = BSP_LCD_SPI_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = BSP_LCD_H_RES * BSP_LCD_V_RES * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(BSP_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");
    // 液晶屏控制IO初始化
    ESP_LOGI(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = BSP_LCD_DC,
        .cs_gpio_num = BSP_LCD_SPI_CS,
        .pclk_hz = BSP_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 2,
        .trans_queue_depth = 10,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_NUM, &io_config, &io_handle), err, TAG, "New panel IO failed");
    // 初始化液晶屏驱动芯片ST7789
    ESP_LOGD(TAG, "Install LCD driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = BSP_LCD_BITS_PER_PIXEL,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle), err, TAG, "New panel failed");
    
    esp_lcd_panel_reset(panel_handle);  // 液晶屏复位
    esp_lcd_panel_init(panel_handle);  // 初始化配置寄存器
    esp_lcd_panel_invert_color(panel_handle, true); // 颜色反转
    esp_lcd_panel_swap_xy(panel_handle, true);  // 显示翻转 
    esp_lcd_panel_mirror(panel_handle, false, true); // 镜像

    return ret;

err:
    if (panel_handle) {
        esp_lcd_panel_del(panel_handle);
    }
    if (io_handle) {
        esp_lcd_panel_io_del(io_handle);
    }
    spi_bus_free(BSP_LCD_SPI_NUM);
    return ret;
}

// LCD显示初始化
esp_err_t bsp_lcd_init(void)
{
    esp_err_t ret = ESP_OK;

    
    ret = bsp_display_new(); // 液晶屏驱动初始化
    lcd_set_color(0x0000); // 设置整屏背景黑色
    ret = esp_lcd_panel_disp_on_off(panel_handle, true); // 打开液晶屏显示
    ret = bsp_display_backlight_on(); // 打开背光显示

    return  ret;
}

// 液晶屏初始化+添加LVGL接口
static lv_disp_t *bsp_display_lcd_init(void)
{
    /* 初始化液晶屏 */
    bsp_display_new(); // 液晶屏驱动初始化
    lcd_set_color(0xffff); // 设置整屏背景白色
    esp_lcd_panel_disp_on_off(panel_handle, true); // 打开液晶屏显示

    /* 液晶屏添加LVGL接口 */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = BSP_LCD_H_RES * BSP_LCD_DRAW_BUF_HEIGHT,   // LVGL缓存大小 
        .double_buffer = false, // 是否开启双缓存
        .hres = BSP_LCD_H_RES, // 液晶屏的宽
        .vres = BSP_LCD_V_RES, // 液晶屏的高
        .monochrome = false,  // 是否单色显示器
        /* Rotation的值必须和液晶屏初始化里面设置的 翻转 和 镜像 一样 */
        .rotation = {
            .swap_xy = true,  // 是否翻转
            .mirror_x = true, // x方向是否镜像
            .mirror_y = false, // y方向是否镜像
        },
        .flags = {
            .buff_dma = false,  // 是否使用DMA 注意：dma与spiram不能同时为true
            .buff_spiram = true, // 是否使用PSRAM 注意：dma与spiram不能同时为true
        }
    };

    return lvgl_port_add_disp(&disp_cfg);
}

// 触摸屏初始化
esp_err_t bsp_touch_new(esp_lcd_touch_handle_t *ret_touch)
{
    /* Initialize touch */
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_V_RES,
        .y_max = BSP_LCD_H_RES,
        .rst_gpio_num = GPIO_NUM_NC, // Shared with LCD reset
        .int_gpio_num = GPIO_NUM_NC, 
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 1,
            .mirror_x = 0,
            .mirror_y = 1,
        },
    };
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();

    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)BSP_I2C_NUM, &tp_io_config, &tp_io_handle), TAG, "");
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, ret_touch));

    return ESP_OK;
}

// 触摸屏初始化+添加LVGL接口
static lv_indev_t *bsp_display_indev_init(lv_disp_t *disp)
{
    /* 初始化触摸屏 */
    ESP_ERROR_CHECK(bsp_touch_new(&tp));
    assert(tp);

    /* 添加LVGL接口 */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = tp,
    };

    return lvgl_port_add_touch(&touch_cfg);
}

// 开发板显示初始化
void bsp_lvgl_start(void)
{
    /* 初始化LVGL */
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    /* 初始化液晶屏 并添加LVGL接口 */
    disp = bsp_display_lcd_init();

    /* 初始化触摸屏 并添加LVGL接口 */
    disp_indev = bsp_display_indev_init(disp);

    /* 打开液晶屏背光 */
    bsp_display_backlight_on();

}

// 显示图片
void lcd_draw_pictrue(int x_start, int y_start, int x_end, int y_end, const unsigned char *gImage)
{
    // 分配内存 分配了需要的字节大小 且指定在外部SPIRAM中分配
    size_t pixels_byte_size = (x_end - x_start)*(y_end - y_start) * 2;
    uint16_t *pixels = (uint16_t *)heap_caps_malloc(pixels_byte_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (NULL == pixels)
    {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
        return;
    }
    memcpy(pixels, gImage, pixels_byte_size);  // 把图片数据拷贝到内存
    esp_lcd_panel_draw_bitmap(panel_handle, x_start, y_start, x_end, y_end, (uint16_t *)pixels); // 显示整张图片数据
    heap_caps_free(pixels);  // 释放内存
}

// 设置液晶屏颜色
void lcd_set_color(uint16_t color)
{
    // 分配内存 这里分配了液晶屏一行数据需要的大小
    uint16_t *buffer = (uint16_t *)heap_caps_malloc(BSP_LCD_H_RES * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    
    if (NULL == buffer)
    {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
    }
    else
    {
        for (size_t i = 0; i < BSP_LCD_H_RES; i++) // 给缓存中放入颜色数据
        {
            buffer[i] = color;
        }
        for (int y = 0; y < 240; y++) // 显示整屏颜色
        {
            esp_lcd_panel_draw_bitmap(panel_handle, 0, y, 320, y+1, buffer);
        }
        free(buffer); // 释放内存
    }
}
/***************    LCD显示屏 ↑   *************************/
/***********************************************************/
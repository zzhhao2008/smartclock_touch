#ifndef __TOUCH_H__
#define __TOUCH_H__

#include "lvgl.h"
#include "esp_err.h"    // Add this to resolve esp_err_t type
#include "driver/i2c.h" // Add this for I2C defines

// I2C configuration for touch panel
#define I2C_MASTER_SCL_IO 42 // GPIO for I2C SCL (modify based on your hardware)
#define I2C_MASTER_SDA_IO 2  // GPIO for I2C SDA (modify based on your hardware)
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 36000 // I2C clock frequency

// FT6636U configuration
#define FT6636U_I2C_ADDR 0x38 // Default I2C address for FT6636U
#define TOUCH_INT_PIN -1       // GPIO for touch interrupt (optional)
#define TOUCH_RST_PIN -1       // GPIO for touch reset (optional)

typedef struct
{
    uint8_t id;
    uint16_t x;
    uint16_t y;
    uint8_t pressure;
} touch_point_t;

esp_err_t touch_init(void);
void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data);
bool touch_detect_device(void);

#endif // __TOUCH_H__
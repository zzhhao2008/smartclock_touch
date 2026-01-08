#include "ft6336u_driver.h"
#include "driver/i2c.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "esp_log.h"

#define TOUCH_I2C_PORT      -1

#define FT6336U_ADDR    0x38

static const char *TAG = "ft6336u";

//边界值
static uint16_t s_usLimitX = 0;
static uint16_t s_usLimitY = 0;

static i2c_master_bus_handle_t ft6336u_i2c_master = NULL;
static i2c_master_dev_handle_t ft6336u_i2c_device  =NULL;

/** CST816T初始化
 * @param cfg 配置
 * @return err
*/
esp_err_t   ft6336u_init(ft6336u_cfg_t* cfg)
{
    i2c_master_bus_config_t bus_config = 
    {
        .i2c_port = TOUCH_I2C_PORT,
        .sda_io_num = cfg->sda,
        .scl_io_num = cfg->scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .trans_queue_depth = 0,
    };
    
    i2c_new_master_bus(&bus_config,&ft6336u_i2c_master);
    
    i2c_device_config_t dev_config = 
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = FT6336U_ADDR,
        .scl_speed_hz = cfg->fre,
    };
    i2c_master_bus_add_device(ft6336u_i2c_master, &dev_config, &ft6336u_i2c_device);
    
    s_usLimitX = cfg->x_limit;
    s_usLimitY = cfg->y_limit;

    
    uint8_t read_data_buf[1];
    uint8_t write_data_buf[1];
    write_data_buf[0] = 0xA3;
    esp_err_t ret;
    bool touch_detected = false;
    ret = i2c_master_transmit_receive(ft6336u_i2c_device, &write_data_buf[0], 1, read_data_buf, 1, 200);
    if(ret == ESP_OK) touch_detected = true;
    ESP_LOGI(TAG, "\tChip ID: 0x%02x", read_data_buf[0]);

    write_data_buf[0] = 0xA6;
    ret = i2c_master_transmit_receive(ft6336u_i2c_device, &write_data_buf[0], 1, read_data_buf, 1, 200);
    if(ret == ESP_OK) touch_detected = true;
    ESP_LOGI(TAG, "\tFirmware version: 0x%02x", read_data_buf[0]);

    write_data_buf[0] = 0xA8;
    ret = i2c_master_transmit_receive(ft6336u_i2c_device, &write_data_buf[0], 1, read_data_buf, 1, 200);
    if(ret == ESP_OK) touch_detected = true;
    ESP_LOGI(TAG, "\tCTPM Vendor ID: 0x%02x", read_data_buf[0]);

    return touch_detected?ESP_OK:ESP_FAIL;
}

/** 读取坐标值
 * @param  x x坐标
 * @param  y y坐标
 * @param state 松手状态 0,松手 1按下
 * @return 无
*/
void ft6336u_read(int16_t *x,int16_t *y,int *state)
{
    uint8_t data_x[2];        // 2 bytesX
    uint8_t data_y[2];        // 2 bytesY
    
    static int16_t last_x = 0;  // 12bit pixel value
    static int16_t last_y = 0;  // 12bit pixel value
    uint8_t write_buf[1];
    uint8_t touch_pnt_cnt[1];        // Number of detected touch points

    write_buf[0] = 0x02;
    i2c_master_transmit_receive(ft6336u_i2c_device, &write_buf[0], 1, &touch_pnt_cnt[0], 1, 500);    
    if ((touch_pnt_cnt[0]&0x0f) != 1) {    // ignore no touch & multi touch
        *x = last_x;
        *y = last_y;
        *state = 0;
        return;
    }

    //读取X坐标
    write_buf[0] = 0x03;
    i2c_master_transmit_receive(ft6336u_i2c_device, &write_buf[0], 1, &data_x[0], 2, 500);

    //读取Y坐标
    write_buf[0] = 0x05;
    i2c_master_transmit_receive(ft6336u_i2c_device, &write_buf[0], 1, &data_y[0], 2, 500);

    int16_t current_x = ((data_x[0] & 0x0F) << 8) | (data_x[1] & 0xFF);
    int16_t current_y = ((data_y[0] & 0x0F) << 8) | (data_y[1] & 0xFF);

    if(last_x != current_x || current_y != last_y)
    {
        last_x = current_x;
        last_y = current_y;
        //ESP_LOGI(TAG,"touch x:%d,y:%d",last_x,last_y);
    }
    

    if(last_x >= s_usLimitX)
        last_x = s_usLimitX - 1;
    if(last_y >= s_usLimitY)
        last_y = s_usLimitY - 1;

    *x = last_x;
    *y = last_y;
    *state = 1;
}


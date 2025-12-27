#include "touch.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_rom_gpio.h"  // 包含正确的ROM GPIO函数

static const char *TAG = "FT6636U";

// Touch data registers
#define FT6636U_REG_NUM_TOUCHES  0x02
#define FT6636U_REG_TOUCH1_XH    0x03
#define FT6636U_REG_TOUCH1_XL    0x04
#define FT6636U_REG_TOUCH1_YH    0x05
#define FT6636U_REG_TOUCH1_YL    0x06
#define FT6636U_REG_DEVICE_MODE  0x00
#define FT6636U_REG_THRESHOLD    0x80
#define FT6636U_REG_VENDID       0xA8
#define FT6636U_REG_CHIPID       0xA3
#define FT6636U_REG_MODE         0x00

static esp_err_t i2c_master_init(void)
{
    ESP_LOGI(TAG, "Initializing I2C with internal pullups");
    
    // Use ROM GPIO functions for pin configuration
    esp_rom_gpio_pad_select_gpio(I2C_MASTER_SCL_IO);
    esp_rom_gpio_pad_select_gpio(I2C_MASTER_SDA_IO);
    
    // Set pin directions and pull modes
    gpio_set_direction(I2C_MASTER_SCL_IO, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_direction(I2C_MASTER_SDA_IO, GPIO_MODE_INPUT_OUTPUT_OD);
    
    gpio_set_pull_mode(I2C_MASTER_SCL_IO, GPIO_PULLUP_ONLY);  // Enable internal pull-up
    gpio_set_pull_mode(I2C_MASTER_SDA_IO, GPIO_PULLUP_ONLY);  // Enable internal pull-up
    
    ESP_LOGI(TAG, "Internal pull-ups enabled for SDA(%d) and SCL(%d)", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,    // Software pull-up enable
        .scl_pullup_en = GPIO_PULLUP_ENABLE,    // Software pull-up enable
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c param config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "I2C initialized successfully on SDA=%d, SCL=%d with internal pull-ups", 
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    return ESP_OK;
}

static esp_err_t ft6636u_read_register(uint8_t reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (FT6636U_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (FT6636U_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGD(TAG, "I2C read failed for reg 0x%02X: %s", reg, esp_err_to_name(ret));
    }
    
    return ret;
}

static esp_err_t ft6636u_write_register(uint8_t reg, uint8_t value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (FT6636U_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, value, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGD(TAG, "I2C write failed for reg 0x%02X: %s", reg, esp_err_to_name(ret));
    }
    
    return ret;
}

static void touch_reset_device(void)
{
    ESP_LOGI(TAG, "Resetting touch device on pin %d", TOUCH_RST_PIN);
    
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TOUCH_RST_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Reset sequence: active low reset
    gpio_set_level(TOUCH_RST_PIN, 0);  // Reset active
    vTaskDelay(pdMS_TO_TICKS(10));     // Hold reset for 10ms
    gpio_set_level(TOUCH_RST_PIN, 1);  // Release reset
    vTaskDelay(pdMS_TO_TICKS(100));    // Wait for device to stabilize
    
    ESP_LOGI(TAG, "Touch device reset complete");
}

static bool touch_verify_device(void)
{
    uint8_t chip_id = 0;
    uint8_t vendor_id = 0;
    uint8_t mode = 0;
    
    // Read chip ID
    esp_err_t ret = ft6636u_read_register(FT6636U_REG_CHIPID, &chip_id, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read chip ID");
        return false;
    }
    
    // Read vendor ID  
    ret = ft6636u_read_register(FT6636U_REG_VENDID, &vendor_id, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read vendor ID");
        return false;
    }
    
    // Read mode register
    ret = ft6636u_read_register(FT6636U_REG_MODE, &mode, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read mode register");
        return false;
    }
    
    ESP_LOGI(TAG, "FT6636U Chip ID: 0x%02X, Vendor ID: 0x%02X, Mode: 0x%02X", 
             chip_id, vendor_id, mode);
    
    // Common FT6636U chip ID values: 0x06, 0x11, 0x64
    if (chip_id == 0x06 || chip_id == 0x11 || chip_id == 0x64) {
        return true;
    } else {
        ESP_LOGW(TAG, "Unexpected chip ID 0x%02X, but continuing anyway", chip_id);
        return true; // Continue even if chip ID is unexpected
    }
}

esp_err_t touch_init(void)
{
    esp_err_t ret;
    
    // Initialize I2C with internal pull-ups
    ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed");
        return ret;
    }
    
    // Reset touch device
    if (TOUCH_RST_PIN >= 0) {
        touch_reset_device();
    }
    
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Verify device is responding
    if (!touch_verify_device()) {
        ESP_LOGE(TAG, "FT6636U verification failed! Please check:");
        ESP_LOGE(TAG, "1. Hardware connections: SDA=%d, SCL=%d, RST=%d", 
                 I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, TOUCH_RST_PIN);
        ESP_LOGE(TAG, "2. Internal pull-ups are enabled (no external resistors needed)");
        ESP_LOGE(TAG, "3. Touch controller has 3.3V power supply");
        ESP_LOGE(TAG, "4. I2C address 0x%02X is correct for your module", FT6636U_I2C_ADDR);
        return ESP_FAIL;
    }
    
    // Configure touch parameters
    ret = ft6636u_write_register(FT6636U_REG_THRESHOLD, 0x0F); // Touch threshold
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set threshold, continuing anyway");
    }
    
    // Set to normal operation mode
    ret = ft6636u_write_register(FT6636U_REG_MODE, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set normal mode");
    }
    
    // Configure interrupt mode if interrupt pin is available
    if (TOUCH_INT_PIN >= 0) {
        ret = ft6636u_write_register(0xA4, 0x01); // Enable interrupt
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to enable interrupt mode");
        }
    }
    
    ESP_LOGI(TAG, "FT6636U initialized successfully!");
    return ESP_OK;
}

static esp_err_t ft6636u_read_touch_points(touch_point_t *points, uint8_t *num_points)
{
    esp_err_t ret;
    uint8_t touches = 0;
    
    ret = ft6636u_read_register(FT6636U_REG_NUM_TOUCHES, &touches, 1);
    if (ret != ESP_OK) {
        *num_points = 0;
        return ret;
    }
    
    // FT6636U supports up to 2 touches
    if (touches > 2) touches = 2;
    *num_points = touches;
    
    if (touches == 0) {
        return ESP_OK;
    }
    
    uint8_t touch_data[4];
    ret = ft6636u_read_register(FT6636U_REG_TOUCH1_XH, touch_data, 4);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Extract 12-bit coordinates
    points[0].x = ((touch_data[0] & 0x0F) << 8) | touch_data[1];
    points[0].y = ((touch_data[2] & 0x0F) << 8) | touch_data[3];
    points[0].pressure = touch_data[0] >> 6;  // Extract pressure from high bits
    
    return ESP_OK;
}

void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    static touch_point_t last_point = {0};
    touch_point_t current_point;
    uint8_t num_points = 0;
    
    esp_err_t ret = ft6636u_read_touch_points(&current_point, &num_points);
    if (ret != ESP_OK || num_points == 0 || current_point.pressure == 0) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }
    
    // For horizontal orientation (320x240):
    // - Raw X (0-320) maps to screen Y (0-240)
    // - Raw Y (0-240) maps to screen X (0-320) with inversion
    data->point.x = current_point.y;           // Raw Y becomes screen X
    data->point.y = 320 - current_point.x;     // Raw X becomes screen Y (inverted)
    
    data->state = LV_INDEV_STATE_PR;
    
    // Debug touch coordinates every second
    static uint32_t last_debug_time = 0;
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    if (current_time - last_debug_time > 1000) {
        ESP_LOGD(TAG, "Raw Touch: X=%3d, Y=%3d, Pressure=%d | Screen: X=%3d, Y=%3d", 
                current_point.x, current_point.y, current_point.pressure,
                data->point.x, data->point.y);
        last_debug_time = current_time;
    }
}
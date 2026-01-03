#include "sys_s.h"

static const char *TAG = "SYSFC";

/*
    系统操作函数封装
    已初始化littlefs至"/littlefs"
    nvs已初始化
    FOR:
    ESP32S3 N16R8
    ESP-IDF v5.5.1
*/

uint32_t sys_get_unix_time(void)
{
    time_t now;
    time(&now);
    return (uint32_t)now;
}

char *sys_get_date(const char *format)
{
    static char date_str[64];
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    strftime(date_str, sizeof(date_str), format, &timeinfo);
    return date_str;
}

char *sys_nvs_get(const char *key)
{
    nvs_handle_t handle;
    static char value[256];
    size_t required_size = sizeof(value);
    esp_err_t err;
    
    err = nvs_open("storage", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return NULL;
    }
    
    err = nvs_get_str(handle, key, value, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error getting NVS key '%s': %s", key, esp_err_to_name(err));
        nvs_close(handle);
        return NULL;
    }
    
    nvs_close(handle);
    return value;
}

esp_err_t sys_nvs_set(const char *key, const char *value)
{
    nvs_handle_t handle;
    esp_err_t err;
    
    err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    
    err = nvs_set_str(handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error setting NVS key '%s': %s", key, esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }
    
    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS changes: %s", esp_err_to_name(err));
    }
    
    nvs_close(handle);
    return err;
}

char *sys_read_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file %s", path);
        return NULL;
    }
    
    // 获取文件大小
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size <= 0) {
        fclose(f);
        return NULL;
    }
    
    // 分配内存并读取内容
    char* content = (char*)malloc(size + 1);
    if (!content) {
        ESP_LOGE(TAG, "Memory allocation failed");
        fclose(f);
        return NULL;
    }
    
    size_t bytes_read = fread(content, 1, size, f);
    fclose(f);
    
    if (bytes_read != size) {
        ESP_LOGE(TAG, "Read error: expected %ld bytes, read %zu bytes", size, bytes_read);
        free(content);
        return NULL;
    }
    
    content[size] = '\0';
    return content;
}

esp_err_t sys_write_file(const char *path, const char *content)
{
    FILE *f = fopen(path, "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file %s for writing", path);
        return ESP_FAIL;
    }
    
    size_t len = strlen(content);
    size_t bytes_written = fwrite(content, 1, len, f);
    fclose(f);
    
    if (bytes_written != len) {
        ESP_LOGE(TAG, "Write error: expected %zu bytes, written %zu bytes", len, bytes_written);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t sys_delete_file(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        if (unlink(path) == 0) {
            ESP_LOGI(TAG, "Deleted: %s", path);
        } else {
            ESP_LOGE(TAG, "Failed to delete %s, errno=%d (%s)",
                    path, errno, strerror(errno));
            return ESP_FAIL;
        }
    } else {
        if (errno == ENOENT) {
            ESP_LOGE(TAG, "Path does not exist: %s", path);
            return ESP_FAIL;
        } else {
            ESP_LOGE(TAG, "stat() failed for %s, errno=%d (%s)",
                    path, errno, strerror(errno));
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}
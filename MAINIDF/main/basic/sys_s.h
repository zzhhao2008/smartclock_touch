/**
 * @file sys_s.h
 * @brief 核心系统功能支持
 * 
 * 提供文件读写、NVS存储等基础功能接口。
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "esp_littlefs.h"
#include "driver/gpio.h"
#include "hardware/hw_key.h"
#include "pm.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "beepdrive.h"
#include "nvs.h"
#include "esp_sntp.h"

/**
 * 获取当前UNIX
 * @param none
 * @return uint32_t 当前UNIX时间戳
 */
uint32_t sys_get_unix_time(void);

/**
 * 获取当前日期
 * @param format 日期格式
 * @return char* 当前日期字符串
 */
char *sys_get_date(const char* format);

/**
 * NVS获取键值
 * @param key 键
 * @return char* 值字符串
 */
char *sys_nvs_get(const char* key);

/**
 * NVS设置键值
 * @param key 键
 * @param value 值
 * @return esp_err_t 操作结果
 */
esp_err_t sys_nvs_set(const char* key, const char* value);

/**
 * 读取文件
 * @param path 文件路径
 * @return char* 文件内容字符串
 */
char *sys_read_file(const char *path);

/**
 * 写入文件
 * @param path 文件路径
 * @param content 文件内容
 * @return esp_err_t 操作结果
 */
esp_err_t sys_write_file(const char *path, const char *content);

/** 
 * 删除文件
 * @param path 文件路径
 * @return esp_err_t 操作结果
 */
esp_err_t sys_delete_file(const char *path);
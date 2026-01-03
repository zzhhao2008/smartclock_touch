/**
 * @file beepdrive.h
 * @brief 蜂鸣器驱动程序头文件
 * 
 * 本驱动程序使用LEDC模块控制蜂鸣器发声，提供同步和异步播放功能。
 * 蜂鸣器连接到GPIO 42，支持频率范围200Hz-2700Hz，最大占空比50%。
 * 
 * @note 在使用任何蜂鸣器功能前，必须先调用beep_init()进行初始化。
 * @note 本驱动程序是线程安全的，支持多任务环境下的并发访问。
 */

#ifndef _BEEPDRIVE_H_
#define _BEEPDRIVE_H_

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化蜂鸣器驱动
 * 
 * 配置LEDC模块和GPIO，为蜂鸣器操作做准备。
 * 
 * @return esp_err_t
 *         - ESP_OK: 初始化成功
 *         - ESP_ERR_NO_MEM: 内存分配失败
 *         - 其他: LEDC配置错误
 * 
 * @note 可以重复调用，不会产生副作用。
 */
esp_err_t beep_init(void);

/**
 * @brief 设置蜂鸣器频率
 * 
 * 设置蜂鸣器的发声频率，频率范围为200Hz-2700Hz。
 * 
 * @param freq 频率值(Hz)
 * @return esp_err_t
 *         - ESP_OK: 设置成功
 *         - ESP_ERR_INVALID_STATE: 未初始化
 *         - ESP_ERR_INVALID_ARG: 频率超出范围
 *         - ESP_ERR_TIMEOUT: 获取互斥锁超时
 *         - 其他: LEDC操作错误
 */
esp_err_t beep_set_freq(uint16_t freq);

/**
 * @brief 停止蜂鸣器发声
 * 
 * 将蜂鸣器的占空比设置为0，停止发声。
 * 
 * @return esp_err_t
 *         - ESP_OK: 停止成功
 *         - ESP_ERR_INVALID_STATE: 未初始化
 *         - ESP_ERR_TIMEOUT: 获取互斥锁超时
 *         - 其他: LEDC操作错误
 */
esp_err_t beep_stop(void);

/**
 * @brief 播放音符(同步)
 * 
 * 播放指定频率的音符，持续指定时间后自动停止。
 * 该函数会阻塞当前任务直到播放完成。
 * 
 * @param freq 频率值(Hz)，范围200-2700
 * @param duration_ms 持续时间(毫秒)
 * @return esp_err_t
 *         - ESP_OK: 播放成功
 *         - ESP_ERR_INVALID_STATE: 未初始化
 *         - ESP_ERR_INVALID_ARG: 频率超出范围
 *         - ESP_ERR_TIMEOUT: 获取互斥锁超时
 *         - 其他: LEDC操作错误
 */
esp_err_t play_note(uint16_t freq, uint32_t duration_ms);

/**
 * @brief 播放音符(异步)
 * 
 * 播放指定频率的音符，持续指定时间后在后台自动停止。
 * 该函数不会阻塞当前任务，立即返回。
 * 
 * @param freq 频率值(Hz)，范围200-2700
 * @param duration_ms 持续时间(毫秒)
 * @return esp_err_t
 *         - ESP_OK: 播放开始成功
 *         - ESP_ERR_INVALID_STATE: 未初始化
 *         - ESP_ERR_INVALID_ARG: 频率超出范围
 *         - ESP_ERR_TIMEOUT: 获取互斥锁超时
 *         - ESP_ERR_NO_MEM: 任务创建或内存分配失败
 *         - 其他: LEDC操作错误
 * 
 * @note 异步播放时，如果再次调用播放函数，会覆盖之前的播放。
 */
esp_err_t play_note_async(uint16_t freq, uint32_t duration_ms);

/**
 * @brief 反初始化蜂鸣器驱动
 * 
 * 释放蜂鸣器驱动占用的资源，停止所有正在进行的操作。
 * 
 * @return esp_err_t
 *         - ESP_OK: 反初始化成功
 *         - ESP_ERR_TIMEOUT: 获取互斥锁超时
 */
esp_err_t beep_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* _BEEPDRIVE_H_ */
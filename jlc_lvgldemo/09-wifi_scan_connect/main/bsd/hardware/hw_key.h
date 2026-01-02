#ifndef HW_KEY_H
#define HW_KEY_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file hw_key.h
 * @brief 按键（硬件）模块接口（中文注释）
 * @author Cookie_987
 * @date 2026-01-01
 *
 * 本模块提供按键的中断 + 队列 + 去抖机制，并在后台任务中转换为
 * Press / Release 事件回调。适用于连接到普通 GPIO 的按键（上拉或下拉）。
 */

/** 事件类型：按下 / 释放 */
typedef enum { KEY_EVT_PRESS = 1, KEY_EVT_RELEASE = 2 } key_event_t;

/** 回调签名：gpio - 按键GPIO编号；evt - 事件类型；arg - 用户自定义参数 */
typedef void (*key_callback_t)(int gpio, key_event_t evt, void* arg);

/**
 * 初始化按键模块：
 * - 安装 GPIO ISR 服务（若尚未安装）
 * - 创建用于处理按键事件的队列和任务
 * 返回：true 初始化成功，false 初始化失败
 */
bool hw_key_init(void);

/**
 * 添加要管理的按键：
 * @param gpio 按键连接的 GPIO (使用 esp 的 gpio num)
 * @param active_level 按键按下时的电平（例如：1=高电平按下，0=低电平按下）
 * @return true 添加成功，false（空间不足或参数错误）
 *
 * 该函数会为该 GPIO 设置为 ANYEDGE 中断，ISR 将把 GPIO 编号发送到队列，
 * 后台任务会做去抖并判断 Press / Release，然后调用注册的回调。
 */
bool hw_key_add(int gpio, int active_level);

/**
 * 注册按键回调：
 * @param gpio 要注册回调的 GPIO
 * @param cb 回调函数（NULL 表示取消）
 * @param arg 用户自定义参数，会原样传给回调
 * @return true 查找到对应 GPIO 并设置回调成功，false 未找到该 GPIO
 */
bool hw_key_register_callback(int gpio, key_callback_t cb, void* arg);

#ifdef __cplusplus
}
#endif

#endif // HW_KEY_H

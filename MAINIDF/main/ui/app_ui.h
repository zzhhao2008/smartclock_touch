#pragma once

void start_wifi_app(void);

#ifndef MAINSCR_H
#define MAINSCR_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // 创建主屏幕
    void create_main_screen(void);

    // 更新主屏幕时间
    void update_main_screen_time(const char *time_str);

    // 初始化主屏幕
    void mainscr_init(void);

    // 返回主屏幕函数（外部可调用）
    void backToMS(void);

#ifdef __cplusplus
}
#endif

#endif // MAINSCR_H

// fonts/siyuan_20.h
#ifndef SIYUAN_20_H
#define SIYUAN_20_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// 外部声明字体变量
extern const lv_font_t siyuan_20;

#ifdef __cplusplus
}
#endif

#endif // SIYUAN_20_H
#include "basic/jlc_lcd.h"
#include "app_ui.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "mainscr";

LV_FONT_DECLARE(font_alipuhui20);

// 全局主屏幕对象
lv_obj_t *main_screen = NULL;

// WiFi应用按钮回调
static void btn_wifi_app_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "WiFi APP button clicked");
        
        lvgl_port_lock(0);
        // 清理主屏幕
        if(main_screen) {
            lv_obj_del(main_screen);
            main_screen = NULL;
        }
        lvgl_port_unlock();
        
        // 启动WiFi应用
        start_wifi_app();
    }
}

// 设置按钮回调
static void btn_settings_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Settings button clicked");
        // 这里可以添加设置应用的启动代码
    }
}

// 关于按钮回调
static void btn_about_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "About button clicked");
        // 这里可以添加关于页面的代码
    }
}

// 创建主屏幕
void create_main_screen(void)
{
    lvgl_port_lock(0);
    
    // 清理当前屏幕
    lv_obj_clean(lv_scr_act());
    
    // 创建主屏幕容器
    main_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(main_screen, 320, 240);
    lv_obj_set_style_border_width(main_screen, 0, 0);
    lv_obj_set_style_pad_all(main_screen, 0, 0);
    lv_obj_set_style_radius(main_screen, 0, 0);
    
    // 创建标题
    lv_obj_t *title = lv_label_create(main_screen);
    lv_label_set_text(title, "触屏时钟");
    lv_obj_set_style_text_font(title, &siyuan_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // 创建WiFi应用按钮
    lv_obj_t *btn_wifi = lv_btn_create(main_screen);
    lv_obj_set_size(btn_wifi, 120, 60);
    lv_obj_align(btn_wifi, LV_ALIGN_CENTER, 0, -40);
    lv_obj_add_event_cb(btn_wifi, btn_wifi_app_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *label_wifi = lv_label_create(btn_wifi);
    lv_label_set_text(label_wifi, "WiFi设置");
    lv_obj_set_style_text_font(label_wifi, &siyuan_20, 0);
    lv_obj_center(label_wifi);
    
    // 创建设置按钮
    lv_obj_t *btn_settings = lv_btn_create(main_screen);
    lv_obj_set_size(btn_settings, 120, 60);
    lv_obj_align(btn_settings, LV_ALIGN_CENTER, 0, 30);
    lv_obj_add_event_cb(btn_settings, btn_settings_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *label_settings = lv_label_create(btn_settings);
    lv_label_set_text(label_settings, "设置");
    lv_obj_set_style_text_font(label_settings, &siyuan_20, 0);
    lv_obj_center(label_settings);
    
    // 创建关于按钮
    lv_obj_t *btn_about = lv_btn_create(main_screen);
    lv_obj_set_size(btn_about, 120, 60);
    lv_obj_align(btn_about, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_add_event_cb(btn_about, btn_about_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *label_about = lv_label_create(btn_about);
    lv_label_set_text(label_about, "关于");
    lv_obj_set_style_text_font(label_about, &siyuan_20, 0);
    lv_obj_center(label_about);
    
    // 显示当前时间（模拟）
    lv_obj_t *time_label = lv_label_create(main_screen);
    lv_label_set_text(time_label, "12:34:56");
    lv_obj_set_style_text_font(time_label, &siyuan_20, 0);
    lv_obj_align(time_label, LV_ALIGN_TOP_RIGHT, -10, 10);
    
    lvgl_port_unlock();
    
    ESP_LOGI(TAG, "Main screen created");
}

// 更新主屏幕时间
void update_main_screen_time(const char *time_str)
{
    lvgl_port_lock(0);
    lv_obj_t *time_label = lv_obj_get_child(main_screen, 3); // 根据创建顺序获取时间标签
    if(time_label && lv_obj_check_type(time_label, &lv_label_class)) {
        lv_label_set_text(time_label, time_str);
    }
    lvgl_port_unlock();
}

// 初始化主屏幕
void mainscr_init(void)
{
    // 创建主屏幕
    create_main_screen();
    
    ESP_LOGI(TAG, "Main screen initialized");
}
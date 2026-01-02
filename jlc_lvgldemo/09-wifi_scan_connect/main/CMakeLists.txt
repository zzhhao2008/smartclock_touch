idf_component_register(SRCS "bsd/jlc_lcd.c" "main.c" "font_alipuhui20.c" "app_ui.c"
                       INCLUDE_DIRS "."
                       REQUIRES littlefs nvs_flash esp_wifi)
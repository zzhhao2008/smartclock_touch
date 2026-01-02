idf_component_register(
SRCS "basic/jlc_lcd.c" "main.c" "basic/pm.c" "basic/hardware/hw_key.c" "app_ui.c" "font_alipuhui20.c"
INCLUDE_DIRS "."
REQUIRES littlefs nvs_flash esp_wifi)
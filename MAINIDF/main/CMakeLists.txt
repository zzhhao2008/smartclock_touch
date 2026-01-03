file(GLOB_RECURSE SOURCES ./*.c)

idf_component_register(
SRCS 
    ${SOURCES}
INCLUDE_DIRS 
    "." 
    "basic" 
    "fonts"
REQUIRES 
    littlefs 
    nvs_flash 
    esp_wifi 
    esp_adc 
    driver)
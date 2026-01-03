#include "driver/gpio.h"
#include "esp_log.h"
#include "soc/adc_channel.h"
#include "driver/adc.h"

void ACC(int level);
void init_adc(void);
float read_bat_voltage(void);
float read_bat_percentage(void);
float read_usb_voltage(void);
bool is_charging(void);
bool is_charger_connected(void);

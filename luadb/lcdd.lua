--[[
LCD	ESP32S3
CS	6
RESET	7
D/C	15
MOSI	17
SCK	18
BL	8
MISO	16
]]
local spi_id, pin_reset, pin_dc, pin_cs, bl = 2,7,15,6,8

spi_lcd = spi.deviceSetup(spi_id, pin_cs, 0, 0, 8, 40 * 1000 * 1000, spi.MSB, 1, 0)

lcd.init("st7789", {
    port = "device",
    pin_dc = pin_dc,
    pin_pwr = bl,
    pin_rst = pin_reset,
    direction = 3,
    w = 320,
    h = 240,
    xoffset = 0,
    yoffset = 0
}, spi_lcd)

lcd.clear()

--lcd.drawStr(50,80,"Hello World!")
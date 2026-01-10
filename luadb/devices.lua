if S_CFG == nil or EVENTS == nil then
    require "consts"
end

powerKey = gpio.setup(S_CFG.DEVICE.ACC, 1)
BEEPKey = gpio.setup(S_CFG.DEVICE.BEEP, 0)

-- 初始化LCD
local lcd_cfg = S_CFG.DEVICE.LCD
local lcd_spi = spi.deviceSetup(lcd_cfg.SPIID, lcd_cfg.CS, 0, 0, 8, 40 * 1000 * 1000, spi.MSB, 1, 0)

lcd.init("st7789", {
    port = "device",
    pin_dc = lcd_cfg.DC,
    pin_pwr = -1,
    pin_rst = lcd_cfg.RST,
    direction = lcd_cfg.DIR,
    w = lcd_cfg.W,
    h = lcd_cfg.H,
    xoffset = lcd_cfg.XOFF,
    yoffset = lcd_cfg.YOFF
}, lcd_spi)

pwm.open(lcd_cfg.BL, 5000, 0)

LCD_BL_LIGHT = 100

function lcd_setBL(target, duration)
    if target == nil or target > 100 or target < 0 then
        log.error("lcd", "setBrightness", "invalid target")
        return
    end
    if (duration == nil) then
        duration = 500
    end
    -- 映射亮度
    local brightness = math.floor((target / 100) * (S_CFG.MAX_BRIGHTNESS - S_CFG.MIN_BRIGHTNESS) + S_CFG.MIN_BRIGHTNESS)
    pwm.fade(lcd_cfg.BL, brightness, duration)
    LCD_BL_LIGHT = brightness
end

function lcd_closeBL()
    pwm.fade(lcd_cfg.BL, 0, 300)
end

function lcd_openBL()
    lcd_setBL(LCD_BL_LIGHT, 300)
end

function lcd_turnSleep()
    lcd_setBL(0, 300)
    sys.wait(150)
    lcd.sleep()
end

function lcd_turnWake()
    lcd.wakeup()
    lcd_setBL(LCD_BL_LIGHT, 300)
end

sys.publish(EVENTS.LCD_INITED)
STATUS.LCDINITED = true
-- ADC

function adc_usbin()
    local adcId = S_CFG.DEVICE.USBIN
    adc.open(adcId)
    local usbV = adc.get(adcId)
    adc.close(adcId)
    return usbV / S_CFG.BAT_ATTEN / 1000
end

function adc_battery()
    local adcId = S_CFG.DEVICE.BAT
    adc.open(adcId)
    local batV = adc.get(adcId)
    adc.close(adcId)
    return batV / S_CFG.BAT_ATTEN / 1000
end

function update_adc()
    local usbV = adc_usbin()
    local batV = adc_battery()
    if (usbV > 4) then
        if (not STATUS.USB_Connect) then
            sys.publish(EVENTS.CHARGER_CONNECT)
        end
        STATUS.USB_Connect = true
    else
        if (STATUS.USB_Connect) then
            sys.publish(EVENTS.CHARGER_DISCONNECT)
        end
        STATUS.USB_Connect = false
    end

    STATUS.BAT_Vol = batV
    STATUS.BAT_Percent = math.floor((batV - 3.55) / (4.15 - 3.55) * 100)

    if STATUS.BAT_Percent > 100 then
        STATUS.BAT_Percent = 100
    elseif STATUS.BAT_Percent < 0 then
        STATUS.BAT_Percent = 0
    end
    if (batV < 3.5) then
        sys.publish(EVENTS.BAT_LOW)
    end
    if (batV > 4.2) then
        sys.publish(EVENTS.BAT_FULL)
    end
end

sys.taskInit(function()
    while true do
        update_adc()
        sys.wait(1000)
    end
end)

gpio.debounce(S_CFG.DEVICE.CHG, 200, 0)
CHGING = gpio.setup(S_CFG.DEVICE.CHG, function()
    local val = gpio.get(S_CFG.DEVICE.CHG)
    if val == 1 then
        sys.publish(EVENTS.CHARING)
        sys.publish(EVENTS.CHARGER_CONNECT)
        STATUS.BAT_Charging = true
    else
        STATUS.BAT_Charging = false
        if (STATUS.BAT_Vol > 4.15 and STATUS.USB_Connect) then
            sys.publish(EVENTS.BAT_FULL)
        end
    end
end, nil, gpio.BOTH)

-- BEEP
function BEEP(frec, volu, time)
    if (MUTE == 1) then
        return
    end
    if (frec == nil) then
        frec = 1000
    end
    if (volu == nil) then
        volu = 100
    end
    if (time == nil) then
        time = 300
    end
    pwm.open(9, frec, volu)

    sys.timerStart(function()
        pwm.close(9)
    end, time)
end
-- PM
function powersave(flag, super)
    log.info("powersave", flag)
    if flag == 1 or flag == nil then
        if super ~= nil and STATUS.WLAN == false then
            mcu.setClk(40)
            return
        end
        mcu.setClk(80)
    else
        mcu.setClk(240)
    end
    wlan.powerSave(wlan.PS_MIN_MODEM)
end

psCfg = {
    clk = 80,
    mode = "normal"
}
function poweroff(force)
    log.info("poweroff!")
    if force == nil then
        lcd_closeBL()
        sys.wait(300)
    end
    powerKey(0)
    sys.wait(500)
    rtos.reboot()
end

function sleep()
    lcd.turnSleep()
    psCfg = {
        clk = mcu.getClk(),
        mode = "lcdoff"
    }
    powersave(1, 1)
    pm.wakeupPin(S_CFG.DEVICE.PWK, 1)
    wlan.powerSave(wlan.PS_MAX_MODEM)
    pm.request(pm.LIGHT)
    log.info("powersave", "SYSLEEP")
    sys.wait(100)
    log.info("powersave", "normal")
    registerPWK()
    psCfg = {
        clk = mcu.getClk(),
        mode = "normal"
    }
end

function delayPowerOff(d)
    log.debug("poweroff!", d)
    if d == nil then
        d = 5000
    end
    if d <= 1000 then
        d = 1000
    end
    playSound("warn");
    sys.timerStart(poweroff, d)
end

function cancelPowerOff()
    sys.timerStopAll(poweroff)
end

function registerPWK()
    gpio.debounce(S_CFG.DEVICE.PWK, 100, 0)
    Key_PW = gpio.setup(S_CFG.DEVICE.PWK, function()
        local vol = gpio.get(S_CFG.DEVICE.PWK)
        log.info("pwk", "pwk:", vol)
        if vol == 1 then
            sys.publish(EVENTS.PWK_PRESS)
        else
            sys.publish(EVENTS.PWK_RELEASE)
        end
    end, nil, gpio.BOTH)
end

gpio.debounce(S_CFG.DEVICE.HOME, 100, 0)
Key_HOME = gpio.setup(S_CFG.DEVICE.HOME, function()
    local vol = gpio.get(S_CFG.DEVICE.HOME)
    log.info("home", "home:", vol)
    if vol == 0 then
        sys.publish(EVENTS.HOME_PRESS)
    else
        sys.publish(EVENTS.HOME_RELEASE)
    end
end, nil, gpio.BOTH)

sys.taskInit(function()
    registerPWK()
    -- 为了节省时间，我们把LVGL的初始化放在协程处理
    -- 启动LVGL
    lvgl.init()
    sys.publish(EVENTS.LVGL_INITED)
    STATUS.LVGLINITED = true
    -- 初始化触摸屏
    local touchcfg = S_CFG.DEVICE.TOUCH
    local cfg = {
        sda = touchcfg.SDA,
        scl = touchcfg.SCL,
        rst = touchcfg.RST,
        int = touchcfg.INT,
        fre = touchcfg.FREQ,
        x_limit = touchcfg.X_LIMIT,
        y_limit = touchcfg.Y_LIMIT,
        swapxy = touchcfg.SWAPXY,
        mirror_x = touchcfg.MIRROR_X,
        mirror_y = touchcfg.MIRROR_Y
    }

    local res = ft6336u.init(cfg)
    if not res then
        log.error("ft6336u", "initialization failed")
        return
    end

    res = ft6336u.lvgl_register()
    if not res then
        log.error("ft6336u", "LVGL registration failed")
        return
    end

    sys.publish(EVENTS.LVGL_TOUCH_REG)
    STATUS.TOUCHINITED = true
    log.info("ft6336u", "successfully registered with LVGL")

end)

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

-- 电池电压-电量映射表 (3.7V 1500mAh 锂聚合物电池)
-- 数据点基于典型放电曲线，在关键拐点处增加采样密度
local BAT_CURVE = {{
    v = 3.50,
    p = 0
}, -- 截止电压
{
    v = 3.60,
    p = 10
}, -- 低电量区开始快速下降
{
    v = 3.65,
    p = 15
}, {
    v = 3.70,
    p = 25
}, -- 中低电量区
{
    v = 3.75,
    p = 40
}, {
    v = 3.80,
    p = 55
}, -- 中电量平台区
{
    v = 3.85,
    p = 70
}, {
    v = 3.95,
    p = 85
}, -- 高电量区开始
{
    v = 4.10,
    p = 95
}, -- 高电量区缓慢下降
{
    v = 4.20,
    p = 100
} -- 满电
}

-- 通过线性插值计算电池百分比
local function calculate_battery_percent(voltage)
    -- 电压保护
    if voltage >= BAT_CURVE[#BAT_CURVE].v then
        return 100
    end
    if voltage <= BAT_CURVE[1].v then
        return 0
    end

    -- 查找电压所在的区间
    for i = 1, #BAT_CURVE - 1 do
        local point1 = BAT_CURVE[i]
        local point2 = BAT_CURVE[i + 1]

        if voltage >= point1.v and voltage < point2.v then
            -- 线性插值: p = p1 + (p2-p1)*(v-v1)/(v2-v1)
            local ratio = (voltage - point1.v) / (point2.v - point1.v)
            return math.floor(point1.p + ratio * (point2.p - point1.p))
        end
    end

    return 50 -- 默认值，理论上不会执行到
end

function update_adc()
    local usbV = adc_usbin()
    local batV = adc_battery()

    -- USB连接检测
    if usbV > 4.0 then
        if not STATUS.USB_Connect then
            sys.publish(EVENTS.CHARGER_CONNECT)
        end
        STATUS.USB_Connect = true
    else
        if STATUS.USB_Connect then
            sys.publish(EVENTS.CHARGER_DISCONNECT)
        end
        STATUS.USB_Connect = false
    end

    -- 保存原始电压值
    STATUS.BAT_Vol = batV

    -- 使用曲线拟合计算精确电量
    STATUS.BAT_Percent = calculate_battery_percent(batV)

    -- 电量状态事件
    if batV < 3.55 and not STATUS.BAT_Low then
        STATUS.BAT_Low = true
        sys.publish(EVENTS.BAT_LOW)
    elseif batV > 3.65 and STATUS.BAT_Low then
        STATUS.BAT_Low = false
    end

    -- 充电完成检测 (更精确的判断)
    if STATUS.BAT_Charging and batV > 4.18 and not STATUS.BAT_Full then
        STATUS.BAT_Full = true
        sys.publish(EVENTS.BAT_FULL)
    elseif not STATUS.BAT_Charging and batV < 4.15 then
        STATUS.BAT_Full = false
    end

    -- 充电状态同步
    if STATUS.BAT_Charging and batV < 3.8 then
        STATUS.BAT_Full = false
    end

    --log.debug("bat", string.format("V=%.2fV, %%=%d", batV, STATUS.BAT_Percent))
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
    volu = math.floor(volu / 2)
    pwm.open(S_CFG.DEVICE.BEEP, frec, volu)

    sys.timerStart(function()
        pwm.close(S_CFG.DEVICE.BEEP)
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
    -- BEEP(2500, 100, 20)
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

--[[
常量列表
--]] -- 创建只读表的辅助函数
-- 防止常量被意外修改
local function readonly(t)
    return setmetatable({}, {
        __index = t,
        __newindex = function(_, k, _)
            error("Attempt to modify constant '" .. tostring(k) .. "'", 2)
        end,
        __metatable = false -- 防止获取或修改 metatable
    })
end

-- 系统配置常量
S_CFG = ({
    DEVICE = {
        LCD = {
            SPIID = 2, -- SPI接口ID
            CS = 6, -- 片选引脚
            RST = 7, -- 复位引脚
            DC = 15, -- 数据/命令选择引脚
            MOSI = 17, -- 主出从入引脚
            SCK = 18, -- 串行时钟引脚
            BL = 8, -- 背光控制引脚
            MISO = 16, -- 主入从出引脚
            W = 320, -- 屏幕宽度
            H = 240, -- 屏幕高度
            DIR = 3, -- 显示方向
            XOFF = 0, -- X轴偏移
            YOFF = 0 -- Y轴偏移
        },
        TOUCH = {
            SCL = 9, -- 触摸屏时钟线
            SDA = 11, -- 触摸屏数据线
            RST = 10, -- 触摸屏复位引脚
            INT = 13, -- 触摸屏中断引脚
            FREQ = ft6336u.I2C_FREQ_400K, -- I2C频率
            X_LIMIT = 240, -- X轴范围限制
            Y_LIMIT = 320, -- Y轴范围限制
            SWAPXY = true, -- 是否交换X/Y坐标
            MIRROR_X = false, -- X轴镜像
            MIRROR_Y = true -- Y轴镜像
        },
        USBIN = 0, -- USB输入检测引脚
        BAT = 1, -- 电池电压检测引脚
        HOME = 0, -- HOME键引脚
        PWK = 39, -- 电源键引脚
        ACC = 40, -- ACC点火信号引脚
        BEEP = 42, -- 蜂鸣器引脚
        CHG = 42 -- 充电状态检测引脚
    },
    MAX_BRIGHTNESS = 100, -- 最大背光亮度
    MIN_BRIGHTNESS = 40, -- 最小背光亮度
    BAT_ATTEN = 0.203 -- 电池电压分压系数 上20k，下5.1k
})

-- 系统事件定义
EVENTS = ({
    SYSCTLC = 0, -- 系统控制
    POWERON = 1, -- 开机事件
    POWEROFF = 2, -- 关机事件
    CHARGER_CONNECT = 3, -- 充电器连接
    CHARGER_DISCONNECT = 4, -- 充电器断开
    CHARING = 5, -- 正在充电
    BAT_LOW = 6, -- 电量低
    BAT_FULL = 7, -- 电池充满
    ACC_ON = 8, -- ACC ON
    ACC_OFF = 9, -- ACC OFF
    HOME_PRESS = 10, -- HOME键按下
    HOME_RELEASE = 11, -- HOME键释放
    PWK_PRESS = 12, -- 电源键按下
    PWK_RELEASE = 13, -- 电源键释放
    BEEP_ON = 14, -- 蜂鸣器开启
    BEEP_OFF = 15, -- 蜂鸣器关闭

    LCD_INITED = 100, -- LCD初始化完成
    LVGL_INITED = 101, -- LVGL初始化完成
    LVGL_TOUCH_REG = 102 -- 触摸屏注册完成

})

-- 系统状态变量
STATUS = ({
    LCDINITED = false, -- LCD是否初始化完成
    LVGLINITED = false, -- LVGL是否初始化完成
    LVGLTOUCHREG = false, -- 触摸屏是否注册完成

    WLAN = false, -- WiFi连接状态
    BAT_Vol = 0, -- 电池电压
    BAT_Percent = 0, -- 电池电量百分比
    BAT_Low = false,
    BAT_Full = false,
    BAT_Charging = false,

    USB_Connect = false -- USB连接状态
})

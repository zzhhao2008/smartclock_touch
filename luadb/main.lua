PROJECT = "SMC_T"
VERSION = "1.0.0"
-- sys库是标配
_G.sys = require("sys")
_G.sysplus = require("sysplus")

if wdt then
    wdt.init(9000) -- 初始化watchdog设置为9s
    sys.timerLoopStart(wdt.feed, 3000) -- 3s喂一次狗
end

require "consts"
require "glib"
require "devices"

-- 主任务初始化
sys.taskInit(function()
    -- 等待LVGL图形库初始化完成
    if(not STATUS.LCDINITED) then
        sys.waitUntil(EVENTS.LCD_INITED)
    end
    sys.wait(200)
    -- 打开LCD背光
    lcd_openBL()

    -- 日历控件事件处理函数
    local function event_handler(obj, event)
        if (event == lvgl.EVENT_VALUE_CHANGED) then
            local date = lvgl.calendar_get_pressed_date(obj);
            log.info("calendar", "Date pressed: ", date.year, "-", date.month, "-", date.day)
        end
    end

    -- 创建日历控件并设置基本属性
    local calendar = lvgl.calendar_create(lvgl.scr_act(), nil);
    lvgl.obj_set_size(calendar, 235, 235);  -- 设置日历控件尺寸
    lvgl.obj_align(calendar, nil, lvgl.ALIGN_CENTER, 0, 0);  -- 居中对齐
    lvgl.obj_set_event_cb(calendar, event_handler);  -- 绑定事件处理函数

    -- 获取当前真实时间作为今日日期
    local now = getLocalTime()  -- 使用glib.lua中已有的函数获取本地时间
    local today = lvgl.calendar_date_t()
    today.year = now.year
    today.month = now.month
    today.day = now.day

    -- 设置日历的"今天"日期显示
    lvgl.calendar_set_today_date(calendar, today);
    -- 设置日历初始显示的日期
    lvgl.calendar_set_showed_date(calendar, today);

    -- 高亮显示特殊日期（如今天）
    local highlighted_today = lvgl.calendar_date_t()
    highlighted_today.year = now.year
    highlighted_today.month = now.month
    highlighted_today.day = now.day

    -- 创建高亮日期数组并设置到日历控件
    local highlighted_days = {highlighted_today}
    lvgl.calendar_set_highlighted_dates(calendar, highlighted_days, 1);
end)

-- 启动系统主循环
sys.run()
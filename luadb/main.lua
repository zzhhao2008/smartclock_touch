PROJECT = "SMC_T"
VERSION = "1.0.0"
-- sys库是标配
_G.sys = require("sys")
_G.sysplus = require("sysplus")

if wdt then
    wdt.init(9000) -- 初始化watchdog设置为9s
    sys.timerLoopStart(wdt.feed, 3000) -- 3s喂一次狗
end

require "lcdd"

sys.taskInit(function()
    local config = {
        sda = 11, -- SDA引脚
        scl = 9, -- SCL引脚
        rst = 10, -- RST引脚（可选）
        int = 13, -- INT引脚（可选）
        fre = ft6336u.I2C_FREQ_400K, -- I2C频率
        x_limit = 320,
        y_limit = 320,
        swapxy = true,
        mirror_x = false,
        mirror_y = true
    }
    ft6336u.init(config)
    -- 初始化触摸屏
    local success = ft6336u.init(config)
    if not success then
        log.error("ft6336u", "initialization failed")
        return
    end

    lvgl.init()
    -- 获取触摸屏信息
    local info = ft6336u.info()
    log.info("ft6336u", "chip_id", info.chip_id, "firmware", info.firmware_version)
    -- 将触摸屏注册到LVGL
    success = ft6336u.lvgl_register()
    if not success then
        log.error("ft6336u", "LVGL registration failed")
        return
    end

    log.info("ft6336u", "successfully registered with LVGL")

    local list_demo = {}

    local function event_handler(obj, event)
        if (event == lvgl.EVENT_CLICKED) then
            print(string.format("Clicked: %s\n", lvgl.list_get_btn_text(obj)));
        end
    end

    -- Create a list
    local list1 = lvgl.list_create(lvgl.scr_act(), nil);
    lvgl.obj_set_size(list1, 160, 200);
    lvgl.obj_align(list1, nil, lvgl.ALIGN_CENTER, 0, 0);

    -- Add buttons to the list
    local list_btn;

    list_btn = lvgl.list_add_btn(list1, lvgl.SYMBOL_FILE, "New");
    lvgl.obj_set_event_cb(list_btn, event_handler);

    list_btn = lvgl.list_add_btn(list1, lvgl.SYMBOL_DIRECTORY, "Open");
    lvgl.obj_set_event_cb(list_btn, event_handler);

    list_btn = lvgl.list_add_btn(list1, lvgl.SYMBOL_CLOSE, "Delete");
    lvgl.obj_set_event_cb(list_btn, event_handler);

    list_btn = lvgl.list_add_btn(list1, lvgl.SYMBOL_EDIT, "Edit");
    lvgl.obj_set_event_cb(list_btn, event_handler);

    list_btn = lvgl.list_add_btn(list1, lvgl.SYMBOL_SAVE, "Save");
    lvgl.obj_set_event_cb(list_btn, event_handler);

    list_btn = lvgl.list_add_btn(list1, lvgl.SYMBOL_BELL, "Notify");
    lvgl.obj_set_event_cb(list_btn, event_handler);

    list_btn = lvgl.list_add_btn(list1, lvgl.SYMBOL_BATTERY_FULL, "Battery");
    lvgl.obj_set_event_cb(list_btn, event_handler);

    return list_demo
end)

sys.run()

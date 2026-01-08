PROJECT = "SMC_T"
VERSION = "1.0.0"
-- sys库是标配
_G.sys = require("sys")
_G.sysplus = require("sysplus")

if wdt then
    wdt.init(9000) -- 初始化watchdog设置为9s
    sys.timerLoopStart(wdt.feed, 3000) -- 3s喂一次狗
end

ft6636u = require "ft6636u"
require "lcdd"

local colorList = {0xF800, 0x07E0, 0x001F, 0xFFFF, 0x0000, 0xF81F, 0xFFE0, 0x07FF, 0x03FF, 0x01FF, 0x07F0}
nowColor = 1

vButton = {{
    x = 0,
    y = 0,
    w = 30,
    h = 30
}, {
    x = 40,
    y = 0,
    w = 30,
    h = 30
}}

sys.taskInit(function()
    ft6636u.init({
        sda = 11,
        scl = 9,
        fre = i2c.FAST,
        x_limit = 320,
        y_limit = 320,
        swapxy = true,
        mirror_x = false,
        mirror_y = true
    })

    for i = 1, #vButton do
        local color = 0x0
        if i==1 then
            color = colorList[i]
        end
        lcd.fill(vButton[i].x, vButton[i].y, vButton[i].x + vButton[i].w, vButton[i].y + vButton[i].h, color)
    end

    local lastX = -1
    local lastY = -1
    while true do
        sys.wait(10)
        local x, y, sta = ft6636u.getState()
        -- log.info("main", "TOUCH READ X,Y,STA", x,y,sta)
        if (sta == 1) then
            -- 判断在不在虚拟按钮内部
            local buttonId = -1
            for i = 1, #vButton do
                if (x >= vButton[i].x and x <= vButton[i].x + vButton[i].w and y >= vButton[i].y and y <= vButton[i].y +
                    vButton[i].h) then
                    buttonId = i
                    break
                end
            end
            if buttonId ~= -1 then
                if (buttonId == 1) then
                    nowColor = nowColor + 1
                    if nowColor > #colorList then
                        nowColor = 1
                    end
                    -- 显示当前颜色
                    local bx = vButton[buttonId].x
                    local by = vButton[buttonId].y
                    local bw = vButton[buttonId].w
                    local bh = vButton[buttonId].h
                    lcd.fill(bx, by, bx + bw, by + bh, colorList[nowColor])
                elseif (buttonId == 2) then
                    lcd.clear()
                end
            else
                if (lastX ~= x or lastY ~= y) and x ~= -1 and y ~= -1 and lastX ~= -1 and lastY ~= -1 then
                    lcd.drawLine(lastX, lastY, x, y, colorList[nowColor])
                else
                    lcd.drawPoint(x, y, colorList[nowColor])
                end
                lastX = x
                lastY = y
            end
        else
            lastX = -1
            lastY = -1
        end
    end
end)

sys.run()

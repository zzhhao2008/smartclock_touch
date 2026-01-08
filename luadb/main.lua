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
    local lastX = -1
    local lastY = -1
    while true do
        sys.wait(10)
        local x, y, sta = ft6636u.getState()
        -- log.info("main", "TOUCH READ X,Y,STA", x,y,sta)
        if (sta == 1) then
            if (lastX ~= x or lastY ~= y) and x ~= -1 and y ~= -1 and lastX~=-1 and lastY~=-1 then
                lcd.drawLine(lastX, lastY, x, y, 0xF800)

            else
                lcd.drawPoint(x, y, 0xF800) -- 红色
            end
            lastX = x
            lastY = y
        else
            lastX = -1
            lastY = -1
        end
    end
end)

sys.run()

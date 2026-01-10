-- 添加前导零到数字或字符串
-- @param str 输入的数字或字符串
-- @param num 需要的最小长度，默认为2
function addZero(str, num)
    -- log.info("addZreo",str)
    -- 判断是不是字符串
    str = str .. ""
    if num == nil then
        num = 2
    end
    while #str < num do
        str = "0" .. str
    end
    return str
end

-- 读取文件内容
-- @param filename 文件名
function readfile(filename)
    if io.exists(filename) then
        return io.readFile(filename)
    else
        return ""
    end
end

-- 初始化文件系统键值存储
fskv.init()

-- 从数据库获取值
-- @param key 键名
function dbGet(key)
    return fskv.get(key)
end

-- 向数据库存入键值对
-- @param key 键名
-- @param str 值
function dbPut(key, str)
    if key == nil or str == nil then
        return 0
    end
    return fskv.set(key, str)
end

-- 从数据库删除键值对
-- @param key 键名
function dbDel(key)
    return fskv.del(key)
end

-- 清空数据库
function dbClear()
    return fskv.clear()
end

-- 获取系统配置
function getSysConfig_DB()
    sysConfig = dbGet("sysConfig")
    return sysConfig
end

-- 保存系统配置
function saveSysConfig_DB()
    dbPut("sysConfig", sysConfig)
    return sysConfig
end

-- 辅助函数：安全加8小时（处理跨天/月/年）
-- @return 包含本地时间信息的表
function getLocalTime()
    local t = rtc.get() -- 假设 rtc.get() 返回 table: {year, mon, day, hour, min, sec}
    -- 先转换为秒（简化处理，假设无闰秒）
    local secs = os.time({
        year = t.year,
        month = t.mon,
        day = t.day,
        hour = t.hour,
        min = t.min,
        sec = t.sec
    })
    -- 加 8 小时（28800 秒）
    local localSecs = secs + 8 * 3600
    local lt = os.date("*t", localSecs)
    return {
        year = lt.year,
        month = lt.month,
        day = lt.day,
        hour = lt.hour,
        min = lt.min,
        sec = lt.sec
    }
end

-- 格式化时间（用于显示时、分、秒）
-- @return 小时、分钟、秒钟
function formatedHourMinSec()
    local now = getLocalTime()
    return addZero(now.hour), addZero(now.min), addZero(now.sec)
end

-- 格式化日期：仅 月-日
-- @return MM-DD格式的日期字符串
function formatedMonthDay()
    local now = getLocalTime()
    return addZero(now.mon) .. "-" .. addZero(now.day)
end

-- 格式化日期
-- @param date 日期表
function dateformat(date)
    local now = getLocalTime()
    if (date == nil) then
        date = now
    end
    log.info("dateformat", json.encode(date), json.encode(now))
    if date.m then
        -- 先检查参数是否完整
        date.y = date.y or now.year
        date.m = date.m or now.mon
        date.d = date.d or now.day
        if date.y == now.year then
            return string.format("%02d-%02d", date.m, date.d)
        end
        return string.format("%04d-%02d-%02d", date.y, date.m, date.d)
    else -- 是year mon day参数
        -- 先检查参数是否完整
        date.year = date.year or now.year
        date.mon = date.mon or now.mon
        date.day = date.day or now.day
        if date.year == now.year then
            return string.format("%02d-%02d", date.mon, date.day)
        end
        return string.format("%04d-%02d-%02d", date.year, date.mon, date.day)
    end
end

-- 格式化时间
-- @param time 时间表
function timeformat(time)
    time = time or {}
    time.h = time.h or 0
    time.i = time.i or 0
    if time.s ~= nil and time.s ~= 0 then
        return string.format("%02d:%02d:%02d", time.h, time.i, time.s)
    else
        return string.format("%02d:%02d", time.h, time.i)
    end
end

-- 格式化日期时间
-- @param time 时间表
function datetimeformat(time)
    return dateformat(time) .. " " .. timeformat(time)
end

-- 工具函数：补零为两位字符串
-- @param n 数字
local function pad2(n)
    n = tonumber(n) or 0
    return n < 10 and "0" .. n or tostring(n)
end

-- 计算到目标时间的剩余/已过时间（仅 d,h,i,s）
-- 参数：h, i, s, y, m, d → 目标时间（本地时间，已含+8小时偏移）
-- 返回：flag, d, h, i, s （全部为两位字符串）
function calcTimeDiffToTarget(h, i, s, y, m, d)
    -- 获取当前本地时间（已+8小时）
    local now = getLocalTime() -- 确保此函数已定义

    -- 构造目标时间表
    local target = {
        year = y,
        month = m,
        day = d,
        hour = h,
        min = i, -- 注意：参数 i 是分钟
        sec = s
    }

    local now_ts = os.time(now)
    local target_ts = os.time(target)

    -- 安全检查
    if not now_ts or not target_ts then
        return 0, "00", "00", "00", "00"
    end

    local delta = target_ts - now_ts
    local flag = delta >= 0 and 0 or 1
    local abs_delta = math.abs(delta)

    -- 精确分解：秒 → 分 → 时 → 天
    local secs = abs_delta % 60
    local total_mins = (abs_delta - secs) // 60
    local mins = total_mins % 60
    local total_hours = (total_mins - mins) // 60
    local hours = total_hours % 24
    local days = (total_hours - hours) // 24

    -- 补零为两位字符串
    local str_d = pad2(days)
    local str_h = pad2(hours)
    local str_i = pad2(mins)
    local str_s = pad2(secs)

    return flag, str_d, str_h, str_i, str_s
end

-- 为文本添加颜色标签
-- @param str 文本内容
-- @param color 颜色值
function colorText(str, color)
    if color == nil then
        color = "000000"
    end
    if str == nil then
        log.warn("EmptyStrIn!")
        str = ""
    end
    return "#" .. color .. " " .. str .. "#"
end

-- 检查值是否在列表中
-- @param value 要检查的值
-- @param list 列表
function isValueInList(value, list)
    for i, v in ipairs(list) do
        if v == value then
            return true
        end
    end
end

-- 将唯一值合并到列表中
-- @param listReceiver 接收列表
-- @param listTransmitter 发送列表
function listMergeUnique(listReceiver, listTransmitter)
    for i, value in ipairs(listTransmitter) do
        if not isValueInList(value, listReceiver) then
            table.insert(listReceiver, value)
        end
    end
end
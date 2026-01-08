--[[
汇顶 FT6636U 触摸屏 I2C驱动
TOUCH_I2C	ESP32S3
SCL	9 需要内部上拉
RST	10 需要内部上拉
SDA	11 需要内部上拉
INT	13 需要内部上拉
需要PORT到LVGL
]] local ft6636u = {}

-- FT6636U I2C地址
local FT6336U_ADDR = 0x38

-- 寄存器地址定义
local FT6336U_REG_CHIP_ID = 0xA3 -- 芯片ID寄存器
local FT6336U_REG_FIRMWARE_VERSION = 0xA6 -- 固件版本寄存器
local FT6336U_REG_VENDOR_ID = 0xA8 -- 供应商ID寄存器
local FT6336U_REG_TOUCH_POINTS = 0x02 -- 触摸点数量寄存器
local FT6336U_REG_TOUCH1_XH = 0x03 -- 第一个触摸点X坐标高位寄存器
local FT6336U_REG_TOUCH1_YH = 0x05 -- 第一个触摸点Y坐标高位寄存器

-- 模块内部变量
local i2c_id = nil
local sda_pin = nil
local scl_pin = nil
local x_limit = 0
local y_limit = 0

local swapxy = false
local mirror_x = false
local mirror_y = false

-- 保存上一次的坐标值
local last_x = 0
local last_y = 0

--- 初始化FT6636U触摸屏
-- @param config 配置参数表
-- @usage config = {sda=11, scl=9, fre=i2c.FAST, x_limit=480, y_limit=320}
function ft6636u.init(config)
    -- 保存配置参数
    sda_pin = config.sda
    scl_pin = config.scl
    x_limit = config.x_limit or 0
    y_limit = config.y_limit or 0
    swapxy = config.swapxy or false
    mirror_x = config.mirror_x or false
    mirror_y = config.mirror_y or false

    -- 创建软件I2C
    i2c_id = i2c.createSoft(scl_pin, sda_pin, 5)
    if not i2c_id then
        print("Failed to create I2C interface")
        return false
    end

    -- 检查芯片是否连接
    local chip_id = i2c.readReg(i2c_id, FT6336U_ADDR, FT6336U_REG_CHIP_ID, 1)
    if chip_id then
        print("Chip ID: 0x" .. string.format("%02X", string.byte(chip_id)))
    else
        print("Failed to read chip ID, FT6636U not connected")
        return false
    end

    -- 读取固件版本
    local firmware_version = i2c.readReg(i2c_id, FT6336U_ADDR, FT6336U_REG_FIRMWARE_VERSION, 1)
    if firmware_version then
        print("Firmware version: 0x" .. string.format("%02X", string.byte(firmware_version)))
    else
        print("Failed to read firmware version")
        return false
    end

    -- 读取供应商ID
    local vendor_id = i2c.readReg(i2c_id, FT6336U_ADDR, FT6336U_REG_VENDOR_ID, 1)
    if vendor_id then
        print("CTPM Vendor ID: 0x" .. string.format("%02X", string.byte(vendor_id)))
    end

    print("FT6636U initialized successfully")
    return true
end

--- 读取触摸坐标
-- @return x坐标, y坐标, 触摸状态(0=无触摸, 1=触摸)
function ft6636u.read()
    -- 读取触摸点数量
    local touch_pnt_cnt = i2c.readReg(i2c_id, FT6336U_ADDR, FT6336U_REG_TOUCH_POINTS, 1)
    if not touch_pnt_cnt then
        return last_x, last_y, 0
    end

    -- 检查是否有有效的触摸点
    local cnt = string.byte(touch_pnt_cnt) & 0x0F
    if cnt ~= 1 then -- 只处理单点触摸
        return last_x, last_y, 0
    end

    -- 读取X坐标
    local data_x = i2c.readReg(i2c_id, FT6336U_ADDR, FT6336U_REG_TOUCH1_XH, 2)
    if not data_x or string.len(data_x) < 2 then
        return last_x, last_y, 0
    end

    -- 读取Y坐标
    local data_y = i2c.readReg(i2c_id, FT6336U_ADDR, FT6336U_REG_TOUCH1_YH, 2)
    if not data_y or string.len(data_y) < 2 then
        return last_x, last_y, 0
    end

    -- 解析坐标值 (X: [11:0], Y: [11:0])
    local x_high = string.byte(data_x, 1) or 0
    local x_low = string.byte(data_x, 2) or 0
    local y_high = string.byte(data_y, 1) or 0
    local y_low = string.byte(data_y, 2) or 0

    local current_x = ((x_high & 0x0F) << 8) | x_low
    local current_y = ((y_high & 0x0F) << 8) | y_low

    --log.info("Touch RAW", "x", current_x, "y", current_y)

    -- 处理翻转和交换坐标
    if mirror_x then
        current_x = x_limit - current_x
    end
    if mirror_y then
        current_y = y_limit - current_y
    end
    if swapxy then
        current_x, current_y = current_y, current_x
    end

    -- 更新坐标值
    if last_x ~= current_x or last_y ~= current_y then
        last_x = current_x
        last_y = current_y
    end

    -- 边界检查
    if last_x >= x_limit then
        last_x = x_limit - 1
    end
    if last_y >= y_limit then
        last_y = y_limit - 1
    end

    return last_x, last_y, 1
end

--- 获取当前触摸状态
-- @return x坐标, y坐标, 触摸状态
function ft6636u.getState()
    return ft6636u.read()
end

return ft6636u

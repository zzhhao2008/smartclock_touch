# hw_key 模块使用说明（中文）

## 简介

`hw_key` 是一个轻量的按键管理模块，提供按键的中断接收、去抖处理，以及 Press/Release 事件回调。

## 特性

- 基于 GPIO 中断（ANYEDGE）触发事件
- ISR 中最小化处理：仅向队列发送 GPIO 编号
- 后台任务延时去抖并判断 Press / Release
- 支持为每个按键注册独立回调

## API

- bool hw_key_init(void)
  - 初始化模块（安装 ISR 服务、创建队列和任务）

- bool hw_key_add(int gpio, int active_level)
  - 添加按键；`active_level` 表示按下时的电平（1 或 0）

- bool hw_key_register_callback(int gpio, key_callback_t cb, void* arg)
  - 注册回调，`arg` 会在回调时原样传回；传入 NULL 取消回调

## 使用示例

```c
// 初始化（推荐在 app_main 中）：
if (!hw_key_init()) {
    ESP_LOGE("KEY", "Key module init failed");
}
// 添加按键（GPIO0，按下为高电平）
hw_key_add(GPIO_NUM_0, 1);
// 注册回调
hw_key_register_callback(GPIO_NUM_0, my_key_callback, NULL);
```

回调示例：
```c
void my_key_callback(int gpio, key_event_t evt, void* arg) {
    if (evt == KEY_EVT_PRESS) {
        // 按下处理
    } else {
        // 释放处理
    }
}
```

## 注意事项

- 默认去抖时间为 35 ms，可根据实际按键硬件调整 `DEBOUNCE_MS` 宏
- 回调在任务上下文中执行，请避免在回调中长时间阻塞
- 最大同时管理按键数量由 `MAX_KEYS` 宏决定，可以根据需要调整

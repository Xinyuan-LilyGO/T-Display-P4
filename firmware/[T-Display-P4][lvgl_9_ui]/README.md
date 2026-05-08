# Firmware Modification Log

## 202601211405
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202601211405`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202601211405`
### Change content
- 修改gps解析gnss数据函数，使用更安全的解析方式

## 202512201556
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202512201556`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202512201556`
### Change content
- 修改音乐、铃声、壁纸

## 202512081426
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202512081426`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202512081426`
### Change content
- bq27220修改设计电池容量为1000mah

## 202512080937
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202512080937`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202512080937`
### Change content
- 将屏幕复位调到程序开头

## 202512061136
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202512061136`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202512061136`
### Change content
- 延长电源复位时间为200ms，延长屏幕触摸复位时间为200ms

## 202512051607
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202512051607`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202512051607`
### Change content
- 降低ov2710摄像头dovdd电压为1.7v，avdd为3.0v
- 修改CONFIG_FREERTOS_TIMER_TASK_STACK_DEPTH为8192

## 202512051123
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202512051123`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202512051123`
### Change content
- 修复sgm38121的avdd电压计算错误问题，添加电压越界日志

## 202512041351
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202512041351`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202512041351`
### Change content
- 删除icm20948初始化读取id信息，修复icm20948初始化错误

## 202512021413
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202512021413`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202512021413`
### Change content
- 将mipi总线电压抬升至1.83v修复部分板子在初始化mipi总线触发看门狗异常

## 202511280947
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202511280947`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202511280947`
### Change content
- 调节马达振动判定阈值范围为1300
- 修改麦克风测试数值为正值，转盘范围限制在0-1000，使其显示更加平滑

## 202511121027
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202511121027`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202511121027`
### Change content
- 删除多余中断清除标志

## 202511111518
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202511111518`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202511111518`
### Change content
- 添加sx1262发送中断清除，发送中断完成判断改为读取中断寄存器

## 202511071441
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202511071441`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202511071441`
### Change content
- 修复sx1262 lora的snr值没有负值bug

## 202510241555
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202510241555`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202510241555`
### Change content
- 修复cit测试按键“pass”和“fail”按钮导致程序重启问题

## 202510230957
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202510230957`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202510230957`
### Change content
- 将摄像头ppa操作的输出数据换为独立内存解决摄像头撕裂问题

## 202510151042
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202510151042`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202510151042`
### Change content
- 更新esp-idf v5.5.0版本
- 修复摄像头ppa裁剪错误问题

## 202509261215
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202509261215`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202509261215`
### Change content
- 添加系统设备状态通知

## 202509101652
### Changed firmware
- `[T-Display-P4][lvgl_9_ui][hi8561][ov2710]_firmware_202509101652`
- `[T-Display-P4][lvgl_9_ui][rm69a10][ov2710]_firmware_202509101652`
### Change content
- 修复一些bug

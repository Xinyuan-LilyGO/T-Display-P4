<h1 align="center">T-Display-P4</h1>

## [English](./README.md) | **中文**

[![License](https://img.shields.io/github/license/Xinyuan-LilyGO/T-Display-P4?style=flat-square)](./LICENSE)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5.4%2B-ff6f00?style=flat-square)](https://github.com/espressif/esp-idf)
[![C++](https://img.shields.io/badge/C%2B%2B-17%2B-00599c?style=flat-square)](https://isocpp.org/)

<p align="center">
  <img src="image/1.jpg" alt="T-Display-P4 产品图" width="720">
</p>

## 概述

T-Display-P4 是一款以 **ESP32-P4** 为主控的高性能开发平台。产品搭载高分辨率 MIPI-DSI 触摸屏和 **ESP32-C6** 无线协处理器，并集成音频、LoRa、GPS、以太网、惯性传感器、电池监测、SD 卡存储和 MIPI-CSI 摄像头等功能。

产品支持两种屏幕组件：4.05 英寸 540 x 1168 TFT 屏，以及 4.1 英寸 568 x 1232 AMOLED 屏。可选的 T-Display-P4-Keyboard 扩展板提供键盘、背光控制和 T-MixRF 射频/NFC 接口。

## 目录

- [概述](#概述)
- [硬件版本](#硬件版本)
- [预览](#预览)
- [支持框架](#支持框架)
- [快速开始](#快速开始)
- [硬件模块](#硬件模块)
- [引脚总览](#引脚总览)
- [项目资料](#项目资料)
- [常见问题](#常见问题)

## 硬件版本

| 版本 | 日期 | 说明 |
| :---: | :---: | --- |
| T-Display-P4 V1.0 | 2025-06-13 | 初始硬件版本 |
| T-Display-P4-Keyboard V1.0 | 2025-09-12 | 初始键盘扩展板版本 |
| T-Display-P4 V2.0 Evaluation Preview | 2026-05-22 | 评估预览版硬件资料 |

## 预览

<p align="center">
  <img src="image/2.jpg" alt="T-Display-P4 预览图 1" width="24%">
  <img src="image/3.jpg" alt="T-Display-P4 预览图 2" width="24%">
  <img src="image/4.jpg" alt="T-Display-P4 预览图 3" width="24%">
  <img src="image/5.jpg" alt="T-Display-P4 预览图 4" width="24%">
</p>

## 支持框架

| 框架 | 状态 | 版本 |
| --- | --- | --- |
| ESP-IDF | 推荐 | `>= v5.5.4` |

## 快速开始

### 使用 ESP-IDF 构建

请先安装 ESP-IDF。环境安装与配置请参考官方文档：
[ESP-IDF 入门指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32p4/get-started/index.html)

克隆仓库时请同时拉取子模块：

```bash
git clone --recursive https://github.com/Xinyuan-LilyGO/T-Display-P4.git
cd T-Display-P4
```

如果克隆时没有使用 `--recursive`，请手动初始化子模块：

```bash
git submodule update --init --recursive
```

在 `menuconfig` 中选择示例，然后构建并烧录工程：

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py flash monitor
```

```text
Example Configuration
`-- Select the example to build
```

### 示例

| 示例集合 | 链接 | 说明 |
| --- | --- | --- |
| 通用设备驱动示例 | [`lilygo_device_driver_example`](https://github.com/Xinyuan-LilyGO/lilygo_device_driver_example) | 在独立仓库中集中维护的通用示例 |
| T-Display-P4 示例 | [`main/examples`](./main/examples) | T-Display-P4 板级专用示例 |
| T-Display-P4-Keyboard 示例 | [`main/keyboard_examples`](./main/keyboard_examples) | 键盘扩展板专用示例 |

请通过上方入口查看最新源码和当前支持的示例。

### 预编译固件

以下是已经编译好的固件。

烧录预编译固件时，可参考乐鑫官方 [ESP 固件在线烧录平台说明](https://docs.espressif.com/projects/esp-techpedia/zh_CN/latest/esp-friends/get-started/try-firmware/try-firmware-platform.html)。

| 固件 | 烧录地址 | 说明 |
| --- | --- | --- |
| [`[T-Display-P4][lvgl_9_ui]`](<./firmware/[T-Display-P4][lvgl_9_ui]>) | `0x0` | T-Display-P4 出厂 UI 固件 |
| [`[T-Display-P4-Keyboard][lvgl_9_ui]`](<./firmware/[T-Display-P4-Keyboard][lvgl_9_ui]>) | `0x0` | 键盘扩展板出厂 UI 固件 |
| [`[T-Display-P4][esp32c6_at_slave]`](<./firmware/[T-Display-P4][esp32c6_at_slave]>) | `0x0` | ESP32-C6 ESP-AT 协处理器固件 |
| [`[T-Display-P4][esp32c6][network_adapter]`](<./firmware/[T-Display-P4][esp32c6][network_adapter]>) | `0x0` | ESP32-C6 SDIO 网络适配器固件 |
| [`[T-Display-P4][edge_agent]`](<./firmware/[T-Display-P4][edge_agent]>) | `0x0` | Edge Agent 应用固件 |
| [`[T-Display-P4][xiaozhi]`](<./firmware/[T-Display-P4][xiaozhi]>) | `0x0` | 小智应用固件 |

> [!IMPORTANT]
> 烧录 T-Display-P4 和 T-Display-P4-Keyboard 固件时选择 `ESP32-P4`；烧录 ESP32-C6 AT 或网络适配器固件时选择 `ESP32-C6`。固件不能跨芯片混用。

## 硬件模块

### T-Display-P4 V1.0

| 功能 | 器件 | 接口 |
| --- | --- | --- |
| 主控 MCU | ESP32-P4，16 MB Flash | — |
| 无线协处理器 | ESP32-C6-MINI-1U / ESP32-C6-FH4，4 MB Flash | SDIO |
| TFT 屏幕与触摸 | H0405S002T002-V0 / HI8561，4.05 英寸，540 x 1168 | MIPI-DSI / I2C |
| AMOLED 屏幕与触摸 | H0410S001AMT001-V0 / RM69A10 / GT9895，4.1 英寸，568 x 1232 | MIPI-DSI / I2C |
| 音频 | ES8311 / NS4150B / 驻极体电容麦克风 | I2S |
| LoRa | SX1262 / LR2021 | SPI |
| GPS | L76K | UART |
| RTC | PCF8563 | I2C |
| 惯性传感器 | ICM20948 | I2C |
| 振动 | AW86224AFCR | I2C |
| 充电 | LGS4056H，支持电池 NTC | — |
| 电量计 | BQ27220 | I2C |
| 摄像头 | OV2710 | MIPI-CSI |
| SD 卡 | SD 卡槽 | SDMMC / SPI |
| 以太网 | IP101GRI | RMII |
| IO 扩展 | XL9535 | I2C |

- 相关资料：
  - [Espressif 技术文档](https://www.espressif.com/zh-hans/support/documents/technical-documents)
  - [ESP32-C6-MINI-1/MINI-1U 数据手册](https://www.espressif.com/sites/default/files/documentation/esp32-c6-mini-1_mini-1u_datasheet_cn.pdf)
  - [HI8561](<./docs/HI8561_Preliminary _DS_V0.00_20230511.pdf>)
  - [RM69A10](<./docs/RM69A10_DataSheet_V0.2_20230330%C2%A0(Public%C2%A0version).pdf>)
  - [GT9895](./docs/GT9895_Datasheet_V1.1.pdf)
  - [ES8311](./docs/ES8311.pdf)
  - [NS4150B](./docs/NS4150B.pdf)
  - [SX1262](./docs/DS_SX1261-2_V2_1.pdf)
  - [LR2021](<./参考资料/semtech_cpp_bus_driver/LR2021_22_12_Datasheet_v2.1.pdf>)
  - [L76K](./docs/L76KB-A58.pdf)
  - [PCF8563](./docs/PCF8563.pdf)
  - [ICM20948](./docs/ICM20948.pdf)
  - [AW86224AFCR](./docs/AW86224AFCR.pdf)
  - [LGS4056H](./docs/LGS4056H.pdf)
  - [BQ27220](./docs/bq27220_en.pdf)
  - [OV2710](<./docs/OV2710_CSP3_DS_2.0_KING HORN ENTERPRISES Ltd..pdf>)
  - [IP101GRI](./docs/IP101GRI.pdf)
  - [XL9535](./docs/XL95x5.pdf)

### T-Display-P4-Keyboard V1.0

| 功能 | 器件 | 接口 |
| --- | --- | --- |
| 键盘控制器 | TCA8418 | I2C |
| 键盘背光 | SY7200A | PWM |
| IO 扩展 | XL9555 | I2C |
| Sub-GHz 射频 | T-MixRF 上的 CC1101 | SPI |
| 2.4 GHz 射频 | T-MixRF 上的 NRF24L01 | SPI |
| NFC | T-MixRF 上的 ST25R3916 | SPI |

- 相关资料：
  - [TCA8418](./docs/tca8418.pdf)
  - [SY7200A](./docs/SY7200AABC.pdf)
  - [XL9555](./docs/XL95x5.pdf)
  - [CC1101](./docs/cc1101.pdf)
  - [NRF24L01](./docs/NRF24L01P-R.pdf)
  - [ST25R3916](./docs/st25r3916.pdf)

## 引脚总览

板级引脚定义位于设备驱动配置文件中：

- [`t_display_p4_config.h`](./libraries/lilygo_device_driver/src/device/t_display_p4/t_display_p4_config.h)
- [`t_display_p4_keyboard_config.h`](./libraries/lilygo_device_driver/src/device/t_display_p4/t_display_p4_keyboard_config.h)

## 项目资料

| 文档 | 说明 |
| --- | --- |
| [`T-Display-P4 V1.0 TFT`](<./project/T-Display-P4_V1.0(H0405S002T002-V0)_202601061148.pdf>) | 4.05 英寸 TFT 组件的 V1.0 硬件项目 PDF |
| [`T-Display-P4 V1.0 AMOLED`](<./project/T-Display-P4_V1.0(H0410S001AMT001-V0)_202601061148.pdf>) | 4.1 英寸 AMOLED 组件的 V1.0 硬件项目 PDF |
| [`T-Display-P4 V2.0 Evaluation Preview`](<./project/T-Display-P4_v2.0(Evaluation Preview)_202605221021.pdf>) | V2.0 评估预览版硬件项目 PDF |
| [`docs`](./docs) | 数据手册与外设文档 |
| [`功耗测试日志`](<./relevant_test/PowerConsumptionTestLog_[T-Display-P4_V1.0]_20250605.pdf>) | T-Display-P4 V1.0 功耗测试数据 |

## 常见问题

<details>
<summary>Q. 为什么开发板一直烧录失败？</summary>

A. 开始下载时按住 `BOOT` 按键，烧录开始后再松开。

</details>

<details>
<summary>Q. 为什么缺少子模块头文件或组件？</summary>

A. 请初始化仓库的全部子模块：

```bash
git submodule update --init --recursive
```

</details>

<details>
<summary>Q. 如何切换不同的示例？</summary>

A. 执行 `idf.py menuconfig`，打开 `Example Configuration`，选择需要的示例，保存后重新构建工程。

</details>

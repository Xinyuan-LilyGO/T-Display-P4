<h1 align="center">T-Display-P4</h1>

## **English** | [Chinese](./README_CN.md)

[![License](https://img.shields.io/github/license/Xinyuan-LilyGO/T-Display-P4?style=flat-square)](./LICENSE)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5.4%2B-ff6f00?style=flat-square)](https://github.com/espressif/esp-idf)
[![C++](https://img.shields.io/badge/C%2B%2B-17%2B-00599c?style=flat-square)](https://isocpp.org/)

<p align="center">
  <img src="image/1.jpg" alt="T-Display-P4 product photo" width="720">
</p>

## Overview

T-Display-P4 is a high-performance development platform built around the **ESP32-P4**. It combines a high-resolution MIPI-DSI touch display with an **ESP32-C6** wireless coprocessor and integrates audio, LoRa, GPS, Ethernet, motion sensing, battery monitoring, SD card storage, and MIPI-CSI camera support.

Two display assemblies are supported: a 4.05-inch 540 x 1168 TFT panel and a 4.1-inch 568 x 1232 AMOLED panel. The optional T-Display-P4-Keyboard expansion board adds a keyboard, backlight control, and T-MixRF radio/NFC interfaces.

## Directory

- [Overview](#overview)
- [Hardware Versions](#hardware-versions)
- [Preview](#preview)
- [Supported Frameworks](#supported-frameworks)
- [Quick Start](#quick-start)
- [Hardware Modules](#hardware-modules)
- [Pin Overview](#pin-overview)
- [Project Materials](#project-materials)
- [FAQ](#faq)

## Hardware Versions

| Version | Date | Description |
| :---: | :---: | --- |
| T-Display-P4 V1.0 | 2025-06-13 | Initial hardware version |
| T-Display-P4-Keyboard V1.0 | 2025-09-12 | Initial keyboard expansion board |
| T-Display-P4 V2.0 Evaluation Preview | 2026-05-22 | Evaluation-preview hardware documentation |

## Preview

<p align="center">
  <img src="image/2.jpg" alt="T-Display-P4 preview 1" width="24%">
  <img src="image/3.jpg" alt="T-Display-P4 preview 2" width="24%">
  <img src="image/4.jpg" alt="T-Display-P4 preview 3" width="24%">
  <img src="image/5.jpg" alt="T-Display-P4 preview 4" width="24%">
</p>

## Supported Frameworks

| Framework | Status | Version |
| --- | --- | --- |
| ESP-IDF | Recommended | `>= v5.5.4` |

## Quick Start

### Build With ESP-IDF

Install ESP-IDF first. For environment setup, refer to the official guide:
[ESP-IDF Get Started](https://docs.espressif.com/projects/esp-idf/en/latest/esp32p4/get-started/index.html)

Clone the repository together with its submodules:

```bash
git clone --recursive https://github.com/Xinyuan-LilyGO/T-Display-P4.git
```

If the repository was cloned without `--recursive`, initialize the submodules manually:

```bash
git submodule update --init --recursive
```

Select an example in `menuconfig`, then build and flash the project:

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

### Examples

| Example Link | Description |
| --- | --- |
| [`lilygobox-espidf`](./apps/lilygobox-espidf) | Factory application example for T-Display-P4 |
| [`lilygo_device_driver_example`](./apps/lilygo_device_driver_example) | Shared examples maintained in a dedicated application project |
| [`main/examples`](./main/examples) | Examples specific to the T-Display-P4 board |
| [`main/keyboard_examples`](./main/keyboard_examples) | Examples for the keyboard expansion board |

Open an example link above for the current source and supported example list.

### Prebuilt Firmware

The following firmware is prebuilt.

To flash prebuilt firmware, refer to Espressif's official [ESP firmware online flashing platform guide](https://docs.espressif.com/projects/esp-techpedia/en/latest/esp-friends/get-started/try-firmware/try-firmware-platform.html).

| Firmware | Flash Address | Description |
| --- | --- | --- |
| [`[T-Display-P4][esp32c6][network_adapter]`](<./firmware/[T-Display-P4][esp32c6][network_adapter]>) | `0x0` | Factory network-adapter firmware for the ESP32-C6 onboard the T-Display-P4 |
| [`[T-Display-P4][edge_agent]`](<./firmware/[T-Display-P4][edge_agent]>) | `0x0` | Edge Agent application firmware |
| [`[T-Display-P4][xiaozhi]`](<./firmware/[T-Display-P4][xiaozhi]>) | `0x0` | Xiaozhi application firmware |

> [!IMPORTANT]
> Select `ESP32-P4` when flashing T-Display-P4 application firmware. Select `ESP32-C6` when flashing ESP32-C6 network-adapter firmware. Firmware images are not interchangeable between the two chips.

## Hardware Modules

### T-Display-P4 V1.0

| Function | Device | Interface |
| --- | --- | --- |
| Main MCU | ESP32-P4, 16 MB flash | — |
| Wireless coprocessor | ESP32-C6-MINI-1U / ESP32-C6-FH4, 4 MB flash | SDIO |
| TFT display and touch | H0405S002T002-V0 / HI8561, 4.05 inch, 540 x 1168 | MIPI-DSI / I2C |
| AMOLED display and touch | H0410S001AMT001-V0 / RM69A10 / GT9895, 4.1 inch, 568 x 1232 | MIPI-DSI / I2C |
| Audio | ES8311 / NS4150B / electret condenser microphone | I2S |
| LoRa | SX1262 / LR2021 | SPI |
| GPS | L76K | UART |
| RTC | PCF8563 | I2C |
| IMU | ICM20948 | I2C |
| Vibration | AW86224AFCR | I2C |
| Charging | LGS4056H with battery NTC support | — |
| Battery gauge | BQ27220 | I2C |
| Camera | OV2710 | MIPI-CSI |
| SD card | SD card slot | SDMMC / SPI |
| Ethernet | IP101GRI | RMII |
| IO expansion | XL9535 | I2C |

- Related documents:
  - [Espressif technical documents](https://www.espressif.com/en/support/documents/technical-documents)
  - [ESP32-C6-MINI-1/MINI-1U datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c6-mini-1_mini-1u_datasheet_en.pdf)
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

| Function | Device | Interface |
| --- | --- | --- |
| Keyboard controller | TCA8418 | I2C |
| Keyboard backlight | SY7200A | PWM |
| IO expansion | XL9555 | I2C |
| Sub-GHz radio | CC1101 on T-MixRF | SPI |
| 2.4 GHz radio | NRF24L01 on T-MixRF | SPI |
| NFC | ST25R3916 on T-MixRF | SPI |

- Related documents:
  - [TCA8418](./docs/tca8418.pdf)
  - [SY7200A](./docs/SY7200AABC.pdf)
  - [XL9555](./docs/XL95x5.pdf)
  - [CC1101](./docs/cc1101.pdf)
  - [NRF24L01](./docs/NRF24L01P-R.pdf)
  - [ST25R3916](./docs/st25r3916.pdf)

## Pin Overview

Board pin definitions are provided in the device-driver configuration files:

- [`t_display_p4_config.h`](./libraries/lilygo_device_driver/src/device/t_display_p4/t_display_p4_config.h)
- [`t_display_p4_keyboard_config.h`](./libraries/lilygo_device_driver/src/device/t_display_p4/t_display_p4_keyboard_config.h)

## Project Materials

| Document | Description |
| --- | --- |
| [`T-Display-P4 V1.0 TFT`](<./project/T-Display-P4_V1.0(H0405S002T002-V0)_202601061148.pdf>) | V1.0 hardware project PDF for the 4.05-inch TFT assembly |
| [`T-Display-P4 V1.0 AMOLED`](<./project/T-Display-P4_V1.0(H0410S001AMT001-V0)_202601061148.pdf>) | V1.0 hardware project PDF for the 4.1-inch AMOLED assembly |
| [`T-Display-P4 V2.0 Evaluation Preview`](<./project/T-Display-P4_v2.0(Evaluation Preview)_202605221021.pdf>) | V2.0 evaluation-preview hardware project PDF |
| [`docs`](./docs) | Datasheets and peripheral documentation |
| [`Power Consumption Test Log`](<./relevant_test/PowerConsumptionTestLog_[T-Display-P4_V1.0]_20250605.pdf>) | T-Display-P4 V1.0 power-consumption measurements |

## FAQ

<details>
<summary>Q. Why does the board continuously fail to flash?</summary>

A. Hold down the `BOOT` button while starting the download, then release it after flashing begins.

</details>

<details>
<summary>Q. Why are submodule headers or components missing?</summary>

A. Initialize all repository submodules:

```bash
git submodule update --init --recursive
```

</details>

<details>
<summary>Q. How do I select a different example?</summary>

A. Run `idf.py menuconfig`, open `Example Configuration`, select the desired example, save, and rebuild the project.

</details>

# MeshCore Multi-Tool Firmware

## Project: LilyGo T-Display-P4 Unified Firmware

**Version:** 0.1.0 (Planning)  
**Date:** May 19, 2026  
**Repository:** [hasmeni/T-Display-P4](https://github.com/hasmeni/T-Display-P4)  
**License:** GPL 3.0  

---

## Table of Contents

1. [Overview](#overview)
2. [Hardware Platform](#hardware-platform)
3. [Architecture](#architecture)
4. [Module Breakdown](#module-breakdown)
   - [MeshCore Communications](#1-meshcore-communications)
   - [Security/Pen-test Tools](#2-securitypen-test-tools)
   - [Weather Station](#3-weather-station--environmental-monitor)
   - [Camera Surveillance](#4-camera--surveillance)
   - [Music Player](#5-music--media-player)
   - [Smart Home Controller](#6-smart-home-controller)
   - [Game Console](#7-portable-game-console)
   - [Radio Scanner](#8-digital-radio-scanner)
5. [Implementation Phases](#implementation-phases)
6. [Technical Considerations](#technical-considerations)
7. [Dependencies](#dependencies)
8. [Build System](#build-system)

---

## Overview

A unified, multi-function firmware for the LilyGo T-Display-P4 that combines off-grid mesh communication (MeshCore protocol), security analysis tools, environmental monitoring, camera surveillance, media playback, smart home control, gaming, and RF spectrum analysis into a single modular application.

The firmware uses an **app launcher shell** architecture where the MeshCore mesh protocol runs as an always-on background service, and individual tool modules are launched on-demand from a touchscreen UI.

---

## Hardware Platform

### Core Specifications

| Component | Details |
|-----------|---------|
| **MCU** | ESP32-P4 (high-performance dual-core, 16MB flash, hex-mode PSRAM @ 200MHz) |
| **Auxiliary MCU** | ESP32-C6-MINI-1U (WiFi 6 / BLE 5.0 via SDIO, 4MB flash) |
| **Display Option 1** | HI8561 - 4.05" TFT, 540x1168px, MIPI DSI, 326 PPI, 10-point touch |
| **Display Option 2** | RM69A10 - 4.1" AMOLED, 568x1232px, MIPI DSI, 10-point touch, 20000:1 contrast |
| **Camera** | OV2710 / SC2336 / OV5645 (MIPI CSI, up to 1920x1080) |
| **Flash** | 16MB |
| **PSRAM** | Hex-mode @ 200MHz |
| **L2 Cache** | 256KB |

### Onboard Peripherals

| Category | Hardware | Interface |
|----------|----------|-----------|
| **LoRa Radio** | SX1262 + SKY13453 RF switch | SPI |
| **GPS** | L76K | UART |
| **Ethernet** | IP101GRI | RMII |
| **Audio DAC** | ES8311 | I2S |
| **Amplifier** | NS4150B | - |
| **Microphone** | Electret condenser | I2S (via ES8311) |
| **IMU** | ICM20948 (9-axis) | I2C |
| **Haptics** | AW86224 vibration motor | I2C |
| **RTC** | PCF8563 | I2C |
| **Battery Gauge** | BQ27220 | I2C |
| **Charger** | LGS4056H | - |
| **I/O Expander** | XL9535 | I2C |
| **Power Mgmt** | SGM38121 LDO | I2C |
| **Storage** | SD card | SDMMC |
| **USB** | TinyUSB CDC | USB |

### Keyboard Expansion Board (Optional)

| Component | Details | Interface |
|-----------|---------|-----------|
| **Keyboard** | TCA8418 matrix (10x7 QWERTY) | I2C |
| **Sub-GHz Radio** | CC1101 (300-928MHz) | SPI |
| **2.4GHz Radio** | NRF24L01 | SPI |
| **NFC** | ST25R3916 | SPI |
| **Charger** | BQ25896 | I2C |
| **I/O Expander** | XL9555 | I2C |
| **Backlight** | SY7200A | PWM |

---

## Architecture

### System Architecture Diagram

```
+-------------------------------------------------------------+
|                      LVGL 9 UI Shell                         |
|  +----------+ +----------+ +----------+ +--------+          |
|  | MeshCore | | Security | |  Weather | | Camera |          |
|  |   Comms  | |  Tools   | |  Station | |  Surv. |          |
|  +----------+ +----------+ +----------+ +--------+          |
|  +----------+ +----------+ +----------+ +--------+          |
|  |  Music   | |  Smart   | |   Games  | | Radio  |          |
|  |  Player  | |  Home    | |  Console | |Scanner |          |
|  +----------+ +----------+ +----------+ +--------+          |
+-------------------------------------------------------------+
|                    Core Services Layer                        |
|  +---------+ +--------+ +------+ +------+ +-----+ +------+  |
|  |MeshCore | | Radio  | | GPS  | |Audio | | Net | |Battery|  |
|  |Protocol | |Manager | | Svc  | | Svc  | | Svc | | Svc  |  |
|  +---------+ +--------+ +------+ +------+ +-----+ +------+  |
+-------------------------------------------------------------+
|              Hardware Abstraction (cpp_bus_driver)            |
|  SX1262 | CC1101 | NRF24 | NFC | WiFi | Camera | IMU       |
|  GPS | Audio | Display | Touch | Battery | RTC | SD         |
+-------------------------------------------------------------+
|                    ESP-IDF v5.4.x / FreeRTOS                 |
+-------------------------------------------------------------+
```

### Design Principles

1. **Modular App System** - Each feature is an independent module with its own UI window
2. **Background Services** - MeshCore, GPS, and battery monitoring run continuously as FreeRTOS tasks
3. **Radio Resource Manager** - Arbitrates access to shared radio hardware (SX1262 between MeshCore and scanner)
4. **Event-Driven** - Inter-module communication via FreeRTOS event groups and queues
5. **Memory-Aware** - Apps that use heavy resources (camera, games) release memory on exit
6. **Power-Conscious** - Deep sleep integration, radio duty cycling, display dimming

### App Lifecycle

```
App States: UNLOADED -> LOADING -> ACTIVE -> SUSPENDED -> UNLOADED
                                      |
                                      v
                               BACKGROUND (for MeshCore)
```

---

## Module Breakdown

### 1. MeshCore Communications

**Priority:** Critical (Phase 1)  
**Status:** Planned  
**MeshCore Repository:** [github.com/meshcore-dev/MeshCore](https://github.com/meshcore-dev/MeshCore)

#### Description

Integration of the MeshCore mesh networking protocol for off-grid, encrypted, multi-hop LoRa communication. This is the backbone service that runs continuously and provides communication infrastructure for all other modules.

#### Features

- **Mesh Networking:** Self-organizing multi-hop routing via SX1262 LoRa
- **Device Roles:** Selectable via UI - Client (Companion), Repeater, or Room Server
- **Encryption:** End-to-end encrypted messaging (public key based)
- **Contact Management:** Add/remove contacts, vanity key generation
- **Message Types:** Direct messages, room/group messages, broadcast
- **Background Service:** Always-on mesh participation even when other apps are active
- **GPS Integration:** Share location with mesh contacts
- **Alert Forwarding:** Other modules (camera, weather) can send alerts via mesh

#### Technical Integration

- MeshCore uses RadioLib for radio abstraction - already a component in this project
- SX1262 is the primary radio for MeshCore
- BLE companion mode via ESP32-C6 for mobile app connectivity
- Message store on SD card for Room Server mode

#### UI Elements

- Chat interface with conversation threads
- Network topology visualizer (node map)
- Contact list with signal strength indicators
- Settings: frequency, power, role selection, sync word
- Notification badges on app icon for new messages

---

### 2. Security/Pen-test Tools

**Priority:** High (Phase 3)  
**Status:** Planned

#### Description

Multi-radio security analysis toolkit leveraging all available radios for educational RF security research and network analysis.

#### Features

##### WiFi (via ESP32-C6)
- Network scanner (SSID, BSSID, channel, signal strength, encryption type)
- Deauthentication packet detector
- Beacon frame analysis
- Connected client enumeration
- Channel utilization graph

##### Sub-GHz (CC1101 - Keyboard expansion)
- Frequency scanner (300-928MHz range)
- Signal recording and analysis
- Protocol identification (common IoT protocols)
- Signal strength vs. frequency waterfall display
- Known device signature database

##### 2.4GHz (NRF24L01 - Keyboard expansion)
- BLE advertisement scanner
- Device discovery and fingerprinting
- Channel hopping analysis
- Signal strength heatmap per channel

##### NFC (ST25R3916 - Keyboard expansion)
- Tag type identification (NTAG, Mifare, etc.)
- UID reading and display
- NDEF record parsing
- Memory dump viewer (hex display)
- Tag information summary

#### UI Elements

- Tabbed interface per radio type
- Real-time signal graphs and waterfall displays
- Device/network list with details panel
- Capture log with export to SD card

---

### 3. Weather Station / Environmental Monitor

**Priority:** Medium (Phase 4)  
**Status:** Planned

#### Description

Environmental monitoring with local display and remote reporting via MeshCore mesh network. Can receive sensor data from remote MeshCore-connected nodes.

#### Features

- **Local Sensors:** IMU-derived barometric estimation, GPS altitude
- **External Sensors:** Support for I2C/UART sensors via expansion headers (BME280, SHT31, etc.)
- **LoRa Reporting:** Periodic environmental data broadcast over MeshCore
- **Remote Node Support:** Receive and display data from other mesh sensor nodes
- **Data Logging:** Store readings to SD card with timestamps
- **Trend Analysis:** Historical graphs with configurable time windows
- **Alerts:** Configurable thresholds trigger MeshCore alerts

#### UI Elements

- Dashboard with large current readings
- Multi-day trend graphs (pressure, altitude changes)
- Remote node status cards
- Alert configuration panel
- Data export options

---

### 4. Camera / Surveillance

**Priority:** High (Phase 2)  
**Status:** Planned

#### Description

Camera-based surveillance system with motion detection and LoRa-based alerting via MeshCore.

#### Features

- **Live Preview:** Real-time camera feed on display (up to 1080p downscaled)
- **Motion Detection:** Frame differencing algorithm with configurable sensitivity
- **Photo Capture:** Save images to SD card with timestamp naming
- **Time-lapse Mode:** Configurable interval capture
- **LoRa Alerts:** Send MeshCore message to contacts when motion detected
- **Alert Zones:** Define regions of interest for motion detection
- **Gallery:** Browse captured images on-screen
- **Night Mode:** Low-light enhancement (if camera supports)

#### Technical Notes

- Uses esp_video component with ISP pipeline
- PPA (Pixel Processing Accelerator) for efficient frame rotation/scaling
- Frame buffer in PSRAM for motion comparison
- Motion detection runs on separate FreeRTOS task

#### UI Elements

- Full-screen viewfinder with overlay controls
- Motion sensitivity slider
- Alert zone drawing tool (touch-based)
- Capture gallery with thumbnails
- Settings: resolution, capture interval, alert contacts

---

### 5. Music / Media Player

**Priority:** High (Phase 2)  
**Status:** Planned (factory firmware already has basic WAV playback)

#### Description

Enhanced media player with audio visualizations, playlist management, and Bluetooth streaming capability.

#### Features

- **Playback:** WAV/PCM files from SD card
- **BLE Audio Streaming:** Via ESP32-C6 as Bluetooth audio source to headphones/speakers
- **Visualizations:** Real-time FFT spectrum analyzer, waveform display, VU meters
- **Playlist Management:** Create, edit, shuffle, repeat playlists
- **EQ Settings:** Software-based equalizer presets
- **Haptic Feedback:** Vibration on UI interactions
- **Background Playback:** Continue playing while in other apps
- **Album Art:** Display embedded or folder-based artwork

#### Technical Notes

- ES8311 DAC for local speaker output
- I2S DMA for smooth playback
- FFT computed on dedicated task for visualizations
- SD card buffering for gapless playback

#### UI Elements

- Now Playing screen with album art and visualizer
- Playlist/folder browser
- Progress bar with seek
- EQ configuration
- Volume control with haptic detents

---

### 6. Smart Home Controller

**Priority:** Medium (Phase 4)  
**Status:** Planned

#### Description

Touchscreen dashboard for controlling smart home devices via multiple protocols.

#### Features

- **WiFi/MQTT:** (via ESP32-C6) Connect to Home Assistant, Node-RED, or custom MQTT brokers
- **HTTP REST:** Control devices via REST APIs
- **LoRa/MeshCore:** Control off-grid IoT nodes via mesh
- **NFC Automation:** Tap NFC tags to trigger actions/scenes
- **Device Dashboard:** Room-based tile layout with real-time status
- **Scenes:** One-tap activation of multi-device configurations
- **Automation Rules:** Simple if/then rules engine (if sensor > threshold, then action)
- **Notifications:** Receive device state changes as alerts

#### UI Elements

- Room selector with device tiles (lights, switches, sensors, thermostats)
- Scene buttons with icons
- Device detail view with controls
- Automation rule builder
- MQTT connection settings
- NFC tag programming interface

---

### 7. Portable Game Console

**Priority:** Medium (Phase 5)  
**Status:** Planned

#### Description

LVGL-based gaming platform leveraging the high-resolution display, touch input, IMU, and optional keyboard.

#### Features

- **Game Engine:** Simple 2D engine built on LVGL canvas
- **Input Methods:** Touch, IMU tilt, keyboard (expansion board)
- **Built-in Games:** Snake, Tetris, 2048, Pong, Flappy Bird clone, simple RPG
- **Sound Effects:** Via ES8311 speaker
- **Haptic Feedback:** Vibration on collisions/events
- **High Scores:** Persistent storage on SD card
- **Multiplayer:** LoRa peer-to-peer gaming via MeshCore (turn-based)
- **Custom Games:** Potential for loading game scripts from SD

#### Technical Notes

- LVGL canvas widget for pixel-level game rendering
- Dedicated game loop task with configurable FPS
- IMU polling at 100Hz for tilt-based games
- Audio mixing for simultaneous sound effects

#### UI Elements

- Game launcher with cover art
- In-game overlay (pause, settings)
- High score tables
- Multiplayer lobby (via MeshCore)
- Control configuration

---

### 8. Digital Radio Scanner

**Priority:** High (Phase 3)  
**Status:** Planned

#### Description

Multi-band RF spectrum analysis tool utilizing all available radio hardware for visualizing and analyzing the electromagnetic environment.

#### Features

##### SX1262 (LoRa Band)
- LoRa signal scanner (868/915MHz ISM band)
- RSSI sweep across frequency range
- Packet detection and protocol identification
- Signal strength over time (waterfall)

##### CC1101 (Sub-GHz - Keyboard expansion)
- Wideband frequency sweep (300-928MHz)
- Signal strength per channel
- Continuous monitoring mode
- Signal capture and replay (educational)
- Bookmark interesting frequencies

##### NRF24L01 (2.4GHz - Keyboard expansion)
- All 125 channel scan (2.400-2.525GHz)
- WiFi/BLE/Zigbee interference visualization
- Channel occupancy percentage
- Temporal activity patterns

##### Combined View
- Multi-band overview dashboard
- Cross-reference detected signals
- Export scan data to SD card

#### UI Elements

- Spectrum analyzer (amplitude vs. frequency)
- Waterfall display (frequency vs. time vs. amplitude)
- Channel list with activity indicators
- Signal detail view (modulation, bandwidth estimation)
- Frequency bookmarks and notes

---

## Implementation Phases

### Phase 1: Core Shell + MeshCore (Weeks 1-4)

| Task | Description | Priority |
|------|-------------|----------|
| App Shell | LVGL app launcher with navigation, status bar, app lifecycle management | Critical |
| MeshCore Port | Integrate MeshCore library with RadioLib/SX1262 | Critical |
| Background Services | FreeRTOS tasks for MeshCore, battery, RTC | Critical |
| Chat UI | Basic messaging interface | Critical |
| Radio Manager | SX1262 sharing between MeshCore and other apps | Critical |

### Phase 2: Camera + Music (Weeks 5-8)

| Task | Description | Priority |
|------|-------------|----------|
| Camera Surveillance | Motion detection, capture, LoRa alerts | High |
| Music Player | Enhanced playback, visualizations, playlists | High |
| BLE Audio | ESP32-C6 Bluetooth audio streaming | High |
| Background Audio | Play music while in other apps | High |

### Phase 3: Radio Scanner + Security (Weeks 9-12)

| Task | Description | Priority |
|------|-------------|----------|
| Radio Scanner | Multi-band spectrum display | High |
| WiFi Scanner | Network analysis via ESP32-C6 | High |
| NFC Tools | Tag reading/writing interface | High |
| Sub-GHz Analysis | CC1101 frequency scanning | High |

### Phase 4: Weather + Smart Home (Weeks 13-16)

| Task | Description | Priority |
|------|-------------|----------|
| Weather Station | Sensor dashboard, data logging, mesh reporting | Medium |
| Smart Home | MQTT integration, device dashboard | Medium |
| Automation | Rules engine, NFC triggers | Medium |
| Remote Sensors | Receive data from mesh sensor nodes | Medium |

### Phase 5: Games + Polish (Weeks 17-20)

| Task | Description | Priority |
|------|-------------|----------|
| Game Engine | 2D game framework on LVGL canvas | Medium |
| Built-in Games | 4-6 classic games | Medium |
| Multiplayer | LoRa peer-to-peer turn-based gaming | Medium |
| Polish | UI refinement, performance optimization, bug fixes | Medium |
| Documentation | User guide, developer API docs | Medium |

---

## Technical Considerations

### Memory Management

- **App Partition:** 15MB factory app space
- **PSRAM:** Used for frame buffers, audio buffers, large data structures
- **Heap Strategy:** Apps allocate on entry, free on exit; heavy apps (camera, games) cannot coexist in memory
- **L2 Cache:** 256KB configured for optimal DMA/display performance

### Radio Resource Sharing

```
Radio Priority System:
1. MeshCore (always-on, uses SX1262 time slots)
2. Active app radio requests (scanner, security tools)
3. Background reporting (weather, smart home)

Resolution: Time-division multiplexing with MeshCore getting
priority slots. Other apps request radio access via RadioManager
service which queues and schedules transmissions.
```

### Power Management

- **Active Mode:** Full speed, all peripherals enabled (~500mA with screen + radio)
- **Idle Mode:** Screen dimmed, radio duty-cycled (~100mA)
- **Sleep Mode:** Deep sleep with RTC wakeup, MeshCore light-sleep relay (~1.2mA)
- **Battery Life:** BQ27220 fuel gauge provides accurate SOC, charging via LGS4056H

### Display Performance

- **MIPI DSI:** 2 lanes @ 1000 Mbps
- **DPI Clock:** 60MHz
- **Refresh:** 30ms LVGL refresh period
- **Color:** RGB565 (16-bit) for performance, RGB888 (24-bit) optional
- **Rotation:** PPA hardware accelerator for screen rotation
- **DMA2D:** Hardware-accelerated buffer copies

### Build Configuration

The firmware uses ESP-IDF's Kconfig system for compile-time configuration:
- Board type (T-Display-P4 / T-Display-P4-Keyboard)
- Screen type (HI8561 TFT / RM69A10 AMOLED)
- Camera type (OV2710 / SC2336 / OV5645)
- Pixel format (RGB565 / RGB888)
- Module enable/disable flags

---

## Dependencies

### Core Framework
- **ESP-IDF:** v5.4.x
- **LVGL:** v9.3.0
- **FreeRTOS:** (included in ESP-IDF)

### Libraries
- **MeshCore:** [github.com/meshcore-dev/MeshCore](https://github.com/meshcore-dev/MeshCore)
- **RadioLib:** Multi-radio library (SX1262, CC1101, NRF24L01)
- **cpp_bus_driver:** Hardware abstraction (I2C, SPI, UART devices)
- **esp_video:** Camera ISP pipeline
- **esp-sr:** Speech recognition (optional future enhancement)
- **esp_hosted:** WiFi networking via ESP32-C6
- **esp_tinyusb:** USB CDC serial
- **ICM20948_WE:** IMU driver
- **ST25R3916 / NFC-RFAL:** NFC stack
- **kode_bq25896:** Battery charger driver (keyboard variant)

### External Tools
- **ESP-IDF v5.4.1:** Build toolchain
- **VS Code + ESP-IDF Extension:** Development environment
- **esptool.py:** Firmware flashing

---

## Build System

### Project Structure (Proposed)

```
T-Display-P4/
+-- CMakeLists.txt
+-- sdkconfig.defaults
+-- partitions.csv
+-- main/
|   +-- CMakeLists.txt
|   +-- Kconfig.projbuild
|   +-- app_shell/              # App launcher & lifecycle
|   |   +-- app_shell.cpp
|   |   +-- app_shell.h
|   |   +-- app_registry.cpp
|   +-- services/               # Background services
|   |   +-- meshcore_service.cpp
|   |   +-- radio_manager.cpp
|   |   +-- gps_service.cpp
|   |   +-- battery_service.cpp
|   |   +-- audio_service.cpp
|   |   +-- network_service.cpp
|   +-- apps/                   # Application modules
|   |   +-- meshcore_chat/
|   |   +-- security_tools/
|   |   +-- weather_station/
|   |   +-- camera_surveillance/
|   |   +-- music_player/
|   |   +-- smart_home/
|   |   +-- game_console/
|   |   +-- radio_scanner/
|   +-- ui/                     # Shared UI components
|       +-- widgets/
|       +-- themes/
|       +-- fonts/
|       +-- images/
+-- components/
|   +-- meshcore/               # MeshCore protocol library
|   +-- cpp_bus_driver/         # Hardware abstraction
|   +-- RadioLib/               # Multi-radio library
|   +-- private_library/        # Board-specific drivers
|   +-- esp_video/              # Camera pipeline
|   +-- ST25R3916/              # NFC stack
|   +-- NFC-RFAL/
|   +-- ICM20948_WE/
|   +-- kode_bq25896/
+-- docs/
    +-- PROJECT_PLAN.md
    +-- PROJECT_PLAN.html
```

### Build Commands

```bash
# Configure target
idf.py set-target esp32p4

# Open menuconfig for module selection
idf.py menuconfig

# Build
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## Contributing

This is a personal project. Contributions via pull request are welcome for:
- Bug fixes
- New game implementations
- Additional smart home integrations
- Radio protocol decoders
- UI improvements

---

## License

GPL 3.0 - See [LICENSE](../LICENSE) file for details.

---

*Document generated: May 19, 2026*

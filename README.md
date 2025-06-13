<!--
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-06-13 15:12:02
 * @LastEditTime: 2025-06-13 18:07:58
 * @License: GPL 3.0
-->
<h1 align = "center">T-Display-P4</h1>

## **English | [中文](./README_CN.md)**

## VersionIteration:
| Version                               | Update date                       |Update description|
| :-------------------------------: | :-------------------------------: |:--------------: |
| T-Display-P4_V1.0                      | 2025-06-13                    |   Original version      |

## PurchaseLink

| Product                     | SOC           |  FLASH  |  PSRAM   | Link                   |
| :------------------------: | :-----------: |:-------: | :---------: | :------------------: |
| T-Display-P4_V1.0   | NULL |   NULL   | NULL |  [NULL]()   |

## Directory
- [Describe](#describe)
- [Preview](#preview)
- [Module](#module)
- [SoftwareDeployment](#SoftwareDeployment)
- [PinOverview](#pinoverview)
- [RelatedTests](#RelatedTests)
- [FAQ](#faq)
- [Project](#project)

## Describe

The T-Display-P4 is a versatile development board based on the ESP32-P4 core. Its features include:  

1. **High Processing Power**: Equipped with the high-performance ESP32-P4 core processor, it can handle more complex graphics and video tasks, delivering smoother display performance.  
2. **Low Power Design**: Offers multiple selectable power modes to effectively reduce energy consumption and extend battery life.  
3. **High-Resolution Display**: Supports high resolution (default with a large MIPI interface screen at 540x1168px), providing sharp and clear visuals.  
4. **Rich Peripheral Support**: Onboard peripherals include an HD MIPI touchscreen, ESP32-C6 module, speaker, microphone, LoRa module, GPS module, Ethernet, a linear vibration motor, an independent battery gauge for monitoring battery health and percentage, and an MIPI camera. Multiple GPIOs of both the ESP32-P4 and ESP32-C6 are exposed, enhancing the device's expandability.  

## Preview

### Actual Product Image

## Module
Here is the format-preserved English translation of your text:  

---  

### 1. Core Processor  

* Chip: ESP32-P4  
* FLASH: 32M  
* Related Documents:  
    >[Espressif](https://www.espressif.com/en/support/documents/technical-documents)  

### 2. Display & Touch  

> #### Model: H0405S002T002-V0  
> * Display Size (Diagonal): 4.05 inch  
> * LCD Type: α-Si TFT  
> * Resolution: 540(H) × 1168(V) px  
> * Active Area: 41.9904(W) × 91.1040(H) mm  
> * Module Dimensions: 44(H) × 95.5(V) × 1.46(T) mm  
> * Display Colors: 16.7M  
> * Display Interface: MIPI  
> * Touch Interface: IIC
> * Display & Touch Driver IC: HI8561  
> * Related Documents:  
>    >[HI8561](./information/HI8561_Preliminary%20_DS_V0.00_20230511.pdf)  

* Dependent Libraries:  
    >[cpp_bus_driver-v1.0.0](https://github.com/Llgok/cpp_bus_driver)  

### 3. Speaker & Microphone  

* DAC Chip: ES8311  
* Amplifier Chip: NS4150B  
* Microphone: Electret Condenser Mic  
* Communication Protocol: IIS
* Related Documents:  
    >[ES8311](./information/ES8311.pdf)  
    >[NS4150B](./information/NS4150B.pdf)  
* Dependent Libraries:  
    >[cpp_bus_driver-v1.0.0](https://github.com/Llgok/cpp_bus_driver)  

### 4. Vibration  

* Driver IC: AW86224AFCR  
* Communication Protocol: IIC
* Related Documents:  
    >[AW86224](./information/AW86224AFCR.pdf)  
* Dependent Libraries:  
    >[cpp_bus_driver-v1.0.0](https://github.com/Llgok/cpp_bus_driver)  

### 5. LoRa  

* Module: HPD16A  
* Chip: SX1262  
* Communication Protocol: Standard SPI  
* Related Documents:  
    >[HPD16A](./information/HPDTEK_HPD16A_TCXO_V1.1.pdf)  <br />  
    >[SX1261-2](./information/DS_SX1261-2_V2_1.pdf)  
* Dependent Libraries:  
    >[cpp_bus_driver-v1.0.0](https://github.com/Llgok/cpp_bus_driver)  

### 6. GPS  

* Module: L76K  
* Communication Protocol: Uart
* Related Documents:  
    >[L76K](./information/L76KB-A58.pdf)  
* Dependent Libraries:  
    >[cpp_bus_driver-v1.0.0](https://github.com/Llgok/cpp_bus_driver)  

### 7. RTC  

* Chip: PCF8563  
* Communication Protocol: IIC
* Related Documents:  
    >[PCF8563](./information/PCF8563.pdf)  
* Dependent Libraries:  
    >[cpp_bus_driver-v1.0.0](https://github.com/Llgok/cpp_bus_driver)  

### 8. Charging IC  

* Chip: LGS4056H  
* Additional Notes: The NTC pin of the 3-wire battery is connected to the LGS4056H charging IC. Over-temperature protection during charging is automatically controlled by the chip.  
* Related Documents:  
    >[LGS4056H](./information/LGS4056H.pdf)  

### 9. Battery Gauge  

* Chip: BQ27220  
* Communication Protocol: IIC
* Related Documents:  
    >[BQ27220](./information/bq27220_en.pdf)  
* Dependent Libraries:  
    >[cpp_bus_driver-v1.0.0](https://github.com/Llgok/cpp_bus_driver)  

### 10. Camera  

> #### Model: OV2710  
> * Interface: MIPI  
> * Related Documents:  
>    >[OV2710](./information/OV2710_CSP3_DS_2.0_KING%20HORN%20ENTERPRISES%20Ltd..pdf)  


## SoftwareDeployment

### Examples Support

| Example | `[Platformio IDE][espressif32-v6.5.0]`<br />`[Arduino IDE][esp32_v2.0.14]` | Description | Picture |
| ------  | ------ | ------ | ------ | 
| [CST226SE](./examples/CST226SE) |  <p align="center">![alt text][supported] | | |
| [Deep_Sleep_Wake_Up](./examples/Deep_Sleep_Wake_Up) |  <p align="center">![alt text][supported] | | |
| [DMIC_ReadData](./examples/DMIC_ReadData) |  <p align="center">![alt text][supported] | | |
| [DMIC_SD](./examples/DMIC_SD) |  <p align="center">![alt text][supported] | | |
| [Get_HTTP_Response_Time](./examples/Get_HTTP_Response_Time) |  <p align="center">![alt text][supported] | | |
| [GFX](./examples/GFX) |  <p align="center">![alt text][supported] | | |
| [IIC_Scan_2](./examples/IIC_Scan_2) |  <p align="center">![alt text][supported] | | |
| [LR1121_PingPong](./examples/LR1121_PingPong) |  <p align="center">![alt text][supported] | | |
| [LR1121_PingPong_2](./examples/LR1121_PingPong_2) |  <p align="center">![alt text][supported] | | |
| [LR1121_Walkie_Talkie](./examples/LR1121_Walkie_Talkie) |  <p align="center">![alt text][supported] | | |
| [Original_Test](./examples/Original_Test) |  <p align="center">![alt text][supported] | Original factory program | |
| [PCF85063](./examples/PCF85063) |  <p align="center">![alt text][supported] | | |
| [PCF85063_Scheduled_INT](./examples/PCF85063_Scheduled_INT) |  <p align="center">![alt text][supported] | | |
| [PCF85063_Timer_INT](./examples/PCF85063_Timer_INT) |  <p align="center">![alt text][supported] | | |
| [RT9080](./examples/RT9080) |  <p align="center">![alt text][supported] | | |
| [SD_Explorer_Music](./examples/SD_Explorer_Music) |  <p align="center">![alt text][supported] | | |
| [SD_Music](./examples/SD_Music) |  <p align="center">![alt text][supported] | | |
| [SY6970](./examples/SY6970) |  <p align="center">![alt text][supported] | | |
| [SY6970_OTG](./examples/SY6970_OTG) |  <p align="center">![alt text][supported] | | |
| [USB_Host_Camera_Screen](./examples/USB_Host_Camera_Screen) |  <p align="center">![alt text][supported] | | |
| [Vibration_Motor](./examples/Vibration_Motor) |  <p align="center">![alt text][supported] | | |
| [Voice_Codec2_Speaker](./examples/Voice_Codec2_Speaker) |  <p align="center">![alt text][supported] | | |
| [Voice_Speaker](./examples/Voice_Speaker) |  <p align="center">![alt text][supported] | | |
| [WIFI_HTTP_Download_File](./examples/WIFI_HTTP_Download_File) |  <p align="center">![alt text][supported] | | |
| [WIFI_HTTP_Download_SD_file](./examples/WIFI_HTTP_Download_SD_file) |  <p align="center">![alt text][supported] | | |
| [Wifi_Music](./examples/Wifi_Music) |  <p align="center">![alt text][supported] | | |

[supported]: https://img.shields.io/badge/-supported-green "example"

| Firmware | Description | Picture |
| ------  | ------  | ------ |
| [Original_Test](./firmware/[T-Display-S3-Pro-MVSRLora_V1.0][Original_Test]_firmware_202412131703.bin) | Original factory program |  |

### PlatformIO
1. Install [VisualStudioCode](https://code.visualstudio.com/Download) ,Choose installation based on your system type.

2. Open the "Extension" section of the Visual Studio Code software sidebar(Alternatively, use "<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>X</kbd>" to open the extension),Search for the "PlatformIO IDE" extension and download it.

3. During the installation of the extension, you can go to GitHub to download the program. You can download the main branch by clicking on the "<> Code" with green text, or you can download the program versions from the "Releases" section in the sidebar.

4. After the installation of the extension is completed, open the Explorer in the sidebar(Alternatively, use "<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>E</kbd>" go open it),Click on "Open Folder," locate the project code you just downloaded (the entire folder), and click "Add." At this point, the project files will be added to your workspace.

5. Open the "platformio.ini" file in the project folder (PlatformIO will automatically open the "platformio.ini" file corresponding to the added folder). Under the "[platformio]" section, uncomment and select the example program you want to burn (it should start with "default_envs = xxx") Then click "<kbd>[√](image/4.png)</kbd>" in the bottom left corner to compile,If the compilation is correct, connect the microcontroller to the computer and click "<kbd>[→](image/5.png)</kbd>" in the bottom left corner to download the program.

### Arduino
1. Install [Arduino](https://www.arduino.cc/en/software) ,Choose installation based on your system type.

2. Open the "example" directory within the project folder, select the example project folder, and open the file ending with ".ino" to open the Arduino IDE project workspace.

3. Open the "Tools" menu at the top right -> Select "Board" -> "Board Manager." Find or search for "esp32" and download the board files from the author named "Espressif Systems." Then, go back to the "Board" menu and select the development board type under "ESP32 Arduino." The selected development board type should match the one specified in the "platformio.ini" file under the [env] section with the header "board = xxx." If there is no corresponding development board, you may need to manually add the development board from the "board" directory within your project folder.

4. Open menu bar "[File](image/6.png)" -> "[Preferences](image/6.png)" ,Find "[Sketchbook location](image/7.png)"  here,copy and paste all library files and folders from the "libraries" folder in the project directory into the "libraries" folder in this directory.

5. Select the correct settings in the Tools menu, as shown in the table below.

#### ESP32-S3
| Setting                               | Value                                 |
| :-------------------------------: | :-------------------------------: |
| Board                                 | ESP32S3 Dev Module           |
| Upload Speed                     | 921600                               |
| USB Mode                           | Hardware CDC and JTAG     |
| USB CDC On Boot                | Enabled                              |
| USB Firmware MSC On Boot | Disabled                             |
| USB DFU On Boot                | Disabled                             |
| CPU Frequency                   | 240MHz (WiFi)                    |
| Flash Mode                         | QIO 80MHz                         |
| Flash Size                           | 16MB (128Mb)                    |
| Core Debug Level                | None                                 |
| Partition Scheme                | 16M Flash (3MB APP/9.9MB FATFS) |
| PSRAM                                | OPI PSRAM                         |
| Arduino Runs On                  | Core 1                               |
| Events Run On                     | Core 1                               |           

6. Select the correct port.

7. Click "<kbd>[√](image/8.png)</kbd>" in the upper right corner to compile,If the compilation is correct, connect the microcontroller to the computer,Click "<kbd>[→](image/9.png)</kbd>" in the upper right corner to download.

### firmware download
1. Open the project file "tools" and locate the ESP32 burning tool. Open it.

2. Select the correct burning chip and burning method, then click "OK." As shown in the picture, follow steps 1->2->3->4->5 to burn the program. If the burning is not successful, press and hold the "BOOT-0" button and then download and burn again.

3. Burn the file in the root directory of the project file "[firmware](./firmware/)" file,There is a description of the firmware file version inside, just choose the appropriate version to download.

<p align="center" width="100%">
    <img src="image/10.png" alt="example">
    <img src="image/11.png" alt="example">
</p>


## PinOverview

| Speaker pins  | ESP32S3 pins|
| :------------------: | :------------------:|
| BCLK         | IO4       |
| LRCLK         | IO15       |
| DATA         | IO11       |
| SD_MODE         | IO41       |

| Microphone pins  | ESP32S3 pins|
| :------------------: | :------------------:|
| LRCLK         | IO1       |
| DATA         | IO2       |
| EN         | IO3       |

| Vibration motor pins  | ESP32S3 pins|
| :------------------: | :------------------:|
| DATA         | IO45       |

| RT9080 power pins  | ESP32S3 pins|
| :------------------: | :------------------:|
| EN         | IO42       |

| RTC pins  | ESP32S3 pins|
| :------------------: | :------------------:|
| SDA         | IO5       |
| SCL         | IO6       |
| INT         | IO21       |

| LR1121 pins | ESP32S3 pins|
| :------------------: | :------------------:|
| BUSY         | IO46       |
| INT         | IO40       |
| SCLK         | IO18       |
| MOSI         | IO17       |
| MISO         | IO8       |
| CS         | IO7       |
| RST         | IO10       |

## RelatedTests

### Power Dissipation
| Firmware | Program| Description | Picture |
| ------  | ------  | ------ | ------ | 
| [Deep_Sleep_Wake_Up](./firmware/[T-Display-S3-Pro-MVSRLora_V1.0][Deep_Sleep_Wake_Up]_firmware_202502061152.bin) | [Deep_Sleep_Wake_Up](./examples/Deep_Sleep_Wake_Up) | Static current: 29.99 μA for more information please refer to [Power Consumption Test Log](./relevant_test/PowerConsumptionTestLog_[T-Display-S3-Pro-MVSRLora_V1.0]_20241214.pdf) | |

## FAQ

* Q. After reading the above tutorials, I still don't know how to build a programming environment. What should I do?
* A. If you still don't understand how to build an environment after reading the above tutorials, you can refer to the [LilyGo-Document](https://github.com/Xinyuan-LilyGO/LilyGo-Document) document instructions to build it.

<br />

* Q. Why does Arduino IDE prompt me to update library files when I open it? Should I update them or not?
* A. Choose not to update library files. Different versions of library files may not be mutually compatible, so it is not recommended to update library files.

<br />

* Q. Why is there no serial data output on the "Uart" interface on my board? Is it defective and unusable?
* A. The default project configuration uses the USB interface as Uart0 serial output for debugging purposes. The "Uart" interface is connected to Uart0, so it won't output any data without configuration.<br />For PlatformIO users, please open the project file "platformio.ini" and modify the option under "build_flags = xxx" from "-D ARDUINO_USB_CDC_ON_BOOT=true" to "-D ARDUINO_USB_CDC_ON_BOOT=false" to enable external "Uart" interface.<br />For Arduino users, open the "Tools" menu and select "USB CDC On Boot: Disabled" to enable the external "Uart" interface.

<br />

* Q. Why is my board continuously failing to download the program?
* A. Please hold down the "BOOT-0" button and try downloading the program again.

## Project
* [T-Display-S3-Pro-MVSRLora_V1.0](./project/T-Display-S3-Pro-MVSRLora_V1.0.pdf)



![diyPresso Logo](images/logo_main.png) 

[www.diyPresso.com](https://www.diyPresso.com) world's first DIY espresso machine open source software


![diyPresso Logo](images/diyPresso.jpg)


## Hardware

This repository contains the code for the diyPresso One (arduino) control board with an 4x20 LCD display and rotary encoder.

![diyPresso Logo](images/pcb.png)
![diyPresso LCD](images/lcd.png)
![diyPresso Encoder button](images/encoder.png)


## Software

(Tested) releases of the firmware can be found 
[here](https://github.com/diyPresso/diyPresso-One/releases)

main branch build status: [![Build](https://github.com/diyPresso/diyPresso-One/actions/workflows/main.yml/badge.svg)](https://github.com/diyPresso/diyPresso-One/actions/workflows/main.yml)


## Safety

You can break your hardware (and other things) with the software! Be careful. 
With open-source DiY hardware and software breaking things is part of life (and of the fun!), 
but we are working with high voltage, water, high temperature and pressurized containers, 
so: **be careful!**
There are some sanity checks, watchdogs and other things in the software that provide some protection,
but this is not failsafe. The authors of this software cannot provide any garantee on the
correct and safe functioning of this software. **You** are responsible for the safe usage of this software and hardware.

### Safety Improvements (v2.x)
- **Commissioning Water Verification**: During initial setup, the system now verifies that at least 125 grams (~125ml) of water was pumped from the reservoir to the boiler before allowing commissioning to complete.
- **Dry Boiler Detection**: Temperature rate monitoring detects when the boiler is heating without water by monitoring heating rate. If temperature rises faster than 25°C/min (while above 50°C), heating is automatically shut down with a "DRY_BOILER" error.
- **Commissioning Assumptions**: After successful commissioning, the system assumes the boiler contains water. Always ensure commissioning completes with proper water flow to avoid damage.

### Safety Improvements (v1.9.0-dev)
- **Automatic Boiler Level Checking**: The system now automatically checks and refills the boiler at critical times:
  - **Before sleep**: Ensures boiler is full before entering sleep mode
  - **After startup**: Verifies boiler level after power-on
  - **After brewing**: Replenishes water lost during brewing
  - **Emergency refill**: Automatic refill when dry boiler is detected
- **Smart Refill Detection**: Uses pump + weight scale to detect if boiler is full (no weight change = full)
- **Non-intrusive Operation**: Boiler checks happen at natural transition points, not during normal operation
- **Automatic Shutdown**: System automatically shuts down after 4 hours in sleep mode for safety and energy conservation
- **Easy Wake-up**: Press the dial button to wake up from shutdown (no need for RESET button)

### New Features (v1.8.0) 
- **Sleep Minimum Temperature**: Optional setting to maintain boiler temperature during sleep mode (0°C = disabled, 1-100°C = minimum temperature)
- **Energy Management**: Users can choose between maximum energy savings (sleep temp = 0°C) or faster wake-up times (higher sleep temperatures)
- **Visual Sleep Indicator**: Sleep display shows current minimum temperature when temperature maintenance is active (e.g., "Zzz 65°C")


## Functions
* Commissioning (initial filling of boiler)
* Settings (with EEPROM retention)
* Basic brewing
* Sleep mode with optional minimum temperature maintenance
* Wifi setup
* MQTT communication

## Factory Reset
*  Holding the rotary encoder button WHEN POWERING ON the machine will reset all settings to default values (including the "WiFo Modi" setting). Wifi credential (AP name and password are not stored on the main CPU, but on the WiFi chip, these settings are retained when the firmware is updated or a factory reset is executed.
* When the software is flashed, this will also erase all EEPROM settings (preferences) and you need to re-execute the commissioning steps.
* Enabling the "CONFIG-AP" mode will erase previously configured WiFi credentials



## Wifi Configuration
* Wifi settings can be chaged by setting the "WIFI MODE" setting to "CONFIG-AP" (Configuration Access Point), then SAVE setting and REBOOT the machine . This will create  ad "diyPresso-One" Wifi access point that you can connect to with your phone or laptop.
* Your browser should open a "hotspot" login page where you can enter your Wifi credentials. if not: point your browser to [http://192.168.11.1/
](http://192.168.11.1/)
* Enter the number of your network and the `[SUBMIT]` button.

![diyPresso Wifi config](images/wifi.png)


## Sleep Mode

The diyPresso One supports an energy-saving sleep mode that can be activated manually or automatically after a period of inactivity. When in sleep mode, the machine can optionally maintain a minimum boiler temperature to reduce wake-up time.

### Sleep Mode Features
- **Manual Sleep**: Press and hold the rotary encoder button to enter sleep mode
- **Auto Sleep**: Automatically enters sleep mode after 30 minutes of inactivity (configurable)
- **Wake Up**: Press and hold the rotary encoder button again to wake up from sleep mode
- **Minimum Temperature**: Optional setting to maintain boiler temperature during sleep (0°C = disabled)
- **Visual Indicator**: When maintaining temperature, the sleep display shows "Zzz 65°C" instead of just "Zzz"

### Sleep Minimum Temperature Setting
- **Default**: Disabled (0°C)
- **Range**: 0-100°C (whole degrees only)
- **Purpose**: Keeps boiler warm during sleep to reduce heating time when waking up
- **Energy Trade-off**: Uses more energy but provides faster brewing readiness
- **Setting Location**: Available in the settings menu as "Sleep min temp"

### Usage Recommendations
- Set to 0°C (disabled) for maximum energy savings
- Set to 65-75°C for moderate energy use with reasonable wake-up time
- Set to 80-90°C for fastest wake-up but higher energy consumption
- Consider your usage patterns when choosing a temperature


## Development

The software is written for an Arduino [mkr1010](https://docs.arduino.cc/hardware/mkr-wifi-1010/) board with WiFi. The [Arduino development environment](https://docs.arduino.cc/software/ide/) is to be used to compile and upload the software to the board. Some additional libraries need to be installed. See the [diyp-controler source](diyp-controller/diyp-controller.ino) for more information.

In addition the code should compile with [PlatformIO](https://piolabs.com/). On linux you can perform a compilation and upload with the following command:

```pio run -t upload --upload-port=/dev/ttyACM0```

You may also use [ArduinoCLI](https://arduino.github.io/arduino-cli/1.0/), but the recipe is not included yet...


## Branches and tags

The ```main``` branch is used for development and
```release``` is used for tested releases. They are tagged with a semver and the releases are published [here](https://github.com/diyPresso/diyPresso-One/releases). Use [topic branches](https://git-scm.com/book/en/v2/Git-Branching-Branching-Workflows) for your work on new features and bugs. Use the name ```issue_[xx]_[description]``` for this.


## (Code) Contributions

You can clone this repo, create [topic branches](https://git-scm.com/book/en/v2/Git-Branching-Branching-Workflows) for your work and make [pull-requests](https://github.com/diyPresso/diyPresso-One/pulls). Make sure your branch merges cleanly with the main branch.

## Issues

If you find any bugs, or have ideas for new features, please use github [issues](https://github.com/diyPresso/diyPresso-One/issues) to submit these.

Also, feel free to test topic branches, reproduce bufgs and comment on issues to assist development.

## ToDo
* ~~Create ```platformio.ini```~~
* ~~Build with docker container~~
* Setup CI in gitub
* OTA updates
* Refactor Wifi code (use other library?)
* MQTT credentials setup
* Remote wakeup (via MQTT)
* Support for graphical color display
* Advanced brewing recipes (more steps)
* Brewing recipe storage and retrieval
* Extracted volume weight sensor (and volume based steps)
* Pressure sensor integration

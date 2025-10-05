# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

Overview
- This is a PlatformIO-based Arduino project targeting the Arduino MKR WiFi 1010 (env: mkr_wifi1010). The main firmware lives under diyp-controller/, with an additional OTA example project under OTA/.
- CI builds use a containerized PlatformIO environment and publish the firmware artifact at .pio/build/mkr_wifi1010/firmware.bin.

Common commands
Prereq: PlatformIO Core (pio) must be installed locally, or use the Docker command below.

- Build (root project):
  ```bash path=null start=null
  pio run -e mkr_wifi1010
  ```
- Clean build artifacts:
  ```bash path=null start=null
  pio run -e mkr_wifi1010 -t clean
  ```
- Upload firmware over USB (MKR WiFi 1010):
  - Find the serial device on macOS:
    ```bash path=null start=null
    ls /dev/cu.usbmodem*
    ```
  - Upload (replace the port with the one found above):
    ```bash path=null start=null
    pio run -e mkr_wifi1010 -t upload --upload-port=/dev/cu.usbmodemXXXX
    ```
- Serial monitor (115200 baud):
  ```bash path=null start=null
  pio device monitor -b 115200
  # optionally pin the port
  # pio device monitor -b 115200 -p /dev/cu.usbmodemXXXX
  ```
- Build using Docker (mirrors CI environment):
  ```bash path=null start=null
  docker run --rm \
    -e HOME="$PWD" \
    -w "$PWD" \
    -v "$PWD":"$PWD" \
    ghcr.io/pbrier/platformio:latest \
    platformio run -e mkr_wifi1010
  ```
- OTA example project (under OTA/):
  ```bash path=null start=null
  # build the OTA project without changing directories
  pio -d OTA run -e mkr_wifi1010
  ```
- Tests: There are currently no PlatformIO unit tests in this repository.

High-level architecture
The firmware follows a modular, non-blocking design centered around singleton-style modules and explicit state machines. Key concepts and modules:

- Entry point: diyp-controller/diyp-controller.ino
  - Performs system setup (serial, display, settings load, WiFi, MQTT), then drives the application by delegating to module-level control loops.

- Hardware definition: diyp-controller/dp_hardware.h
  - Central mapping for board revision and pins:
    - SSR outputs (heater, pump), brew switch, HX711 pins for reservoir scale, PT1000 via MAX31865 (SPI pins), I2C display (address 0x27), rotary encoder pins.

- State machines (non-blocking):
  - Boiler state machine (diyp-controller/dp_boiler.h/.cpp)
    - Controls temperature via PID (diyp-controller/dp_pid.*) with feed-forward terms for heat/ready/brew modes.
    - Enforces safety/timeouts and guards (over/under temperature, SSR timeout, control loop timeout) and transitions across OFF/HEATING/READY/BREW/ERROR.
    - Uses MAX31865_NonBlocking for PT1000 readings and heaterDevice PWM control.
  - Brew process (diyp-controller/dp_brew.h/.cpp)
    - Orchestrates a multi-phase brew flow (pre-infuse, infuse, extract, finished), coordinated with reservoir readings and boiler readiness.
    - Implements its own finite-state machine with timers and error handling (purge/fill/timeout/no-water).

- UI and input:
  - Menu and display (diyp-controller/dp_menu.* and dp_display.*)
    - 4x20 character LCD with custom characters and multiple menu screens (main, settings, error, wifi, saved, sleep, warning).
  - Rotary encoder (diyp-controller/dp_encoder.*)
    - Button counts are used for actions like factory reset at boot.

- Peripherals and devices:
  - Heater (diyp-controller/dp_heater.*) — PWM period and power control.
  - Reservoir/scale (diyp-controller/dp_reservoir.*) — HX711-based weight and level tracking, tare functionality.
  - Pump and brew switch (diyp-controller/dp_pump.* and dp_brew_switch.*) — device abstractions for brew actuation.

- Settings and persistence:
  - Settings (diyp-controller/dp_settings.*)
    - Stores brew and control parameters, retains them in flash/EEPROM, supports factory defaults and apply() to push settings into modules.
    - Compile-time version and build metadata: diyp-controller/dp.h (SOFTWARE_VERSION, BUILD_DATE; WATCHDOG_ENABLED toggles watchdog behavior).

- Connectivity:
  - WiFi (diyp-controller/dp_wifi.*) — setup/loop/erase helpers. WiFi credentials are managed on the WiFi module; AP-based configuration is supported per README.
  - MQTT (diyp-controller/dp_mqtt.*)
    - Publishes measurements in an InfluxDB line-like format via ArduinoMqttClient. Key fields published include: t_set, t_act, h_pwr, h_avg, r_lvl, r_wgt, w_cur, w_end, shots, plus state/error strings for boiler/brew/reservoir.

- Serial interface:
  - diyp-controller/dp_serial.* provides a simple 115200 baud text protocol for inspecting and configuring the device at runtime.
    - Commands:
      - GET info — firmware/hardware versions and current states/errors
      - GET settings — dumps current settings
      - PUT settings key1=val1,key2=val2 — updates settings and persists them

Notes from README and CI
- The project can be built with Arduino IDE, but PlatformIO is supported and validated in CI and is the recommended path for consistent CLI usage.
- CI builds run with ghcr.io/pbrier/platformio:latest and archive the firmware at .pio/build/mkr_wifi1010/firmware.bin.

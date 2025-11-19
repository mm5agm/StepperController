# StepperController

## Overview

Use at your own risk. 

This version works for me, it might not work for you. 

I've stopped work on this project for the time being.

StepperController is an ESP32-based stepper motor controller for MagLoop antenna positioning, featuring ESP-NOW wireless communication with a touchscreen GUI (stepperGUI). This version uses a TCRT5000 optical sensor for homing and position detection. Limit switches are not used; all position sensing is handled by the IR sensor.

## Hardware Requirements
* ESP32 DevKit V1
* DM542 Stepper Motor Driver
* NEMA 23 Stepper Motor
* TCRT5000 IR Optical Sensor (for homing)
* 24V DC Power Supply
## Pin Connections

| ESP32 Pin | Connection   | Description                       |
|-----------|-------------|-----------------------------------|
| GPIO 18   | DIR         | Direction signal to DM542          |
| GPIO 19   | STEP        | Step pulse to DM542                |
| GPIO 2    | TCRT5000    | Optical sensor digital output (INPUT) |

## Communication Protocol

ESP-NOW wireless protocol with StepperGUI touchscreen controller.

### Message Format
```cpp
typedef struct __attribute__((packed)) {
    uint8_t messageId;
    CommandType command;
    int32_t param;
} Message;
```

### Supported Commands

## Features
* Non-blocking operation
* Real-time position feedback to GUI
* Optical sensor homing (TCRT5000)
* Multiple speed settings
* Message queuing for ESP-NOW
## Build and Upload

```powershell
platformio run
platformio run -e esp32doit-devkit-v1 --target upload --upload-port COM3
platformio device monitor --baud 115200
```

## Project layout

```
.  (limit switches version)
├── src/
│  └─ main.cpp
├─ MagLoop_Common_Files/   # Git submodule with shared headers
├─ lib/
├─ platformio.ini
├─ download_latest_all.bat
└─ upload_changes_all.bat
```

## Configuration

Pulse delays and limits are declared in `src/main.cpp`. The runtime PD values are persisted using the ESP32 Preferences API under the `stepper` namespace.

Update the GUI MAC address in `src/main.cpp` if needed:

```cpp
const uint8_t GUI_MAC[] = { 0x98, 0xA3, 0x16, 0xE3, 0xFD, 0x4C };
```

## Updating shared files

Run the included batch scripts to synchronize shared files and GUI code:

```bat
download_latest_all.bat
upload_changes_all.bat
```

## State machine

States include `STATE_IDLE`, `STATE_MOVING_UP`, `STATE_MOVING_DOWN`, `STATE_MOVING_TO`, `STATE_MOVE_TO_DOWN_LIMIT`, and `STATE_RESETTING`.

## Troubleshooting
* ESP-NOW issues: verify MAC addresses and peer configuration
* Motor not moving: check wiring, DM542 enable/config and power
* Optical sensor: verify TCRT5000 wiring and digital output logic
## Contributing

1. Fork
2. Create branch
3. Make changes
4. Update `MagLoop_Common_Files` if shared definitions change
5. Test controller + GUI
6. Submit PR


See also: StepperGUI (GUI project) and MagLoop_Common_Files (shared headers)
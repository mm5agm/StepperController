# StepperController

ESP32-based stepper motor controller with ESP-NOW wireless communication for MagLoop antenna positioning.

## 🎯 Overview

This project provides precise stepper motor control for antenna positioning systems, featuring wireless communication with a touchscreen GUI controller. The system uses ESP-NOW for low-latency, peer-to-peer communication between the controller and GUI.

## 🔧 Hardware Requirements

### Main Components
- **ESP32 DevKit V1** - Main microcontroller
- **DM542 Stepper Motor Driver** - Digital stepper motor controller
- **Stepper Motor** - NEMA 23 or compatible
- **Limit Switches** - For position sensing and safety
- **Power Supply** - 24V DC for stepper driver

### Pin Connections

| ESP32 Pin | Connection | Description |
|-----------|------------|-------------|
| GPIO 18   | DIR        | Direction signal to DM542 |
| GPIO 19   | STEP       | Step pulse to DM542 |
| GPIO 22   | DOWN_LIMIT | Down limit switch (INPUT_PULLUP) |
| GPIO 23   | UP_LIMIT   | Up limit switch (INPUT_PULLUP) |

### DM542 Stepper Driver Configuration
- **Microstepping**: Configurable via DIP switches
- **Current Setting**: Adjust via DIP switches based on motor specifications
- **Power**: 24V DC input recommended
- **Control**: Step/Direction interface from ESP32

## 📡 Communication Protocol

Uses ESP-NOW for wireless communication with the StepperGUI touchscreen controller.

### Message Format
```cpp
typedef struct __attribute__((packed)) {
    uint8_t messageId;
    CommandType command;
    int32_t param;
} Message;
```

### Supported Commands
- **Movement**: `CMD_UP_SLOW`, `CMD_UP_MEDIUM`, `CMD_UP_FAST`
- **Movement**: `CMD_DOWN_SLOW`, `CMD_DOWN_MEDIUM`, `CMD_DOWN_FAST`
- **Positioning**: `CMD_MOVE_TO` (absolute position)
- **Control**: `CMD_STOP`, `CMD_RESET`
- **Status**: `CMD_GET_POSITION`, `CMD_DOWN_LIMIT_STATUS`

## 🚀 Features

- **Non-blocking operation** - Responsive to commands during movement
- **Position feedback** - Real-time position updates to GUI (100ms intervals)
- **Safety limits** - Hardware limit switch integration
- **Speed control** - Multiple speed settings (slow/medium/fast)
- **Automatic reset cascade** - Controller restart triggers GUI restart
- **Message queuing** - ESP-NOW callbacks queue commands for main loop processing

## 🛠 Build and Upload

Quick start (PowerShell)

```powershell
# Build the project
platformio run

# Upload to the esp32doit-devkit-v1 environment (replace COM3 with your port)
platformio run -e esp32doit-devkit-v1 --target upload --upload-port COM3

# Open the serial monitor at 115200 baud
platformio device monitor --baud 115200
```

## Project layout

```
.
├─ src/
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

- ESP-NOW issues: verify MAC addresses and peer configuration
- Motor not moving: check wiring, DM542 enable/config and power
- Limit switches: confirm INPUT_PULLUP wiring and logic

## Contributing

1. Fork
2. Create branch
3. Make changes
4. Update `MagLoop_Common_Files` if shared definitions change
5. Test controller + GUI
6. Submit PR

---

See also: StepperGUI (GUI project) and MagLoop_Common_Files (shared headers)
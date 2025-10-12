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

### Prerequisites
- PlatformIO IDE or VS Code with PlatformIO extension
- ESP32 board support

### Build Commands
```bash
# Build project
platformio run

# Upload to ESP32
platformio run --target upload

# Monitor serial output
platformio device monitor
```

## 📁 Project Structure

```
StepperController/
├── src/
│   └── main.cpp              # Main controller logic
├── MagLoop_Common_Files/     # Shared headers (Git submodule)
│   ├── stepper_commands.h    # Command definitions
│   ├── stepper_helpers.h     # Helper functions
│   └── circular_buffer.h     # Message queue implementation
├── snippets/
│   └── StepperGUI_missing_functions.txt  # GUI helper code
├── update-common.bat         # Windows submodule update script
├── update-submodules.ps1     # PowerShell update script
├── update-common.sh          # Bash update script
└── platformio.ini           # PlatformIO configuration
```

## 🔗 Related Projects

- **[StepperGUI](https://github.com/mm5agm/StepperGUI)** - Touchscreen GUI controller
- **[MagLoop_Common_Files](https://github.com/mm5agm/MagLoop_Common_Files)** - Shared header files

## ⚙️ Configuration

### Motor Settings
Adjust these constants in `main.cpp`:
```cpp
constexpr long SLOW_PD = 40;    // Slow speed pulse delay (microseconds)
constexpr long MEDIUM_PD = 20;  // Medium speed pulse delay
constexpr long FAST_PD = 10;    // Fast speed pulse delay
```

### Position Limits
```cpp
constexpr int STEPPER_POSITION_MIN = 0;      // Minimum position
constexpr int STEPPER_POSITION_MAX = 16000;  // Maximum position
```

### ESP-NOW Configuration
Update the GUI MAC address in `main.cpp`:
```cpp
const uint8_t GUI_MAC[] = { 0x98, 0xA3, 0x16, 0xE3, 0xFD, 0x4C };
```

## 🔄 Updating Shared Files

This project uses Git submodules for shared header files. When changes are made to common files:

### Quick Update (Recommended)
```bash
# Windows
update-common.bat

# PowerShell
.\update-submodules.ps1

# Cross-platform
./update-common.sh
```

### Git Aliases
```bash
git sync-common    # Update, commit, and push
git update-subs    # Update only
```

## 📊 State Machine

The controller operates using a non-blocking state machine:

| State | Description |
|-------|-------------|
| `STATE_IDLE` | Waiting for commands |
| `STATE_MOVING_UP` | Moving upward continuously |
| `STATE_MOVING_DOWN` | Moving downward continuously |
| `STATE_MOVING_TO` | Moving to specific position |
| `STATE_MOVE_TO_DOWN_LIMIT` | Moving to down limit switch |
| `STATE_RESETTING` | System reset in progress |

## 🔍 Debugging

### Serial Monitor Output
- ESP-NOW message reception/transmission
- Command processing
- Position updates
- Limit switch status
- Error messages

### Common Issues
1. **ESP-NOW communication fails**: Check MAC addresses match
2. **Motor doesn't move**: Verify DM542 wiring and power
3. **Limit switches not working**: Check INPUT_PULLUP configuration
4. **Build errors**: Ensure submodules are updated (`git sync-common`)

## 📄 License

This project is part of the MagLoop stepper control system.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Update shared files in MagLoop_Common_Files if needed
5. Test with both controller and GUI
6. Submit a pull request

---

*For GUI interface, see [StepperGUI](https://github.com/mm5agm/StepperGUI)*
# ESP32 ASCOM Alpaca Flat Panel Calibrator

An ASCOM Alpaca compliant flat panel calibrator for astrophotography using the ESP32 microcontroller.

## Features

- **ASCOM Alpaca Compatible**: Full implementation of the CoverCalibrator device interface
- **Manual Positioning**: Designed for manually positioned flat panels (no cover functionality)
- **PWM Brightness Control**: 10-bit resolution (0-1023) brightness control
- **Web Interface**: Full-featured web UI accessible on port 80
- **Serial Control**: USB serial command interface for direct control
- **WiFi Configuration**: Built-in WiFi setup and AP mode for initial configuration
- **Alpaca Discovery**: Automatic device discovery via UDP broadcast
- **Configurable Settings**: Device name, brightness limits, and debug options

## Hardware Requirements

### Essential Components
- ESP32 development board (any variant with WiFi)
- LED flat panel with PWM brightness control
- Current limiting resistor or driver circuit (as appropriate for your LED panel)

### Wiring
```
ESP32 GPIO4 → LED Panel PWM Input (via appropriate driver/resistor)
ESP32 GND → LED Panel GND
ESP32 VIN/3.3V → LED Panel VCC (or use external power supply)
```

**Note**: Ensure your LED panel power requirements match your power supply. For high-power panels, use an external power supply and appropriate MOSFET driver circuit.

## Important Notes for Arduino IDE

### File Naming
- **Rename** `main.ino` to `ESP32_Flat_Panel_Calibrator.ino` (or your preferred project name)
- The `.ino` file must have the same name as the folder containing it
- Arduino IDE will automatically include all `.h` and `.cpp` files in the project folder

### Library Installation
Only **one external library** is required:
- **ArduinoJson**: Install via Library Manager (Tools → Manage Libraries → Search "ArduinoJson")

All other libraries are included with the ESP32 board package:
- WiFi, WebServer, ESPmDNS, Preferences, etc.

### Compilation Tips
- If you get compilation errors, check that all files are in the same folder
- Ensure the ESP32 board package is properly installed
- Try a lower upload speed (115200) if upload fails
- Make sure the correct COM port is selected

## Hardware Requirements

### Arduino IDE Setup

1. **Install ESP32 Board Package**:
   - Open Arduino IDE
   - File → Preferences → Additional Boards Manager URLs
   - Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board → Boards Manager → Search "ESP32" → Install "esp32 by Espressif Systems"

2. **Install Required Library**:
   - Tools → Manage Libraries
   - Search for "ArduinoJson" by Benoit Blanchon
   - Install the latest version (6.21.0 or newer)
   
   **Note**: All other libraries (WiFi, WebServer, ESPmDNS, Preferences) are built-in with the ESP32 package.

3. **Board Configuration**:
   - Tools → Board → ESP32 Arduino → "ESP32 Dev Module" (or your specific board)
   - Tools → Flash Frequency → 40MHz
   - Tools → Upload Speed → 921600 (try 115200 if upload fails)
   - Tools → Flash Mode → QIO
   - Tools → Partition Scheme → "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"
   - Tools → Port → Select your ESP32's COM port

### Project Setup

1. **Create Project Folder**:
   - Create a new folder named `ESP32_Flat_Panel_Calibrator`
   - Copy all project files (.h, .cpp, and .ino) into this folder
   - The main sketch file should be named `ESP32_Flat_Panel_Calibrator.ino`

2. **File Structure** (Arduino IDE will auto-detect all files):
   ```
   ESP32_Flat_Panel_Calibrator/
   ├── ESP32_Flat_Panel_Calibrator.ino  (main sketch file)
   ├── config.h
   ├── calibrator_controller.h
   ├── calibrator_controller.cpp
   ├── serial_handler.h
   ├── serial_handler.cpp
   ├── Debug.h
   ├── Debug.cpp
   ├── alpaca_handler.h
   ├── alpaca_handler.cpp
   ├── web_ui_handler.h
   ├── web_ui_handler.cpp
   └── html_templates.h
   ```

3. **Configure WiFi Credentials** (Optional):
   - Edit `config.h` in Arduino IDE
   - Update `DEFAULT_WIFI_SSID` and `DEFAULT_WIFI_PASSWORD`
   - Or configure via web interface after upload

4. **Upload the Firmware**:
   - Open `ESP32_Flat_Panel_Calibrator.ino` in Arduino IDE
   - Verify all files appear as tabs in the IDE
   - Connect ESP32 via USB
   - Click Upload button or press Ctrl+U

## Initial Setup

### First Boot - AP Mode
If no WiFi credentials are configured, the device will start in AP mode:

1. **Connect to WiFi Network**:
   - SSID: `FlatPanelCalibrator`
   - Password: `FlatPanel123`

2. **Configure WiFi**:
   - Open browser to `http://192.168.4.1`
   - Go to "WiFi Config"
   - Select your network and enter password
   - Click "Save & Connect"
   - Device will restart and connect to your network

### Normal Operation
Once connected to WiFi, the device provides:
- **Web Interface**: `http://[device-ip]` (port 80)
- **ASCOM Alpaca API**: `http://[device-ip]:11111`
- **Serial Interface**: USB connection at 115200 baud

## Usage

### Web Interface
Access the web interface by navigating to the device's IP address:

- **Home Page**: Device status and quick brightness controls
- **Calibrator Page**: Full brightness control with preset buttons
- **Setup Page**: Device configuration and settings
- **WiFi Config**: Network configuration

### Serial Commands
Connect via USB serial at 115200 baud:

#### Legacy Bracketed Commands (compatible with original test sketch):
```
<00>         - Turn calibrator OFF
<01>         - Turn calibrator ON (max brightness)
<02#50>      - Set brightness to 50%
```

#### Text Commands:
```
ON                  - Turn calibrator ON
OFF                 - Turn calibrator OFF
BRIGHTNESS 75       - Set brightness to 75%
MAXBRIGHTNESS 80    - Set maximum brightness to 80%
DEBUG ON/OFF        - Enable/disable debug output
STATUS              - Show current status
HELP                - Show available commands
```

### ASCOM Alpaca Integration

#### Discovery
The device supports ASCOM Alpaca discovery protocol:
- **Discovery Port**: 32227 (UDP)
- **API Port**: 11111 (HTTP)
- **Device Type**: CoverCalibrator
- **Device Number**: 0

#### ASCOM Client Setup
1. Use any ASCOM Alpaca compatible client
2. Run device discovery or manually add:
   - **Address**: `http://[device-ip]:11111`
   - **Device**: CoverCalibrator #0

#### Supported ASCOM Methods
- **Calibrator Methods**:
  - `Brightness` (get/set) - Current brightness (0-100%)
  - `MaxBrightness` (get) - Maximum brightness setting
  - `CalibratorState` (get) - Current state (Off=1, Ready=3)
  - `CalibratorOn()` - Turn on at max brightness
  - `CalibratorOff()` - Turn off

- **Cover Methods**: Not implemented (returns NotImplemented error)
  - `CoverState` returns NotPresent (0)
  - `OpenCover()`, `CloseCover()`, `HaltCover()` not supported

- **Common Properties**:
  - `Connected`, `Description`, `DriverInfo`, `DriverVersion`
  - `InterfaceVersion`, `Name`, `SupportedActions`

## Configuration Options

### Device Settings (via web interface or serial):
- **Device Name**: Custom name for the device
- **Maximum Brightness**: Limit maximum brightness (1-100%)
- **Debug Output**: Enable/disable verbose serial output

### Network Settings:
- **WiFi SSID/Password**: Network credentials
- **Static IP**: Configure via your router's DHCP settings

## API Reference

### REST Endpoints
The device implements the full ASCOM Alpaca CoverCalibrator interface:

```
GET  /api/v1/covercalibrator/0/brightness
PUT  /api/v1/covercalibrator/0/brightness
GET  /api/v1/covercalibrator/0/maxbrightness
GET  /api/v1/covercalibrator/0/calibratorstate
PUT  /api/v1/covercalibrator/0/calibratoron
PUT  /api/v1/covercalibrator/0/calibratoroff
```

### Management API:
```
GET  /management/apiversions
GET  /management/v1/description
GET  /management/v1/configureddevices
```

## Troubleshooting

### WiFi Connection Issues
1. **Check Credentials**: Verify SSID and password
2. **Signal Strength**: Ensure device is within WiFi range
3. **AP Mode**: Device will start AP mode if connection fails
4. **Reset Settings**: Use serial command or reflash firmware

### Serial Communication
1. **Baud Rate**: Ensure 115200 baud
2. **Driver Issues**: Install CH340/CP2102 drivers if needed
3. **Debug Output**: Enable with `DEBUG ON` command

### ASCOM Client Issues
1. **Discovery**: Ensure client and device on same network
2. **Firewall**: Check Windows Firewall settings
3. **Port Access**: Verify port 11111 is accessible
4. **Manual Setup**: Add device manually if discovery fails

### Brightness Control
1. **No Response**: Check wiring and PWM connections
2. **Flickering**: Verify power supply capacity
3. **Dim Output**: Check maximum brightness setting
4. **Driver Circuit**: Ensure proper current limiting

## Technical Specifications

- **Microcontroller**: ESP32 (any variant)
- **PWM Resolution**: 10-bit (0-1023 steps)
- **PWM Frequency**: 1 kHz
- **Brightness Range**: 0-100% (configurable maximum)
- **Network Protocols**: WiFi 802.11 b/g/n, HTTP, UDP
- **ASCOM Compliance**: Alpaca API v1, CoverCalibrator interface v1
- **Power Requirements**: 5V via USB or external supply
- **Operating Temperature**: -10°C to +60°C (ESP32 specifications)

## License

This project is released under the MIT License. See LICENSE file for details.

## Support

For issues, questions, or contributions:
1. Check the troubleshooting section above
2. Review the ASCOM Alpaca API documentation
3. Submit issues with detailed descriptions and serial output logs

## Contributing

Contributions welcome! Please follow these guidelines:
1. Test thoroughly on actual hardware
2. Maintain ASCOM Alpaca compliance
3. Update documentation for any new features
4. Follow existing code style and structure

## Version History

- **v1.0.0**: Initial release
  - Full ASCOM Alpaca CoverCalibrator implementation
  - Web interface with brightness control
  - Serial command interface
  - WiFi configuration and AP mode
  - 10-bit PWM brightness control

## Acknowledgments

Based on the ESP32 ASCOM Alpaca Roll-Off Roof Controller project and inspired by the ASCOM Standards community.
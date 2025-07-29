/*
 * ESP32 ASCOM Alpaca Flat Panel Calibrator
 * Configuration Header File - UPDATED WITH 10-BIT PWM RESOLUTION
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Debug level setting
#define DEBUG_LEVEL 1  // 0=Off, 1=Basic, 2=Verbose

// Version Information
#define DEVICE_VERSION "1.0.0"
#define DEVICE_MANUFACTURER "SmartC Observatory"
#define DEVICE_NAME "ESP32 Flat Panel Calibrator"

// GPIO pin definitions
const int PWM_OUTPUT_PIN = 4;           // PWM output for LED panel brightness

// PWM settings - CHANGED FROM 12-bit to 10-bit resolution as requested
const int PWM_FREQUENCY = 1000;         // 1 kHz frequency
const int PWM_RESOLUTION = 10;          // 10-bit resolution (0-1023) - CHANGED FROM 12
const int MAX_PWM_VALUE = 1023;         // 2^10 - 1 (CHANGED FROM 4095)
const int MIN_PWM_VALUE = 0;            // Minimum PWM value

// Calibrator brightness settings
const int MAX_BRIGHTNESS = 100;         // Maximum brightness percentage
const int MIN_BRIGHTNESS = 0;           // Minimum brightness percentage

// Default WiFi credentials (will be overridden by stored settings if available)
#define DEFAULT_WIFI_SSID "your_wifi_ssid"
#define DEFAULT_WIFI_PASSWORD "your_wifi_password"

// AP Mode SSID and password
#define AP_SSID "FlatPanelCalibrator"
#define AP_PASSWORD "FlatPanel123"
#define AP_TIMEOUT 300000               // 5 minutes in milliseconds

// ASCOM Alpaca Configuration
const int ALPACA_PORT = 11111;
const int WEB_UI_PORT = 80;
const int ALPACA_DISCOVERY_PORT = 32227;
inline const char* ALPACA_DISCOVERY_MESSAGE = "alpacadiscovery1";

// Buffer sizes
#define SSID_SIZE 32
#define PASSWORD_SIZE 64
#define DEVICE_NAME_SIZE 64

// Preferences namespace and keys
#define PREFERENCES_NAMESPACE "flatPanelConfig"
#define PREF_WIFI_SSID "ssid"
#define PREF_WIFI_PASSWORD "wifiPassword"
#define PREF_DEVICE_NAME "deviceName"
#define PREF_MAX_BRIGHTNESS "maxBrightness"
#define PREF_SERIAL_DEBUG "serialDebug"

// Serial command settings
#define SERIAL_BAUD_RATE 115200
#define COMMAND_BUFFER_SIZE 64
#define COMMAND_TIMEOUT 5000            // 5 seconds

// Enum for calibrator status - matches ASCOM CalibratorStatus values
enum CalibratorStatus {
  CALIBRATOR_NOT_PRESENT = 0,           // Device does not have a calibrator
  CALIBRATOR_OFF = 1,                   // Calibrator is present but turned off
  CALIBRATOR_NOT_READY = 2,             // Calibrator is present, but not available for use
  CALIBRATOR_READY = 3,                 // Calibrator is present and ready for use
  CALIBRATOR_UNKNOWN = 4,               // Calibrator status is unknown
  CALIBRATOR_ERROR = 5                  // Calibrator is in an error state
};

// Enum for cover status - matches ASCOM CoverStatus values
enum CoverStatus {
  COVER_NOT_PRESENT = 0,                // No cover is present
  COVER_CLOSED = 1,                     // Cover is closed
  COVER_MOVING = 2,                     // Cover is moving (opening or closing)
  COVER_OPEN = 3,                       // Cover is open
  COVER_UNKNOWN = 4,                    // Cover status is unknown
  COVER_ERROR = 5                       // Cover is in an error state
};

#endif // CONFIG_H
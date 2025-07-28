/*
 * ESP32 ASCOM Alpaca Flat Panel Calibrator
 * Calibrator Controller Implementation
 */

#include "calibrator_controller.h"
#include "Debug.h"
#include <Arduino.h>
#include <Preferences.h>

// Global variables
CalibratorStatus calibratorState = CALIBRATOR_OFF;
CoverStatus coverState = COVER_NOT_PRESENT;  // Manual flat panel has no cover
bool isConnected = true;                     // Device is always connected
int currentBrightness = 0;                   // Current brightness percentage (0-100)
int maxBrightness = MAX_BRIGHTNESS;          // Maximum brightness (configurable)
bool serialDebugEnabled = false;            // Serial debug disabled by default
String deviceName = "Flat Panel Calibrator";
unsigned long lastStateChange = 0;

void initializeCalibratorController() {
  Debug.println("Initializing Flat Panel Calibrator Controller...");
  
  // Load configuration from preferences
  Preferences prefs;
  prefs.begin(PREFERENCES_NAMESPACE, true);  // Read-only
  
  // Load device settings
  deviceName = prefs.getString(PREF_DEVICE_NAME, "Flat Panel Calibrator");
  maxBrightness = prefs.getInt(PREF_MAX_BRIGHTNESS, MAX_BRIGHTNESS);
  serialDebugEnabled = prefs.getBool(PREF_SERIAL_DEBUG, false);
  
  prefs.end();
  
  // Configure PWM output pin
  pinMode(PWM_OUTPUT_PIN, OUTPUT);
  
  // Configure PWM (ESP32 core 3.x API)
  if (!ledcAttach(PWM_OUTPUT_PIN, PWM_FREQUENCY, PWM_RESOLUTION)) {
    Debug.println("ERROR: Failed to configure PWM");
    calibratorState = CALIBRATOR_ERROR;
    return;
  }
  
  // Set initial PWM value to off
  ledcWrite(PWM_OUTPUT_PIN, 0);
  currentBrightness = 0;
  
  // Set initial state
  calibratorState = CALIBRATOR_OFF;
  coverState = COVER_NOT_PRESENT;
  lastStateChange = millis();
  
  Debug.println("Calibrator Controller initialized successfully");
  Debug.printf("Device Name: %s\n", deviceName.c_str());
  Debug.printf("Max Brightness: %d%%\n", maxBrightness);
  Debug.printf("PWM Pin: %d, Frequency: %dHz, Resolution: %d-bit\n", 
               PWM_OUTPUT_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
}

void updateCalibratorStatus() {
  // For a simple LED flat panel, the status is straightforward:
  // - CALIBRATOR_OFF when brightness is 0
  // - CALIBRATOR_READY when brightness is > 0
  // - CALIBRATOR_ERROR if PWM configuration failed
  
  if (calibratorState == CALIBRATOR_ERROR) {
    return; // Don't change error state
  }
  
  CalibratorStatus newState = (currentBrightness > 0) ? CALIBRATOR_READY : CALIBRATOR_OFF;
  
  if (newState != calibratorState) {
    calibratorState = newState;
    lastStateChange = millis();
    Debug.printf("Calibrator state changed to: %s\n", getCalibratorStateString().c_str());
  }
}

bool setCalibratorBrightness(int brightness) {
  // Validate brightness range
  if (brightness < MIN_BRIGHTNESS || brightness > maxBrightness) {
    Debug.printf("Invalid brightness value: %d (valid range: %d-%d)\n", 
                 brightness, MIN_BRIGHTNESS, maxBrightness);
    return false;
  }
  
  // Convert brightness percentage to PWM value
  int pwmValue = convertBrightnessToPWM(brightness);
  
  // Set PWM output
  ledcWrite(PWM_OUTPUT_PIN, pwmValue);
  currentBrightness = brightness;
  
  // Update calibrator state
  updateCalibratorStatus();
  
  Debug.printf("Brightness set to %d%% (PWM: %d)\n", brightness, pwmValue);
  return true;
}

bool turnCalibratorOn() {
  // Turn on at maximum brightness
  return setCalibratorBrightness(maxBrightness);
}

bool turnCalibratorOff() {
  // Turn off (set brightness to 0)
  return setCalibratorBrightness(0);
}

int getCurrentBrightness() {
  return currentBrightness;
}

int getMaxBrightness() {
  return maxBrightness;
}

void setMaxBrightness(int brightness) {
  if (brightness >= MIN_BRIGHTNESS && brightness <= MAX_BRIGHTNESS) {
    maxBrightness = brightness;
    
    // Save to preferences
    Preferences prefs;
    prefs.begin(PREFERENCES_NAMESPACE, false);
    prefs.putInt(PREF_MAX_BRIGHTNESS, maxBrightness);
    prefs.end();
    
    Debug.printf("Max brightness set to %d%%\n", maxBrightness);
    
    // If current brightness exceeds new max, reduce it
    if (currentBrightness > maxBrightness) {
      setCalibratorBrightness(maxBrightness);
    }
  }
}

CalibratorStatus getCalibratorState() {
  return calibratorState;
}

CoverStatus getCoverState() {
  return coverState;  // Always COVER_NOT_PRESENT for manual flat panel
}

String getCalibratorStateString() {
  return getCalibratorStateString(calibratorState);
}

String getCalibratorStateString(CalibratorStatus status) {
  switch (status) {
    case CALIBRATOR_NOT_PRESENT:
      return "NotPresent";
    case CALIBRATOR_OFF:
      return "Off";
    case CALIBRATOR_NOT_READY:
      return "NotReady";
    case CALIBRATOR_READY:
      return "Ready";
    case CALIBRATOR_UNKNOWN:
      return "Unknown";
    case CALIBRATOR_ERROR:
      return "Error";
    default:
      return "Unknown";
  }
}

String getCoverStateString() {
  return getCoverStateString(coverState);
}

String getCoverStateString(CoverStatus status) {
  switch (status) {
    case COVER_NOT_PRESENT:
      return "NotPresent";
    case COVER_CLOSED:
      return "Closed";
    case COVER_MOVING:
      return "Moving";
    case COVER_OPEN:
      return "Open";
    case COVER_UNKNOWN:
      return "Unknown";
    case COVER_ERROR:
      return "Error";
    default:
      return "Unknown";
  }
}

bool isCalibratorReady() {
  return calibratorState == CALIBRATOR_READY;
}

int convertBrightnessToPWM(int brightness) {
  // Convert brightness percentage (0-100) to PWM value (0-1023)
  // Using 10-bit resolution: 0-1023
  if (brightness <= 0) return 0;
  if (brightness >= 100) return MAX_PWM_VALUE;
  
  return (brightness * MAX_PWM_VALUE) / 100;
}

int convertPWMToBrightness(int pwmValue) {
  // Convert PWM value (0-1023) to brightness percentage (0-100)
  if (pwmValue <= 0) return 0;
  if (pwmValue >= MAX_PWM_VALUE) return 100;
  
  return (pwmValue * 100) / MAX_PWM_VALUE;
}
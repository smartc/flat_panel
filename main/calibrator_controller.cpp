/*
 * ESP32 ASCOM Alpaca Flat Panel Calibrator
 * Calibrator Controller Implementation - FIXED STATE LOGIC
 */

#include "calibrator_controller.h"
#include "Debug.h"
#include <Arduino.h>
#include <Preferences.h>

// Global variables
CalibratorStatus calibratorState = CALIBRATOR_OFF;
CoverStatus coverState = COVER_NOT_PRESENT;
bool isConnected = true;
int currentBrightness = 0;
int maxBrightness = MAX_BRIGHTNESS;
bool serialDebugEnabled = false;
String deviceName = "Flat Panel Calibrator";
unsigned long lastStateChange = 0;

void initializeCalibratorController() {
  Debug.println("Initializing Flat Panel Calibrator Controller...");
  
  Preferences prefs;
  prefs.begin(PREFERENCES_NAMESPACE, true);
  
  deviceName = prefs.getString(PREF_DEVICE_NAME, "Flat Panel Calibrator");
  maxBrightness = prefs.getInt(PREF_MAX_BRIGHTNESS, MAX_BRIGHTNESS);
  serialDebugEnabled = prefs.getBool(PREF_SERIAL_DEBUG, false);
  
  prefs.end();
  
  pinMode(PWM_OUTPUT_PIN, OUTPUT);
  
  if (!ledcAttach(PWM_OUTPUT_PIN, PWM_FREQUENCY, PWM_RESOLUTION)) {
    Debug.println("ERROR: Failed to configure PWM");
    calibratorState = CALIBRATOR_ERROR;
    return;
  }
  
  ledcWrite(PWM_OUTPUT_PIN, 0);
  currentBrightness = 0;
  
  // FIXED: Start in OFF state, will change to READY when first commanded
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
  if (calibratorState == CALIBRATOR_ERROR) {
    return;
  }
  
  // FIXED: Once a calibrator command has been issued, state should be READY
  // regardless of brightness level (including 0). Only OFF during initialization.
  // This matches ASCOM behavior where state indicates readiness, not current brightness.
  
  // Don't automatically change from READY back to OFF
  if (calibratorState == CALIBRATOR_OFF) {
    // Only change to READY when explicitly commanded
    return;
  }
}

bool setCalibratorBrightness(int brightness) {
  if (brightness < MIN_BRIGHTNESS || brightness > maxBrightness) {
    Debug.printf("Invalid brightness value: %d (valid range: %d-%d)\n", 
                 brightness, MIN_BRIGHTNESS, maxBrightness);
    return false;
  }
  
  int pwmValue = convertBrightnessToPWM(brightness);
  ledcWrite(PWM_OUTPUT_PIN, pwmValue);
  currentBrightness = brightness;
  
  // FIXED: Set state to READY when any brightness command is issued
  calibratorState = CALIBRATOR_READY;
  lastStateChange = millis();
  
  Debug.printf("Brightness set to %d%% (PWM: %d), State: READY\n", brightness, pwmValue);
  return true;
}

bool turnCalibratorOn() {
  return setCalibratorBrightness(maxBrightness);
}

bool turnCalibratorOff() {
  // FIXED: CalibratorOff sets brightness to 0 but keeps state as READY
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
    
    Preferences prefs;
    prefs.begin(PREFERENCES_NAMESPACE, false);
    prefs.putInt(PREF_MAX_BRIGHTNESS, maxBrightness);
    prefs.end();
    
    Debug.printf("Max brightness set to %d%%\n", maxBrightness);
    
    if (currentBrightness > maxBrightness) {
      setCalibratorBrightness(maxBrightness);
    }
  }
}

CalibratorStatus getCalibratorState() {
  return calibratorState;
}

CoverStatus getCoverState() {
  return coverState;
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
  if (brightness <= 0) return 0;
  if (brightness >= 100) return MAX_PWM_VALUE;
  
  return (brightness * MAX_PWM_VALUE) / 100;
}

int convertPWMToBrightness(int pwmValue) {
  if (pwmValue <= 0) return 0;
  if (pwmValue >= MAX_PWM_VALUE) return 100;
  
  return (pwmValue * 100) / MAX_PWM_VALUE;
}
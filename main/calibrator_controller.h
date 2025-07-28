/*
 * ESP32 ASCOM Alpaca Flat Panel Calibrator
 * Calibrator Controller Header
 */

#ifndef CALIBRATOR_CONTROLLER_H
#define CALIBRATOR_CONTROLLER_H

#include "config.h"

// Global state variables
extern CalibratorStatus calibratorState;
extern CoverStatus coverState;
extern bool isConnected;
extern int currentBrightness;
extern int maxBrightness;
extern bool serialDebugEnabled;
extern String deviceName;
extern unsigned long lastStateChange;

// Function prototypes
void initializeCalibratorController();
void updateCalibratorStatus();
bool setCalibratorBrightness(int brightness);
bool turnCalibratorOn();
bool turnCalibratorOff();
int getCurrentBrightness();
int getMaxBrightness();
void setMaxBrightness(int brightness);
CalibratorStatus getCalibratorState();
CoverStatus getCoverState();
String getCalibratorStateString();
String getCalibratorStateString(CalibratorStatus status);
String getCoverStateString();
String getCoverStateString(CoverStatus status);
bool isCalibratorReady();
int convertBrightnessToPWM(int brightness);
int convertPWMToBrightness(int pwmValue);

#endif // CALIBRATOR_CONTROLLER_H
/*
 * ESP32 ASCOM Alpaca Flat Panel Calibrator
 * Web UI Handler Header
 */

#ifndef WEB_UI_HANDLER_H
#define WEB_UI_HANDLER_H

#include <WebServer.h>
#include <Preferences.h>
#include "config.h"

// External references
extern WebServer webUiServer;
extern char ssid[SSID_SIZE];
extern char password[PASSWORD_SIZE];
extern bool apMode;
extern String deviceName;
extern bool serialDebugEnabled;

// Function prototypes
void loadConfiguration();
void saveConfiguration();
void initWebUI();
void handleWebUI();
void handleRoot();
void handleSetup();
void handleSetupPost();
void handleWifiConfig();
void handleWifiConfigPost();
void handleCalibrator();
void handleCalibratorPost();
void handleRestart();

#endif // WEB_UI_HANDLER_H
/*
 * ESP32 ASCOM Alpaca Flat Panel Calibrator
 * ASCOM Alpaca API Handler Header
 */

#ifndef ALPACA_HANDLER_H
#define ALPACA_HANDLER_H

#include <WebServer.h>
#include <WiFiUdp.h>
#include "config.h"

// External references
extern WebServer alpacaServer;
extern WiFiUDP udp;
extern String uniqueID;
extern unsigned int serverTransactionID;

// Function prototypes for setup and handling
void setupAlpacaAPI();
void setupAlpacaRoutes();
void handleAlpacaDiscovery();
void handleAlpacaAPI();
void sendAlpacaResponse(int clientID, int clientTransactionID, int errorNumber, const String& errorMessage, const String& value);

// Management API handlers
void handleAPIVersions();
void handleDescription();
void handleConfiguredDevices();

// Common device property handlers
void handleConnected();
void handleSetConnected();
void handleDeviceDescription();
void handleDriverInfo();
void handleDriverVersion();
void handleInterfaceVersion();
void handleName();
void handleSupportedActions();
void handleAction();

// CoverCalibrator property handlers
void handleBrightness();
void handleCalibratorState();
void handleCoverState();
void handleMaxBrightness();

// CoverCalibrator control handlers
void handleCalibratorOn();
void handleCalibratorOff();
void handleOpenCover();
void handleCloseCover();
void handleHaltCover();

// Setup handlers
void handleSetupRedirect();
void handleCoverCalibratorSetup();

#endif // ALPACA_HANDLER_H
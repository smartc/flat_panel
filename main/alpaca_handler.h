/*
 * ESP32 ASCOM Alpaca Flat Panel Calibrator
 * ASCOM Alpaca API Handler Header
 */

#ifndef ALPACA_HANDLER_H
#define ALPACA_HANDLER_H

#include <WebServer.h>
#include <WiFiUdp.h>
#include "config.h"

// External server instance
extern WebServer alpacaServer;
extern WiFiUDP udp;
extern String uniqueID;
extern unsigned int serverTransactionID;

// Function prototypes
void setupAlpacaAPI();
void handleAlpacaDiscovery();
void setupAlpacaRoutes();
void sendAlpacaResponse(int clientID, int clientTransactionID, int errorNumber, String errorMessage, String value = "");

// Management API handlers
void handleApiVersions();
void handleDescription();
void handleConfiguredDevices();

// Setup web interface handlers
void handleCoverCalibratorSetup();
void handleRedirectSetup();

// ASCOM Alpaca Common handlers
void handleConnected();
void handleSetConnected();
void handleDeviceDescription();
void handleDriverInfo();
void handleDriverVersion();
void handleInterfaceVersion();
void handleName();
void handleSupportedActions();
void handleAction();

// Handle methods that are not implemented
void handleNotFound();
void handleNotImplemented();

// CoverCalibrator specific handlers
void handleBrightness();
void handleSetBrightness();
void handleMaxBrightness();
void handleCalibratorState();
void handleCoverState();
void handleCalibratorOn();
void handleCalibratorOff();
void handleOpenCover();
void handleCloseCover();
void handleHaltCover();

#endif // ALPACA_HANDLER_H
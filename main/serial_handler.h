/*
 * ESP32 ASCOM Alpaca Flat Panel Calibrator
 * Serial Command Handler Header
 */

#ifndef SERIAL_HANDLER_H
#define SERIAL_HANDLER_H

#include "config.h"

// Serial command structure
struct SerialCommand {
  String command;
  String parameter;
  bool isValid;
};

// Function prototypes
void initSerialHandler();
void handleSerialCommands();
void processSerialCommand(const String& command);
SerialCommand parseSerialCommand(const String& input);
void sendSerialResponse(const String& response);
void printSerialHelp();
void printSerialStatus();
void enableDebug(bool enable);

// Command handlers
void handleBrightnessCommand(const String& parameter);
void handleOnCommand();
void handleOffCommand();
void handleMaxBrightnessCommand(const String& parameter);
void handleDebugCommand(const String& parameter);
void handleStatusCommand();
void handleHelpCommand();

#endif // SERIAL_HANDLER_H
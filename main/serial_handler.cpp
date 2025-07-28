/*
 * ESP32 ASCOM Alpaca Flat Panel Calibrator
 * Serial Command Handler Implementation
 */

#include "serial_handler.h"
#include "calibrator_controller.h"
#include "Debug.h"
#include <Preferences.h>
#include <WiFi.h>

// Buffer for serial input
String serialBuffer = "";
bool commandStarted = false;

void initSerialHandler() {
  Debug.println("Serial command handler initialized");
  Debug.println("Available commands:");
  Debug.println("  <00> = Turn calibrator off");
  Debug.println("  <01> = Turn calibrator on (max brightness)");
  Debug.println("  <02#xxx> = Set brightness (0-100)");
  Debug.println("  DEBUG ON/OFF = Enable/disable debug output");
  Debug.println("  STATUS = Show current status");
  Debug.println("  HELP = Show this help");
  Debug.println("");
}

void handleSerialCommands() {
  while (Serial.available()) {
    char inChar = Serial.read();
    
    if (inChar == '<') {
      // Start of bracketed command
      serialBuffer = "";
      commandStarted = true;
    } else if (inChar == '>') {
      // End of bracketed command
      if (commandStarted) {
        processSerialCommand("<" + serialBuffer + ">");
        commandStarted = false;
        serialBuffer = "";
      }
    } else if (inChar == '\n' || inChar == '\r') {
      // End of line command
      if (!commandStarted && serialBuffer.length() > 0) {
        processSerialCommand(serialBuffer);
        serialBuffer = "";
      }
    } else {
      // Add character to buffer
      if (commandStarted) {
        serialBuffer += inChar;
      } else {
        if (inChar != ' ' || serialBuffer.length() > 0) {  // Skip leading spaces
          serialBuffer += inChar;
        }
      }
      
      // Prevent buffer overflow
      if (serialBuffer.length() > COMMAND_BUFFER_SIZE) {
        serialBuffer = "";
        commandStarted = false;
        sendSerialResponse("Error: Command too long");
      }
    }
  }
}

void processSerialCommand(const String& command) {
  String cmd = command;
  cmd.trim();
  cmd.toUpperCase();
  
  Debug.printf(2, "Processing serial command: %s\n", cmd.c_str());
  
  // Handle bracketed commands (legacy format from original sketch)
  if (cmd.startsWith("<") && cmd.endsWith(">")) {
    String innerCmd = cmd.substring(1, cmd.length() - 1);
    
    if (innerCmd == "00") {
      handleOffCommand();
    } else if (innerCmd == "01") {
      handleOnCommand();
    } else if (innerCmd.startsWith("02#")) {
      String brightnessStr = innerCmd.substring(3);
      handleBrightnessCommand(brightnessStr);
    } else {
      sendSerialResponse("Error: Unknown bracketed command: " + cmd);
    }
    return;
  }
  
  // Handle text commands
  if (cmd.startsWith("DEBUG ")) {
    String param = cmd.substring(6);
    handleDebugCommand(param);
  } else if (cmd == "DEBUG ON") {
    handleDebugCommand("ON");
  } else if (cmd == "DEBUG OFF") {
    handleDebugCommand("OFF");
  } else if (cmd == "STATUS") {
    handleStatusCommand();
  } else if (cmd == "HELP") {
    handleHelpCommand();
  } else if (cmd.startsWith("BRIGHTNESS ")) {
    String param = cmd.substring(11);
    handleBrightnessCommand(param);
  } else if (cmd.startsWith("MAXBRIGHTNESS ")) {
    String param = cmd.substring(14);
    handleMaxBrightnessCommand(param);
  } else if (cmd == "ON") {
    handleOnCommand();
  } else if (cmd == "OFF") {
    handleOffCommand();
  } else {
    sendSerialResponse("Error: Unknown command: " + cmd);
    sendSerialResponse("Type HELP for available commands");
  }
}

void handleBrightnessCommand(const String& parameter) {
  int brightness = parameter.toInt();
  
  if (parameter.length() == 0) {
    sendSerialResponse("Error: Missing brightness value");
    return;
  }
  
  if (brightness < 0 || brightness > getMaxBrightness()) {
    sendSerialResponse("Error: Brightness out of range (0-" + String(getMaxBrightness()) + ")");
    return;
  }
  
  if (setCalibratorBrightness(brightness)) {
    sendSerialResponse("Brightness set to " + String(brightness) + "%");
  } else {
    sendSerialResponse("Error: Failed to set brightness");
  }
}

void handleOnCommand() {
  if (turnCalibratorOn()) {
    sendSerialResponse("Calibrator turned ON (brightness: " + String(getCurrentBrightness()) + "%)");
  } else {
    sendSerialResponse("Error: Failed to turn on calibrator");
  }
}

void handleOffCommand() {
  if (turnCalibratorOff()) {
    sendSerialResponse("Calibrator turned OFF");
  } else {
    sendSerialResponse("Error: Failed to turn off calibrator");
  }
}

void handleMaxBrightnessCommand(const String& parameter) {
  int maxBright = parameter.toInt();
  
  if (parameter.length() == 0) {
    sendSerialResponse("Current max brightness: " + String(getMaxBrightness()) + "%");
    return;
  }
  
  if (maxBright < 1 || maxBright > MAX_BRIGHTNESS) {
    sendSerialResponse("Error: Max brightness out of range (1-" + String(MAX_BRIGHTNESS) + ")");
    return;
  }
  
  setMaxBrightness(maxBright);
  sendSerialResponse("Max brightness set to " + String(maxBright) + "%");
}

void handleDebugCommand(const String& parameter) {
  if (parameter == "ON") {
    enableDebug(true);
    sendSerialResponse("Debug output ENABLED");
  } else if (parameter == "OFF") {
    enableDebug(false);
    sendSerialResponse("Debug output DISABLED");
  } else {
    sendSerialResponse("Usage: DEBUG ON/OFF");
  }
}

void handleStatusCommand() {
  printSerialStatus();
}

void handleHelpCommand() {
  printSerialHelp();
}

void sendSerialResponse(const String& response) {
  Serial.println(response);
}

void printSerialHelp() {
  Serial.println();
  Serial.println("ESP32 Flat Panel Calibrator - Serial Commands");
  Serial.println("=============================================");
  Serial.println("Bracketed Commands (legacy format):");
  Serial.println("  <00>         = Turn calibrator OFF");
  Serial.println("  <01>         = Turn calibrator ON (max brightness)");
  Serial.println("  <02#xxx>     = Set brightness (0-" + String(getMaxBrightness()) + ")");
  Serial.println();
  Serial.println("Text Commands:");
  Serial.println("  ON           = Turn calibrator ON");
  Serial.println("  OFF          = Turn calibrator OFF");
  Serial.println("  BRIGHTNESS x = Set brightness (0-" + String(getMaxBrightness()) + ")");
  Serial.println("  MAXBRIGHTNESS x = Set maximum brightness (1-" + String(MAX_BRIGHTNESS) + ")");
  Serial.println("  DEBUG ON/OFF = Enable/disable debug output");
  Serial.println("  STATUS       = Show current status");
  Serial.println("  HELP         = Show this help");
  Serial.println();
  Serial.println("Examples:");
  Serial.println("  <02#50>      = Set 50% brightness");
  Serial.println("  BRIGHTNESS 75 = Set 75% brightness");
  Serial.println("  DEBUG ON     = Enable debug messages");
  Serial.println();
}

void printSerialStatus() {
  Serial.println();
  Serial.println("Current Status:");
  Serial.println("==============");
  Serial.println("Device: " + deviceName);
  Serial.println("Firmware: " + String(DEVICE_VERSION));
  Serial.println("Calibrator State: " + getCalibratorStateString());
  Serial.println("Cover State: " + getCoverStateString());
  Serial.println("Current Brightness: " + String(getCurrentBrightness()) + "%");
  Serial.println("Max Brightness: " + String(getMaxBrightness()) + "%");
  Serial.println("Connected: " + String(isConnected ? "Yes" : "No"));
  Serial.println("Debug Enabled: " + String(serialDebugEnabled ? "Yes" : "No"));
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi: Connected to " + WiFi.SSID());
    Serial.println("IP Address: " + WiFi.localIP().toString());
    Serial.println("Web Interface: http://" + WiFi.localIP().toString());
    Serial.println("ASCOM Alpaca: http://" + WiFi.localIP().toString() + ":" + String(ALPACA_PORT));
  } else {
    Serial.println("WiFi: Not connected");
  }
  
  Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  Serial.println();
}

void enableDebug(bool enable) {
  serialDebugEnabled = enable;
  
  // Save to preferences
  Preferences prefs;
  prefs.begin(PREFERENCES_NAMESPACE, false);
  prefs.putBool(PREF_SERIAL_DEBUG, serialDebugEnabled);
  prefs.end();
  
  // Update debug level
  if (enable) {
    Debug.setLevel(2);  // Verbose
  } else {
    Debug.setLevel(0);  // Off
  }
}
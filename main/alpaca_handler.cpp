/*
 * ESP32 ASCOM Alpaca Flat Panel Calibrator
 * ASCOM Alpaca API Handler Implementation
 */

#include "alpaca_handler.h"
#include "calibrator_controller.h"
#include "Debug.h"
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <WiFi.h>

// Web server and UDP for discovery
WebServer alpacaServer(ALPACA_PORT);
WiFiUDP udp;
String uniqueID;
unsigned int serverTransactionID = 1;

// Set up the Alpaca API
void setupAlpacaAPI() {
  // Generate a unique ID based on ESP32's MAC address
  uint8_t mac[6];
  WiFi.macAddress(mac);
  uniqueID = "ESP32_FPC_";
  for (int i = 0; i < 6; i++) {
    char buf[3];
    sprintf(buf, "%02X", mac[i]);
    uniqueID += buf;
  }
  
  // Setup MDNS responder
  if (MDNS.begin("flatpanelcalibrator")) {
    Debug.println("MDNS responder started");
    MDNS.addService("http", "tcp", ALPACA_PORT);
  }
  
  // Initialize UDP for Alpaca discovery
  Debug.print("Starting UDP listener on port ");
  Debug.print(ALPACA_DISCOVERY_PORT);
  Debug.print("... ");
  
  if (udp.begin(ALPACA_DISCOVERY_PORT)) {
    Debug.println("SUCCESS!");
  } else {
    Debug.println("FAILED!");
  }
  
  // Print network info
  Debug.print("ESP32 IP address: ");
  Debug.println(WiFi.localIP());
  Debug.print("Discovery port: ");
  Debug.println(ALPACA_DISCOVERY_PORT);
  Debug.print("Alpaca API port: ");
  Debug.println(ALPACA_PORT);
  
  // Setup server routes for ASCOM Alpaca API
  setupAlpacaRoutes();
  
  // Start the web server
  alpacaServer.begin();
  Debug.printf("Alpaca server started on port %d\n", ALPACA_PORT);
}

// Handle Alpaca discovery requests
void handleAlpacaDiscovery() {
  int packetSize = udp.parsePacket();
  
  if (packetSize) {
    char packet[64];
    memset(packet, 0, sizeof(packet));
    int len = udp.read(packet, sizeof(packet) - 1);
    
    if (len > 0) {
      packet[len] = 0;
      
      Debug.print("Received UDP packet (");
      Debug.print(len);
      Debug.print(" bytes) from ");
      Debug.print(udp.remoteIP());
      Debug.print(":");
      Debug.print(udp.remotePort());
      Debug.print(" - Content: '");
      Debug.print(packet);
      Debug.println("'");
      
      // Check if this is an Alpaca discovery message
      if (strncmp(packet, ALPACA_DISCOVERY_MESSAGE, strlen(ALPACA_DISCOVERY_MESSAGE)) == 0) {
        // Create the response JSON
        DynamicJsonDocument doc(128);
        doc["AlpacaPort"] = ALPACA_PORT;
        
        String response;
        serializeJson(doc, response);
        
        // Send the response back to the client
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.print(response);
        udp.endPacket();
        
        Debug.print("Sent discovery response: ");
        Debug.println(response);
      } else {
        Debug.println("Received non-Alpaca UDP packet");
      }
    }
  }
}

// Set up all Alpaca routes
void setupAlpacaRoutes() {
  // Add redirect from /setup on Alpaca port to Web UI port
  alpacaServer.on("/setup", HTTP_GET, handleRedirectSetup);

  // Management API routes
  alpacaServer.on("/management/apiversions", HTTP_GET, handleApiVersions);
  alpacaServer.on("/management/v1/description", HTTP_GET, handleDescription);
  alpacaServer.on("/management/v1/configureddevices", HTTP_GET, handleConfiguredDevices);
  
  // Alpaca setup routes
  alpacaServer.on("/setup/v1/covercalibrator/0/setup", HTTP_GET, handleCoverCalibratorSetup);
  
  // CoverCalibrator API routes
  alpacaServer.on("/api/v1/covercalibrator/0/connected", HTTP_GET, handleConnected);
  alpacaServer.on("/api/v1/covercalibrator/0/connected", HTTP_PUT, handleSetConnected);
  alpacaServer.on("/api/v1/covercalibrator/0/description", HTTP_GET, handleDeviceDescription);
  alpacaServer.on("/api/v1/covercalibrator/0/driverinfo", HTTP_GET, handleDriverInfo);
  alpacaServer.on("/api/v1/covercalibrator/0/driverversion", HTTP_GET, handleDriverVersion);
  alpacaServer.on("/api/v1/covercalibrator/0/interfaceversion", HTTP_GET, handleInterfaceVersion);
  alpacaServer.on("/api/v1/covercalibrator/0/name", HTTP_GET, handleName);
  alpacaServer.on("/api/v1/covercalibrator/0/supportedactions", HTTP_GET, handleSupportedActions);
  alpacaServer.on("/api/v1/covercalibrator/0/action", HTTP_PUT, handleAction);
  
  // CoverCalibrator specific methods
  alpacaServer.on("/api/v1/covercalibrator/0/brightness", HTTP_GET, handleBrightness);
  alpacaServer.on("/api/v1/covercalibrator/0/brightness", HTTP_PUT, handleSetBrightness);
  alpacaServer.on("/api/v1/covercalibrator/0/maxbrightness", HTTP_GET, handleMaxBrightness);
  alpacaServer.on("/api/v1/covercalibrator/0/calibratorstate", HTTP_GET, handleCalibratorState);
  alpacaServer.on("/api/v1/covercalibrator/0/coverstate", HTTP_GET, handleCoverState);
  alpacaServer.on("/api/v1/covercalibrator/0/calibratoron", HTTP_PUT, handleCalibratorOn);
  alpacaServer.on("/api/v1/covercalibrator/0/calibratoroff", HTTP_PUT, handleCalibratorOff);
  alpacaServer.on("/api/v1/covercalibrator/0/opencover", HTTP_PUT, handleOpenCover);
  alpacaServer.on("/api/v1/covercalibrator/0/closecover", HTTP_PUT, handleCloseCover);
  alpacaServer.on("/api/v1/covercalibrator/0/haltcover", HTTP_PUT, handleHaltCover);
  
  // Handle not found
  alpacaServer.onNotFound(handleNotFound);
}

// Helper function to send a standard JSON response
void sendAlpacaResponse(int clientID, int clientTransactionID, int errorNumber, String errorMessage, String value) {
  DynamicJsonDocument doc(1024);
  
  // Common response fields
  doc["ClientTransactionID"] = clientTransactionID;
  doc["ServerTransactionID"] = serverTransactionID++;
  doc["ErrorNumber"] = errorNumber;
  doc["ErrorMessage"] = errorMessage;
  
  // Include Value field if provided
  if (value.length() > 0) {
    // Check if value is a JSON object or array
    if ((value.startsWith("{") && value.endsWith("}")) || 
        (value.startsWith("[") && value.endsWith("]"))) {
      // Parse the JSON string into a JsonDocument
      DynamicJsonDocument valueDoc(512);
      DeserializationError error = deserializeJson(valueDoc, value);
      
      if (!error) {
        doc["Value"] = valueDoc;
      } else {
        doc["Value"] = value;
      }
    } else if (value == "true") {
      doc["Value"] = true;
    } else if (value == "false") {
      doc["Value"] = false;
    } else if (value.toInt() != 0 || value == "0") {
      doc["Value"] = value.toInt();
    } else {
      doc["Value"] = value;
    }
  }
  
  String response;
  serializeJson(doc, response);
  
  alpacaServer.send(200, "application/json", response);
}

void handleNotFound() {
  String message = "Path Not Found\n\n";
  message += "URI: ";
  message += alpacaServer.uri();
  message += "\n";
  message += "Method: ";
  message += (alpacaServer.method() == HTTP_GET ? "GET" : "PUT");
  message += "\n";
  
  alpacaServer.send(400, "text/plain", message);
}

// Management API handlers
void handleApiVersions() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  DynamicJsonDocument doc(128);
  doc["ServerTransactionID"] = serverTransactionID++;
  
  JsonArray valueArray = doc.createNestedArray("Value");
  valueArray.add(1); // We only support version 1
  
  if (clientTransactionID > 0) {
    doc["ClientTransactionID"] = clientTransactionID;
  }
  
  doc["ErrorNumber"] = 0;
  doc["ErrorMessage"] = "";
  
  String response;
  serializeJson(doc, response);
  
  alpacaServer.send(200, "application/json", response);
}

void handleDescription() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  DynamicJsonDocument doc(1024);
  doc["ServerName"] = DEVICE_NAME;
  doc["Manufacturer"] = DEVICE_MANUFACTURER;
  doc["ManufacturerVersion"] = DEVICE_VERSION;
  doc["Location"] = "Observatory";
  
  String value;
  serializeJson(doc, value);
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", value);
}

void handleConfiguredDevices() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  DynamicJsonDocument arrayDoc(1024);
  JsonArray array = arrayDoc.to<JsonArray>();
  
  JsonObject device = array.createNestedObject();
  device["DeviceName"] = deviceName;
  device["DeviceType"] = "CoverCalibrator";
  device["DeviceNumber"] = 0;
  device["UniqueID"] = uniqueID;
  
  String value;
  serializeJson(array, value);
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", value);
}

// Setup handlers
void handleCoverCalibratorSetup() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><title>Flat Panel Calibrator Setup</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; }";
  html += "h1 { color: #2c3e50; }";
  html += "button { background-color: #3498db; color: white; border: none; padding: 10px 15px; margin: 5px; border-radius: 4px; cursor: pointer; }";
  html += "button:hover { background-color: #2980b9; }";
  html += "input[type=range] { width: 300px; }";
  html += "</style></head>";
  html += "<body>";
  html += "<h1>Flat Panel Calibrator Setup</h1>";
  html += "<p>Calibrator State: " + getCalibratorStateString() + "</p>";
  html += "<p>Current Brightness: " + String(getCurrentBrightness()) + "%</p>";
  html += "<p>Max Brightness: " + String(getMaxBrightness()) + "%</p>";
  html += "<p>";
  html += "<button onclick='calibratorOn()'>Turn ON</button> ";
  html += "<button onclick='calibratorOff()'>Turn OFF</button>";
  html += "</p>";
  html += "<p>";
  html += "<label for='brightness'>Brightness: </label>";
  html += "<input type='range' id='brightness' min='0' max='" + String(getMaxBrightness()) + "' value='" + String(getCurrentBrightness()) + "' onchange='setBrightness(this.value)'>";
  html += "<span id='brightnessValue'>" + String(getCurrentBrightness()) + "%</span>";
  html += "</p>";
  html += "<script>";
  html += "function calibratorOn() {";
  html += "  fetch('/api/v1/covercalibrator/0/calibratoron', {method: 'PUT', body: 'ClientID=0&ClientTransactionID=0'}).then(() => { setTimeout(() => location.reload(), 1000); });";
  html += "}";
  html += "function calibratorOff() {";
  html += "  fetch('/api/v1/covercalibrator/0/calibratoroff', {method: 'PUT', body: 'ClientID=0&ClientTransactionID=0'}).then(() => { setTimeout(() => location.reload(), 1000); });";
  html += "}";
  html += "function setBrightness(value) {";
  html += "  document.getElementById('brightnessValue').innerText = value + '%';";
  html += "  fetch('/api/v1/covercalibrator/0/brightness', {method: 'PUT', body: 'ClientID=0&ClientTransactionID=0&Brightness=' + value});";
  html += "}";
  html += "</script>";
  html += "</body></html>";
  
  alpacaServer.send(200, "text/html", html);
}

void handleRedirectSetup() {
  String redirectUrl = "http://" + WiFi.localIP().toString() + ":" + String(WEB_UI_PORT) + "/setup";
  
  alpacaServer.sendHeader("Location", redirectUrl, true);
  alpacaServer.send(302, "text/plain", "Redirecting to setup page...");
}

// ASCOM Alpaca Common handlers
void handleConnected() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", isConnected ? "true" : "false");
}

void handleSetConnected() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  if (alpacaServer.hasArg("Connected")) {
    isConnected = alpacaServer.arg("Connected").equalsIgnoreCase("true");
    sendAlpacaResponse(clientID, clientTransactionID, 0, "", "");
  } else {
    sendAlpacaResponse(clientID, clientTransactionID, 1025, "Invalid value", "");
  }
}

void handleDeviceDescription() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", "ESP32 based ASCOM Alpaca Flat Panel Calibrator");
}

void handleDriverInfo() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", "ESP32 ASCOM Alpaca Flat Panel Calibrator");
}

void handleDriverVersion() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", DEVICE_VERSION);
}

void handleInterfaceVersion() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;

  sendAlpacaResponse(clientID, clientTransactionID, 0, "", "1");
}

void handleName() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", deviceName);
}

void handleSupportedActions() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  DynamicJsonDocument doc(256);
  JsonArray array = doc.to<JsonArray>();
  array.add("status");
  
  String value;
  serializeJson(doc, value);
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", value);
}

void handleAction() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  String actionName = alpacaServer.hasArg("Action") ? alpacaServer.arg("Action") : "";
  
  if (actionName == "status") {
    String status = "State: " + getCalibratorStateString() + ", Brightness: " + String(getCurrentBrightness()) + "%";
    sendAlpacaResponse(clientID, clientTransactionID, 0, "", status);
  } else {
    sendAlpacaResponse(clientID, clientTransactionID, 1036, "Action not implemented", "");
  }
}

// CoverCalibrator specific handlers
void handleBrightness() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  if (!isConnected) {
    sendAlpacaResponse(clientID, clientTransactionID, 1031, "Not connected", "");
    return;
  }
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", String(getCurrentBrightness()));
}

void handleSetBrightness() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  if (!isConnected) {
    sendAlpacaResponse(clientID, clientTransactionID, 1031, "Not connected", "");
    return;
  }
  
  if (alpacaServer.hasArg("Brightness")) {
    int brightness = alpacaServer.arg("Brightness").toInt();
    
    if (setCalibratorBrightness(brightness)) {
      sendAlpacaResponse(clientID, clientTransactionID, 0, "", "");
    } else {
      sendAlpacaResponse(clientID, clientTransactionID, 1025, "Invalid brightness value", "");
    }
  } else {
    sendAlpacaResponse(clientID, clientTransactionID, 1025, "Missing brightness parameter", "");
  }
}

void handleMaxBrightness() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", String(getMaxBrightness()));
}

void handleCalibratorState() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  if (!isConnected) {
    sendAlpacaResponse(clientID, clientTransactionID, 1031, "Not connected", "");
    return;
  }
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", String(static_cast<int>(getCalibratorState())));
}

void handleCoverState() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  // Cover is always NotPresent for manual flat panel
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", String(static_cast<int>(getCoverState())));
}

void handleCalibratorOn() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  if (!isConnected) {
    sendAlpacaResponse(clientID, clientTransactionID, 1031, "Not connected", "");
    return;
  }
  
  if (turnCalibratorOn()) {
    sendAlpacaResponse(clientID, clientTransactionID, 0, "", "");
  } else {
    sendAlpacaResponse(clientID, clientTransactionID, 1035, "Failed to turn on calibrator", "");
  }
}

void handleCalibratorOff() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  if (!isConnected) {
    sendAlpacaResponse(clientID, clientTransactionID, 1031, "Not connected", "");
    return;
  }
  
  if (turnCalibratorOff()) {
    sendAlpacaResponse(clientID, clientTransactionID, 0, "", "");
  } else {
    sendAlpacaResponse(clientID, clientTransactionID, 1035, "Failed to turn off calibrator", "");
  }
}

void handleOpenCover() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  // Cover functionality not implemented - manual flat panel
  sendAlpacaResponse(clientID, clientTransactionID, 1024, "Method not implemented", "");
}

void handleCloseCover() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  // Cover functionality not implemented - manual flat panel
  sendAlpacaResponse(clientID, clientTransactionID, 1024, "Method not implemented", "");
}

void handleHaltCover() {
  int clientID = alpacaServer.hasArg("ClientID") ? alpacaServer.arg("ClientID").toInt() : 0;
  int clientTransactionID = alpacaServer.hasArg("ClientTransactionID") ? alpacaServer.arg("ClientTransactionID").toInt() : 0;
  
  // Cover functionality not implemented - manual flat panel
  sendAlpacaResponse(clientID, clientTransactionID, 1024, "Method not implemented", "");
}
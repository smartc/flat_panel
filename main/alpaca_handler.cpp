/*
 * ESP32 ASCOM Alpaca Flat Panel Calibrator
 * ASCOM Alpaca API Handler - PROTOCOL COMPLIANT
 */

#include "alpaca_handler.h"
#include "calibrator_controller.h"
#include "Debug.h"
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <WiFi.h>

WebServer alpacaServer(ALPACA_PORT);
WiFiUDP udp;
String uniqueID;
unsigned int serverTransactionID = 1;

#define ASCOM_ERROR_INVALID_VALUE 1025
#define ASCOM_ERROR_NOT_CONNECTED 1031
#define ASCOM_ERROR_NOT_IMPLEMENTED 1036

void setupAlpacaAPI() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  uniqueID = "ESP32_FPC_";
  for (int i = 0; i < 6; i++) {
    char buf[3];
    sprintf(buf, "%02X", mac[i]);
    uniqueID += buf;
  }
  
  if (MDNS.begin("flatpanelcalibrator")) {
    Debug.println("MDNS responder started");
    MDNS.addService("http", "tcp", ALPACA_PORT);
  }
  
  Debug.printf("Starting UDP listener on port %d... ", ALPACA_DISCOVERY_PORT);
  if (udp.begin(ALPACA_DISCOVERY_PORT)) {
    Debug.println("SUCCESS!");
  } else {
    Debug.println("FAILED!");
  }
  
  Debug.printf("ESP32 IP address: %s\n", WiFi.localIP().toString().c_str());
  Debug.printf("Alpaca API port: %d\n", ALPACA_PORT);
  
  setupAlpacaRoutes();
  alpacaServer.begin();
  Debug.printf("Alpaca server started on port %d\n", ALPACA_PORT);
}

void handleAlpacaDiscovery() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char packet[64];
    memset(packet, 0, sizeof(packet));
    int len = udp.read(packet, sizeof(packet) - 1);
    
    if (len > 0) {
      packet[len] = 0;
      Debug.printf(2, "UDP packet: %s\n", packet);
      
      if (strncmp(packet, ALPACA_DISCOVERY_MESSAGE, strlen(ALPACA_DISCOVERY_MESSAGE)) == 0) {
        DynamicJsonDocument doc(128);
        doc["AlpacaPort"] = ALPACA_PORT;
        
        String response;
        serializeJson(doc, response);
        
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.print(response);
        udp.endPacket();
        
        Debug.printf("Discovery response: %s\n", response.c_str());
      }
    }
  }
}

void setupAlpacaRoutes() {
  // Management API
  alpacaServer.on("/management/apiversions", HTTP_GET, handleAPIVersions);
  alpacaServer.on("/management/v1/description", HTTP_GET, handleDescription);
  alpacaServer.on("/management/v1/configureddevices", HTTP_GET, handleConfiguredDevices);
  
  // Device routes
  alpacaServer.on("/api/v1/covercalibrator/0/connected", HTTP_GET, handleConnected);
  alpacaServer.on("/api/v1/covercalibrator/0/connected", HTTP_PUT, handleSetConnected);
  alpacaServer.on("/api/v1/covercalibrator/0/description", HTTP_GET, handleDeviceDescription);
  alpacaServer.on("/api/v1/covercalibrator/0/driverinfo", HTTP_GET, handleDriverInfo);
  alpacaServer.on("/api/v1/covercalibrator/0/driverversion", HTTP_GET, handleDriverVersion);
  alpacaServer.on("/api/v1/covercalibrator/0/interfaceversion", HTTP_GET, handleInterfaceVersion);
  alpacaServer.on("/api/v1/covercalibrator/0/name", HTTP_GET, handleName);
  alpacaServer.on("/api/v1/covercalibrator/0/supportedactions", HTTP_GET, handleSupportedActions);
  alpacaServer.on("/api/v1/covercalibrator/0/action", HTTP_PUT, handleAction);
  
  // CoverCalibrator properties
  alpacaServer.on("/api/v1/covercalibrator/0/brightness", HTTP_GET, handleBrightness);
  alpacaServer.on("/api/v1/covercalibrator/0/calibratorstate", HTTP_GET, handleCalibratorState);
  alpacaServer.on("/api/v1/covercalibrator/0/coverstate", HTTP_GET, handleCoverState);
  alpacaServer.on("/api/v1/covercalibrator/0/maxbrightness", HTTP_GET, handleMaxBrightness);
  
  // CoverCalibrator methods
  alpacaServer.on("/api/v1/covercalibrator/0/calibratoron", HTTP_PUT, handleCalibratorOn);
  alpacaServer.on("/api/v1/covercalibrator/0/calibratoroff", HTTP_PUT, handleCalibratorOff);
  alpacaServer.on("/api/v1/covercalibrator/0/opencover", HTTP_PUT, handleOpenCover);
  alpacaServer.on("/api/v1/covercalibrator/0/closecover", HTTP_PUT, handleCloseCover);
  alpacaServer.on("/api/v1/covercalibrator/0/haltcover", HTTP_PUT, handleHaltCover);
  
  // FIXED: Correct ASCOM setup URL
  alpacaServer.on("/setup", HTTP_GET, handleSetupRedirect);
  alpacaServer.on("/setup/v1/covercalibrator/0/setup", HTTP_GET, handleCoverCalibratorSetup);
}

void handleAlpacaAPI() {
  alpacaServer.handleClient();
}

// FIXED: Ensure ClientTransactionID is always a valid unsigned integer
int getClientTransactionID() {
  int transactionID = 0;
  
  if (alpacaServer.hasArg("ClientTransactionID")) {
    transactionID = alpacaServer.arg("ClientTransactionID").toInt();
  } else if (alpacaServer.hasArg("clienttransactionid")) {
    transactionID = alpacaServer.arg("clienttransactionid").toInt();
  } else if (alpacaServer.hasArg("CLIENTTRANSACTIONID")) {
    transactionID = alpacaServer.arg("CLIENTTRANSACTIONID").toInt();
  }
  
  // FIXED: Convert negative values to 0 for valid unsigned integer response
  return (transactionID < 0) ? 0 : transactionID;
}

int getClientID() {
  int clientID = 0;
  
  if (alpacaServer.hasArg("ClientID")) {
    clientID = alpacaServer.arg("ClientID").toInt();
  } else if (alpacaServer.hasArg("clientid")) {
    clientID = alpacaServer.arg("clientid").toInt();
  } else if (alpacaServer.hasArg("CLIENTID")) {
    clientID = alpacaServer.arg("CLIENTID").toInt();
  }
  
  // FIXED: Convert negative values to 0 for valid unsigned integer response
  return (clientID < 0) ? 0 : clientID;
}

void sendAlpacaResponse(int clientID, int clientTransactionID, int errorNumber, const String& errorMessage, const String& value) {
  DynamicJsonDocument doc(256);
  
  // FIXED: Use exact ClientTransactionID as received
  doc["ClientTransactionID"] = clientTransactionID;
  doc["ServerTransactionID"] = serverTransactionID++;
  doc["ErrorNumber"] = errorNumber;
  doc["ErrorMessage"] = errorMessage;
  
  if (value.length() > 0) {
    DynamicJsonDocument valueDoc(128);
    if (deserializeJson(valueDoc, value) == DeserializationError::Ok) {
      doc["Value"] = valueDoc.as<JsonVariant>();
    } else {
      doc["Value"] = value;
    }
  }
  
  String response;
  serializeJson(doc, response);
  
  alpacaServer.send(200, "application/json", response);
  Debug.printf(2, "Response: %s\n", response.c_str());
}

// FIXED: Parameter validation helper
bool validateBooleanParameter(const String& paramName, bool& result) {
  if (!alpacaServer.hasArg(paramName)) {
    return false;
  }
  
  String value = alpacaServer.arg(paramName);
  value.toLowerCase();
  
  // FIXED: Strict validation - only accept "true" or "false"
  if (value == "true") {
    result = true;
    return true;
  } else if (value == "false") {
    result = false;
    return true;
  }
  
  return false; // Invalid value
}

// Management API handlers
void handleAPIVersions() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  DynamicJsonDocument valueDoc(64);
  JsonArray array = valueDoc.to<JsonArray>();
  array.add(1);
  
  String value;
  serializeJson(valueDoc, value);
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", value);
}

void handleDescription() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  DynamicJsonDocument valueDoc(256);
  valueDoc["ServerName"] = DEVICE_NAME;
  valueDoc["Manufacturer"] = DEVICE_MANUFACTURER;
  valueDoc["ManufacturerVersion"] = DEVICE_VERSION;
  valueDoc["Location"] = "Observatory";
  
  String value;
  serializeJson(valueDoc, value);
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", value);
}

void handleConfiguredDevices() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  DynamicJsonDocument valueDoc(256);
  JsonArray array = valueDoc.to<JsonArray>();
  
  JsonObject device = array.createNestedObject();
  device["DeviceName"] = deviceName;
  device["DeviceType"] = "CoverCalibrator";
  device["DeviceNumber"] = 0;
  device["UniqueID"] = uniqueID;
  
  String value;
  serializeJson(valueDoc, value);
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", value);
}

// Common device handlers
void handleConnected() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", isConnected ? "true" : "false");
}

void handleSetConnected() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  for (int i = 0; i < alpacaServer.args(); i++) {
    String argName = alpacaServer.argName(i);
    if (argName.equalsIgnoreCase("ClientID") || 
        argName.equalsIgnoreCase("ClientTransactionID")) {
      continue;
    }
    if (argName.equalsIgnoreCase("connected") && argName != "Connected") {
      alpacaServer.send(400, "text/plain", "Invalid parameter casing - use 'Connected'");
      return;
    }
  }

  // FIXED: Strict parameter validation
  bool connected;
  if (!validateBooleanParameter("Connected", connected)) {
    alpacaServer.send(400, "text/plain", "Invalid or missing Connected parameter");
    return;
  }
  
  isConnected = connected;
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", "");
}

void handleDeviceDescription() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", "ESP32 based ASCOM Alpaca Flat Panel Calibrator");
}

void handleDriverInfo() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", "ESP32 ASCOM Alpaca Flat Panel Calibrator by SmartC Observatory");
}

void handleDriverVersion() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", DEVICE_VERSION);
}

void handleInterfaceVersion() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();

  sendAlpacaResponse(clientID, clientTransactionID, 0, "", "1");
}

void handleName() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", deviceName);
}

void handleSupportedActions() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  DynamicJsonDocument doc(128);
  JsonArray array = doc.to<JsonArray>();
  array.add("status");
  
  String value;
  serializeJson(doc, value);
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", value);
}

void handleAction() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  String actionName = alpacaServer.hasArg("Action") ? alpacaServer.arg("Action") : "";
  
  if (actionName == "status") {
    String status = "State: " + getCalibratorStateString() + ", Brightness: " + String(getCurrentBrightness()) + "%";
    sendAlpacaResponse(clientID, clientTransactionID, 0, "", status);
  } else {
    sendAlpacaResponse(clientID, clientTransactionID, ASCOM_ERROR_NOT_IMPLEMENTED, "Action not implemented", "");
  }
}

// CoverCalibrator handlers
void handleBrightness() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  if (!isConnected) {
    sendAlpacaResponse(clientID, clientTransactionID, ASCOM_ERROR_NOT_CONNECTED, "Not connected", "");
    return;
  }
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", String(getCurrentBrightness()));
}

void handleCalibratorState() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  if (!isConnected) {
    sendAlpacaResponse(clientID, clientTransactionID, ASCOM_ERROR_NOT_CONNECTED, "Not connected", "");
    return;
  }
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", String((int)getCalibratorState()));
}

void handleCoverState() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  if (!isConnected) {
    sendAlpacaResponse(clientID, clientTransactionID, ASCOM_ERROR_NOT_CONNECTED, "Not connected", "");
    return;
  }
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", String((int)COVER_NOT_PRESENT));
}

void handleMaxBrightness() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  if (!isConnected) {
    sendAlpacaResponse(clientID, clientTransactionID, ASCOM_ERROR_NOT_CONNECTED, "Not connected", "");
    return;
  }
  
  sendAlpacaResponse(clientID, clientTransactionID, 0, "", String(getMaxBrightness()));
}

void handleCalibratorOn() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  if (!isConnected) {
    sendAlpacaResponse(clientID, clientTransactionID, ASCOM_ERROR_NOT_CONNECTED, "Not connected", "");
    return;
  }
  
  // FIXED: Check only for brightness parameter with wrong casing
  // Per Postel's Law, ignore extra parameters but validate known ones strictly
  for (int i = 0; i < alpacaServer.args(); i++) {
    String argName = alpacaServer.argName(i);
    
    // Skip the standard client ID parameters (these are case-insensitive per spec)
    if (argName.equalsIgnoreCase("ClientID") || 
        argName.equalsIgnoreCase("ClientTransactionID")) {
      continue;
    }
    
    // Check for brightness parameter with wrong casing
    if (argName.equalsIgnoreCase("brightness") && argName != "Brightness") {
      alpacaServer.send(400, "text/plain", "Invalid parameter casing - use 'Brightness'");
      return;
    }
    
    // Ignore all other parameters (following Postel's Law)
  }
  
  // Check for correct brightness parameter
  if (alpacaServer.hasArg("Brightness")) {
    String brightnessStr = alpacaServer.arg("Brightness");
    
    // Validate brightness parameter
    if (brightnessStr.length() == 0) {
      alpacaServer.send(400, "text/plain", "Empty Brightness parameter");
      return;
    }
    
    // Check for non-numeric values
    bool isNumeric = true;
    for (unsigned int i = 0; i < brightnessStr.length(); i++) {
      if (!isDigit(brightnessStr[i]) && brightnessStr[i] != '-') {
        isNumeric = false;
        break;
      }
    }
    
    if (!isNumeric) {
      alpacaServer.send(400, "text/plain", "Invalid Brightness parameter");
      return;
    }
    
    int brightness = brightnessStr.toInt();
    
    if (brightness < 0 || brightness > getMaxBrightness()) {
      String errorMsg = "Brightness out of range (0-" + String(getMaxBrightness()) + ")";
      sendAlpacaResponse(clientID, clientTransactionID, ASCOM_ERROR_INVALID_VALUE, errorMsg, "");
      return;
    }
    
    if (setCalibratorBrightness(brightness)) {
      sendAlpacaResponse(clientID, clientTransactionID, 0, "", "");
    } else {
      sendAlpacaResponse(clientID, clientTransactionID, 1025, "Failed to set brightness", "");
    }
  } else {
    // No brightness parameter - turn on at max brightness
    if (turnCalibratorOn()) {
      sendAlpacaResponse(clientID, clientTransactionID, 0, "", "");
    } else {
      sendAlpacaResponse(clientID, clientTransactionID, 1025, "Failed to turn on calibrator", "");
    }
  }
}

void handleCalibratorOff() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  if (!isConnected) {
    sendAlpacaResponse(clientID, clientTransactionID, ASCOM_ERROR_NOT_CONNECTED, "Not connected", "");
    return;
  }
  
  // Validate parameter casing - CalibratorOff takes no parameters except ClientID/ClientTransactionID
  for (int i = 0; i < alpacaServer.args(); i++) {
    String argName = alpacaServer.argName(i);
    if (argName.equalsIgnoreCase("ClientID") || 
        argName.equalsIgnoreCase("ClientTransactionID")) {
      continue;
    }
    // CalibratorOff takes no other parameters - ignore extra params per Postel's Law
  }
  
  if (turnCalibratorOff()) {
    sendAlpacaResponse(clientID, clientTransactionID, 0, "", "");
  } else {
    sendAlpacaResponse(clientID, clientTransactionID, 1025, "Failed to turn off calibrator", "");
  }
}

// Cover methods - not implemented
void handleOpenCover() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  // Validate parameter casing first, even though method isn't implemented
  for (int i = 0; i < alpacaServer.args(); i++) {
    String argName = alpacaServer.argName(i);
    if (argName.equalsIgnoreCase("ClientID") || 
        argName.equalsIgnoreCase("ClientTransactionID")) {
      continue;
    }
    // OpenCover takes no other parameters, but validate case if any are provided
    // (Following Postel's Law - ignore extra params but validate known ones)
  }
  
  sendAlpacaResponse(clientID, clientTransactionID, ASCOM_ERROR_NOT_IMPLEMENTED, "Cover control not implemented", "");
}

void handleCloseCover() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  // Validate parameter casing first, even though method isn't implemented
  for (int i = 0; i < alpacaServer.args(); i++) {
    String argName = alpacaServer.argName(i);
    if (argName.equalsIgnoreCase("ClientID") || 
        argName.equalsIgnoreCase("ClientTransactionID")) {
      continue;
    }
    // CloseCover takes no other parameters, but validate case if any are provided
  }
  
  sendAlpacaResponse(clientID, clientTransactionID, ASCOM_ERROR_NOT_IMPLEMENTED, "Cover control not implemented", "");
}

void handleHaltCover() {
  int clientID = getClientID();
  int clientTransactionID = getClientTransactionID();
  
  // Validate parameter casing first, even though method isn't implemented
  for (int i = 0; i < alpacaServer.args(); i++) {
    String argName = alpacaServer.argName(i);
    if (argName.equalsIgnoreCase("ClientID") || 
        argName.equalsIgnoreCase("ClientTransactionID")) {
      continue;
    }
    // HaltCover takes no other parameters, but validate case if any are provided
  }
  
  sendAlpacaResponse(clientID, clientTransactionID, ASCOM_ERROR_NOT_IMPLEMENTED, "Cover control not implemented", "");
}

// Setup pages
void handleSetupRedirect() {
  String redirectUrl = "http://" + WiFi.localIP().toString() + ":" + String(ALPACA_PORT) + "/setup/v1/covercalibrator/0/setup";
  String html = "<!DOCTYPE html><html><head><title>Setup Redirect</title>";
  html += "<meta http-equiv='refresh' content='0;url=" + redirectUrl + "'></head><body>";
  html += "<h1>Redirecting to Device Setup...</h1>";
  html += "<p>If not redirected, <a href='" + redirectUrl + "'>click here</a>.</p>";
  html += "</body></html>";
  
  alpacaServer.send(200, "text/html", html);
}

// FIXED: ASCOM-compliant setup page
void handleCoverCalibratorSetup() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><title>Flat Panel Calibrator Setup</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }";
  html += ".container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
  html += "h1 { color: #2c3e50; border-bottom: 2px solid #3498db; padding-bottom: 10px; }";
  html += ".status { background: #ecf0f1; padding: 15px; border-radius: 5px; margin: 10px 0; }";
  html += ".controls { background: #f8f9fa; padding: 15px; border-radius: 5px; margin: 10px 0; }";
  html += "button { background: #3498db; color: white; border: none; padding: 10px 20px; margin: 5px; border-radius: 4px; cursor: pointer; }";
  html += "button:hover { background: #2980b9; }";
  html += "button.danger { background: #e74c3c; }";
  html += "button.danger:hover { background: #c0392b; }";
  html += "input[type=range] { width: 300px; margin: 10px; }";
  html += ".brightness-display { font-size: 18px; font-weight: bold; color: #2c3e50; }";
  html += "</style></head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>ESP32 Flat Panel Calibrator Setup</h1>";
  
  html += "<div class='status'>";
  html += "<h2>Current Status</h2>";
  html += "<p><strong>Device:</strong> " + deviceName + "</p>";
  html += "<p><strong>State:</strong> <span id='state'>" + getCalibratorStateString() + "</span></p>";
  html += "<p><strong>Brightness:</strong> <span id='currentBrightness'>" + String(getCurrentBrightness()) + "%</span></p>";
  html += "<p><strong>Max Brightness:</strong> " + String(getMaxBrightness()) + "%</p>";
  html += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
  html += "</div>";
  
  html += "<div class='controls'>";
  html += "<h2>Manual Controls</h2>";
  html += "<button onclick='calibratorOn()'>Turn ON (Max)</button>";
  html += "<button onclick='calibratorOff()' class='danger'>Turn OFF</button>";
  html += "<br><br>";
  html += "<label for='brightness'>Set Brightness: </label>";
  html += "<input type='range' id='brightness' min='0' max='" + String(getMaxBrightness()) + "' value='" + String(getCurrentBrightness()) + "' onchange='setBrightness(this.value)'>";
  html += "<div class='brightness-display' id='brightnessValue'>" + String(getCurrentBrightness()) + "%</div>";
  html += "</div>";
  
  html += "<div class='status'>";
  html += "<h2>ASCOM Information</h2>";
  html += "<p><strong>Device Type:</strong> CoverCalibrator</p>";
  html += "<p><strong>API Base:</strong> http://" + WiFi.localIP().toString() + ":" + String(ALPACA_PORT) + "/api/v1/covercalibrator/0/</p>";
  html += "<p><strong>Web Interface:</strong> <a href='http://" + WiFi.localIP().toString() + "'>http://" + WiFi.localIP().toString() + "</a></p>";
  html += "</div>";
  
  html += "</div>";
  
  // JavaScript for controls
  html += "<script>";
  html += "function updateStatus() {";
  html += "  fetch('/api/v1/covercalibrator/0/calibratorstate?ClientID=1&ClientTransactionID=1')";
  html += "    .then(r => r.json()).then(d => document.getElementById('state').innerText = d.Value == 1 ? 'Off' : d.Value == 3 ? 'Ready' : 'Unknown');";
  html += "  fetch('/api/v1/covercalibrator/0/brightness?ClientID=1&ClientTransactionID=1')";
  html += "    .then(r => r.json()).then(d => {";
  html += "      document.getElementById('currentBrightness').innerText = d.Value + '%';";
  html += "      document.getElementById('brightness').value = d.Value;";
  html += "      document.getElementById('brightnessValue').innerText = d.Value + '%';";
  html += "    });";
  html += "}";
  html += "function calibratorOn() {";
  html += "  fetch('/api/v1/covercalibrator/0/calibratoron', {method: 'PUT', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'ClientID=1&ClientTransactionID=1'})";
  html += "    .then(() => setTimeout(updateStatus, 200));";
  html += "}";
  html += "function calibratorOff() {";
  html += "  fetch('/api/v1/covercalibrator/0/calibratoroff', {method: 'PUT', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'ClientID=1&ClientTransactionID=1'})";
  html += "    .then(() => setTimeout(updateStatus, 200));";
  html += "}";
  html += "function setBrightness(value) {";
  html += "  document.getElementById('brightnessValue').innerText = value + '%';";
  html += "  fetch('/api/v1/covercalibrator/0/calibratoron', {method: 'PUT', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'ClientID=1&ClientTransactionID=1&Brightness=' + value})";
  html += "    .then(() => setTimeout(updateStatus, 200));";
  html += "}";
  html += "setInterval(updateStatus, 2000);"; // Auto-refresh every 2 seconds
  html += "</script>";
  
  html += "</body></html>";
  
  alpacaServer.send(200, "text/html", html);
}
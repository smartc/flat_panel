/*
 * ESP32 ASCOM Alpaca Flat Panel Calibrator
 * Web UI Handler Implementation - FIXED VERSION
 */

#include "web_ui_handler.h"
#include <ArduinoJson.h>  // MISSING INCLUDE - THIS FIXES THE COMPILATION ERROR
#include "calibrator_controller.h"
#include "html_templates.h"
#include "Debug.h"

// Web server instance
WebServer webUiServer(WEB_UI_PORT);

// Load configuration from preferences
void loadConfiguration() {
  Preferences preferences;
  preferences.begin(PREFERENCES_NAMESPACE, true); // Read-only
  
  // Load WiFi settings
  String storedSSID = preferences.getString(PREF_WIFI_SSID, DEFAULT_WIFI_SSID);
  String storedPassword = preferences.getString(PREF_WIFI_PASSWORD, DEFAULT_WIFI_PASSWORD);
  
  storedSSID.toCharArray(ssid, SSID_SIZE);
  storedPassword.toCharArray(password, PASSWORD_SIZE);
  
  // Load device settings
  deviceName = preferences.getString(PREF_DEVICE_NAME, "Flat Panel Calibrator");
  serialDebugEnabled = preferences.getBool(PREF_SERIAL_DEBUG, false);
  
  preferences.end();
  
  Debug.println("Configuration loaded from preferences");
}

// Save configuration to preferences
void saveConfiguration() {
  Preferences preferences;
  preferences.begin(PREFERENCES_NAMESPACE, false); // Read-write
  
  // Save WiFi settings
  preferences.putString(PREF_WIFI_SSID, ssid);
  preferences.putString(PREF_WIFI_PASSWORD, password);
  
  // Save device settings
  preferences.putString(PREF_DEVICE_NAME, deviceName);
  preferences.putBool(PREF_SERIAL_DEBUG, serialDebugEnabled);
  
  preferences.end();
  
  Debug.println("Configuration saved to preferences");
}

// Initialize Web UI
void initWebUI() {
  // Handle root page
  webUiServer.on("/", HTTP_GET, handleRoot);
  
  // Handle setup page
  webUiServer.on("/setup", HTTP_GET, handleSetup);
  webUiServer.on("/setup", HTTP_POST, handleSetupPost);
  
  // Handle calibrator control
  webUiServer.on("/calibrator", HTTP_GET, handleCalibrator);
  webUiServer.on("/calibrator", HTTP_POST, handleCalibratorPost);
  
  // Add status API for JavaScript updates - FIXED WITH ARDUINOJSON INCLUDE
  webUiServer.on("/api/status", HTTP_GET, []() {
    DynamicJsonDocument doc(200);
    doc["brightness"] = getCurrentBrightness();
    doc["state"] = getCalibratorStateString();
    doc["maxBrightness"] = getMaxBrightness();
    doc["connected"] = isConnected;
    
    String response;
    serializeJson(doc, response);
    webUiServer.send(200, "application/json", response);
  });
  
  // Add WiFi configuration routes
  webUiServer.on("/wificonfig", HTTP_GET, handleWifiConfig);
  webUiServer.on("/wificonfig", HTTP_POST, handleWifiConfigPost);
  
  // Restart handler
  webUiServer.on("/restart", HTTP_POST, handleRestart);
  
  // Start server
  webUiServer.begin();
  Debug.printf("Web UI server started on port %d\n", WEB_UI_PORT);
}

// Handle Web UI requests in the main loop
void handleWebUI() {
  webUiServer.handleClient();
}

// Handle the root page - shows device status and controls
void handleRoot() {
  String html = getHomePage();
  webUiServer.send(200, "text/html", html);
}

// Handle setup page
void handleSetup() {
  String html = getSetupPage();
  webUiServer.send(200, "text/html", html);
}

// Handle setup form submission
void handleSetupPost() {
  bool settingsChanged = false;
  
  // Process device name
  if (webUiServer.hasArg("deviceName")) {
    String newDeviceName = webUiServer.arg("deviceName");
    if (newDeviceName.length() > 0 && newDeviceName != deviceName) {
      deviceName = newDeviceName;
      settingsChanged = true;
      Debug.println("Device name changed");
    }
  }
  
  // Process max brightness
  if (webUiServer.hasArg("maxBrightness")) {
    int newMaxBrightness = webUiServer.arg("maxBrightness").toInt();
    if (newMaxBrightness > 0 && newMaxBrightness <= MAX_BRIGHTNESS && newMaxBrightness != getMaxBrightness()) {
      setMaxBrightness(newMaxBrightness);
      settingsChanged = true;
      Debug.println("Max brightness changed");
    }
  }
  
  // Process debug setting
  if (webUiServer.hasArg("debugEnabled")) {
    bool newDebugEnabled = webUiServer.arg("debugEnabled") == "true";
    if (newDebugEnabled != serialDebugEnabled) {
      serialDebugEnabled = newDebugEnabled;
      settingsChanged = true;
      Debug.println("Debug setting changed");
    }
  }
  
  // Save settings if changed
  if (settingsChanged) {
    saveConfiguration();
  }
  
  String message = "Settings updated.";
  if (!settingsChanged) {
    message = "No changes detected.";
  }
  message += "<br><a href='/setup'>Back to setup page</a>";
  
  webUiServer.send(200, "text/html", message);
}

// Handle calibrator control page
void handleCalibrator() {
  String html = getCalibratorPage();
  webUiServer.send(200, "text/html", html);
}

// Handle calibrator control form submission
void handleCalibratorPost() {
  if (webUiServer.hasArg("action")) {
    String action = webUiServer.arg("action");
    
    if (action == "on") {
      turnCalibratorOn();
      webUiServer.send(200, "text/plain", "Calibrator turned ON");
    } else if (action == "off") {
      turnCalibratorOff();
      webUiServer.send(200, "text/plain", "Calibrator turned OFF");
    } else if (action == "brightness" && webUiServer.hasArg("brightness")) {
      int brightness = webUiServer.arg("brightness").toInt();
      if (setCalibratorBrightness(brightness)) {
        webUiServer.send(200, "text/plain", "Brightness set to " + String(brightness) + "%");
      } else {
        webUiServer.send(400, "text/plain", "Invalid brightness value");
      }
    } else {
      webUiServer.send(400, "text/plain", "Invalid action");
    }
  } else {
    webUiServer.send(400, "text/plain", "Missing action parameter");
  }
}

// Handle WiFi configuration page
void handleWifiConfig() {
  String html = getWifiConfigPage();
  webUiServer.send(200, "text/html", html);
}

// Handle WiFi configuration form submission
void handleWifiConfigPost() {
  bool wifiChanged = false;
  
  if (webUiServer.hasArg("ssid") && webUiServer.hasArg("password")) {
    String newSSID = webUiServer.arg("ssid");
    String newPassword = webUiServer.arg("password");
    
    if (newSSID.length() > 0 && newSSID != String(ssid)) {
      newSSID.toCharArray(ssid, SSID_SIZE);
      wifiChanged = true;
    }
    
    if (newPassword != String(password)) {
      newPassword.toCharArray(password, PASSWORD_SIZE);
      wifiChanged = true;
    }
    
    if (wifiChanged) {
      saveConfiguration();
      
      String html = "<!DOCTYPE html><html><head><title>WiFi Updated</title>";
      html += "<meta http-equiv='refresh' content='5;url=/'></head><body>";
      html += "<h1>WiFi Settings Updated</h1>";
      html += "<p>The device will restart to apply new WiFi settings...</p>";
      html += "<p>You will be redirected to the main page in 5 seconds.</p>";
      html += "</body></html>";
      
      webUiServer.send(200, "text/html", html);
      
      delay(2000);
      ESP.restart();
    } else {
      String html = "<!DOCTYPE html><html><head><title>No Changes</title>";
      html += "<meta http-equiv='refresh' content='3;url=/wificonfig'></head><body>";
      html += "<h1>No Changes Detected</h1>";
      html += "<p>Redirecting back to WiFi configuration...</p>";
      html += "</body></html>";
      
      webUiServer.send(200, "text/html", html);
    }
  } else {
    webUiServer.send(400, "text/plain", "Missing SSID or password");
  }
}

// Handle restart request
void handleRestart() {
  String html = "<!DOCTYPE html><html><head><title>Restarting</title>";
  html += "<meta http-equiv='refresh' content='10;url=/'></head><body>";
  html += "<h1>Device Restarting...</h1>";
  html += "<p>Please wait while the device restarts.</p>";
  html += "<p>You will be redirected automatically in 10 seconds.</p>";
  html += "</body></html>";
  
  webUiServer.send(200, "text/html", html);
  
  delay(1000);
  ESP.restart();
}
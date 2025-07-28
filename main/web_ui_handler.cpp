/*
 * ESP32 ASCOM Alpaca Flat Panel Calibrator
 * Web UI Handler Implementation
 */

#include "web_ui_handler.h"
#include "html_templates.h"
#include "calibrator_controller.h"
#include "Debug.h"
#include <WiFi.h>

// Standard Web Server
WebServer webUiServer(WEB_UI_PORT);

// Preferences instance
Preferences preferences;

// Load configuration from preferences
void loadConfiguration() {
  preferences.begin(PREFERENCES_NAMESPACE, false);
  
  // Load WiFi settings if they exist
  if (preferences.isKey(PREF_WIFI_SSID)) {
    String savedSSID = preferences.getString(PREF_WIFI_SSID, "");
    String savedPassword = preferences.getString(PREF_WIFI_PASSWORD, "");
    
    if (savedSSID.length() > 0) {
      strncpy(ssid, savedSSID.c_str(), sizeof(ssid) - 1);
      ssid[sizeof(ssid) - 1] = '\0';
    }
    
    if (savedPassword.length() > 0) {
      strncpy(password, savedPassword.c_str(), sizeof(password) - 1);
      password[sizeof(password) - 1] = '\0';
    }
  }
  
  // Load device settings
  if (preferences.isKey(PREF_DEVICE_NAME)) {
    deviceName = preferences.getString(PREF_DEVICE_NAME, "Flat Panel Calibrator");
  }
  
  if (preferences.isKey(PREF_SERIAL_DEBUG)) {
    serialDebugEnabled = preferences.getBool(PREF_SERIAL_DEBUG, false);
  }
  
  preferences.end();
  
  Debug.println("Configuration loaded from preferences");
}

// Save configuration to preferences
void saveConfiguration() {
  preferences.begin(PREFERENCES_NAMESPACE, false);
  
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
  
  // Add status API for JavaScript updates
  webUiServer.on("/api/status", HTTP_GET, []() {
    DynamicJsonDocument doc(200);
    doc["brightness"] = getCurrentBrightness();
    doc["state"] = getCalibratorStateString();
    doc["maxBrightness"] = getMaxBrightness();
    
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

// WiFi Configuration page handler
void handleWifiConfig() {
  String html = getWifiConfigPage();
  webUiServer.send(200, "text/html", html);
}

// WiFi Configuration form submission handler
void handleWifiConfigPost() {
  if (webUiServer.hasArg("ssid") && webUiServer.hasArg("password")) {
    String newSSID = webUiServer.arg("ssid");
    String newPassword = webUiServer.arg("password");
    
    if (newSSID.length() > 0) {
      strncpy(ssid, newSSID.c_str(), sizeof(ssid) - 1);
      ssid[sizeof(ssid) - 1] = '\0';
      
      if (newPassword.length() > 0) {
        strncpy(password, newPassword.c_str(), sizeof(password) - 1);
        password[sizeof(password) - 1] = '\0';
      }
      
      // Save to preferences
      saveConfiguration();
      
      Debug.println("WiFi settings saved via config page");
      
      // Display success message and restart options
      String html = "<!DOCTYPE html><html>";
      html += "<head><title>WiFi Configuration</title>";
      html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
      html += "<style>" + getCommonStyles() + "</style>";
      html += "</head>";
      html += "<body>";
      html += "<div class='container'>";
      html += "<h1>WiFi Configuration</h1>";
      html += "<p class='success'>WiFi settings saved successfully!</p>";
      html += "<p>SSID: " + String(ssid) + "</p>";
      html += "<p>To apply the new settings, the device needs to restart.</p>";
      html += "<button onclick='restart()'>Restart Now</button>";
      html += "<button onclick='goBack()'>Back to Home</button>";
      html += "<script>";
      html += "function restart() {";
      html += "  fetch('/restart', { method: 'POST' })";
      html += "    .then(response => {";
      html += "      alert('Device is restarting with new WiFi settings.');";
      html += "    });";
      html += "}";
      html += "function goBack() {";
      html += "  window.location.href = '/';";
      html += "}";
      html += "</script>";
      html += "</div></body></html>";
      
      webUiServer.send(200, "text/html", html);
    } else {
      webUiServer.send(400, "text/plain", "SSID is required!");
    }
  } else {
    webUiServer.send(400, "text/plain", "Missing required parameters!");
  }
}

// Handle device restart
void handleRestart() {
  webUiServer.send(200, "text/plain", "Restarting device...");
  Debug.println("Device restart requested via web interface");
  delay(1000);
  ESP.restart();
}
/*
 * ESP32 ASCOM Alpaca Flat Panel Calibrator
 * HTML Page Templates
 */

#ifndef HTML_TEMPLATES_H
#define HTML_TEMPLATES_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "calibrator_controller.h"
#include "alpaca_handler.h"

// Function prototypes
String getPageHeader(String pageTitle);
String getNavBar();
String getCommonStyles();
String getHomePage();
String getSetupPage();
String getCalibratorPage();
String getWifiConfigPage();

// Common CSS styles used across pages
inline String getCommonStyles() {
  String styles = 
    "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f8ff; }\n"
    "h1, h2 { color: #2c3e50; }\n"
    "a { color: #3498db; text-decoration: none; }\n"
    "a:hover { text-decoration: underline; }\n"
    ".container { max-width: 800px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }\n"
    ".card { background: #f8f9fa; border-radius: 4px; padding: 15px; margin-bottom: 20px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n"
    ".nav-bar { margin-bottom: 20px; padding: 10px; background-color: #f8f9fa; border-radius: 4px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n"
    ".nav-button { display: inline-block; margin: 5px; padding: 8px 15px; background-color: #3498db; color: white; border-radius: 4px; text-decoration: none; }\n"
    ".nav-button:hover { background-color: #2980b9; text-decoration: none; color: white; }\n"
    "label { display: block; margin-bottom: 5px; font-weight: bold; }\n"
    "input[type=text], input[type=password], input[type=number] { width: 100%; padding: 8px; margin-bottom: 15px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }\n"
    "input[type=range] { width: 100%; margin: 10px 0; }\n"
    "input[type=submit], button { background: #3498db; color: white; border: none; padding: 10px 15px; border-radius: 4px; cursor: pointer; margin: 5px; }\n"
    "input[type=submit]:hover, button:hover { background: #2980b9; }\n"
    "table { border-collapse: collapse; width: 100%; }\n"
    "table, th, td { border: 1px solid #ddd; }\n"
    "th, td { padding: 8px; text-align: left; }\n"
    "th { background-color: #f2f2f2; }\n"
    ".status-on { color: green; font-weight: bold; }\n"
    ".status-off { color: red; font-weight: bold; }\n"
    ".status-ready { color: blue; font-weight: bold; }\n"
    ".status-error { color: darkred; font-weight: bold; }\n"
    ".button-row { display: flex; flex-wrap: wrap; gap: 10px; margin-top: 15px; }\n"
    ".button-primary { background-color: #3498db; }\n"
    ".button-success { background-color: #2ecc71; }\n"
    ".button-warning { background-color: #f39c12; }\n"
    ".button-danger { background-color: #e74c3c; }\n"
    ".brightness-control { margin: 20px 0; }\n"
    ".brightness-display { font-size: 24px; font-weight: bold; margin: 10px 0; }\n"
    ".success { color: green; font-weight: bold; }\n"
    ".error { color: red; font-weight: bold; }\n"
    ".center { text-align: center; }\n";
  
  return styles;
}

// Common HTML page header
inline String getPageHeader(String pageTitle) {
  String header = "<!DOCTYPE html><html>\n"
    "<head><title>" + pageTitle + "</title>\n"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>\n"
    "<style>\n" + 
    getCommonStyles() + 
    "</style>\n"
    "</head>\n"
    "<body>\n"
    "<div class='container'>\n";
  
  return header;
}

// Navigation links 
inline String getNavBar() {
  String navbar = 
    "<div class='nav-bar'>\n"
    "<a href='/' class='nav-button'>Home</a>\n"
    "<a href='/calibrator' class='nav-button'>Calibrator</a>\n"
    "<a href='/setup' class='nav-button'>Setup</a>\n"
    "<a href='/wificonfig' class='nav-button'>WiFi Config</a>\n"
    "<a href='http://" + WiFi.localIP().toString() + ":" + String(ALPACA_PORT) + "/setup/v1/covercalibrator/0/setup' class='nav-button'>ASCOM Controls</a>\n"
    "</div>\n";
  
  return navbar;
}

// Generate the home page HTML
inline String getHomePage() {
  String html = getPageHeader("ESP32 Flat Panel Calibrator");
  
  html += "<h1>ESP32 Flat Panel Calibrator</h1>\n";
  html += "<p>Version: " + String(DEVICE_VERSION) + "</p>\n";
  
  // Add navigation
  html += getNavBar();
  
  // Status Card
  html += "<div class='card'>\n";
  html += "<h2>Current Status</h2>\n";
  html += "<table>\n";
  html += "<tr><td>Device Name</td><td>" + deviceName + "</td></tr>\n";
  html += "<tr><td>Firmware Version</td><td>" + String(DEVICE_VERSION) + "</td></tr>\n";
  html += "<tr><td>Unique ID</td><td>" + uniqueID + "</td></tr>\n";
  html += "<tr><td>IP Address</td><td>" + WiFi.localIP().toString() + "</td></tr>\n";
  
  // Status with color coding
  String statusClass = "";
  String statusString = getCalibratorStateString();
  if (statusString == "Ready") {
    statusClass = "status-ready";
  } else if (statusString == "Off") {
    statusClass = "status-off";
  } else if (statusString == "Error") {
    statusClass = "status-error";
  }
  
  html += "<tr><td>Calibrator State</td><td class='" + statusClass + "'>" + statusString + "</td></tr>\n";
  html += "<tr><td>Current Brightness</td><td>" + String(getCurrentBrightness()) + "%</td></tr>\n";
  html += "<tr><td>Max Brightness</td><td>" + String(getMaxBrightness()) + "%</td></tr>\n";
  html += "<tr><td>Connected</td><td>" + String(isConnected ? "Yes" : "No") + "</td></tr>\n";
  html += "</table>\n";
  html += "</div>\n";
  
  // Quick Controls Card
  html += "<div class='card'>\n";
  html += "<h2>Quick Controls</h2>\n";
  html += "<div class='button-row'>\n";
  html += "<button onclick='calibratorOn()' class='button-success'>Turn ON</button>\n";
  html += "<button onclick='calibratorOff()' class='button-danger'>Turn OFF</button>\n";
  html += "</div>\n";
  html += "<div class='brightness-control'>\n";
  html += "<label for='brightness'>Brightness Control:</label>\n";
  html += "<input type='range' id='brightness' min='0' max='" + String(getMaxBrightness()) + "' value='" + String(getCurrentBrightness()) + "' onchange='setBrightness(this.value)'>\n";
  html += "<div class='brightness-display center' id='brightnessValue'>" + String(getCurrentBrightness()) + "%</div>\n";
  html += "</div>\n";
  html += "</div>\n";
  
  // Network Information Card
  html += "<div class='card'>\n";
  html += "<h2>Network Information</h2>\n";
  html += "<table>\n";
  if (WiFi.status() == WL_CONNECTED) {
    html += "<tr><td>WiFi Status</td><td class='status-on'>Connected</td></tr>\n";
    html += "<tr><td>SSID</td><td>" + WiFi.SSID() + "</td></tr>\n";
    html += "<tr><td>Signal Strength</td><td>" + String(WiFi.RSSI()) + " dBm</td></tr>\n";
  } else {
    html += "<tr><td>WiFi Status</td><td class='status-off'>Disconnected</td></tr>\n";
  }
  html += "<tr><td>MAC Address</td><td>" + WiFi.macAddress() + "</td></tr>\n";
  html += "<tr><td>Web Interface</td><td><a href='http://" + WiFi.localIP().toString() + "'>http://" + WiFi.localIP().toString() + "</a></td></tr>\n";
  html += "<tr><td>ASCOM Alpaca API</td><td><a href='http://" + WiFi.localIP().toString() + ":" + String(ALPACA_PORT) + "'>http://" + WiFi.localIP().toString() + ":" + String(ALPACA_PORT) + "</a></td></tr>\n";
  html += "<tr><td>Free Heap</td><td>" + String(ESP.getFreeHeap()) + " bytes</td></tr>\n";
  html += "</table>\n";
  html += "</div>\n";
  
  // JavaScript for controls
  html += "<script>\n";
  html += "function updateStatus() {\n";
  html += "  fetch('/api/status')\n";
  html += "    .then(response => response.json())\n";
  html += "    .then(data => {\n";
  html += "      const brightness = data.brightness;\n";
  html += "      document.getElementById('brightness').value = brightness;\n";
  html += "      document.getElementById('brightnessValue').innerText = brightness + '%';\n";
  html += "      // Update status table if it exists\n";
  html += "      const statusRows = document.querySelectorAll('td');\n";
  html += "      statusRows.forEach(cell => {\n";
  html += "        if (cell.previousElementSibling && cell.previousElementSibling.innerText === 'Current Brightness') {\n";
  html += "          cell.innerText = brightness + '%';\n";
  html += "        }\n";
  html += "      });\n";
  html += "    })\n";
  html += "    .catch(err => console.log('Status update failed:', err));\n";
  html += "}\n";
  html += "function calibratorOn() {\n";
  html += "  fetch('/calibrator', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'action=on' })\n";
  html += "    .then(response => response.text())\n";
  html += "    .then(data => { \n";
  html += "      setTimeout(updateStatus, 500);\n";
  html += "      setTimeout(() => location.reload(), 1000); \n";
  html += "    });\n";
  html += "}\n";
  html += "function calibratorOff() {\n";
  html += "  fetch('/calibrator', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'action=off' })\n";
  html += "    .then(response => response.text())\n";
  html += "    .then(data => { \n";
  html += "      setTimeout(updateStatus, 500);\n";
  html += "      setTimeout(() => location.reload(), 1000); \n";
  html += "    });\n";
  html += "}\n";
  html += "function setBrightness(value) {\n";
  html += "  document.getElementById('brightnessValue').innerText = value + '%';\n";
  html += "  // Update status table immediately\n";
  html += "  const statusRows = document.querySelectorAll('td');\n";
  html += "  statusRows.forEach(cell => {\n";
  html += "    if (cell.previousElementSibling && cell.previousElementSibling.innerText === 'Current Brightness') {\n";
  html += "      cell.innerText = value + '%';\n";
  html += "    }\n";
  html += "  });\n";
  html += "  fetch('/calibrator', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'action=brightness&brightness=' + value })\n";
  html += "    .then(response => response.text());\n";
  html += "}\n";
  html += "// Update status every 10 seconds\n";
  html += "setInterval(updateStatus, 10000);\n";
  html += "</script>\n";
  
  html += "</div></body></html>";
  
  return html;
}

// Generate the setup page HTML
inline String getSetupPage() {
  String html = getPageHeader("Device Setup");
  
  html += "<h1>Device Setup</h1>\n";
  
  // Add navigation
  html += getNavBar();
  
  // Device Settings Card
  html += "<div class='card'>\n";
  html += "<h2>Device Settings</h2>\n";
  html += "<form method='post' action='/setup'>\n";
  html += "<label for='deviceName'>Device Name:</label>\n";
  html += "<input type='text' id='deviceName' name='deviceName' value='" + deviceName + "'>\n";
  html += "<label for='maxBrightness'>Maximum Brightness (%):</label>\n";
  html += "<input type='number' id='maxBrightness' name='maxBrightness' min='1' max='" + String(MAX_BRIGHTNESS) + "' value='" + String(getMaxBrightness()) + "'>\n";
  html += "<label><input type='checkbox' name='debugEnabled' value='true'" + String(serialDebugEnabled ? " checked" : "") + "> Enable Serial Debug Output</label><br><br>\n";
  html += "<input type='submit' value='Save Settings'>\n";
  html += "</form>\n";
  html += "</div>\n";
  
  // Current Status Card
  html += "<div class='card'>\n";
  html += "<h2>Current Status</h2>\n";
  html += "<table>\n";
  html += "<tr><td>Calibrator State</td><td>" + getCalibratorStateString() + "</td></tr>\n";
  html += "<tr><td>Current Brightness</td><td>" + String(getCurrentBrightness()) + "%</td></tr>\n";
  html += "<tr><td>Max Brightness</td><td>" + String(getMaxBrightness()) + "%</td></tr>\n";
  html += "<tr><td>Debug Enabled</td><td>" + String(serialDebugEnabled ? "Yes" : "No") + "</td></tr>\n";
  html += "</table>\n";
  html += "</div>\n";
  
  // System Management Card
  html += "<div class='card'>\n";
  html += "<h2>System Management</h2>\n";
  html += "<div class='button-row'>\n";
  html += "<button onclick='restartDevice()' class='button-danger'>Restart Device</button>\n";
  html += "</div>\n";
  html += "</div>\n";
  
  // JavaScript
  html += "<script>\n";
  html += "function restartDevice() {\n";
  html += "  if (confirm('Are you sure you want to restart the device?')) {\n";
  html += "    fetch('/restart', { method: 'POST' })\n";
  html += "      .then(response => { alert('Device is restarting...'); });\n";
  html += "  }\n";
  html += "}\n";
  html += "</script>\n";
  
  html += "</div></body></html>";
  
  return html;
}

// Generate the calibrator control page HTML
inline String getCalibratorPage() {
  String html = getPageHeader("Calibrator Control");
  
  html += "<h1>Calibrator Control</h1>\n";
  
  // Add navigation
  html += getNavBar();
  
  // Calibrator Control Card
  html += "<div class='card'>\n";
  html += "<h2>Brightness Control</h2>\n";
  html += "<div class='center'>\n";
  html += "<div class='brightness-display'>Current: " + String(getCurrentBrightness()) + "%</div>\n";
  html += "<div class='brightness-display'>State: " + getCalibratorStateString() + "</div>\n";
  html += "</div>\n";
  html += "<div class='brightness-control'>\n";
  html += "<label for='brightness'>Brightness:</label>\n";
  html += "<input type='range' id='brightness' min='0' max='" + String(getMaxBrightness()) + "' value='" + String(getCurrentBrightness()) + "' onchange='setBrightness(this.value)'>\n";
  html += "<div class='brightness-display center' id='brightnessValue'>" + String(getCurrentBrightness()) + "%</div>\n";
  html += "</div>\n";
  html += "<div class='button-row center'>\n";
  html += "<button onclick='calibratorOff()' class='button-danger'>Turn OFF</button>\n";
  html += "<button onclick='setBrightness(25)' class='button-primary'>25%</button>\n";
  html += "<button onclick='setBrightness(50)' class='button-primary'>50%</button>\n";
  html += "<button onclick='setBrightness(75)' class='button-primary'>75%</button>\n";
  html += "<button onclick='calibratorOn()' class='button-success'>100%</button>\n";
  html += "</div>\n";
  html += "</div>\n";
  
  // JavaScript for controls
  html += "<script>\n";
  html += "function updateDisplay() {\n";
  html += "  fetch('/api/v1/covercalibrator/0/brightness?ClientID=1&ClientTransactionID=1')\n";
  html += "    .then(response => response.json())\n";
  html += "    .then(data => {\n";
  html += "      if (data.ErrorNumber === 0) {\n";
  html += "        const brightness = data.Value;\n";
  html += "        document.getElementById('brightness').value = brightness;\n";
  html += "        document.getElementById('brightnessValue').innerText = brightness + '%';\n";
  html += "        // Update the current brightness display\n";
  html += "        const currentDisplay = document.querySelector('.brightness-display');\n";
  html += "        if (currentDisplay) {\n";
  html += "          currentDisplay.innerHTML = 'Current: ' + brightness + '%';\n";
  html += "        }\n";
  html += "      }\n";
  html += "    });\n";
  html += "}\n";
  html += "function calibratorOn() {\n";
  html += "  fetch('/calibrator', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'action=on' })\n";
  html += "    .then(response => response.text())\n";
  html += "    .then(data => { updateDisplay(); setTimeout(() => location.reload(), 500); });\n";
  html += "}\n";
  html += "function calibratorOff() {\n";
  html += "  fetch('/calibrator', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'action=off' })\n";
  html += "    .then(response => response.text())\n";
  html += "    .then(data => { updateDisplay(); setTimeout(() => location.reload(), 500); });\n";
  html += "}\n";
  html += "function setBrightness(value) {\n";
  html += "  document.getElementById('brightness').value = value;\n";
  html += "  document.getElementById('brightnessValue').innerText = value + '%';\n";
  html += "  // Update current brightness display immediately\n";
  html += "  const currentDisplay = document.querySelector('.brightness-display');\n";
  html += "  if (currentDisplay) {\n";
  html += "    currentDisplay.innerHTML = 'Current: ' + value + '%';\n";
  html += "  }\n";
  html += "  fetch('/calibrator', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'action=brightness&brightness=' + value })\n";
  html += "    .then(response => response.text());\n";
  html += "}\n";
  html += "// Update display every 5 seconds\n";
  html += "setInterval(updateDisplay, 5000);\n";
  html += "</script>\n";
  
  html += "</div></body></html>";
  
  return html;
}

// Generate the WiFi configuration page HTML
inline String getWifiConfigPage() {
  String html = getPageHeader("WiFi Configuration");
  
  html += "<h1>WiFi Configuration</h1>\n";
  
  // Add navigation
  html += getNavBar();
  
  // Network scanning
  html += "<div class='card'>\n";
  html += "<h2>Available Networks</h2>\n";
  html += "<p>Click on a network to select it:</p>\n";
  
  int numNetworks = WiFi.scanNetworks();
  if (numNetworks == 0) {
    html += "<p>No WiFi networks found</p>\n";
  } else {
    html += "<div style='max-height: 200px; overflow-y: auto; border: 1px solid #ddd; padding: 10px; border-radius: 4px;'>\n";
    for (int i = 0; i < numNetworks; i++) {
      html += "<div style='padding: 8px; margin: 2px 0; border: 1px solid #eee; border-radius: 4px; cursor: pointer;' onclick='selectNetwork(\"" + WiFi.SSID(i) + "\")'>\n";
      html += "<strong>" + WiFi.SSID(i) + "</strong><br>\n";
      html += "Signal: " + String(WiFi.RSSI(i)) + " dBm, ";
      html += "Security: " + String(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Secured") + "\n";
      html += "</div>\n";
    }
    html += "</div>\n";
  }
  html += "</div>\n";
  
  // WiFi Configuration Form
  html += "<div class='card'>\n";
  html += "<h2>WiFi Settings</h2>\n";
  html += "<form method='post' action='/wificonfig'>\n";
  html += "<label for='ssid'>WiFi SSID:</label>\n";
  html += "<input type='text' id='ssid' name='ssid' value='" + String(ssid) + "'>\n";
  html += "<label for='password'>WiFi Password:</label>\n";
  html += "<input type='password' id='password' name='password' value=''>\n";
  html += "<input type='submit' value='Save & Connect'>\n";
  html += "</form>\n";
  html += "</div>\n";
  
  // JavaScript
  html += "<script>\n";
  html += "function selectNetwork(name) {\n";
  html += "  document.getElementById('ssid').value = name;\n";
  html += "  document.getElementById('password').focus();\n";
  html += "}\n";
  html += "</script>\n";
  
  html += "</div></body></html>";
  
  return html;
}

#endif // HTML_TEMPLATES_H
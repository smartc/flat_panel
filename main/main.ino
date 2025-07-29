/*
 * ESP32 ASCOM Alpaca Flat Panel Calibrator
 * 
 * This firmware implements an ASCOM Alpaca compatible Flat Panel Calibrator
 * using the ESP32 microcontroller. It implements the ASCOM CoverCalibrator interface
 * for controlling panel brightness. The panel is manually positioned, so cover
 * functionality is not implemented.
 *
 * Hardware:
 * - ESP32 development board
 * - PWM controlled LED panel on GPIO4
 * - 10-bit PWM resolution (0-1023)
 *
 * Features:
 * - ASCOM Alpaca CoverCalibrator device implementation
 * - Web interface on port 80
 * - Serial command interface for USB control
 * - Alpaca discovery protocol support
 * - Configurable WiFi and device settings
 *
 * Arduino IDE Setup:
 * 1. Install ESP32 board package via Board Manager
 * 2. Install ArduinoJson library via Library Manager
 * 3. Select your ESP32 board variant
 * 4. Set upload speed to 921600 (or 115200 if issues)
 * 5. Ensure all project files are in the same folder as this .ino file
 *
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// Project includes
#include "config.h"
#include "Debug.h"
#include "calibrator_controller.h"
#include "alpaca_handler.h"
#include "web_ui_handler.h"
#include "serial_handler.h"

// WiFi credentials and configuration
char ssid[SSID_SIZE] = DEFAULT_WIFI_SSID;
char password[PASSWORD_SIZE] = DEFAULT_WIFI_PASSWORD;
bool apMode = false;
unsigned long apStartTime = 0;

// Timing variables
unsigned long lastStatusUpdate = 0;

void setup() {
  // Initialize debug output (disabled by default)
  Debug.begin(115200);
  Debug.println("ESP32 ASCOM Alpaca Flat Panel Calibrator");
  Debug.println("Version: " + String(DEVICE_VERSION));
  Debug.println("Manufacturer: " + String(DEVICE_MANUFACTURER));
  
  // Load configuration from preferences
  loadConfiguration();
  
  // Initialize calibrator hardware
  initializeCalibratorController();
  
  // Initialize serial command handler
  initSerialHandler();
  
  // Initialize WiFi
  initWiFi();
  
  // Initialize Alpaca API
  setupAlpacaAPI();
  
  // Initialize Web UI
  initWebUI();
  
  Debug.println("Setup complete!");
  Debug.println("Available interfaces:");
  Debug.printf("  Web UI: http://%s/\n", WiFi.localIP().toString().c_str());
  Debug.printf("  ASCOM Alpaca API: http://%s:%d/\n", WiFi.localIP().toString().c_str(), ALPACA_PORT);
  Debug.println("  Serial commands: Available via USB");
  Debug.println();
}

void loop() {
  // Handle WiFi connection management
  handleWiFiConnection();
  
  // Handle Alpaca discovery and API requests
  handleAlpacaDiscovery();
  handleAlpacaAPI();
  
  // Handle Web UI requests
  handleWebUI();
  
  // Handle serial commands
  handleSerialCommands();
  
  // Update calibrator status
  updateCalibratorStatus();
  
  // Periodic status updates
  if (millis() - lastStatusUpdate > 30000) { // Every 30 seconds
    lastStatusUpdate = millis();
    Debug.printf(2, "Status: %s, Brightness: %d%%, WiFi: %s\n", 
                 getCalibratorStateString().c_str(), 
                 getCurrentBrightness(),
                 WiFi.isConnected() ? "Connected" : (apMode ? "AP Mode" : "Disconnected"));
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}

void initWiFi() {
  Debug.println("Initializing WiFi...");
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  
  // Check if we have valid credentials
  if (strlen(ssid) == 0 || strcmp(ssid, DEFAULT_WIFI_SSID) == 0) {
    Debug.println("No WiFi credentials configured, starting AP mode");
    startAPMode();
    return;
  }
  
  // Try to connect to WiFi
  Debug.printf("Connecting to WiFi network: %s\n", ssid);
  WiFi.begin(ssid, password);
  
  // Wait for connection with timeout
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < 30000) {
    delay(500);
    Debug.print(".");
  }
  Debug.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Debug.printf("WiFi connected successfully!\n");
    Debug.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    Debug.printf("Signal strength: %d dBm\n", WiFi.RSSI());
    apMode = false;
  } else {
    Debug.println("Failed to connect to WiFi, starting AP mode");
    startAPMode();
  }
}

void startAPMode() {
  Debug.println("Starting Access Point mode...");
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  IPAddress IP = WiFi.softAPIP();
  Debug.printf("AP started successfully!\n");
  Debug.printf("SSID: %s\n", AP_SSID);
  Debug.printf("Password: %s\n", AP_PASSWORD);
  Debug.printf("IP address: %s\n", IP.toString().c_str());
  
  apMode = true;
  apStartTime = millis();
}

void handleWiFiConnection() {
  if (apMode) {
    // In AP mode, check if we should try to connect to WiFi again
    if (millis() - apStartTime > AP_TIMEOUT) {
      Debug.println("AP mode timeout, attempting WiFi connection...");
      apMode = false;
      initWiFi();
    }
  } else {
    // In STA mode, check if connection is lost
    if (WiFi.status() != WL_CONNECTED) {
      static unsigned long lastReconnectAttempt = 0;
      if (millis() - lastReconnectAttempt > 30000) { // Try reconnect every 30 seconds
        Debug.println("WiFi connection lost, attempting reconnection...");
        WiFi.reconnect();
        lastReconnectAttempt = millis();
      }
    }
  }
}
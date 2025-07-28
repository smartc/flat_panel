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
  Debug.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Debug.println("Send 'DEBUG ON' via serial to enable debug output");
  Debug.println("Send 'HELP' via serial for available commands");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Handle WiFi connection
  handleWiFi();
  
  // Handle serial commands
  handleSerialCommands();
  
  // Update calibrator status
  updateCalibratorStatus();
  
  // Handle Alpaca discovery
  handleAlpacaDiscovery();

  // Handle Alpaca endpoint requests
  alpacaServer.handleClient();
  
  // Handle web UI requests
  handleWebUI();
  
  // Periodic status update (every 30 seconds)
  if (currentTime - lastStatusUpdate > 30000) {
    Debug.printf(2, "Status: Calibrator=%s, Brightness=%d, WiFi=%s, Heap=%d\n",
                 getCalibratorStateString().c_str(),
                 getCurrentBrightness(),
                 WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected",
                 ESP.getFreeHeap());
    lastStatusUpdate = currentTime;
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}

// Initialize WiFi connection
void initWiFi() {
  Debug.println("Initializing WiFi...");
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  
  // Try to connect to saved network
  if (strlen(ssid) > 0) {
    Debug.printf("Connecting to WiFi network: %s\n", ssid);
    WiFi.begin(ssid, password);
    
    // Wait up to 30 seconds for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 60) {
      delay(500);
      Debug.print(".");
      attempts++;
    }
    Debug.println("");
    
    if (WiFi.status() == WL_CONNECTED) {
      Debug.println("WiFi connected successfully!");
      Debug.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
      Debug.printf("Signal strength: %d dBm\n", WiFi.RSSI());
      apMode = false;
    } else {
      Debug.println("Failed to connect to WiFi, starting AP mode");
      startAPMode();
    }
  } else {
    Debug.println("No WiFi credentials saved, starting AP mode");
    startAPMode();
  }
}

// Start Access Point mode for configuration
void startAPMode() {
  WiFi.mode(WIFI_AP);
  
  if (WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    Debug.printf("AP mode started - SSID: %s, Password: %s\n", AP_SSID, AP_PASSWORD);
    Debug.printf("AP IP address: %s\n", WiFi.softAPIP().toString().c_str());
    apMode = true;
    apStartTime = millis();
  } else {
    Debug.println("Failed to start AP mode!");
  }
}

// Handle WiFi connection in main loop
void handleWiFi() {
  if (apMode) {
    // Check if we should exit AP mode after timeout
    if (millis() - apStartTime > AP_TIMEOUT) {
      Debug.println("AP mode timeout, attempting to reconnect to WiFi");
      initWiFi();
    }
  } else {
    // Check if WiFi connection is lost
    if (WiFi.status() != WL_CONNECTED) {
      Debug.println("WiFi connection lost, attempting to reconnect...");
      WiFi.reconnect();
      
      // If reconnection fails after 30 seconds, start AP mode
      unsigned long reconnectStart = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - reconnectStart < 30000) {
        delay(500);
        Debug.print(".");
      }
      
      if (WiFi.status() != WL_CONNECTED) {
        Debug.println("\nFailed to reconnect, starting AP mode");
        startAPMode();
      } else {
        Debug.println("\nWiFi reconnected successfully!");
      }
    }
  }
}
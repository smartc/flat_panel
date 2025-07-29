/*
 * ESP32 ASCOM Alpaca Flat Panel Calibrator
 * Debug Utility Header - FIXED VERSION
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>

// Debug level control
// 0 = No debug output
// 1 = Basic debug output
// 2 = Verbose debug output
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 1  // Default to basic debug level
#endif

class DebugClass {
private:
  int currentLevel = DEBUG_LEVEL;
  bool initialized = false;

public:
  // Initialize debug output
  void begin(unsigned long baud) {
    #if DEBUG_LEVEL > 0
      Serial.begin(baud);
      while (!Serial && millis() < 5000) {
        // Wait for serial connection for up to 5 seconds
        delay(100);
      }
      initialized = true;
      Serial.println();
      Serial.println(F("Debug output initialized"));
    #endif
  }
  
  // Set debug level dynamically
  void setLevel(int level) {
    currentLevel = level;
  }
  
  int getLevel() {
    return currentLevel;
  }
  
  // Print empty line - ADDED TO FIX COMPILATION ERROR
  void println() {
    #if DEBUG_LEVEL > 0
      if (initialized) {
        Serial.println();
      }
    #endif
  }
  
  // Print methods for different debug levels
  template <typename T>
  void print(T message, int level = 1) {
    #if DEBUG_LEVEL > 0
      if (initialized && level <= currentLevel) {
        Serial.print(message);
      }
    #endif
  }
  
  template <typename T>
  void println(T message, int level = 1) {
    #if DEBUG_LEVEL > 0
      if (initialized && level <= currentLevel) {
        Serial.println(message);
      }
    #endif
  }
  
  // ADDED: Support for Debug.println(level, message) syntax
  template <typename T>
  void println(int level, T message) {
    #if DEBUG_LEVEL > 0
      if (initialized && level <= currentLevel) {
        Serial.println(message);
      }
    #endif
  }
  
  // Print methods with prefix for organization
  template <typename T>
  void print(const char* prefix, T message, int level = 1) {
    #if DEBUG_LEVEL > 0
      if (initialized && level <= currentLevel) {
        Serial.print(prefix);
        Serial.print(": ");
        Serial.print(message);
      }
    #endif
  }
  
  template <typename T>
  void println(const char* prefix, T message, int level = 1) {
    #if DEBUG_LEVEL > 0
      if (initialized && level <= currentLevel) {
        Serial.print(prefix);
        Serial.print(": ");
        Serial.println(message);
      }
    #endif
  }
  
  // For formatting like printf
  void printf(const char* format, ...) {
    #if DEBUG_LEVEL > 0
      if (initialized && currentLevel >= 1) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        Serial.print(buffer);
      }
    #endif
  }
  
  void printf(int level, const char* format, ...) {
    #if DEBUG_LEVEL > 0
      if (initialized && level <= currentLevel) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        Serial.print(buffer);
      }
    #endif
  }
};

// Create a global instance
extern DebugClass Debug;

#endif // DEBUG_H
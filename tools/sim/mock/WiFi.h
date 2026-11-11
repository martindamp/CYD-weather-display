#pragma once
#include "Arduino.h"
#define WL_CONNECTED 3
#define WIFI_STA 1
struct IPAddress { String toString() const { return "192.168.1.42"; } };
struct WiFiClass { int st = WL_CONNECTED; int status() { return st; } IPAddress localIP() { return {}; }
  void mode(int) {} void setAutoReconnect(bool) {} void begin(const char*, const char*) {} };
extern WiFiClass WiFi;

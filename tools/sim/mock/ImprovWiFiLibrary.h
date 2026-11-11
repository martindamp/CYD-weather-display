#pragma once
#include "Arduino.h"
namespace ImprovTypes { enum class ChipFamily { CF_ESP32 }; }
struct ImprovWiFi { ImprovWiFi(Stream*) {} void handleSerial() {} void setDeviceInfo(ImprovTypes::ChipFamily, const char*, const char*, const char*, const char*) {}
  void onImprovConnected(void(*)(const char*, const char*)) {} };

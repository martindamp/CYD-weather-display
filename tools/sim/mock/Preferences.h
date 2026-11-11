#pragma once
#include "Arduino.h"
struct Preferences { bool begin(const char*, bool) { return true; } void end() {}
  String getString(const char*, const char* d) { return d; } int getInt(const char*, int d) { return d; }
  void putString(const char*, const String&) {} void putInt(const char*, int) {} };

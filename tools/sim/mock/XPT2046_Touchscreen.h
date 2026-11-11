#pragma once
#include "Arduino.h"
struct SPIClass { SPIClass(int) {} void begin(int,int,int,int) {} };
#define VSPI 3
struct TS_Point { int16_t x, y, z; };
struct XPT2046_Touchscreen { XPT2046_Touchscreen(int, int) {} void begin(SPIClass&) {} void setRotation(int) {}
  bool tirqTouched() { return false; } bool touched() { return false; } TS_Point getPoint() { return {0,0,0}; } };

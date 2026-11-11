#pragma once
#include "WiFi.h"
struct WiFiClientSecure : public Stream { void setInsecure() {} void setCACert(const char*) {} };

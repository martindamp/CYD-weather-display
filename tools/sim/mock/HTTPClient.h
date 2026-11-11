#pragma once
#include "WiFiClientSecure.h"
#define HTTP_CODE_OK 200
struct HTTPClient { void useHTTP10(bool) {} void setConnectTimeout(int) {} void setTimeout(int) {}
  bool begin(WiFiClientSecure&, const String&) { return false; } int GET() { return -1; }
  Stream& getStream() { static WiFiClientSecure s; return s; } String getString() { return ""; } void end() {} };

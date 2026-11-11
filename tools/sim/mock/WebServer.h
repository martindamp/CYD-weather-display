#pragma once
#include "Arduino.h"
#include <functional>
enum { HTTP_GET, HTTP_POST };
struct WebServer { WebServer(int) {} void on(const char*, int, std::function<void()>) {} void onNotFound(std::function<void()>) {}
  void begin() {} void handleClient() {} void send(int, const char*, const String&) {} void send_P(int, const char*, const char*) {}
  String arg(const char*) { return ""; } bool hasArg(const char*) { return false; } };

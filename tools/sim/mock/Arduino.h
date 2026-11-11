#pragma once
#define ARDUINO 10819
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <string>
#include <algorithm>
#define PROGMEM
#define F(x) x
typedef const char* PGM_P;
inline uint32_t millis() { static uint32_t t = 100000; return t += 1; }
inline void delay(uint32_t) {}
template<class T, class L, class H> T constrain(T x, L lo, H hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline long map(long x, long a, long b, long c, long d) { return (x - a) * (d - c) / (b - a) + c; }
class Print { public:
  virtual size_t write(uint8_t c) = 0;
  virtual size_t write(const uint8_t* b, size_t n) { size_t k=0; while(n--) k+=write(*b++); return k; }
  virtual ~Print() {}
};
class Stream : public Print { public:
  virtual int available() { return 0; } virtual int read() { return -1; }
  virtual size_t readBytes(char*, size_t) { return 0; }
  size_t write(uint8_t) override { return 1; }
  void setTimeout(unsigned long) {}
};
class __FlashStringHelper;
class String {
public:
  std::string s;
  String() {}
  String(const char* c) : s(c ? c : "") {}
  String(const std::string& x) : s(x) {}
  String(char c) : s(1, c) {}
  String(int v) : s(std::to_string(v)) {}
  String(unsigned v) : s(std::to_string(v)) {}
  String(long v) : s(std::to_string(v)) {}
  String(unsigned long v) : s(std::to_string(v)) {}
  String(float v, int d = 2) { char b[32]; snprintf(b, 32, "%.*f", d, v); s = b; }
  String(double v, int d = 2) { char b[32]; snprintf(b, 32, "%.*f", d, v); s = b; }
  const char* c_str() const { return s.c_str(); }
  unsigned int length() const { return s.size(); }
  bool isEmpty() const { return s.empty(); }
  char operator[](unsigned i) const { return i < s.size() ? s[i] : 0; }
  String& operator+=(const String& o) { s += o.s; return *this; }
  String& operator+=(const char* o) { s += o; return *this; }
  String& operator+=(char c) { s += c; return *this; }
  bool concat(const char* o) { s += o; return true; }
  bool concat(char c) { s += c; return true; }
  bool concat(const char* o, unsigned n) { s.append(o, n); return true; }
  void remove(unsigned i) { if (i < s.size()) s.erase(i); }
  void remove(unsigned i, unsigned n) { if (i < s.size()) s.erase(i, n); }
  void trim() { size_t a = s.find_first_not_of(" \t\r\n"); if (a == std::string::npos) { s.clear(); return; } size_t b = s.find_last_not_of(" \t\r\n"); s = s.substr(a, b - a + 1); }
  String substring(unsigned a, unsigned b = 0xFFFFFFFF) const { if (a > s.size()) return String(); return String(s.substr(a, b == 0xFFFFFFFF ? std::string::npos : b - a)); }
  bool endsWith(const String& x) const { return s.size() >= x.s.size() && s.compare(s.size() - x.s.size(), x.s.size(), x.s) == 0; }
  int indexOf(char c) const { auto p = s.find(c); return p == std::string::npos ? -1 : (int)p; }
  long toInt() const { return atol(s.c_str()); }
  float toFloat() const { return atof(s.c_str()); }
  bool operator==(const String& o) const { return s == o.s; }
  bool operator!=(const String& o) const { return s != o.s; }
  operator bool() const { return true; }
};
inline String operator+(const String& a, const String& b) { String r(a); r += b; return r; }
inline String operator+(const String& a, const char* b) { String r(a); r += b; return r; }
inline String operator+(const char* a, const String& b) { String r(a); r += b; return r; }
inline String operator+(const String& a, char b) { String r(a); r += b; return r; }
struct HardwareSerial : public Stream { void begin(int) {} void printf(const char*, ...) {} void println(const char* = "") {} };
extern HardwareSerial Serial;
#define ARDUINOJSON_ENABLE_PROGMEM 0
#define pgm_read_byte(p) (*(const uint8_t*)(p))
class Printable { public: virtual size_t printTo(Print&) const = 0; virtual ~Printable() {} };
inline void configTime(long, int, const char*, const char*) {}

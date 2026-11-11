/*
  CYD (ESP32-2432S028R) - Weather display   [PlatformIO]
  ----------------------------------------------------------------
  Shows the current weather and a 5-day forecast for one or more locations.
  With several locations the pages rotate automatically.

  - Weather + place search: Open-Meteo (https://open-meteo.com), no API key.
  - Wi-Fi: set via Improv Wi-Fi (Serial) from the web installer.
  - Locations: added on the device (on-screen keyboard with live suggestions)
    or from the device's own web page (http://<device-ip>/). Stored in NVS.
  - OTA: the menu can check GitHub Pages for a newer firmware and install it.

  Screen is used in landscape (320 x 240).
  TFT_eSPI configuration lives in platformio.ini (build_flags).
*/

#include <Arduino.h>
#include <time.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <Update.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <U8g2_for_TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <ImprovWiFiLibrary.h>
#include "ota_certs.h"
#include "weather_icons.h"
#include "i18n.h"
#include "web_page.h"

// The semantic version is injected by GitHub Actions from the VERSION file.
#if __has_include("build_version.h")
  #include "build_version.h"
#endif
#ifndef FW_VERSION_SEM
  #define FW_VERSION_SEM "0.0.0-dev"
#endif

#define FW_NAME        "CYD Weather"

// GitHub Pages - source of OTA updates (and the web installer)
#define PAGES_BASE     "https://martindamp.github.io/CYD-weather-display/"
#define OTA_BIN_URL    PAGES_BASE "firmware/firmware.bin"
#define OTA_VER_URL    PAGES_BASE "version.txt"
#define INSTALLER_URL  "martindamp.github.io/CYD-weather-display"

#define MAX_LOCS       8
#define REFRESH_MS     (15UL * 60UL * 1000UL)   // weather refresh per location
#define RETRY_MS       30000UL                  // retry after a failed fetch
#define DEF_ROTATE_S   10
#define FORECAST_DAYS  5
#define MAX_SUGG       3

// ---- CYD resistive touch (XPT2046) on its own SPI bus ----
#define XPT2046_IRQ   36
#define XPT2046_MOSI  32
#define XPT2046_MISO  39
#define XPT2046_CLK   25
#define XPT2046_CS    33
// Raw ADC range -> screen pixels (landscape). Adjust if taps land off target.
#define TOUCH_MINX    200
#define TOUCH_MAXX    3700
#define TOUCH_MINY    240
#define TOUCH_MAXY    3800
#define TOUCH_FLIP_X  0    // set to 1 if left/right taps are mirrored
#define TOUCH_FLIP_Y  0    // set to 1 if up/down taps are mirrored

#define SCR_W 320
#define SCR_H 240

// ---- fonts (u8g2 "_tf" fonts include Latin-1: æøå, degree sign) ----
#define F_NAME    u8g2_font_helvB14_tf
#define F_TEMP    u8g2_font_fub35_tf
#define F_DESC    u8g2_font_helvB12_tf
#define F_DESC_S  u8g2_font_helvB10_tf
#define F_DETAIL  u8g2_font_helvR10_tf
#define F_DAY     u8g2_font_helvB10_tf
#define F_TMAX    u8g2_font_helvB12_tf
#define F_TMIN    u8g2_font_helvR10_tf
#define F_SMALL   u8g2_font_helvR08_tf
#define F_TITLE   u8g2_font_helvB18_tf
#define F_BODY    u8g2_font_helvR12_tf
#define F_KEY     u8g2_font_helvB12_tf
#define F_KEY_S   u8g2_font_helvB10_tf

#define DEG "\xC2\xB0"     // UTF-8 degree sign

// ---- colours ----
static const uint16_t C_BG     = wicons::rgb( 14,  18,  32);
static const uint16_t C_PANEL  = wicons::rgb( 30,  36,  58);
static const uint16_t C_KEY2   = wicons::rgb( 48,  56,  86);
static const uint16_t C_LINE   = wicons::rgb( 42,  48,  72);
static const uint16_t C_TEXT   = wicons::rgb(236, 239, 245);
static const uint16_t C_DIM    = wicons::rgb(150, 158, 178);
static const uint16_t C_ACC    = wicons::rgb(255, 203,  45);
static const uint16_t C_ACC_FG = wicons::rgb( 26,  21,   0);
static const uint16_t C_BLUE   = wicons::rgb( 96, 172, 255);
static const uint16_t C_RED    = wicons::rgb(255, 107, 107);

// ============================ objects ============================
TFT_eSPI            tft;
U8g2_for_TFT_eSPI   u8f;
SPIClass            touchSPI(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
Preferences         prefs;
WebServer           server(80);
ImprovWiFi          improvSerial(&Serial);

// ============================ data ============================
struct Location {
  String name, region, cc;
  float  lat = 0, lon = 0;
};

struct DayFc {
  int   code = 0;
  float tmax = 0, tmin = 0;
  int   pop = -1;          // precipitation probability %, -1 = n/a
  int   dow = 0;           // 0 = Sunday
};

struct Weather {
  bool     valid = false;
  bool     failed = false;
  uint32_t fetchedAt = 0;
  uint32_t lastTry = 0;
  float    temp = 0, feels = 0, wind = 0;
  int      hum = 0, code = 0;
  bool     isDay = true;
  char     time[6] = "";   // local "HH:MM" of the observation
  DayFc    days[FORECAST_DAYS];
};

Location locs[MAX_LOCS];
Weather  wx[MAX_LOCS];
int      nLocs = 0;
int      cur = 0;              // shown location
int      rotateSec = DEF_ROTATE_S;
String   wifiSsid, wifiPass;

enum Screen { SCR_BOOT, SCR_NOWIFI, SCR_EMPTY, SCR_WEATHER, SCR_MENU, SCR_ADD, SCR_MSG };
Screen   screen = SCR_BOOT;
uint32_t lastSwitch = 0;
bool     uiDirty = false;      // locations/settings changed from the web page
bool     otaRequested = false; // OTA requested from the web page
bool     serverStarted = false;
uint32_t wifiStartMs = 0;

// menu state
enum UpdState { UPD_IDLE, UPD_CHECKING, UPD_UPTODATE, UPD_AVAILABLE, UPD_FAIL };
UpdState updState = UPD_IDLE;
String   latestVer;
bool     removeArmed = false;

// add-location state
String   kbText;
bool     kbNumMode = false;
uint32_t searchDue = 0;        // millis() when a debounced search should run
enum SuggState { SG_HINT, SG_SEARCHING, SG_RESULTS, SG_NONE, SG_ERROR, SG_FULL };
SuggState sgState = SG_HINT;
Location sugg[MAX_SUGG];
int      nSugg = 0;

// ============================ forward declarations ============================
void showMain();
void drawWeatherPage();
void drawEmptyPage();
void drawMenu();
void drawAddScreen();
void drawInputField();
void drawSuggestions();
void drawKeyboard();
void runOta();
String fetchLatestVersion();
long   semverNum(const String& v);
void   startServer();

// ============================ text helpers ============================
// Draw UTF-8 text. align: 0 = left, 1 = centre, 2 = right (x is that edge).
// y is the baseline.
int text(int x, int y, const String& s, const uint8_t* font, uint16_t fg, int align = 0, uint16_t bg = C_BG) {
  u8f.setFont(font);
  u8f.setFontMode(1);
  u8f.setForegroundColor(fg);
  u8f.setBackgroundColor(bg);
  int w = u8f.getUTF8Width(s.c_str());
  if (align == 1) x -= w / 2;
  else if (align == 2) x -= w;
  u8f.drawUTF8(x, y, s.c_str());
  return w;
}

int textWidth(const String& s, const uint8_t* font) {
  u8f.setFont(font);
  return u8f.getUTF8Width(s.c_str());
}

// Remove the last UTF-8 character of s.
void utf8PopBack(String& s) {
  int i = (int)s.length() - 1;
  while (i > 0 && (((uint8_t)s[i]) & 0xC0) == 0x80) i--;
  if (i >= 0) s.remove(i);
}

// Remove the first UTF-8 character of s.
void utf8PopFront(String& s) {
  if (!s.length()) return;
  int i = 1;
  while (i < (int)s.length() && (((uint8_t)s[i]) & 0xC0) == 0x80) i++;
  s.remove(0, i);
}

// Shorten s with "..." until it fits maxW pixels in the given font.
String fitText(String s, int maxW, const uint8_t* font) {
  if (textWidth(s, font) <= maxW) return s;
  while (s.length() && textWidth(s + "...", font) > maxW) utf8PopBack(s);
  s.trim();
  return s + "...";
}

String urlEncode(const String& s) {
  String out;
  const char* hex = "0123456789ABCDEF";
  for (size_t i = 0; i < s.length(); i++) {
    uint8_t c = (uint8_t)s[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += (char)c;
    else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
  }
  return out;
}

String tempStr(float t) {
  long v = lroundf(t);
  if (v == 0) v = 0;                    // avoid "-0"
  return String(v) + DEG;
}

// Day of week for a Gregorian date (0 = Sunday). Sakamoto's method.
int dayOfWeek(int y, int m, int d) {
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

// ============================ storage ============================
void saveLocations() {
  JsonDocument doc;
  JsonArray a = doc.to<JsonArray>();
  for (int i = 0; i < nLocs; i++) {
    JsonObject o = a.add<JsonObject>();
    o["n"]  = locs[i].name;
    o["r"]  = locs[i].region;
    o["c"]  = locs[i].cc;
    o["la"] = locs[i].lat;
    o["lo"] = locs[i].lon;
  }
  String s;
  serializeJson(doc, s);
  prefs.begin("wx", false);
  prefs.putString("locs", s);
  prefs.end();
}

void loadSettings() {
  prefs.begin("wifi", true);
  wifiSsid = prefs.getString("ssid", "");
  wifiPass = prefs.getString("pass", "");
  prefs.end();

  prefs.begin("wx", true);
  String s  = prefs.getString("locs", "[]");
  rotateSec = prefs.getInt("rotate", DEF_ROTATE_S);
  g_lang    = langFromCode(prefs.getString("lang", "en"));
  prefs.end();
  if (rotateSec < 3 || rotateSec > 600) rotateSec = DEF_ROTATE_S;

  JsonDocument doc;
  nLocs = 0;
  if (deserializeJson(doc, s) == DeserializationError::Ok) {
    for (JsonObject o : doc.as<JsonArray>()) {
      if (nLocs >= MAX_LOCS) break;
      Location& L = locs[nLocs++];
      L.name   = o["n"]  | "";
      L.region = o["r"]  | "";
      L.cc     = o["c"]  | "";
      L.lat    = o["la"] | 0.0f;
      L.lon    = o["lo"] | 0.0f;
    }
  }
}

void saveWiFi(const char* ssid, const char* pass) {
  wifiSsid = ssid;
  wifiPass = pass;
  prefs.begin("wifi", false);
  prefs.putString("ssid", wifiSsid);
  prefs.putString("pass", wifiPass);
  prefs.end();
}

void saveLang(Lang l) {
  g_lang = l;
  prefs.begin("wx", false);
  prefs.putString("lang", LANG_CODES[l]);
  prefs.end();
}

void saveRotate(int s) {
  rotateSec = constrain(s, 3, 600);
  prefs.begin("wx", false);
  prefs.putInt("rotate", rotateSec);
  prefs.end();
}

// Returns "" on success, otherwise an error message.
String addLocation(const Location& L) {
  if (nLocs >= MAX_LOCS) return "full";
  for (int i = 0; i < nLocs; i++) {
    if (fabsf(locs[i].lat - L.lat) < 0.01f && fabsf(locs[i].lon - L.lon) < 0.01f) {
      cur = i;                               // already there: just show it
      return "";
    }
  }
  locs[nLocs] = L;
  wx[nLocs]   = Weather();
  cur = nLocs;
  nLocs++;
  saveLocations();
  return "";
}

void removeLocation(int i) {
  if (i < 0 || i >= nLocs) return;
  for (int k = i; k < nLocs - 1; k++) { locs[k] = locs[k + 1]; wx[k] = wx[k + 1]; }
  nLocs--;
  locs[nLocs] = Location();
  wx[nLocs]   = Weather();
  if (cur >= nLocs) cur = nLocs ? nLocs - 1 : 0;
  saveLocations();
}

void moveLocation(int i, int d) {
  int j = i + d;
  if (i < 0 || i >= nLocs || j < 0 || j >= nLocs) return;
  Location tl = locs[i]; locs[i] = locs[j]; locs[j] = tl;
  Weather  tw = wx[i];   wx[i]   = wx[j];   wx[j]   = tw;
  if (cur == i) cur = j; else if (cur == j) cur = i;
  saveLocations();
}

// ============================ HTTP / Open-Meteo ============================
// GET a JSON document over HTTPS. Weather data isn't sensitive, so the
// certificate isn't validated here (OTA downloads ARE validated).
bool httpGetJson(const String& url, JsonDocument& doc, JsonDocument& filter) {
  if (WiFi.status() != WL_CONNECTED) return false;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.useHTTP10(true);                // no chunked encoding -> stream straight into the parser
  http.setConnectTimeout(8000);
  http.setTimeout(10000);
  if (!http.begin(client, url)) return false;
  int code = http.GET();
  bool ok = false;
  if (code == HTTP_CODE_OK) {
    DeserializationError e = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
    ok = !e;
    if (e) Serial.printf("JSON error: %s\n", e.c_str());
  } else {
    Serial.printf("HTTP %d for %s\n", code, url.c_str());
  }
  http.end();
  return ok;
}

bool fetchWeather(int i) {
  if (i < 0 || i >= nLocs) return false;
  char url[420];
  snprintf(url, sizeof(url),
    "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
    "&current=temperature_2m,apparent_temperature,relative_humidity_2m,weather_code,is_day,wind_speed_10m"
    "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max"
    "&wind_speed_unit=ms&timezone=auto&forecast_days=%d",
    locs[i].lat, locs[i].lon, FORECAST_DAYS);

  JsonDocument filter;
  JsonObject fc = filter["current"].to<JsonObject>();
  fc["time"] = true; fc["temperature_2m"] = true; fc["apparent_temperature"] = true;
  fc["relative_humidity_2m"] = true; fc["weather_code"] = true; fc["is_day"] = true;
  fc["wind_speed_10m"] = true;
  JsonObject fd = filter["daily"].to<JsonObject>();
  fd["time"] = true; fd["weather_code"] = true; fd["temperature_2m_max"] = true;
  fd["temperature_2m_min"] = true; fd["precipitation_probability_max"] = true;

  JsonDocument doc;
  if (!httpGetJson(url, doc, filter)) return false;
  JsonObject c = doc["current"];
  JsonObject d = doc["daily"];
  if (c.isNull() || d.isNull()) return false;

  Weather& W = wx[i];
  W.temp  = c["temperature_2m"]       | 0.0f;
  W.feels = c["apparent_temperature"] | W.temp;
  W.hum   = c["relative_humidity_2m"] | 0;
  W.code  = c["weather_code"]         | 0;
  W.isDay = (c["is_day"] | 1) != 0;
  W.wind  = c["wind_speed_10m"]       | 0.0f;
  const char* t = c["time"] | "";      // "2026-11-11T16:15"
  if (strlen(t) >= 16) { memcpy(W.time, t + 11, 5); W.time[5] = 0; }
  else W.time[0] = 0;

  JsonArray dt = d["time"], dc = d["weather_code"], dx = d["temperature_2m_max"],
            dn = d["temperature_2m_min"], dp = d["precipitation_probability_max"];
  for (int k = 0; k < FORECAST_DAYS; k++) {
    DayFc& D = W.days[k];
    D.code = dc[k] | 0;
    D.tmax = dx[k] | 0.0f;
    D.tmin = dn[k] | 0.0f;
    D.pop  = dp[k].isNull() ? -1 : (int)(dp[k] | 0);
    int y = 2000, m = 1, dd = 1;
    sscanf(dt[k] | "2000-01-01", "%d-%d-%d", &y, &m, &dd);
    D.dow = dayOfWeek(y, m, dd);
  }
  W.valid = true;
  return true;
}

// Search Open-Meteo's geocoding API for kbText; fills sugg[].
void runSearch() {
  String q = kbText;
  q.trim();
  nSugg = 0;
  if (q.length() < 2) { sgState = SG_HINT; drawSuggestions(); return; }
  sgState = SG_SEARCHING;
  drawSuggestions();

  String url = "https://geocoding-api.open-meteo.com/v1/search?count=" + String(MAX_SUGG) +
               "&language=" + String(langCode()) + "&format=json&name=" + urlEncode(q);
  JsonDocument filter;
  JsonObject f = filter["results"][0].to<JsonObject>();
  f["name"] = true; f["latitude"] = true; f["longitude"] = true;
  f["country_code"] = true; f["admin1"] = true;

  JsonDocument doc;
  if (!httpGetJson(url, doc, filter)) { sgState = SG_ERROR; drawSuggestions(); return; }
  for (JsonObject r : doc["results"].as<JsonArray>()) {
    if (nSugg >= MAX_SUGG) break;
    Location& L = sugg[nSugg++];
    L.name   = r["name"]         | "";
    L.region = r["admin1"]       | "";
    L.cc     = r["country_code"] | "";
    L.lat    = r["latitude"]     | 0.0f;
    L.lon    = r["longitude"]    | 0.0f;
  }
  sgState = nSugg ? SG_RESULTS : SG_NONE;
  drawSuggestions();
}

// ============================ icons ============================
void drawIconAt(int x, int y, int s, int code, bool day) {
  TFT_eSprite spr(&tft);
  spr.setColorDepth(16);
  if (!spr.createSprite(s, s)) return;   // out of RAM: skip the icon
  spr.fillSprite(C_BG);
  wicons::draw(spr, s, code, day, C_BG);
  spr.pushSprite(x, y);
  spr.deleteSprite();
}

void drawDrop(int x, int y, uint16_t col) {   // tiny rain drop, tip at (x, y-4)
  tft.fillTriangle(x - 3, y + 1, x + 3, y + 1, x, y - 5, col);
  tft.fillSmoothCircle(x, y + 2, 3, col, C_BG);
}

void drawMenuGlyph() {             // hamburger in the top-right corner
  for (int i = 0; i < 3; i++) tft.fillRoundRect(296, 9 + i * 6, 16, 3, 1, C_DIM);
}

// ============================ screens ============================
String ipString() {
  return WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String(tr(S_OFFLINE));
}

void drawMessage(const String& title, const String& l1, const String& l2 = "", const String& l3 = "") {
  screen = SCR_MSG;
  tft.fillScreen(C_BG);
  text(SCR_W / 2, 92, title, F_TITLE, C_TEXT, 1);
  if (l1.length()) text(SCR_W / 2, 128, l1, F_DETAIL, C_DIM, 1);
  if (l2.length()) text(SCR_W / 2, 148, l2, F_DETAIL, C_DIM, 1);
  if (l3.length()) text(SCR_W / 2, 176, l3, F_DAY, C_ACC, 1);
}

void drawNoWifi() {
  screen = SCR_NOWIFI;
  tft.fillScreen(C_BG);
  text(SCR_W / 2, 52, tr(S_CONNECT_WIFI), F_TITLE, C_TEXT, 1);
  if (wifiSsid.length()) {
    text(SCR_W / 2, 80, fitText(trf(S_CANT_REACH, wifiSsid), 300, F_DETAIL), F_DETAIL, C_RED, 1);
  }
  text(SCR_W / 2, 112, tr(S_WIFI_HELP1), F_DETAIL, C_DIM, 1);
  text(SCR_W / 2, 130, tr(S_WIFI_HELP2), F_DETAIL, C_DIM, 1);
  text(SCR_W / 2, 148, tr(S_WIFI_HELP3), F_DETAIL, C_DIM, 1);
  text(SCR_W / 2, 178, INSTALLER_URL, F_DAY, C_ACC, 1);
}

void drawEmptyPage() {
  screen = SCR_EMPTY;
  tft.fillScreen(C_BG);
  drawMenuGlyph();
  text(SCR_W / 2, 62, "CYD Weather", F_TITLE, C_TEXT, 1);
  text(SCR_W / 2, 88, tr(S_NO_LOCATIONS), F_BODY, C_DIM, 1);
  tft.fillSmoothRoundRect(60, 108, 200, 54, 12, C_ACC, C_BG);
  text(SCR_W / 2, 142, fitText(tr(S_ADD_LOCATION_BTN), 188, F_NAME), F_NAME, C_ACC_FG, 1, C_ACC);
  if (WiFi.status() == WL_CONNECTED) {
    text(SCR_W / 2, 198, tr(S_ALSO_BROWSER), F_SMALL, C_DIM, 1);
    text(SCR_W / 2, 216, "http://" + ipString() + "/", F_DAY, C_TEXT, 1);
  }
}

void drawHeader() {
  const Location& L = locs[cur];
  const Weather&  W = wx[cur];
  String info;
  if (WiFi.status() != WL_CONNECTED) info = tr(S_OFFLINE);
  else if (W.valid && W.time[0]) info = String(W.time);
  if (nLocs > 1) info += (info.length() ? "   " : "") + String(cur + 1) + "/" + String(nLocs);
  int iw = info.length() ? textWidth(info, F_SMALL) : 0;
  text(10, 22, fitText(L.name, 280 - iw - 16, F_NAME), F_NAME, C_TEXT);
  if (info.length()) text(288, 20, info, F_SMALL, C_DIM, 2);
  drawMenuGlyph();
}

void drawWeatherPage() {
  screen = SCR_WEATHER;
  if (cur >= nLocs) cur = 0;
  tft.fillScreen(C_BG);
  drawHeader();
  const Weather& W = wx[cur];
  if (!W.valid) {
    StrId m = (WiFi.status() != WL_CONNECTED) ? S_WAIT_WIFI : (W.failed ? S_WX_FAILED : S_WX_LOADING);
    text(SCR_W / 2, 130, fitText(tr(m), SCR_W - 20, F_BODY), F_BODY, C_DIM, 1);
    return;
  }

  // ---- current weather ----
  drawIconAt(6, 34, 92, W.code, W.isDay);
  text(106, 80, tempStr(W.temp), F_TEMP, C_TEXT);
  String desc = trWx(W.code);
  const uint8_t* df = textWidth(desc, F_DESC) > 206 ? F_DESC_S : F_DESC;
  text(108, 104, fitText(desc, 206, df), df, C_TEXT);
  // "Feels x   Wind y m/s   hum%": tighten the gaps before dropping humidity.
  String feels = String(tr(S_FEELS)) + " " + tempStr(W.feels);
  String wind  = String(tr(S_WIND)) + " " + String(lroundf(W.wind)) + " m/s";
  String humS  = String(W.hum) + "%";
  String det; bool showHum = false;
  for (const char* gap : {"    ", "   "}) {          // >= 3 spaces leaves room for the drop
    det = feels + gap + wind + gap + humS;
    if (textWidth(det, F_DETAIL) <= 206) { showHum = true; break; }
  }
  if (!showHum) {
    det = feels + "    " + wind;
    if (textWidth(det, F_DETAIL) > 206) det = feels + "  " + wind;
  }
  text(108, 124, fitText(det, 206, F_DETAIL), F_DETAIL, C_DIM);
  if (showHum) {                               // little drop in front of the humidity
    int hx = 108 + textWidth(det, F_DETAIL) - textWidth(humS, F_DETAIL) - 6;
    drawDrop(hx, 119, C_BLUE);
  }

  tft.drawFastHLine(8, 136, SCR_W - 16, C_LINE);

  // ---- 5-day forecast ----
  const int colW = SCR_W / FORECAST_DAYS;
  for (int k = 0; k < FORECAST_DAYS; k++) {
    const DayFc& D = W.days[k];
    int x0 = k * colW, cx = x0 + colW / 2;
    if (k) tft.drawFastVLine(x0, 146, 86, C_LINE);
    text(cx, 155, k == 0 ? String(tr(S_TODAY)) : String(trDow(D.dow)), F_DAY, k == 0 ? C_ACC : C_TEXT, 1);
    drawIconAt(cx - 21, 160, 42, D.code, true);
    String hi = tempStr(D.tmax), lo = tempStr(D.tmin);
    int wh = textWidth(hi, F_TMAX), wl = textWidth(lo, F_TMIN);
    int sx = cx - (wh + 5 + wl) / 2;
    text(sx, 218, hi, F_TMAX, C_TEXT);
    text(sx + wh + 5, 218, lo, F_TMIN, C_DIM);
    if (D.pop >= 0) {
      String p = String(D.pop) + "%";
      int wp = textWidth(p, F_SMALL);
      int px = cx - (wp + 9) / 2;
      drawDrop(px + 3, 230, C_BLUE);
      text(px + 9, 234, p, F_SMALL, C_BLUE);
    }
  }
}

void showMain() {
  removeArmed = false;
  if (nLocs == 0) drawEmptyPage();
  else            drawWeatherPage();
  lastSwitch = millis();
}

// ---------------------------------------------------------------- menu
enum MenuItem { MI_ADD, MI_REMOVE, MI_UPDATE, MI_LANG, MI_CLOSE };
MenuItem menuItems[5];
int      nMenu = 0;
#define MENU_TOP  36
#define MENU_ROWH 39

void drawMenu() {
  screen = SCR_MENU;
  tft.fillScreen(C_BG);
  text(12, 25, tr(S_MENU), F_NAME, C_TEXT);
  nMenu = 0;
  menuItems[nMenu++] = MI_ADD;
  if (nLocs) menuItems[nMenu++] = MI_REMOVE;
  menuItems[nMenu++] = MI_UPDATE;
  menuItems[nMenu++] = MI_LANG;
  menuItems[nMenu++] = MI_CLOSE;
  for (int i = 0; i < nMenu; i++) {
    int y = MENU_TOP + i * MENU_ROWH;
    String label; uint16_t col = C_TEXT;
    switch (menuItems[i]) {
      case MI_ADD:
        label = tr(nLocs >= MAX_LOCS ? S_ADD_MAX : S_ADD_LOCATION_BTN);
        if (nLocs >= MAX_LOCS) col = C_DIM;
        break;
      case MI_REMOVE:
        label = trf(removeArmed ? S_REMOVE_CONFIRM : S_REMOVE, locs[cur].name);
        col = removeArmed ? C_RED : C_TEXT;
        break;
      case MI_UPDATE:
        switch (updState) {
          case UPD_IDLE:      label = tr(S_CHECK_UPD); break;
          case UPD_CHECKING:  label = tr(S_CHECKING); col = C_DIM; break;
          case UPD_UPTODATE:  label = trf(S_UP_TO_DATE, FW_VERSION_SEM); col = C_DIM; break;
          case UPD_AVAILABLE: label = trf(S_INSTALL_UPD, latestVer); col = C_ACC; break;
          case UPD_FAIL:      label = tr(S_GITHUB_FAIL); col = C_RED; break;
        }
        break;
      case MI_LANG:  label = trf(S_LANGUAGE, LANG_NAMES[g_lang]); break;
      case MI_CLOSE: label = tr(S_CLOSE); break;
    }
    tft.fillSmoothRoundRect(8, y, SCR_W - 16, MENU_ROWH - 5, 8, C_PANEL, C_BG);
    text(22, y + 23, fitText(label, SCR_W - 52, F_DESC), F_DESC, col, 0, C_PANEL);
  }
  String foot = String("v") + FW_VERSION_SEM + "   " + ipString() + "   " + tr(S_WEATHER_BY);
  text(SCR_W / 2, 236, foot, F_SMALL, C_DIM, 1);
}

void menuTap(int tx, int ty) {
  if (ty < MENU_TOP) { showMain(); return; }
  int i = (ty - MENU_TOP) / MENU_ROWH;
  if (i < 0 || i >= nMenu) { showMain(); return; }
  if (menuItems[i] != MI_REMOVE) removeArmed = false;
  switch (menuItems[i]) {
    case MI_ADD:
      if (nLocs >= MAX_LOCS) return;
      drawAddScreen();
      return;
    case MI_REMOVE:
      if (!removeArmed) { removeArmed = true; drawMenu(); return; }
      removeLocation(cur);
      showMain();
      return;
    case MI_UPDATE:
      if (updState == UPD_AVAILABLE) { runOta(); return; }
      updState = UPD_CHECKING; drawMenu();
      latestVer = fetchLatestVersion();
      if (!latestVer.length()) updState = UPD_FAIL;
      else updState = (semverNum(latestVer) > semverNum(FW_VERSION_SEM)) ? UPD_AVAILABLE : UPD_UPTODATE;
      drawMenu();
      return;
    case MI_LANG:                                 // cycle through the languages
      saveLang((Lang)((g_lang + 1) % L_COUNT));
      drawMenu();
      return;
    case MI_CLOSE:
      showMain();
      return;
  }
}

// ---------------------------------------------------------------- add location
// Layout: input bar (0-36), suggestions (38-106), keyboard (108-240).
#define SUG_TOP   38
#define SUG_ROWH  23
#define KB_Y      108
#define KB_ROWH   33
#define KB_UNITS  11

enum KeyAct : uint8_t { A_CHR, A_DEL, A_SPACE, A_MODE, A_GO };
struct KeyDef { const char* up; const char* lo; uint8_t w; KeyAct act; };

static const KeyDef KL0[] = {{"Q","q",1,A_CHR},{"W","w",1,A_CHR},{"E","e",1,A_CHR},{"R","r",1,A_CHR},
  {"T","t",1,A_CHR},{"Y","y",1,A_CHR},{"U","u",1,A_CHR},{"I","i",1,A_CHR},{"O","o",1,A_CHR},
  {"P","p",1,A_CHR},{"\xC3\x85","\xC3\xA5",1,A_CHR}};                                   // Å
static const KeyDef KL1[] = {{"A","a",1,A_CHR},{"S","s",1,A_CHR},{"D","d",1,A_CHR},{"F","f",1,A_CHR},
  {"G","g",1,A_CHR},{"H","h",1,A_CHR},{"J","j",1,A_CHR},{"K","k",1,A_CHR},{"L","l",1,A_CHR},
  {"\xC3\x86","\xC3\xA6",1,A_CHR},{"\xC3\x98","\xC3\xB8",1,A_CHR}};                     // Æ Ø
static const KeyDef KN0[] = {{"1","1",1,A_CHR},{"2","2",1,A_CHR},{"3","3",1,A_CHR},{"4","4",1,A_CHR},
  {"5","5",1,A_CHR},{"6","6",1,A_CHR},{"7","7",1,A_CHR},{"8","8",1,A_CHR},{"9","9",1,A_CHR},
  {"0","0",1,A_CHR},{".",".",1,A_CHR}};
static const KeyDef KN1[] = {{"\xC3\x84","\xC3\xA4",1,A_CHR},{"\xC3\x96","\xC3\xB6",1,A_CHR},   // Ä Ö
  {"\xC3\x9C","\xC3\xBC",1,A_CHR},{"\xC3\x89","\xC3\xA9",1,A_CHR},{"\xC3\x88","\xC3\xA8",1,A_CHR}, // Ü É È
  {"\xC3\x81","\xC3\xA1",1,A_CHR},{"\xC3\x91","\xC3\xB1",1,A_CHR},{"\xC3\x87","\xC3\xA7",1,A_CHR}, // Á Ñ Ç
  {"(","(",1,A_CHR},{")",")",1,A_CHR},{"/","/",1,A_CHR}};
static const KeyDef K2[]  = {{"Z","z",1,A_CHR},{"X","x",1,A_CHR},{"C","c",1,A_CHR},{"V","v",1,A_CHR},
  {"B","b",1,A_CHR},{"N","n",1,A_CHR},{"M","m",1,A_CHR},{"-","-",1,A_CHR},{"'","'",1,A_CHR},
  {"Del","",2,A_DEL}};
static const KeyDef K3L[] = {{"123","",2,A_MODE},{"space","",6,A_SPACE},{"Search","",3,A_GO}};
static const KeyDef K3N[] = {{"ABC","",2,A_MODE},{"space","",6,A_SPACE},{"Search","",3,A_GO}};

#define NKEYS(a) (int)(sizeof(a) / sizeof(a[0]))

const KeyDef* kbRow(int r, int& n) {
  switch (r) {
    case 0:  n = kbNumMode ? NKEYS(KN0) : NKEYS(KL0); return kbNumMode ? KN0 : KL0;
    case 1:  n = kbNumMode ? NKEYS(KN1) : NKEYS(KL1); return kbNumMode ? KN1 : KL1;
    case 2:  n = NKEYS(K2); return K2;
    default: n = kbNumMode ? NKEYS(K3N) : NKEYS(K3L); return kbNumMode ? K3N : K3L;
  }
}

void keyRect(int r, int j, int& x, int& y, int& w, int& h) {
  int n; const KeyDef* row = kbRow(r, n);
  int u0 = 0;
  for (int k = 0; k < j; k++) u0 += row[k].w;
  x = (u0 * SCR_W) / KB_UNITS;
  w = ((u0 + row[j].w) * SCR_W) / KB_UNITS - x;
  y = KB_Y + r * KB_ROWH;
  h = KB_ROWH;
}

void drawKey(int r, int j, bool pressed) {
  int n; const KeyDef* row = kbRow(r, n);
  const KeyDef& k = row[j];
  int x, y, w, h; keyRect(r, j, x, y, w, h);
  uint16_t fill = (k.act == A_CHR) ? C_PANEL : (k.act == A_GO ? C_ACC : C_KEY2);
  uint16_t fg   = (k.act == A_GO) ? C_ACC_FG : C_TEXT;
  if (pressed) { fill = C_TEXT; fg = C_BG; }
  tft.fillRect(x, y, w, h, C_BG);
  tft.fillSmoothRoundRect(x + 2, y + 2, w - 4, h - 4, 5, fill, C_BG);
  const uint8_t* f = (k.act == A_CHR) ? F_KEY : F_KEY_S;
  const char* label = k.act == A_SPACE ? tr(S_KEY_SPACE) : k.act == A_DEL ? tr(S_KEY_DEL)
                    : k.act == A_GO    ? tr(S_KEY_SEARCH) : k.up;
  text(x + w / 2, y + h / 2 + 5, fitText(label, w - 8, f), f, fg, 1, fill);
}

void drawKeyboard() {
  tft.fillRect(0, KB_Y, SCR_W, SCR_H - KB_Y, C_BG);
  for (int r = 0; r < 4; r++) {
    int n; kbRow(r, n);
    for (int j = 0; j < n; j++) drawKey(r, j, false);
  }
}

void drawInputField() {
  tft.fillRect(0, 0, SCR_W, 36, C_BG);
  tft.fillSmoothRoundRect(4, 4, 36, 28, 6, C_KEY2, C_BG);           // cancel
  text(22, 24, "\xC3\x97", F_NAME, C_TEXT, 1, C_KEY2);               // ×
  tft.fillSmoothRoundRect(46, 4, SCR_W - 50, 28, 6, C_PANEL, C_BG);  // text box
  String shown = kbText;
  const int maxW = SCR_W - 50 - 24;
  while (shown.length() && textWidth(shown, F_NAME) > maxW) utf8PopFront(shown);
  int w = 0;
  if (kbText.length()) w = text(56, 24, shown, F_NAME, C_TEXT, 0, C_PANEL);
  else text(56, 23, fitText(tr(S_SEARCH_PH), maxW, F_DETAIL), F_DETAIL, C_DIM, 0, C_PANEL);
  tft.fillRect(56 + w + 2, 9, 2, 18, C_ACC);                         // cursor
}

void drawSuggestions() {
  if (screen != SCR_ADD) return;
  tft.fillRect(0, SUG_TOP - 2, SCR_W, KB_Y - SUG_TOP + 2, C_BG);
  const char* msg = nullptr;
  switch (sgState) {
    case SG_HINT:      msg = tr(S_HINT); break;
    case SG_SEARCHING: msg = tr(S_SEARCHING); break;
    case SG_NONE:      msg = tr(S_NO_MATCH); break;
    case SG_ERROR:     msg = tr(S_SEARCH_FAIL); break;
    case SG_FULL:      msg = tr(S_FULL); break;
    case SG_RESULTS:   break;
  }
  if (msg) { text(SCR_W / 2, SUG_TOP + 30, fitText(msg, SCR_W - 16, F_DETAIL), F_DETAIL, C_DIM, 1); return; }
  for (int i = 0; i < nSugg; i++) {
    int y = SUG_TOP + i * SUG_ROWH;
    tft.fillSmoothRoundRect(4, y, SCR_W - 8, SUG_ROWH - 2, 5, C_PANEL, C_BG);
    String nm = fitText(sugg[i].name, 200, F_DAY);
    int w = text(12, y + 15, nm, F_DAY, C_TEXT, 0, C_PANEL);
    String rest;
    if (sugg[i].region.length()) rest += ", " + sugg[i].region;
    if (sugg[i].cc.length())     rest += ", " + sugg[i].cc;
    if (rest.length()) text(12 + w, y + 15, fitText(rest, SCR_W - 30 - w, F_DETAIL), F_DETAIL, C_DIM, 0, C_PANEL);
  }
}

void drawAddScreen() {
  screen = SCR_ADD;
  kbText = "";
  kbNumMode = false;
  nSugg = 0;
  searchDue = 0;
  sgState = (nLocs >= MAX_LOCS) ? SG_FULL : SG_HINT;
  tft.fillScreen(C_BG);
  drawInputField();
  drawSuggestions();
  drawKeyboard();
}

void textChanged() {
  drawInputField();
  String q = kbText; q.trim();
  if (q.length() >= 2) {
    searchDue = millis() + 700;              // debounce while typing
  } else {
    searchDue = 0; nSugg = 0;
    if (sgState != SG_HINT) { sgState = SG_HINT; drawSuggestions(); }
  }
}

void chooseSuggestion(int i) {
  if (i < 0 || i >= nSugg) return;
  String err = addLocation(sugg[i]);
  if (err.length()) { sgState = SG_FULL; drawSuggestions(); return; }
  wx[cur].lastTry = 0;                       // fetch right away
  showMain();
}

void addTap(int tx, int ty) {
  if (ty < 36) {
    if (tx < 44) { showMain(); return; }      // cancel
    return;
  }
  if (ty < KB_Y) {
    if (sgState == SG_RESULTS) {
      int i = (ty - SUG_TOP) / SUG_ROWH;
      if (ty >= SUG_TOP) chooseSuggestion(i);
    }
    return;
  }
  int r = (ty - KB_Y) / KB_ROWH;
  if (r > 3) r = 3;
  int n; const KeyDef* row = kbRow(r, n);
  int j = -1;
  for (int k = 0; k < n; k++) {
    int x, y, w, h; keyRect(r, k, x, y, w, h);
    if (tx >= x && tx < x + w) { j = k; break; }
  }
  if (j < 0) return;
  const KeyDef& k = row[j];
  drawKey(r, j, true);                        // brief visual feedback
  delay(60);
  switch (k.act) {
    case A_CHR: {
      if (kbText.length() >= 40) break;
      char last = kbText.length() ? kbText[kbText.length() - 1] : ' ';
      bool cap = (last == ' ' || last == '-');   // auto-capitalise words
      kbText += cap ? k.up : k.lo;
      textChanged();
      break;
    }
    case A_DEL:
      utf8PopBack(kbText);
      textChanged();
      break;
    case A_SPACE:
      if (kbText.length() && !kbText.endsWith(" ")) { kbText += ' '; textChanged(); }
      break;
    case A_MODE:
      kbNumMode = !kbNumMode;
      drawKeyboard();
      return;
    case A_GO:
      searchDue = 0;
      if (nLocs < MAX_LOCS) runSearch();
      break;
  }
  drawKey(r, j, false);
}

// ============================ touch ============================
// Returns true once per press (on touch-down), with the tap in screen pixels.
bool readTap(int& tx, int& ty) {
  static bool     down = false;
  static uint32_t upSince = 0, lastTap = 0;
  bool t = touchscreen.tirqTouched() && touchscreen.touched();
  if (!t) {
    if (down) {
      if (!upSince) upSince = millis();
      else if (millis() - upSince > 60) down = false;     // released
    }
    return false;
  }
  upSince = 0;
  if (down) return false;
  down = true;
  if (millis() - lastTap < 120) return false;
  delay(12);                                   // let the reading settle
  TS_Point p = touchscreen.getPoint();
  if (p.z < 200) { down = false; return false; }
  lastTap = millis();
  tx = map(p.x, TOUCH_MINX, TOUCH_MAXX, 0, SCR_W - 1);
  ty = map(p.y, TOUCH_MINY, TOUCH_MAXY, 0, SCR_H - 1);
  if (TOUCH_FLIP_X) tx = SCR_W - 1 - tx;
  if (TOUCH_FLIP_Y) ty = SCR_H - 1 - ty;
  tx = constrain(tx, 0, SCR_W - 1);
  ty = constrain(ty, 0, SCR_H - 1);
  return true;
}

void handleTouch() {
  int tx, ty;
  if (!readTap(tx, ty)) return;
  switch (screen) {
    case SCR_EMPTY:
      if (ty < 36 && tx > 250) { updState = UPD_IDLE; drawMenu(); return; }
      if (ty >= 100 && ty <= 172 && tx >= 50 && tx <= 270) drawAddScreen();
      return;
    case SCR_WEATHER:
      if (ty < 36) { updState = UPD_IDLE; removeArmed = false; drawMenu(); return; }
      if (nLocs > 1) {                         // left half = previous, right half = next
        cur = (tx < SCR_W / 2) ? (cur + nLocs - 1) % nLocs : (cur + 1) % nLocs;
        drawWeatherPage();
        lastSwitch = millis();
      }
      return;
    case SCR_MENU: menuTap(tx, ty); return;
    case SCR_ADD:  addTap(tx, ty);  return;
    default: return;
  }
}

// ============================ OTA ============================
bool ensureTime() {
  if (time(nullptr) > 1700000000) return true;
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  uint32_t t0 = millis();
  while (time(nullptr) < 1700000000 && millis() - t0 < 8000) delay(100);
  return time(nullptr) > 1700000000;
}

long semverNum(const String& v) {
  int a = 0, b = 0, c = 0;
  sscanf(v.c_str(), "%d.%d.%d", &a, &b, &c);
  return (long)a * 1000000L + (long)b * 1000L + c;
}

String fetchLatestVersion() {
  if (WiFi.status() != WL_CONNECTED) return "";
  ensureTime();                               // cert validation needs the real time
  WiFiClientSecure sc;
  sc.setCACert(OTA_ROOT_CAS);
  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(10000);
  String v;
  if (http.begin(sc, OTA_VER_URL)) {
    if (http.GET() == HTTP_CODE_OK) { v = http.getString(); v.trim(); }
    http.end();
  }
  if (v.length() > 20 || v.indexOf('.') < 0) v = "";
  return v;
}

void drawOtaProgress(int done, int total) {
  static int lastPct = -1;
  int pct = total ? (int)((done * 100LL) / total) : 0;
  if (pct == lastPct) return;
  lastPct = pct;
  const int bx = 40, by = 130, bw = SCR_W - 80, bh = 14;
  tft.drawRoundRect(bx, by, bw, bh, 4, C_DIM);
  tft.fillRoundRect(bx + 2, by + 2, (bw - 4) * pct / 100, bh - 4, 3, C_ACC);
  tft.fillRect(SCR_W / 2 - 30, by + 22, 60, 20, C_BG);
  text(SCR_W / 2, by + 38, String(pct) + "%", F_DESC, C_TEXT, 1);
}

void runOta() {
  drawMessage(tr(S_UPDATING), tr(S_DONT_UNPLUG), tr(S_RESTARTS));
  tft.fillRect(0, 115, SCR_W, 80, C_BG);
  Update.onProgress(drawOtaProgress);
  ensureTime();
  WiFiClientSecure sc;
  sc.setCACert(OTA_ROOT_CAS);
  httpUpdate.rebootOnUpdate(true);
  t_httpUpdate_return r = httpUpdate.update(sc, OTA_BIN_URL);
  if (r == HTTP_UPDATE_NO_UPDATES) drawMessage(tr(S_ALREADY), "");
  else drawMessage(tr(S_UPD_FAILED), httpUpdate.getLastErrorString());
  delay(4000);
  updState = UPD_IDLE;
  showMain();
}

// ============================ web server ============================
void sendJson(JsonDocument& doc) {
  String s; serializeJson(doc, s);
  server.send(200, "application/json", s);
}
void sendOk(const String& err = "") {
  JsonDocument d;
  d["ok"] = err.isEmpty();
  if (err.length()) d["error"] = err;
  sendJson(d);
}

void handleRoot() { server.send_P(200, "text/html", WEB_PAGE); }

void handleState() {
  JsonDocument d;
  d["version"] = FW_VERSION_SEM;
  d["max"]     = MAX_LOCS;
  d["rotate"]  = rotateSec;
  d["lang"]    = langCode();
  JsonArray la = d["langs"].to<JsonArray>();
  for (int i = 0; i < L_COUNT; i++) { JsonObject o = la.add<JsonObject>(); o["code"] = LANG_CODES[i]; o["name"] = LANG_NAMES[i]; }
  JsonArray a = d["locations"].to<JsonArray>();
  for (int i = 0; i < nLocs; i++) {
    JsonObject o = a.add<JsonObject>();
    o["name"] = locs[i].name; o["region"] = locs[i].region; o["cc"] = locs[i].cc;
    o["lat"]  = locs[i].lat;  o["lon"]    = locs[i].lon;
  }
  sendJson(d);
}

void handleAdd() {
  Location L;
  L.name   = server.arg("name");   L.name.trim();
  L.region = server.arg("region"); L.region.trim();
  L.cc     = server.arg("cc");     L.cc.trim();
  L.lat    = server.arg("lat").toFloat();
  L.lon    = server.arg("lon").toFloat();
  if (!L.name.length() || !server.hasArg("lat") || !server.hasArg("lon")) { sendOk("missing"); return; }
  if (L.name.length() > 60) L.name = L.name.substring(0, 60);
  if (L.region.length() > 60) L.region = L.region.substring(0, 60);
  String err = addLocation(L);
  if (err.isEmpty()) { wx[cur].lastTry = 0; uiDirty = true; }
  sendOk(err);
}

void handleRemove() { removeLocation(server.arg("i").toInt()); uiDirty = true; sendOk(); }
void handleMove()   { moveLocation(server.arg("i").toInt(), server.arg("d").toInt()); uiDirty = true; sendOk(); }
void handleSettings() {
  if (server.hasArg("rotate")) saveRotate(server.arg("rotate").toInt());
  if (server.hasArg("lang"))   { saveLang(langFromCode(server.arg("lang"))); uiDirty = true; }
  lastSwitch = millis();
  sendOk();
}
void handleCheckUpdate() {
  String v = fetchLatestVersion();
  JsonDocument d;
  d["current"] = FW_VERSION_SEM;
  d["latest"]  = v;
  d["newer"]   = v.length() && semverNum(v) > semverNum(FW_VERSION_SEM);
  sendJson(d);
}
void handleUpdate() { sendOk(); otaRequested = true; }   // run from loop(), not inside the handler

void startServer() {
  if (serverStarted) return;
  server.on("/",                HTTP_GET,  handleRoot);
  server.on("/api/state",       HTTP_GET,  handleState);
  server.on("/api/add",         HTTP_POST, handleAdd);
  server.on("/api/remove",      HTTP_POST, handleRemove);
  server.on("/api/move",        HTTP_POST, handleMove);
  server.on("/api/settings",    HTTP_POST, handleSettings);
  server.on("/api/checkupdate", HTTP_GET,  handleCheckUpdate);
  server.on("/api/update",      HTTP_POST, handleUpdate);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
  serverStarted = true;
}

// ============================ Improv ============================
void onImprovConnected(const char* ssid, const char* password) {
  saveWiFi(ssid, password);
  startServer();
}

// ============================ weather scheduling ============================
bool fetchDue(int i, uint32_t now) {
  const Weather& W = wx[i];
  if (W.lastTry && now - W.lastTry < RETRY_MS) return false;
  if (!W.valid) return true;
  return now - W.fetchedAt >= REFRESH_MS;
}

// Fetch at most one location per call (current page first) to keep the UI responsive.
void serviceWeather() {
  if (WiFi.status() != WL_CONNECTED || nLocs == 0) return;
  if (screen == SCR_ADD || screen == SCR_MENU || screen == SCR_MSG) return;
  uint32_t now = millis();
  int pick = -1;
  if (fetchDue(cur, now)) pick = cur;
  else for (int i = 0; i < nLocs; i++) if (fetchDue(i, now)) { pick = i; break; }
  if (pick < 0) return;

  wx[pick].lastTry = now ? now : 1;
  bool ok = fetchWeather(pick);
  wx[pick].failed = !ok;
  if (ok) wx[pick].fetchedAt = millis();
  if (pick == cur && screen == SCR_WEATHER) drawWeatherPage();
}

// ============================ setup / loop ============================
void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);                  // landscape, 320 x 240
  u8f.begin(tft);
  tft.fillScreen(C_BG);

  touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchSPI);
  touchscreen.setRotation(1);          // matches the landscape screen

  loadSettings();

  improvSerial.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32,
                             FW_NAME, FW_VERSION_SEM, "CYD Weather", "http://{LOCAL_IPV4}/");
  improvSerial.onImprovConnected(onImprovConnected);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  wifiStartMs = millis();
  if (wifiSsid.length()) {
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
    drawMessage("CYD Weather", trf(S_CONNECTING, wifiSsid));
  } else {
    drawNoWifi();
  }
}

void loop() {
  improvSerial.handleSerial();
  bool online = WiFi.status() == WL_CONNECTED;

  if (online && !serverStarted) startServer();
  if (serverStarted) server.handleClient();

  // Leave the boot / no-Wi-Fi screens once connected.
  static bool wasOnline = false;
  static bool booting = true;          // still on the "Connecting..." / no-Wi-Fi screen
  if (online != wasOnline) {
    wasOnline = online;
    if (online && booting) { booting = false; showMain(); }
    else if (screen == SCR_WEATHER || screen == SCR_EMPTY) showMain();   // refresh "offline" / IP hints
  }
  if (booting && !online && screen == SCR_MSG && millis() - wifiStartMs > 20000) drawNoWifi();

  if (otaRequested) { otaRequested = false; runOta(); }

  if (uiDirty && screen != SCR_ADD && screen != SCR_MSG && screen != SCR_NOWIFI) {
    uiDirty = false;
    showMain();
  }

  handleTouch();

  // Debounced place search while typing.
  if (screen == SCR_ADD && searchDue && (int32_t)(millis() - searchDue) >= 0) {
    searchDue = 0;
    if (nLocs < MAX_LOCS) runSearch();
  }

  serviceWeather();

  // Auto-rotate between locations.
  if (screen == SCR_WEATHER && nLocs > 1 && millis() - lastSwitch >= (uint32_t)rotateSec * 1000UL) {
    cur = (cur + 1) % nLocs;
    drawWeatherPage();
    lastSwitch = millis();
  }

  delay(5);
}

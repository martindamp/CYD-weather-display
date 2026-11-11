#include "../../src/main.cpp"
HardwareSerial Serial; WiFiClass WiFi; HTTPUpdateC httpUpdate; UpdateC Update;
static Location mk(const char* n, const char* r, const char* c) { Location L; L.name = n; L.region = r; L.cc = c; return L; }
static void fake(int i, float t, int code, bool day, std::initializer_list<int> codes, std::initializer_list<float> hi, std::initializer_list<float> lo, std::initializer_list<int> pop) {
  Weather& W = wx[i]; W.valid = true; W.temp = t; W.feels = t - 3.4f; W.wind = 5.6f; W.hum = 81; W.code = code; W.isDay = day; strcpy(W.time, "16:15");
  int k = 0; for (int c : codes) W.days[k++].code = c; k = 0; for (float v : hi) W.days[k++].tmax = v; k = 0; for (float v : lo) W.days[k++].tmin = v; k = 0; for (int v : pop) W.days[k++].pop = v;
  for (k = 0; k < 5; k++) W.days[k].dow = (3 + k) % 7; }
static void shot(const char* name) { char fn[96]; snprintf(fn, sizeof fn, "out/%s_%s.ppm", LANG_CODES[g_lang], name); tft.writePPM(fn); }
int main() {
  tft.init(); tft.setRotation(1); u8f.begin(tft);
  for (int l = 0; l < L_COUNT; l++) {
    g_lang = (Lang)l;
    nLocs = 0; drawEmptyPage(); shot("1_empty");
    nLocs = 3; locs[0] = mk("Vejle", "South Denmark", "DK"); locs[1] = mk("B\xC3\xB8rkop", "South Denmark", "DK"); locs[2] = mk("Troms\xC3\xB8", "Troms", "NO");
    fake(0, 7.4f, 2, true, {2, 61, 80, 3, 0}, {9, 8, 7, 6, 8}, {4, 5, 2, 1, 3}, {10, 80, 60, 20, 0});
    fake(1, -2.6f, 57, true, {71, 85, 3, 45, 95}, {1, 0, -1, 2, 5}, {-4, -6, -5, -1, 2}, {70, 50, 10, 30, 90});
    fake(2, -11.3f, 1, false, {0, 1, 75, 99, 55}, {-8, -6, -9, -3, -2}, {-15, -13, -14, -9, -7}, {0, 5, 90, 40, 70});
    cur = 0; drawWeatherPage(); shot("2_weather");
    cur = 1; drawWeatherPage(); shot("3_weather2");
    cur = 2; drawWeatherPage(); shot("4_weather3");
    removeArmed = true; updState = UPD_AVAILABLE; latestVer = "1.2.0"; drawMenu(); shot("5_menu");
    removeArmed = false; updState = UPD_IDLE;
    drawAddScreen(); shot("6_add_empty");
    kbText = "Vej"; drawInputField();
    nSugg = 3; sugg[0] = mk("Vejle", "South Denmark", "DK"); sugg[1] = mk("Vejen", "South Denmark", "DK"); sugg[2] = mk("Vejers Strand", "South Denmark", "DK");
    sgState = SG_RESULTS; drawSuggestions(); shot("6_add");
    kbNumMode = true; drawKeyboard(); shot("7_add_num");
    wifiSsid = "MyHomeNetwork"; drawNoWifi(); shot("8_nowifi"); wifiSsid = "";
  }
  g_lang = L_EN;
  // icon sheet
  TFT_eSPI sheet(0,0); sheet.W = 7 * 100; sheet.H = 2 * 100 + 2 * 50; sheet.buf.assign(sheet.W * sheet.H, C_BG);
  int codes[14] = {0, 1, 2, 3, 45, 53, 57, 63, 65, 81, 73, 85, 95, 99};
  for (int i = 0; i < 14; i++) { TFT_eSprite s(&sheet); s.createSprite(96, 96); s.fillSprite(C_BG); wicons::draw(s, 96, codes[i], true, C_BG); s.pushSprite((i % 7) * 100 + 2, (i / 7) * 100 + 2);
    TFT_eSprite t(&sheet); t.createSprite(42, 42); t.fillSprite(C_BG); wicons::draw(t, 42, codes[i], i != 0, C_BG); t.pushSprite((i % 7) * 100 + 29, 200 + (i / 7) * 50 + 4); }
  sheet.writePPM("out/0_icons.ppm");
  return 0;
}

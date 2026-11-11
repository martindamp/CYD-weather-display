#include "../../src/main.cpp"
HardwareSerial Serial; WiFiClass WiFi; HTTPUpdateC httpUpdate; UpdateC Update;
static Location mk(const char* n, const char* r, const char* c) { Location L; L.name = n; L.region = r; L.cc = c; return L; }
static void fake(int i, float t, int code, bool day, std::initializer_list<int> codes, std::initializer_list<float> hi, std::initializer_list<float> lo, std::initializer_list<int> pop) {
  Weather& W = wx[i]; W.valid = true; W.temp = t; W.feels = t - 3.4f; W.wind = 5.6f; W.hum = 81; W.code = code; W.isDay = day; strcpy(W.time, "16:15");
  int k = 0; for (int c : codes) W.days[k++].code = c; k = 0; for (float v : hi) W.days[k++].tmax = v; k = 0; for (float v : lo) W.days[k++].tmin = v; k = 0; for (int v : pop) W.days[k++].pop = v;
  for (k = 0; k < 5; k++) W.days[k].dow = (3 + k) % 7; }
int main() {
  tft.init(); tft.setRotation(1); u8f.begin(tft);
  drawEmptyPage(); tft.writePPM("out/1_empty.ppm");
  nLocs = 3; locs[0] = mk("Vejle", "South Denmark", "DK"); locs[1] = mk("Børkop", "South Denmark", "DK"); locs[2] = mk("Tromsø", "Troms", "NO");
  fake(0, 7.4f, 2, true, {2, 61, 80, 3, 0}, {9, 8, 7, 6, 8}, {4, 5, 2, 1, 3}, {10, 80, 60, 20, 0});
  fake(1, -2.6f, 71, true, {71, 85, 3, 45, 95}, {1, 0, -1, 2, 5}, {-4, -6, -5, -1, 2}, {70, 50, 10, 30, 90});
  fake(2, -11.3f, 0, false, {0, 1, 75, 99, 55}, {-8, -6, -9, -3, -2}, {-15, -13, -14, -9, -7}, {0, 5, 90, 40, 70});
  cur = 0; drawWeatherPage(); tft.writePPM("out/2_weather.ppm");
  cur = 1; drawWeatherPage(); tft.writePPM("out/3_weather2.ppm");
  cur = 2; drawWeatherPage(); tft.writePPM("out/4_weather3.ppm");
  drawMenu(); tft.writePPM("out/5_menu.ppm");
  drawAddScreen(); kbText = "Vej"; drawInputField();
  nSugg = 3; sugg[0] = mk("Vejle", "South Denmark", "DK"); sugg[1] = mk("Vejen", "South Denmark", "DK"); sugg[2] = mk("Vejers Strand", "South Denmark", "DK");
  sgState = SG_RESULTS; drawSuggestions(); tft.writePPM("out/6_add.ppm");
  kbNumMode = true; drawKeyboard(); tft.writePPM("out/7_add_num.ppm");
  drawNoWifi(); tft.writePPM("out/8_nowifi.ppm");
  // icon sheet
  TFT_eSPI sheet(0,0); sheet.W = 7 * 100; sheet.H = 2 * 100 + 2 * 50; sheet.buf.assign(sheet.W * sheet.H, C_BG);
  int codes[14] = {0, 1, 2, 3, 45, 53, 57, 63, 65, 81, 73, 85, 95, 99};
  for (int i = 0; i < 14; i++) { TFT_eSprite s(&sheet); s.createSprite(96, 96); s.fillSprite(C_BG); wicons::draw(s, 96, codes[i], true, C_BG); s.pushSprite((i % 7) * 100 + 2, (i / 7) * 100 + 2);
    TFT_eSprite t(&sheet); t.createSprite(42, 42); t.fillSprite(C_BG); wicons::draw(t, 42, codes[i], i != 0, C_BG); t.pushSprite((i % 7) * 100 + 29, 200 + (i / 7) * 50 + 4); }
  sheet.writePPM("out/0_icons.ppm");
  return 0;
}

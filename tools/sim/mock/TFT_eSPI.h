#pragma once
#include "Arduino.h"
#include <vector>
#define TFT_BLACK 0
class TFT_eSPI {
public:
  std::vector<uint16_t> buf; int W = 240, H = 320;
  TFT_eSPI(int w = 240, int h = 320) : W(w), H(h) { buf.assign(W * H, 0); }
  virtual ~TFT_eSPI() {}
  void init() {}
  void setRotation(int r) { if (r & 1) { W = 320; H = 240; } else { W = 240; H = 320; } buf.assign(W * H, 0); }
  int width() { return W; } int height() { return H; }
  uint16_t color565(uint8_t r, uint8_t g, uint8_t b) { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }
  void drawPixel(int32_t x, int32_t y, uint32_t c) { if (x >= 0 && y >= 0 && x < W && y < H) buf[y * W + x] = c; }
  virtual uint16_t readPixel(int32_t x, int32_t y) { return (x >= 0 && y >= 0 && x < W && y < H) ? buf[y * W + x] : 0; }
  void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c) { for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) drawPixel(i, j, c); }
  void fillScreen(uint32_t c) { fillRect(0, 0, W, H, c); }
  void drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t c) { fillRect(x, y, w, 1, c); }
  void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t c) { fillRect(x, y, 1, h, c); }
  static uint16_t blend(float a, uint16_t fg, uint16_t bg) {
    int r1 = (fg >> 11) & 31, g1 = (fg >> 5) & 63, b1 = fg & 31, r2 = (bg >> 11) & 31, g2 = (bg >> 5) & 63, b2 = bg & 31;
    int r = r2 + (r1 - r2) * a + 0.5f, g = g2 + (g1 - g2) * a + 0.5f, b = b2 + (b1 - b2) * a + 0.5f;
    return (r << 11) | (g << 5) | b; }
  void aa(int x, int y, float a, uint32_t fg, uint32_t bg) {
    if (a <= 0) return; if (a > 1) a = 1;
    uint16_t b = (bg == 0x00FFFFFF) ? readPixel(x, y) : bg; drawPixel(x, y, blend(a, fg, b)); }
  void fillSmoothCircle(int32_t cx, int32_t cy, int32_t r, uint32_t c, uint32_t bg = 0x00FFFFFF) {
    for (int y = cy - r - 1; y <= cy + r + 1; y++) for (int x = cx - r - 1; x <= cx + r + 1; x++) {
      float d = hypotf(x - cx, y - cy); aa(x, y, r + 0.5f - d, c, bg); } }
  void drawSpot(float ax, float ay, float r, uint32_t c, uint32_t bg = 0x00FFFFFF) { fillSmoothCircle(ax, ay, r, c, bg); }
  void drawWideLine(float ax, float ay, float bx, float by, float wd, uint32_t c, uint32_t bg = 0x00FFFFFF) {
    float r = wd / 2; int x0 = std::min(ax, bx) - r - 1, x1 = std::max(ax, bx) + r + 1, y0 = std::min(ay, by) - r - 1, y1 = std::max(ay, by) + r + 1;
    float dx = bx - ax, dy = by - ay, L2 = dx * dx + dy * dy;
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
      float t = L2 > 0 ? ((x - ax) * dx + (y - ay) * dy) / L2 : 0; t = std::max(0.f, std::min(1.f, t));
      float d = hypotf(x - (ax + t * dx), y - (ay + t * dy)); aa(x, y, r + 0.5f - d, c, bg); } }
  void fillSmoothRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t c, uint32_t bg = 0x00FFFFFF) {
    for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) {
      float qx = std::max(std::max((x + r) - (i + 0.5f), (i + 0.5f) - (x + w - r)), 0.f);
      float qy = std::max(std::max((y + r) - (j + 0.5f), (j + 0.5f) - (y + h - r)), 0.f);
      float d = hypotf(qx, qy); aa(i, j, r + 0.5f - d, c, bg); } }
  void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t c) { fillSmoothRoundRect(x, y, w, h, r, c, readPixel(x, y)); }
  void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t, uint32_t c) { drawFastHLine(x, y, w, c); drawFastHLine(x, y + h - 1, w, c); drawFastVLine(x, y, h, c); drawFastVLine(x + w - 1, y, h, c); }
  void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t c) {
    int minx = std::min({x0, x1, x2}), maxx = std::max({x0, x1, x2}), miny = std::min({y0, y1, y2}), maxy = std::max({y0, y1, y2});
    auto e = [](float ax, float ay, float bx, float by, float px, float py) { return (bx - ax) * (py - ay) - (by - ay) * (px - ax); };
    for (int y = miny; y <= maxy; y++) for (int x = minx; x <= maxx; x++) {
      float a = e(x0, y0, x1, y1, x, y), b = e(x1, y1, x2, y2, x, y), d = e(x2, y2, x0, y0, x, y);
      if ((a >= 0 && b >= 0 && d >= 0) || (a <= 0 && b <= 0 && d <= 0)) drawPixel(x, y, c); } }
  void writePPM(const char* fn) { FILE* f = fopen(fn, "wb"); fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (uint16_t p : buf) { uint8_t px[3] = {(uint8_t)(((p >> 11) & 31) * 255 / 31), (uint8_t)(((p >> 5) & 63) * 255 / 63), (uint8_t)((p & 31) * 255 / 31)}; fwrite(px, 1, 3, f); } fclose(f); }
};
class TFT_eSprite : public TFT_eSPI {
public:
  TFT_eSPI* parent;
  TFT_eSprite(TFT_eSPI* p) : TFT_eSPI(0, 0), parent(p) {}
  void setColorDepth(int) {}
  bool createSprite(int w, int h) { W = w; H = h; buf.assign(w * h, 0); return true; }
  void deleteSprite() { buf.clear(); }
  void fillSprite(uint32_t c) { fillScreen(c); }
  void pushSprite(int x, int y) { for (int j = 0; j < H; j++) for (int i = 0; i < W; i++) parent->drawPixel(x + i, y + j, buf[j * W + i]); }
};

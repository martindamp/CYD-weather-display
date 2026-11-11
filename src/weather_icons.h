// Vector weather icons, drawn anti-aliased into a TFT_eSprite.
//
// Drawing into a sprite (instead of straight to the panel) lets the smooth
// primitives read back the pixels already drawn, so overlapping shapes (cloud
// puffs, a sun behind a cloud) blend cleanly with no dark seams.
//
// Icons are resolution independent: pass any size, and everything is scaled.
// Weather codes are WMO codes as returned by Open-Meteo.
#pragma once
#include <TFT_eSPI.h>
#include <math.h>

namespace wicons {

// Palette (RGB565)
static inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
static const uint16_t C_SUN      = rgb(255, 196,  38);
static const uint16_t C_RAY      = rgb(255, 160,  30);
static const uint16_t C_MOON     = rgb(232, 228, 205);
static const uint16_t C_CLOUD    = rgb(236, 241, 248);
static const uint16_t C_CLOUD2   = rgb(168, 178, 196);   // back / overcast cloud
static const uint16_t C_DARK     = rgb(112, 120, 140);   // storm cloud
static const uint16_t C_RAIN     = rgb( 80, 164, 255);
static const uint16_t C_ICE      = rgb(150, 225, 255);
static const uint16_t C_SNOW     = rgb(250, 252, 255);
static const uint16_t C_BOLT     = rgb(255, 212,  40);
static const uint16_t C_FOG      = rgb(160, 170, 188);

enum Kind {
  K_CLEAR, K_MAINLY_CLEAR, K_PARTLY, K_OVERCAST, K_FOG,
  K_DRIZZLE, K_FREEZING, K_RAIN, K_HEAVY_RAIN, K_SHOWERS,
  K_SNOW, K_SNOW_SHOWERS, K_THUNDER, K_HAIL
};

static inline Kind kindFor(int code) {
  switch (code) {
    case 0:  return K_CLEAR;
    case 1:  return K_MAINLY_CLEAR;
    case 2:  return K_PARTLY;
    case 3:  return K_OVERCAST;
    case 45: case 48: return K_FOG;
    case 51: case 53: case 55: return K_DRIZZLE;
    case 56: case 57: case 66: case 67: return K_FREEZING;
    case 61: case 63: return K_RAIN;
    case 65: return K_HEAVY_RAIN;
    case 80: case 81: case 82: return K_SHOWERS;
    case 71: case 73: case 75: case 77: return K_SNOW;
    case 85: case 86: return K_SNOW_SHOWERS;
    case 95: return K_THUNDER;
    case 96: case 99: return K_HAIL;
    default: return K_OVERCAST;
  }
}

// ---------------------------------------------------------------- primitives
static void sun(TFT_eSPI& g, float cx, float cy, float r) {
  for (int i = 0; i < 8; i++) {
    float a = i * (float)M_PI / 4.0f;
    float ca = cosf(a), sa = sinf(a);
    g.drawWideLine(cx + ca * r * 1.38f, cy + sa * r * 1.38f,
                   cx + ca * r * 1.78f, cy + sa * r * 1.78f,
                   r * 0.20f, C_RAY);
  }
  g.fillSmoothCircle((int32_t)cx, (int32_t)cy, (int32_t)r, C_SUN);
}

static void moon(TFT_eSPI& g, float cx, float cy, float r, uint16_t bg) {
  g.fillSmoothCircle((int32_t)cx, (int32_t)cy, (int32_t)r, C_MOON);
  g.fillSmoothCircle((int32_t)(cx + r * 0.52f), (int32_t)(cy - r * 0.38f),
                     (int32_t)(r * 0.86f), bg);
}

// A cloud whose flat bottom sits at y = base and is w pixels wide, centred on cx.
static void cloud(TFT_eSPI& g, float cx, float base, float w, uint16_t col) {
  float r1 = w * 0.19f, r2 = w * 0.27f, r3 = w * 0.17f;
  float x1 = cx - w * 0.22f, x2 = cx + w * 0.01f, x3 = cx + w * 0.25f;
  g.fillRect((int32_t)x1, (int32_t)(base - r1 * 1.6f),
             (int32_t)(x3 - x1 + 1), (int32_t)(r1 * 1.6f), col);
  g.fillSmoothCircle((int32_t)x1, (int32_t)(base - r1), (int32_t)r1, col);
  g.fillSmoothCircle((int32_t)x3, (int32_t)(base - r3), (int32_t)r3, col);
  g.fillSmoothCircle((int32_t)x2, (int32_t)(base - r2 - w * 0.04f), (int32_t)r2, col);
  // flat, crisp bottom edge
  g.fillRect((int32_t)x1, (int32_t)(base - r1 * 0.9f),
             (int32_t)(x3 - x1 + 1), (int32_t)(r1 * 0.9f), col);
}

static void drops(TFT_eSPI& g, float s, float y0, int n, float len, uint16_t col) {
  // Slanted streaks under a cloud.
  float span = s * 0.46f, x0 = s * 0.5f - span / 2;
  for (int i = 0; i < n; i++) {
    float x = x0 + (n == 1 ? span / 2 : span * i / (n - 1));
    float yo = (i % 2) ? s * 0.06f : 0;
    g.drawWideLine(x + len * 0.30f, y0 + yo, x - len * 0.05f, y0 + yo + len,
                   s * 0.055f, col);
  }
}

static void flake(TFT_eSPI& g, float cx, float cy, float r, uint16_t col) {
  for (int i = 0; i < 3; i++) {
    float a = i * (float)M_PI / 3.0f + (float)M_PI / 6.0f;
    float dx = cosf(a) * r, dy = sinf(a) * r;
    g.drawWideLine(cx - dx, cy - dy, cx + dx, cy + dy, r * 0.42f, col);
  }
}

static void flakes(TFT_eSPI& g, float s, float y0, int n) {
  float span = s * 0.46f, x0 = s * 0.5f - span / 2;
  for (int i = 0; i < n; i++) {
    float x = x0 + (n == 1 ? span / 2 : span * i / (n - 1));
    float y = y0 + ((i % 2) ? s * 0.13f : s * 0.02f);
    flake(g, x, y, s * 0.065f, C_SNOW);
  }
}

static void bolt(TFT_eSPI& g, float s, float y0) {
  float cx = s * 0.50f, h = s * 0.32f;
  int32_t ax = cx + s * 0.06f, ay = y0;
  int32_t bx = cx - s * 0.09f, by = y0 + h * 0.58f;
  int32_t c1x = cx + s * 0.01f, c1y = y0 + h * 0.52f;
  int32_t dx = cx - s * 0.05f, dy = y0 + h;
  int32_t ex = cx + s * 0.10f, ey = y0 + h * 0.42f;
  int32_t fx = cx + s * 0.01f, fy = y0 + h * 0.48f;
  g.fillTriangle(ax, ay, bx, by, c1x, c1y, C_BOLT);
  g.fillTriangle(c1x, c1y, ax, ay, fx, fy, C_BOLT);
  g.fillTriangle(fx - 2, fy, ex, ey, dx, dy, C_BOLT);
}

// ---------------------------------------------------------------- icon
// Draw a full icon of size s x s at (0,0) of the sprite. bg = sprite background.
static void draw(TFT_eSPI& g, int s, int code, bool day, uint16_t bg) {
  Kind k = kindFor(code);
  float S = (float)s;
  switch (k) {
    case K_CLEAR:
      if (day) sun(g, S * 0.5f, S * 0.5f, S * 0.22f);
      else     moon(g, S * 0.5f, S * 0.52f, S * 0.30f, bg);
      break;

    case K_MAINLY_CLEAR:
    case K_PARTLY: {
      float r = (k == K_MAINLY_CLEAR) ? S * 0.19f : S * 0.16f;
      if (day) sun(g, S * 0.38f, S * 0.36f, r);
      else     moon(g, S * 0.38f, S * 0.36f, r * 1.2f, bg);
      float w = (k == K_MAINLY_CLEAR) ? S * 0.52f : S * 0.66f;
      cloud(g, S * 0.58f, S * 0.84f, w, C_CLOUD);
      break;
    }

    case K_OVERCAST:
      cloud(g, S * 0.62f, S * 0.64f, S * 0.66f, C_CLOUD2);
      cloud(g, S * 0.42f, S * 0.84f, S * 0.72f, C_CLOUD);
      break;

    case K_FOG:
      cloud(g, S * 0.5f, S * 0.60f, S * 0.70f, C_CLOUD2);
      for (int i = 0; i < 3; i++) {
        float y = S * (0.70f + i * 0.10f);
        float x0 = S * (0.18f + (i % 2) * 0.08f), x1 = S * (0.78f + (i % 2) * 0.04f);
        g.drawWideLine(x0, y, x1, y, S * 0.055f, C_FOG);
      }
      break;

    case K_DRIZZLE:
      cloud(g, S * 0.5f, S * 0.62f, S * 0.76f, C_CLOUD);
      drops(g, S, S * 0.70f, 3, S * 0.10f, C_RAIN);
      break;

    case K_FREEZING:
      cloud(g, S * 0.5f, S * 0.62f, S * 0.76f, C_CLOUD2);
      drops(g, S, S * 0.70f, 3, S * 0.14f, C_ICE);
      break;

    case K_RAIN:
    case K_HEAVY_RAIN:
      cloud(g, S * 0.5f, S * 0.60f, S * 0.78f, k == K_HEAVY_RAIN ? C_CLOUD2 : C_CLOUD);
      drops(g, S, S * 0.68f, k == K_HEAVY_RAIN ? 4 : 3, S * 0.18f, C_RAIN);
      break;

    case K_SHOWERS:
      if (day) sun(g, S * 0.33f, S * 0.30f, S * 0.13f);
      cloud(g, S * 0.54f, S * 0.62f, S * 0.70f, C_CLOUD);
      drops(g, S, S * 0.70f, 3, S * 0.16f, C_RAIN);
      break;

    case K_SNOW:
    case K_SNOW_SHOWERS:
      if (k == K_SNOW_SHOWERS && day) sun(g, S * 0.33f, S * 0.30f, S * 0.13f);
      cloud(g, S * 0.52f, S * 0.60f, S * 0.76f, C_CLOUD);
      flakes(g, S, S * 0.74f, 3);
      break;

    case K_THUNDER:
    case K_HAIL:
      cloud(g, S * 0.5f, S * 0.60f, S * 0.80f, C_DARK);
      bolt(g, S, S * 0.58f);
      if (k == K_HAIL) {
        g.fillSmoothCircle((int32_t)(S * 0.28f), (int32_t)(S * 0.80f), (int32_t)(S * 0.04f), C_SNOW);
        g.fillSmoothCircle((int32_t)(S * 0.72f), (int32_t)(S * 0.76f), (int32_t)(S * 0.04f), C_SNOW);
        g.fillSmoothCircle((int32_t)(S * 0.70f), (int32_t)(S * 0.90f), (int32_t)(S * 0.04f), C_SNOW);
      } else {
        drops(g, S, S * 0.70f, 2, S * 0.12f, C_RAIN);
      }
      break;
  }
}

}  // namespace wicons

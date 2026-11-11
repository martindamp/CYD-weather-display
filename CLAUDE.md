# CLAUDE.md — CYD-weather-display

Firmware for the ESP32 "Cheap Yellow Display" (ESP32-2432S028R, 2.8" ILI9341, landscape
320x240): current weather + 5-day forecast for up to 8 locations, auto-rotating pages.
PlatformIO + Arduino. Everything (code, comments, UI) in English.

## Commands
- Build: `pio run -e cyd` (the only env).
- Release (OTA): bump `VERSION` (semver), push to `main`. CI (`.github/workflows/deploy.yml`)
  builds, merges a factory bin, deploys web installer + `firmware/firmware.bin` + `version.txt`
  to GitHub Pages (https://martindamp.github.io/CYD-weather-display/).
- Screenshots on a PC: `tools/sim/render.sh` → `docs/*.png` (mock headers in `tools/sim/mock`).
  Re-run after UI changes so README/installer images stay current.

## Architecture
- `src/main.cpp`: everything except icons and the device web page. Screens are an enum
  (`SCR_EMPTY`, `SCR_WEATHER`, `SCR_MENU`, `SCR_ADD`, ...); each `draw*()` sets `screen`.
- `src/weather_icons.h`: vector icons per WMO code, drawn into a TFT_eSprite so the smooth
  primitives blend with what's already drawn. Also `describe(code)`.
- `src/web_page.h`: device page at `http://<ip>/`; the browser calls Open-Meteo geocoding
  directly (CORS ok) and POSTs results to `/api/add`.
- Data: Open-Meteo forecast + geocoding, no key. HTTPS without cert validation for weather
  (non-sensitive); OTA fetch is strictly cert-validated (`src/ota_certs.h`).
- State in NVS (`Preferences`): namespace `wifi` (ssid/pass via Improv), `wx` (`locs` JSON,
  `rotate`). Nothing secret in the repo or binary.
- Weather fetch: one location per loop pass (`serviceWeather`), refresh 15 min, retry 30 s.
  Skipped while the menu/keyboard is open so typing isn't blocked.

## Conventions / gotchas
- Bump `VERSION` for any firmware change meant to reach devices over OTA.
- Keep ALL TFT_eSPI config in `platformio.ini` build flags. This panel needs
  `TFT_INVERSION_ON` + `TFT_RGB_ORDER=TFT_BGR` (same as cyd-temp-gauge).
- Text uses U8g2 `_tf` fonts (Latin-1: æøå, °). Strings are UTF-8; use `utf8PopBack/Front`
  when trimming.
- `src/build_version.h` is generated in CI and .gitignored.
- Add forward declarations near the top of main.cpp when calling a function before it's defined.
- The cloud sandbox can't download PlatformIO's toolchain; a local arduino-cli build with the
  esp32 core 2.0.17 (same core PlatformIO uses) works for compile checks.

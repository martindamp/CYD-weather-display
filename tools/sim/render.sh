#!/bin/bash
# Render the device screens to PNG on a PC (no hardware needed).
# Compiles src/main.cpp against small mock headers (tools/sim/mock) with the
# real U8g2_for_TFT_eSPI fonts and ArduinoJson, then writes docs/*.png.
# Needs: g++, git, python3 + Pillow.
set -e
cd "$(dirname "$0")"
LIB=${LIB:-/tmp/cydsim-libs}
mkdir -p "$LIB" out
[ -d "$LIB/U8g2_for_TFT_eSPI" ] || git clone -q --depth 1 https://github.com/Bodmer/U8g2_for_TFT_eSPI "$LIB/U8g2_for_TFT_eSPI"
[ -d "$LIB/ArduinoJson" ]       || git clone -q --depth 1 -b v7.4.2 https://github.com/bblanchon/ArduinoJson "$LIB/ArduinoJson"
g++ -std=gnu++17 -O1 -w -Imock -I"$LIB/U8g2_for_TFT_eSPI/src" -I"$LIB/ArduinoJson/src" -o out/sim sim.cpp \
  "$LIB/U8g2_for_TFT_eSPI/src/U8g2_for_TFT_eSPI.cpp" -x c "$LIB/U8g2_for_TFT_eSPI/src/u8g2_fonts.c"
./out/sim
python3 - <<'PY'
from PIL import Image
for src, dst in [('en_2_weather','weather'),('en_3_weather2','weather-snow'),('en_6_add','add-location'),
                 ('en_1_empty','empty'),('en_5_menu','menu'),('da_2_weather','weather-da'),('0_icons','icons')]:
    im = Image.open('out/%s.ppm' % src)
    if dst != 'icons': im = im.resize((im.width*2, im.height*2), Image.NEAREST)
    im.save('../../docs/%s.png' % dst, optimize=True)
print('Wrote docs/*.png')
PY

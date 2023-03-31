#!/usr/bin/env bash
SCRIPT_DIR=$(dirname -- "${BASH_SOURCE[0]}")

# STB
wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_rect_pack.h -O "$SCRIPT_DIR/stb_rect_pack.h"
wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h -O "$SCRIPT_DIR/stb_truetype.h"
wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_image.h -O "$SCRIPT_DIR/stb_image.h"

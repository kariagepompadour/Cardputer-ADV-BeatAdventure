#!/bin/bash
# Copyright (c) 2026 Kariage POMPADOUR Entertainment Corporation. MIT License.
# Builds one BeatAdventure sketch for the M5Stack Cardputer ADV with arduino-cli and writes the app-only image
# (for a launcher) and its checksum next to the sketch.
# Needs: arduino-cli, the "m5stack:esp32" core 3.3.10, libraries M5Cardputer, M5Unified, M5GFX.
#   ./build.sh BeatAdventure_S0
# The compiler is told to replace the build machine's folders ($HOME and this repository) in the paths it embeds,
# so the published .bin files do not carry your user name or folder layout.
# The linker sends TinyUSB's dcd_edpt_iso_activate() through the sketch's guard first (BeatAdventure_S0/UsbIsoFix.h),
# the USB interface names through BeatAdventure_S0/UsbNames.h, and the speaker's descriptor through
# BeatAdventure_S0/UsbAudioDesc.h.
set -e
cd "$(dirname "$0")"
SKETCH=${1:?usage: ./build.sh <sketch folder, e.g. BeatAdventure_S0>}
[ -f "$SKETCH/$SKETCH.ino" ] || { echo "$SKETCH/$SKETCH.ino not found" >&2; exit 1; }
REPO=$(cd .. && pwd)
MAPS="-ffile-prefix-map=$REPO=/repo -ffile-prefix-map=$HOME=/home/user"
for m in $EXTRA_PREFIX_MAPS; do MAPS="$MAPS -ffile-prefix-map=$m"; done
OUT=$(mktemp -d)
arduino-cli compile --clean --fqbn m5stack:esp32:m5stack_cardputer:PartitionScheme=default_8MB,FlashSize=8M,USBMode=default,CDCOnBoot=cdc \
  --build-property "compiler.c.extra_flags=$MAPS" --build-property "compiler.cpp.extra_flags=$MAPS" \
  --build-property "compiler.c.elf.extra_flags=-Wl,--wrap=dcd_edpt_iso_activate -Wl,--wrap=tinyusb_add_string_descriptor -Wl,--wrap=tinyusb_enable_interface" \
  --output-dir "$OUT" "$SKETCH"
# A guard against a crash seen in B16: the game's functions run on the 8 KB loop-task stack, and a function that builds a big
# state as a temporary can overflow it on start-up. Refuse to publish a build where one of the game's functions (beatadv::) takes
# more than 2.5 KB of stack for itself.
OBJDUMP=$(ls "$HOME"/Library/Arduino15/packages/m5stack/tools/esp-x32/*/bin/xtensa-esp32s3-elf-objdump 2>/dev/null | tail -1)
if [ -n "$OBJDUMP" ] && [ -f "$OUT/$SKETCH.ino.elf" ]; then
  BIG=$("$OBJDUMP" -d --no-show-raw-insn "$OUT/$SKETCH.ino.elf" 2>/dev/null | awk '/^[0-9a-f]+ <.*>:/{name=$2} /entry\ta1, [0-9x]+/{n=$NF; v=n; if (n ~ /^0x/) { v=0; h=substr(n,3); for (k=1;k<=length(h);k++) v=v*16+index("0123456789abcdef", substr(h,k,1))-1 } if (v > 2560 && name ~ /beatadv/) print v, name}')
  if [ -n "$BIG" ]; then echo "STACK GUARD: a game function takes too much stack:" >&2; echo "$BIG" >&2; exit 1; fi
fi
cp "$OUT/$SKETCH.ino.bin" "$SKETCH/"
(cd "$SKETCH" && shasum -a 256 "$SKETCH.ino.bin" > SHA256SUMS.txt && cat SHA256SUMS.txt)
rm -rf "$OUT"

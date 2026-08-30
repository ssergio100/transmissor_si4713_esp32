#!/bin/sh

set -eu

ARDUINO_CLI_BIN="${ARDUINO_CLI_BIN:-arduino-cli}"

if ! command -v "$ARDUINO_CLI_BIN" >/dev/null 2>&1; then
  for ARDUINO_CLI_LOCAL in \
      "$HOME"/.vscode/extensions/thelastoutpostworkshop.arduino-maker-workshop-*/arduino_cli/linux/x64/arduino-cli; do
    if [ -x "$ARDUINO_CLI_LOCAL" ]; then
      ARDUINO_CLI_BIN="$ARDUINO_CLI_LOCAL"
      break
    fi
  done
fi

if [ ! -x "$ARDUINO_CLI_BIN" ]; then
  echo "arduino-cli nao encontrado; defina ARDUINO_CLI_BIN" >&2
  exit 127
fi

FQBN="esp32:esp32:esp32s3:UploadSpeed=460800,USBMode=hwcdc,CDCOnBoot=default,MSCOnBoot=default,DFUOnBoot=default,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,DebugLevel=none,PSRAM=opi,LoopCore=1,EventsCore=1,EraseFlash=none,JTAGAdapter=default,ZigbeeMode=default"

PROJETO_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

"$ARDUINO_CLI_BIN" compile \
    --fqbn "$FQBN" \
    --clean \
    --build-path "$PROJETO_DIR/build/cache" \
    --output-dir "$PROJETO_DIR/build/firmware" \
    "$PROJETO_DIR"

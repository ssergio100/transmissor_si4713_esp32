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

PROJETO_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

"$ARDUINO_CLI_BIN" compile \
    --profile esp32s3 \
    --clean \
    --build-path "$PROJETO_DIR/build/cache" \
    --output-dir "$PROJETO_DIR/build/firmware" \
    "$PROJETO_DIR"

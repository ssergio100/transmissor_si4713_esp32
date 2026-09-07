#!/bin/sh
set -eu

PROJETO_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$PROJETO_DIR"
mkdir -p build/testes

# Testa isolamento L/R e restauracao dos canais, sem placa.
g++ -std=c++17 -Wall -Wextra -Werror -Itests/stubs -I. \
    tests/multiplex_test.cpp modelos.cpp -o build/testes/multiplex_test
build/testes/multiplex_test

# Testa as decisoes do menu, sem placa e sem bibliotecas Arduino instaladas.
g++ -std=c++17 -Wall -Wextra -Werror -Itests/stubs -I. \
    tests/menu_test.cpp menu.cpp modelos.cpp -o build/testes/menu_test
build/testes/menu_test

# Opcional: gera imagens com a Adafruit_GFX real, na versao usada pelo projeto.
# MENU_GFX_DIR deve apontar para a pasta que contem Adafruit_GFX.cpp.
if [ -n "${MENU_GFX_DIR:-}" ]; then
  mkdir -p build/previas-menu
  g++ -std=c++17 -DARDUINO=10800 -Wall -Wextra \
      -Itests/gfx_stubs -I. -I"$MENU_GFX_DIR" \
      tests/menu_visual_test.cpp menu.cpp modelos.cpp janela_menu_tft.cpp \
      tela_principal_tft.cpp "$MENU_GFX_DIR/Adafruit_GFX.cpp" \
      -o build/testes/menu_visual_test
  build/testes/menu_visual_test
fi

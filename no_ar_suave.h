#pragma once
#include <Adafruit_GFX.h>

// Desenha somente NO AR. Posicao em pixels; cores RGB565 do tema.
namespace NoArSuave {
void desenhar(Adafruit_GFX& tft, int x, int y, uint16_t texto, uint16_t fundo);
}

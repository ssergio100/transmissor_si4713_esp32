#pragma once
#include <Adafruit_GFX.h>

// Nunito ExtraBold com anti-aliasing. Texto UTF-8: ASCII e Latin-1 (acentos PT).
// Caracteres fora desse conjunto aparecem como '?'. Nao requer arquivos na placa.
namespace FonteSuave {
struct Caixa { int16_t x, y, largura, altura; };
// Tamanhos 1..6: 12, 16, 22, 28, 36, 48 pixels. Reduz se nao couber na caixa.
// centroVertical=false alinha o topo visivel do texto ao y informado.
// Desenha sobre fundo uniforme e respeita os limites da caixa e do display.
void desenhar(Adafruit_GFX& tft, const char* texto, Caixa caixa,
              uint16_t cor, uint16_t fundo, uint8_t tamanho,
              bool centroVertical = true);
}

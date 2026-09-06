#include "no_ar_suave.h"
#include "no_ar_suave_imagem.h"

void NoArSuave::desenhar(Adafruit_GFX& tft, int x, int y,
    uint16_t texto, uint16_t fundo) {
  // Mistura a cor do texto com o fundo em 16 niveis: bordas sem serrilhado duro.
  uint16_t cores[16];
  for (int alpha = 0; alpha < 16; ++alpha) {
    const int r = (((texto >> 11) * alpha) + ((fundo >> 11) * (15-alpha)) + 7) / 15;
    const int g = ((((texto >> 5) & 63) * alpha) + (((fundo >> 5) & 63) * (15-alpha)) + 7) / 15;
    const int b = (((texto & 31) * alpha) + ((fundo & 31) * (15-alpha)) + 7) / 15;
    cores[alpha] = (r << 11) | (g << 5) | b;
  }
  // Uma linha por envio; sem framebuffer completo nem alocacao dinamica.
  uint16_t linha[LARGURA];
  for (int py = 0; py < ALTURA; ++py) {
    for (int px = 0; px < LARGURA; ++px) {
      const int pixel = py * LARGURA + px;
      const uint8_t par = pgm_read_byte(&COBERTURA[pixel / 2]);
      const uint8_t alpha = (pixel & 1) ? (par & 15) : (par >> 4);
      linha[px] = cores[alpha];
    }
    tft.drawRGBBitmap(x, y + py, linha, LARGURA, 1);
  }
}

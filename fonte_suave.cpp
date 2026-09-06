#include "fonte_suave.h"
#include <stdint.h>

namespace {
struct Glifo {
  uint32_t inicio;
  uint8_t largura, altura;
  int8_t x, y;
  uint8_t avanco;
};
#include "assets/fonte_suave_dados.h"

// Consome um caractere UTF-8; sequencias invalidas e simbolos nao incluidos usam '?'.
uint16_t proximo(const char*& texto) {
  const uint8_t primeiro = static_cast<uint8_t>(*texto++);
  if (primeiro < 128) return primeiro;
  unsigned extras = primeiro >= 0xF0 && primeiro <= 0xF4 ? 3
      : primeiro >= 0xE0 && primeiro <= 0xEF ? 2
      : primeiro >= 0xC2 && primeiro <= 0xDF ? 1 : 0;
  if (!extras) return '?';
  uint32_t codigo = primeiro & ((1U << (6-extras))-1);
  const unsigned quantidade = extras;
  while (extras--) {
    const uint8_t c = static_cast<uint8_t>(*texto);
    if ((c & 0xC0) != 0x80) return '?';
    ++texto;
    codigo = (codigo << 6) | (c & 63);
  }
  if ((quantidade == 1 && codigo < 128) || quantidade > 1) return '?';
  return codigo <= 255 ? codigo : '?';
}
const Glifo& glifo(uint16_t codigo, unsigned tamanho) {
  unsigned indice = '?' - 32;
  if (codigo >= 32 && codigo <= 126) indice = codigo - 32;
  if (codigo >= 160 && codigo <= 255) indice = 95 + codigo - 160;
  return GLIFOS[tamanho * GLIFOS_POR_TAMANHO + indice];
}
struct Medida { int esquerda=0, topo=0, direita=0, base=0; };
Medida medir(const char* texto, unsigned tamanho) {
  Medida medida;
  int cursor = 0;
  while (*texto) {
    const Glifo& g = glifo(proximo(texto), tamanho);
    if (g.largura && g.altura) {
      medida.esquerda = min(medida.esquerda, cursor + g.x);
      medida.direita = max(medida.direita, cursor + g.x + g.largura);
      medida.topo = min(medida.topo, int(g.y));
      medida.base = max(medida.base, g.y + int(g.altura));
    }
    cursor += g.avanco;
  }
  return medida;
}
}

void FonteSuave::desenhar(Adafruit_GFX& tft, const char* texto, Caixa caixa,
    uint16_t cor, uint16_t fundo, uint8_t tamanho, bool centroVertical) {
  if (!texto || !*texto || caixa.largura <= 0 || caixa.altura <= 0) return;
  unsigned fonte = tamanho > 0 ? min(unsigned(tamanho-1), 5U) : 0;
  Medida medida = medir(texto, fonte);
  while (fonte && (medida.direita-medida.esquerda > caixa.largura
                    || medida.base-medida.topo > caixa.altura)) {
    medida = medir(texto, --fonte);
  }
  const int origemX = caixa.x + (caixa.largura-(medida.direita-medida.esquerda))/2 - medida.esquerda;
  const int origemY = caixa.y - medida.topo
      + (centroVertical ? (caixa.altura-(medida.base-medida.topo))/2 : 0);
  const int esquerda = max(0, int(caixa.x));
  const int direita = min(int(tft.width()), int(caixa.x)+caixa.largura);
  const int topo = max(0, max(int(caixa.y), origemY+medida.topo));
  const int base = min(int(tft.height()), min(int(caixa.y)+caixa.altura, origemY+medida.base));
  if (direita <= esquerda || base <= topo) return;
  uint16_t paleta[16];
  for (unsigned a=0; a<16; ++a) {
    const unsigned r = ((cor>>11)*a+(fundo>>11)*(15-a)+7)/15;
    const unsigned g = (((cor>>5)&63)*a+((fundo>>5)&63)*(15-a)+7)/15;
    const unsigned b = ((cor&31)*a+(fundo&31)*(15-a)+7)/15;
    paleta[a] = (r<<11)|(g<<5)|b;
  }
  // Buffer fixo em pequenos trechos; nao aloca framebuffer nem heap.
  uint16_t linha[64];
  uint8_t cobertura[64];
  for (int y=topo; y<base; ++y) {
    for (int x=esquerda; x<direita; x+=64) {
      const int largura = min(64, direita-x);
      for (int i=0; i<largura; ++i) cobertura[i]=0;
      const char* atual = texto;
      int cursor=origemX;
      while (*atual) {
        const Glifo& g = glifo(proximo(atual), fonte);
        const int gy = y-origemY-g.y;
        if (gy>=0 && gy<g.altura) {
          for (int i=max(0,cursor+g.x-x); i<min(largura,cursor+g.x+g.largura-x); ++i) {
            const unsigned pixel = gy*g.largura + x+i-cursor-g.x;
            const uint8_t par = pgm_read_byte(&PIXELS[g.inicio+pixel/2]);
            const uint8_t alpha = pixel&1 ? par&15 : par>>4;
            cobertura[i] = max(cobertura[i], alpha);
          }
        }
        cursor+=g.avanco;
      }
      for (int i=0; i<largura; ++i) linha[i]=paleta[cobertura[i]];
      tft.drawRGBBitmap(x,y,linha,largura,1);
    }
  }
}

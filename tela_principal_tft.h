#pragma once
#include <Adafruit_GFX.h>
#include "estado_painel.h"
#include "tema_tft.h"

// Renderizador independente: nao inicializa hardware nem altera o transmissor.
class TelaPrincipalTft {
 public:
  void renderizar(Adafruit_GFX& tft, const EstadoPainel& estado, bool entrada,
                  bool somenteCabecalho = false);

 private:
  enum class Bloco { Transmissao, Rds, Frequencia, Potencia, Rssi,
                     Audio, Modo, PreEnfaseDesvio, Quantidade };
  // Ordem dos campos usada na tabela AREAS; guia de edicao no inicio do .cpp.
  struct Area {
    Bloco bloco;  // Identificacao explicita: a ordem da tabela e livre.
    int16_t x, y, largura, altura;
    const char* titulo;
    uint8_t tamanhoTexto;
    int16_t distanciaValorAoTopo;
  };
  // Guarda o ultimo desenho para evitar redesenhar blocos sem alteracao.
  struct CacheBloco {
    char texto[40] = {};
    TemaTft::EstiloBloco estilo = {};
  };
  CacheBloco cache_[static_cast<unsigned>(Bloco::Quantidade)];
  Adafruit_GFX* tft_ = nullptr;
  bool entrada_ = false;
  char textoRodapeAnterior_[40] = {};

  void desenharTransmissao(const EstadoPainel& estado);
  void desenharRds(const EstadoPainel& estado);
  void desenharFrequencia(const EstadoPainel& estado);
  void desenharPotencia(const EstadoPainel& estado);
  void desenharRssi(const EstadoPainel& estado);
  void desenharAudio(const EstadoPainel& estado);
  void desenharModo(const EstadoPainel& estado);
  void desenharPreEnfaseDesvio(const EstadoPainel& estado);
  void desenharRodape(const EstadoPainel& estado);
  void desenharBloco(Bloco bloco, const char* valor, const TemaTft::EstiloBloco& estilo);
  void desenharPainel(const Area& area, const TemaTft::EstiloBloco& estilo);
  void desenharIconeTransmissao(const Area& area, bool noAr, const TemaTft::EstiloBloco& estilo);
  void escreverTexto(int16_t x, int16_t y, const char* texto,
                     uint8_t tamanho, uint16_t cor);
  static const Area AREAS[];
  static const Area* localizarArea(Bloco bloco);
};

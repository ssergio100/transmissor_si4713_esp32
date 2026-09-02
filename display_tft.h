#pragma once

#include <Adafruit_ST7789.h>

#include "estado_painel.h"

class DisplayTft {
 public:
  DisplayTft();

  // Sem MISO, o ST7789 nao oferece confirmacao de presenca. O retorno indica
  // que o controlador SPI foi configurado e a inicializacao foi enviada.
  bool iniciar();
  void mostrarInicializacao();
  void renderizar(const EstadoPainel& estado);

 private:
  struct LinhaRenderizada {
    char texto[36] = {};
    uint16_t cor = 0;
    uint8_t tamanho = 0;
  };

  void prepararTela(
      const char* titulo,
      TelaPainel tela,
      bool recuperando = false
  );
  void mostrarPrincipal(const EstadoPainel& estado);
  void mostrarRaiz(const EstadoPainel& estado);
  void mostrarRf(const EstadoPainel& estado);
  void mostrarAudio(const EstadoPainel& estado);
  void mostrarRds(const EstadoPainel& estado);
  void mostrarMonitor(const EstadoPainel& estado);
  void mostrarVarredura(const EstadoPainel& estado);
  void mostrarSistema(const EstadoPainel& estado);
  void mostrarRecuperacao();

  void escreverLinha(
      uint8_t linha,
      const char* texto,
      uint16_t cor,
      uint8_t tamanho = 2
  );
  void escreverRodape(const char* texto, uint16_t cor);
  void limparLinhasAPartir(uint8_t primeira);
  void invalidarCache();
  void formatarRdsRolante(
      const ReceptorPainel& receptor,
      char* destino,
      size_t tamanhoDestino
  );

  Adafruit_ST7789 tft_;
  bool pronto_ = false;
  bool telaValida_ = false;
  bool recuperacaoRenderizada_ = false;
  TelaPainel telaRenderizada_ = TelaPainel::PRINCIPAL;
  LinhaRenderizada linhasRenderizadas_[6] = {};
  char ultimoRdsRolante_[65] = {};
  uint8_t deslocamentoRds_ = 0;
  uint32_t ultimaRolagemRdsMs_ = 0;
};

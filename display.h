#pragma once

#include <LiquidCrystal_I2C.h>

#include "menu.h"
#include "modelos.h"
#include "receptor_rda5807.h"

class Display {
 public:
  Display();

  bool iniciar();
  void mostrarInicializacao();
  void mostrarMensagem(const char* linha1, const char* linha2 = nullptr);
  void cancelarMensagem();
  void renderizar(
      const Menu& menu,
      const ConfiguracaoTransmissor& configuracao,
      const TelemetriaTransmissor& telemetria,
      const TelemetriaReceptorRda5807& telemetriaReceptor,
      uint16_t melhorFrequencia,
      uint8_t melhorRuido
  );

 private:
  void mostrarPrincipal(
      const ConfiguracaoTransmissor& configuracao,
      const TelemetriaTransmissor& telemetria,
      const TelemetriaReceptorRda5807& telemetriaReceptor
  );
  void mostrarRaiz(uint8_t item);
  void mostrarRf(
      const ConfiguracaoTransmissor& configuracao,
      uint8_t item,
      bool editando
  );
  void mostrarAudio(
      const ConfiguracaoTransmissor& configuracao,
      uint8_t item,
      bool editando
  );
  void mostrarRds(
      const ConfiguracaoTransmissor& configuracao,
      uint8_t item,
      bool editando,
      uint8_t cursor
  );
  void mostrarMonitor(const TelemetriaTransmissor& telemetria);
  void mostrarVarredura(
      uint8_t item,
      const TelemetriaTransmissor& telemetria,
      uint16_t melhorFrequencia,
      uint8_t melhorRuido
  );
  void mostrarSistema(uint8_t item);
  void mostrarCabecalho(const char* titulo, uint8_t item, uint8_t quantidade);
  void mostrarAjudaEdicao(bool editando);
  void escreverLinha(uint8_t linha, const char* texto);
  void invalidarCache();

  LiquidCrystal_I2C lcd_;
  bool pronto_ = false;
  uint32_t mensagemAteMs_ = 0;
  char linhasRenderizadas_[Configuracao::LCD_LINHAS]
                         [Configuracao::LCD_COLUNAS + 1] = {};
};

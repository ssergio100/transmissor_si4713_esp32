#pragma once

#include <LiquidCrystal_I2C.h>

#include "configuracao.h"
#include "estado_painel.h"

class Display {
 public:
  Display();

  bool iniciar();
  void mostrarInicializacao();
  void mostrarMensagem(const char* linha1, const char* linha2 = nullptr);
  void cancelarMensagem();
  void renderizar(const EstadoPainel& estado);

 private:
  void mostrarPrincipal(const EstadoPainel& estado);
  void mostrarRaiz(const NavegacaoPainel& navegacao);
  void mostrarRf(const EstadoPainel& estado);
  void mostrarAudio(const EstadoPainel& estado);
  void mostrarRds(const EstadoPainel& estado);
  void mostrarMonitor(const EstadoPainel& estado);
  void mostrarVarredura(const EstadoPainel& estado);
  void mostrarSistema(const EstadoPainel& estado);
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

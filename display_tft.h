#pragma once

#include <Adafruit_ST7789.h>
#include "estado_painel.h"
#include "tela_principal_tft.h"
#include "janela_menu_tft.h"

class DisplayTft {
 public:
  DisplayTft();
  // Inicializa SPI e verifica o buffer da janela. Nao confirma presenca do TFT.
  bool iniciar();
  void mostrarInicializacao();
  void renderizar(const EstadoPainel& estado);
  void entrarRepouso();
  void sairRepouso();
  bool emRepouso() const;

 private:
  Adafruit_ST7789 tft_;
  TelaPrincipalTft principal_;
  JanelaMenuTft janela_;
  bool pronto_ = false;
  bool painelValido_ = false;
  bool janelaAberta_ = false;
  bool emRepouso_ = false;
};

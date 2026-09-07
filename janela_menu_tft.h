#pragma once

#include <Adafruit_GFX.h>
#include "estado_painel.h"

// Desenha somente a janela; nao altera o radio nem interpreta o encoder.
class JanelaMenuTft {
 public:
  JanelaMenuTft();
  bool pronta() const;
  // Posicao da janela na tela; preserva a faixa NO AR / RDS.
  static constexpr int16_t X = 16;
  static constexpr int16_t Y = 30;
  // Retorna true somente quando ha uma imagem nova para enviar ao TFT.
  // forcar: use ao reabrir a janela ou depois de apagar a tela.
  bool preparar(const EstadoPainel& estado, bool forcar = false);
  const GFXcanvas16& imagem() const;


 private:
  GFXcanvas16 janela_;
  EstadoPainel anterior_;
  bool imagemValida_ = false;
  bool conteudoMudou(const EstadoPainel& estado) const;
  void mostrarLista(const EstadoPainel& estado);
  void mostrarEdicao(const NavegacaoPainel& edicao);
  void mostrarTexto(const NavegacaoPainel& edicao);
  void mostrarMonitor(const EstadoPainel& estado);
  void mostrarVarredura(const EstadoPainel& estado);
  // y: topo em pixels dentro da janela. tamanho: escala da fonte GFX.
  void textoCentral(int16_t y, const char* texto, uint8_t tamanho, uint16_t cor);
  // y: topo da linha. valor: texto alinhado a direita, ou vazio.
  void linha(int16_t y, const char* nome, const char* valor, bool selecionada);
  void titulo(const char* texto);
};

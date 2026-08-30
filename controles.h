#pragma once

#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>
#include <Bounce2.h>

#include "configuracao.h"

class Controles {
 public:
  void iniciar();
  void processar();
  int8_t consumirGiro();
  bool consumirClique();
  bool consumirPressaoLonga();

 private:
  static void IRAM_ATTR tratarInterrupcaoEncoder();
  static Controles* instanciaAtiva_;

  AiEsp32RotaryEncoder encoder_{
      Configuracao::PIN_ENCODER_DT,
      Configuracao::PIN_ENCODER_CLK,
      Configuracao::PIN_ENCODER_BOTAO,
      Configuracao::PIN_ENCODER_VCC,
      Configuracao::TRANSICOES_ENCODER_POR_DETENTE,
      false
  };
  Bounce2::Button botao_;
  int16_t giroAcumulado_ = 0;
  bool cliquePendente_ = false;
  bool pressaoLongaPendente_ = false;
  bool pressaoLongaReportada_ = false;
  uint32_t inicioPressaoMs_ = 0;
};

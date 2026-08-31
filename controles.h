#pragma once

#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>
#include <Bounce2.h>

#include "configuracao.h"

class Controles {
 public:
  enum class TipoEvento : uint8_t {
    NENHUM = 0,
    GIRO,
    CLIQUE,
    PRESSAO_LONGA
  };

  struct Evento {
    TipoEvento tipo = TipoEvento::NENHUM;
    int8_t deslocamento = 0;
    uint32_t duracaoPressaoMs = 0;
  };

  void iniciar();
  void processar();
  Evento consumirEvento();

 private:
  static void IRAM_ATTR tratarInterrupcaoEncoder();
  void acumularGiro(long deslocamento);
  void limparGirosPendentes();

  static Controles* instanciaAtiva_;
  AiEsp32RotaryEncoder encoder_{
      Configuracao::PIN_ENCODER_DT,
      Configuracao::PIN_ENCODER_CLK,
      -1,
      Configuracao::PIN_ENCODER_VCC,
      Configuracao::TRANSICOES_ENCODER_POR_DETENTE,
      false
  };
  Bounce2::Button botao_;
  int16_t girosPendentes_ = 0;
  TipoEvento eventoBotaoPendente_ = TipoEvento::NENHUM;
  bool botaoEmPressao_ = false;
  uint32_t inicioPressaoMs_ = 0;
  uint32_t duracaoPressaoPendenteMs_ = 0;
};

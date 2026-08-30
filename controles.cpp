#include "controles.h"

#include "configuracao.h"

Controles* Controles::instanciaAtiva_ = nullptr;

void Controles::iniciar() {
  instanciaAtiva_ = this;
  encoder_.begin();
  encoder_.setup(tratarInterrupcaoEncoder);
  encoder_.setBoundaries(-10000, 10000, false);
  encoder_.setEncoderValue(0);
  encoder_.disableAcceleration();

  botao_.attach(Configuracao::PIN_ENCODER_BOTAO, INPUT_PULLUP);
  botao_.interval(10);
  botao_.setPressedState(LOW);
  botao_.update();
}

void Controles::processar() {
  const long deslocamento = encoder_.encoderChanged();
  if (deslocamento != 0) {
    giroAcumulado_ = constrain(
        static_cast<long>(giroAcumulado_) + deslocamento,
        -127L,
        127L
    );
  }

  botao_.update();
  const uint32_t agora = millis();

  if (botao_.pressed()) {
    inicioPressaoMs_ = agora;
    pressaoLongaReportada_ = false;
  }

  if (botao_.isPressed()
      && !pressaoLongaReportada_
      && agora - inicioPressaoMs_
          >= Configuracao::TEMPO_PRESSIONAMENTO_LONGO_MS) {
    pressaoLongaReportada_ = true;
    pressaoLongaPendente_ = true;
  }

  if (botao_.released() && !pressaoLongaReportada_) cliquePendente_ = true;
}

int8_t Controles::consumirGiro() {
  const int8_t giro = static_cast<int8_t>(giroAcumulado_);
  giroAcumulado_ = 0;
  return giro;
}

bool Controles::consumirClique() {
  const bool pendente = cliquePendente_;
  cliquePendente_ = false;
  return pendente;
}

bool Controles::consumirPressaoLonga() {
  const bool pendente = pressaoLongaPendente_;
  pressaoLongaPendente_ = false;
  return pendente;
}

void IRAM_ATTR Controles::tratarInterrupcaoEncoder() {
  if (instanciaAtiva_ != nullptr) instanciaAtiva_->encoder_.readEncoder_ISR();
}

#include "controles.h"

#include <limits.h>

Controles* Controles::instanciaAtiva_ = nullptr;

void Controles::iniciar() {
  instanciaAtiva_ = this;
  encoder_.begin();
  encoder_.setup(tratarInterrupcaoEncoder);
  encoder_.setBoundaries(-10000, 10000, false);
  encoder_.setEncoderValue(0);
  encoder_.disableAcceleration();

  // O botao tem um unico proprietario. O pino -1 passado ao encoder deixa sua
  // classificacao exclusivamente com o debounce e a maquina de estados abaixo.
  botao_.attach(Configuracao::PIN_ENCODER_BOTAO, INPUT_PULLUP);
  botao_.interval(Configuracao::TEMPO_DEBOUNCE_BOTAO_MS);
  botao_.setPressedState(LOW);
  botao_.update();
}

void Controles::processar() {
  botao_.update();
  const uint32_t agora = millis();
  const long deslocamento = encoder_.encoderChanged();

  if (botao_.pressed()) {
    botaoEmPressao_ = true;
    inicioPressaoMs_ = agora;
    eventoBotaoPendente_ = TipoEvento::NENHUM;
    duracaoPressaoPendenteMs_ = 0;
    limparGirosPendentes();
  }

  // Pressionar o eixo pode girar mecanicamente o encoder. Esses passos nao
  // viram navegacao enquanto o botao estiver pressionado.
  if (botao_.isPressed()) {
    limparGirosPendentes();
    return;
  }

  if (botao_.released() && botaoEmPressao_) {
    botaoEmPressao_ = false;
    const uint32_t duracao = agora - inicioPressaoMs_;
    limparGirosPendentes();

    if (duracao >= Configuracao::TEMPO_PRESSIONAMENTO_LONGO_MS) {
      eventoBotaoPendente_ = TipoEvento::PRESSAO_LONGA;
      duracaoPressaoPendenteMs_ = duracao;
    } else if (duracao
        >= Configuracao::TEMPO_PRESSIONAMENTO_CURTO_MINIMO_MS) {
      eventoBotaoPendente_ = TipoEvento::CLIQUE;
      duracaoPressaoPendenteMs_ = duracao;
    }
    return;
  }

  acumularGiro(deslocamento);
}

Controles::Evento Controles::consumirEvento() {
  if (eventoBotaoPendente_ != TipoEvento::NENHUM) {
    const Evento evento{
        eventoBotaoPendente_,
        0,
        duracaoPressaoPendenteMs_
    };
    eventoBotaoPendente_ = TipoEvento::NENHUM;
    duracaoPressaoPendenteMs_ = 0;
    return evento;
  }
  if (botaoEmPressao_) return {};

  if (girosPendentes_ > 0) {
    girosPendentes_--;
    return {TipoEvento::GIRO, 1, 0};
  }
  if (girosPendentes_ < 0) {
    girosPendentes_++;
    return {TipoEvento::GIRO, -1, 0};
  }
  return {};
}

void Controles::acumularGiro(long deslocamento) {
  if (deslocamento == 0) return;
  const long acumulado = static_cast<long>(girosPendentes_) + deslocamento;
  girosPendentes_ = static_cast<int16_t>(constrain(
      acumulado,
      static_cast<long>(INT16_MIN),
      static_cast<long>(INT16_MAX)
  ));
}

void Controles::limparGirosPendentes() {
  girosPendentes_ = 0;
}

void IRAM_ATTR Controles::tratarInterrupcaoEncoder() {
  if (instanciaAtiva_ != nullptr) {
    instanciaAtiva_->encoder_.readEncoder_ISR();
  }
}

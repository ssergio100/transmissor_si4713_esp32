#include "radio_si4713.h"

#include <Wire.h>
#include <string.h>

#include "configuracao.h"
#include "diagnostico_i2c.h"

RadioSi4713::RadioSi4713()
    : radio_(Configuracao::PIN_RESET_SI4713) {}

bool RadioSi4713::iniciar() {
  resetFisico(true);

  const uint8_t resultadoEndereco1 = DiagnosticoI2c::consultarEndereco(
      SI4710_ADDR1
  );
  Serial.printf(
      "[SI4713] probe 0x%02X: %s (codigo=%u)\n",
      SI4710_ADDR1,
      DiagnosticoI2c::descreverResultado(resultadoEndereco1),
      resultadoEndereco1
  );
  if (resultadoEndereco1 == 0 && inicializarNoEndereco(SI4710_ADDR1)) {
    return true;
  }
  if (resultadoEndereco1 == 0) {
    Serial.println("[SI4713] 0x63 respondeu, mas a identificacao falhou");
  }

  const uint8_t resultadoEndereco0 = DiagnosticoI2c::consultarEndereco(
      SI4710_ADDR0
  );
  Serial.printf(
      "[SI4713] probe 0x%02X: %s (codigo=%u)\n",
      SI4710_ADDR0,
      DiagnosticoI2c::descreverResultado(resultadoEndereco0),
      resultadoEndereco0
  );
  if (resultadoEndereco0 == 0 && inicializarNoEndereco(SI4710_ADDR0)) {
    return true;
  }
  if (resultadoEndereco0 == 0) {
    Serial.println("[SI4713] 0x11 respondeu, mas a identificacao falhou");
  }

  telemetria_.si4713Disponivel = false;
  recuperacaoPendente_ = true;
  return false;
}

bool RadioSi4713::aplicar(const ConfiguracaoTransmissor& configuracao) {
  if (!telemetria_.si4713Disponivel
      || telemetria_.varreduraAtiva
      || !configuracao.valoresValidos()) {
    return false;
  }

  const bool primeiraAplicacao = !configurado_;
  const bool sintoniaMudou = primeiraAplicacao
      || configuracao.frequenciaKhz != configuracaoAplicada_.frequenciaKhz;
  const bool potenciaMudou = primeiraAplicacao
      || configuracao.potenciaDbuv != configuracaoAplicada_.potenciaDbuv
      || configuracao.transmissaoHabilitada
          != configuracaoAplicada_.transmissaoHabilitada
      || configuracao.capacitanciaAntena
          != configuracaoAplicada_.capacitanciaAntena;
  const bool baseRdsMudou = primeiraAplicacao
      || configuracao.rdsHabilitado != configuracaoAplicada_.rdsHabilitado
      || configuracao.rdsPi != configuracaoAplicada_.rdsPi;
  const bool nomeRdsMudou = primeiraAplicacao
      || strncmp(configuracao.rdsPs, configuracaoAplicada_.rdsPs, 8) != 0;
  const bool textoRdsMudou = primeiraAplicacao
      || strncmp(configuracao.rdsText, configuracaoAplicada_.rdsText, 32) != 0;

  if (sintoniaMudou
      && !confirmarOperacao(
          radio_.tuneFM(configuracao.frequenciaKhz),
          "sintonia",
          true
      )) {
    return false;
  }

  if (configuracao.rdsHabilitado) {
    if (baseRdsMudou
        && !confirmarOperacao(
            radio_.beginRDS(configuracao.rdsPi),
            "RDS",
            true
        )) {
      return false;
    }
    if ((baseRdsMudou || nomeRdsMudou)
        && !confirmarOperacao(
            radio_.setRDSstation(configuracao.rdsPs),
            "RDS PS",
            true
        )) {
      return false;
    }
    if ((baseRdsMudou || textoRdsMudou)
        && !confirmarOperacao(
            radio_.setRDSbuffer(configuracao.rdsText),
            "RDS RadioText",
            true
        )) {
      return false;
    }
  }

  if (primeiraAplicacao
      || configuracao.preEnfaseUs != configuracaoAplicada_.preEnfaseUs) {
    if (!confirmarOperacao(
            radio_.setProperty(
                SI4713_PROP_TX_PREEMPHASIS,
                configuracao.preEnfaseUs == 50 ? 1 : 0
            ),
            "pre-enfase",
            true
        )) {
      return false;
    }
  }

  if (primeiraAplicacao
      || configuracao.desvioAudioKhz != configuracaoAplicada_.desvioAudioKhz
      || baseRdsMudou) {
    if (!confirmarOperacao(
            radio_.setProperty(
                SI4713_PROP_TX_AUDIO_DEVIATION,
                static_cast<uint16_t>(configuracao.desvioAudioKhz) * 100
            ),
            "desvio de audio",
            true
        )) {
      return false;
    }
  }

  if (primeiraAplicacao
      || configuracao.audioMudo != configuracaoAplicada_.audioMudo) {
    if (!confirmarOperacao(
            radio_.setProperty(
                SI4713_PROP_TX_LINE_INPUT_MUTE,
                configuracao.audioMudo ? 0x0003 : 0x0000
            ),
            "mute de audio",
            true
        )) {
      return false;
    }
  }

  if (primeiraAplicacao
      || configuracao.estereo != configuracaoAplicada_.estereo
      || configuracao.rdsHabilitado != configuracaoAplicada_.rdsHabilitado) {
    uint16_t componentes = 0;
    if (configuracao.estereo) componentes |= 0x0003;
    if (configuracao.rdsHabilitado) componentes |= 0x0004;
    if (!confirmarOperacao(
            radio_.setProperty(SI4713_PROP_TX_COMPONENT_ENABLE, componentes),
            "componentes de audio",
            true
        )) {
      return false;
    }
  }

  if (potenciaMudou) {
    if (configuracao.transmissaoHabilitada) {
      if (!confirmarOperacao(
              radio_.setTXpower(
                  configuracao.potenciaDbuv,
                  configuracao.capacitanciaAntena
              ),
              "potencia TX",
              true
          )) {
        return false;
      }
      telemetria_.transmitindo = true;
    } else {
      if (!confirmarOperacao(
              radio_.setTXpower(0),
              "desligar TX",
              true
          )) {
        return false;
      }
      telemetria_.transmitindo = false;
    }
  }

  const bool estadoRfMudou = sintoniaMudou || potenciaMudou;
  if (estadoRfMudou
      && !confirmarOperacao(
          radio_.readTuneStatus(),
          "leitura de sintonia",
          true
      )) {
    return false;
  }
  configuracaoAplicada_ = configuracao;
  configurado_ = true;
  telemetria_.frequenciaEfetivaKhz = radio_.currFreq;
  telemetria_.potenciaEfetivaDbuv = radio_.currdBuV;
  telemetria_.capacitanciaEfetiva = radio_.currAntCap;
  if (estadoRfMudou) {
    atualizarEstadoRfConfirmado(configuracao, "aplicacao");
  }
  return true;
}

bool RadioSi4713::reiniciarRf() {
  if (!telemetria_.si4713Disponivel
      || telemetria_.varreduraAtiva
      || !configurado_) {
    return false;
  }
  Serial.println("[RF] Reiniciando sintonia e estagio de potencia");
  return aplicarEstadoRf(configuracaoAplicada_, "reinicio RF");
}

void RadioSi4713::processar() {
  const uint32_t agora = millis();

  if (!telemetria_.si4713Disponivel) {
    if (recuperacaoPendente_
        && agora - ultimaTentativaRecuperacaoMs_
            >= Configuracao::INTERVALO_RECUPERACAO_SI4713_MS) {
      ultimaTentativaRecuperacaoMs_ = agora;
      recuperar();
    }
    return;
  }

  if (telemetria_.varreduraAtiva) {
    medirProximaFrequencia();
    return;
  }

  if (recuperacaoRfPendente_
      && agora - ultimaTentativaRecuperacaoRfMs_
          >= Configuracao::INTERVALO_RECUPERACAO_SI4713_MS) {
    ultimaTentativaRecuperacaoRfMs_ = agora;
    aplicarEstadoRf(configuracaoAplicada_, "recuperacao automatica RF");
    return;
  }

  if (!telemetria_.transmitindo) return;

  if (agora - ultimaLeituraAudioMs_
      >= Configuracao::INTERVALO_LEITURA_AUDIO_MS) {
    ultimaLeituraAudioMs_ = agora;
    if (confirmarOperacao(radio_.readASQ(), "telemetria de audio", false)) {
      telemetria_.nivelAudioDbfs = radio_.currInLevel;
      telemetria_.asq = radio_.currASQ;
      telemetria_.sequenciaAudio++;
    }
  }

  if (agora - ultimaLeituraStatusMs_
      >= Configuracao::INTERVALO_STATUS_SI4713_MS) {
    ultimaLeituraStatusMs_ = agora;
    if (confirmarOperacao(
            radio_.readTuneStatus(),
            "telemetria de sintonia",
            false
        )) {
      telemetria_.frequenciaEfetivaKhz = radio_.currFreq;
      telemetria_.potenciaEfetivaDbuv = radio_.currdBuV;
      telemetria_.capacitanciaEfetiva = radio_.currAntCap;
      atualizarEstadoRfConfirmado(
          configuracaoAplicada_,
          "monitoramento"
      );
    }
  }
}

bool RadioSi4713::iniciarVarredura() {
  if (!telemetria_.si4713Disponivel || telemetria_.varreduraAtiva) return false;

  if (!confirmarOperacao(
          radio_.setTXpower(0),
          "pausar TX para scan",
          true
      )) {
    return false;
  }
  telemetria_.transmitindo = false;
  telemetria_.varreduraAtiva = true;
  telemetria_.varreduraConcluida = false;
  telemetria_.progressoVarredura = 0;
  indiceVarredura_ = 0;
  memset(medicoes_, 0, sizeof(medicoes_));
  return true;
}

bool RadioSi4713::cancelarVarredura() {
  if (!telemetria_.varreduraAtiva) return true;
  telemetria_.varreduraAtiva = false;
  telemetria_.varreduraConcluida = false;
  telemetria_.progressoVarredura = 0;
  indiceVarredura_ = 0;
  return restaurarAposVarredura();
}

size_t RadioSi4713::quantidadeMedicoes() const {
  return telemetria_.varreduraConcluida ? QUANTIDADE_MEDICOES : 0;
}

const MedicaoCanal& RadioSi4713::medicao(size_t indice) const {
  static const MedicaoCanal vazia;
  if (indice >= quantidadeMedicoes()) return vazia;
  return medicoes_[indice];
}

uint16_t RadioSi4713::melhorFrequencia() const {
  if (!telemetria_.varreduraConcluida) return 0;
  uint16_t melhor = 0;
  uint8_t menorRuido = UINT8_MAX;
  for (size_t indice = 0; indice < QUANTIDADE_MEDICOES; indice++) {
    if (medicoes_[indice].nivelRuido < menorRuido) {
      menorRuido = medicoes_[indice].nivelRuido;
      melhor = medicoes_[indice].frequenciaKhz;
    }
  }
  return melhor;
}

uint8_t RadioSi4713::melhorNivelRuido() const {
  if (!telemetria_.varreduraConcluida) return 0;
  uint8_t menorRuido = UINT8_MAX;
  for (size_t indice = 0; indice < QUANTIDADE_MEDICOES; indice++) {
    menorRuido = min(menorRuido, medicoes_[indice].nivelRuido);
  }
  return menorRuido;
}

const TelemetriaTransmissor& RadioSi4713::telemetria() const {
  return telemetria_;
}

uint8_t RadioSi4713::endereco() const {
  return endereco_;
}

bool RadioSi4713::recuperar() {
  const bool deveRestaurar = configurado_;
  telemetria_.si4713Disponivel = false;
  telemetria_.transmitindo = false;
  telemetria_.varreduraAtiva = false;
  telemetria_.recuperando = true;

  const uint8_t enderecoTentado = proximoEnderecoRecuperacao_;
  proximoEnderecoRecuperacao_ = enderecoTentado == SI4710_ADDR1
      ? SI4710_ADDR0
      : SI4710_ADDR1;

  const bool respondeu = enderecoResponde(enderecoTentado);
  if (!respondeu || !inicializarNoEndereco(enderecoTentado)) {
    tentativasRecuperacaoDesdeReset_++;
    if (tentativasRecuperacaoDesdeReset_ >= 2) {
      tentativasRecuperacaoDesdeReset_ = 0;
      resetFisico(false);
    }
    recuperacaoPendente_ = true;
    telemetria_.recuperando = true;
    return false;
  }

  configurado_ = false;
  if (deveRestaurar && !aplicar(configuracaoAplicada_)) {
    configurado_ = true;
    telemetria_.si4713Disponivel = false;
    recuperacaoPendente_ = true;
    telemetria_.recuperando = true;
    return false;
  }

  telemetria_.recuperando = false;
  falhasConsecutivas_ = 0;
  tentativasRecuperacaoDesdeReset_ = 0;
  telemetria_.recuperacoes++;
  ultimaLeituraAudioMs_ = millis();
  ultimaLeituraStatusMs_ = millis();
  return true;
}

bool RadioSi4713::confirmarOperacao(
    bool resultado,
    const char* operacao,
    bool critica
) {
  if (resultado) {
    falhasConsecutivas_ = 0;
    return true;
  }

  telemetria_.falhasComunicacao++;
  falhasConsecutivas_++;
  Serial.printf(
      "[SI4713] Falha em %s: %s (consecutivas=%u)\n",
      operacao,
      radio_.ultimaFalha(),
      falhasConsecutivas_
  );

  if (critica || falhasConsecutivas_ >= 3) {
    telemetria_.si4713Disponivel = false;
    telemetria_.recuperando = true;
    recuperacaoPendente_ = true;
    ultimaTentativaRecuperacaoMs_ =
        millis() - Configuracao::INTERVALO_RECUPERACAO_SI4713_MS;
  }
  return false;
}

bool RadioSi4713::inicializarNoEndereco(uint8_t endereco) {
  if (!radio_.begin(endereco)) return false;
  endereco_ = endereco;
  proximoEnderecoRecuperacao_ = endereco;
  telemetria_.si4713Disponivel = true;
  recuperacaoPendente_ = false;
  return true;
}

bool RadioSi4713::enderecoResponde(uint8_t endereco) {
  return DiagnosticoI2c::consultarEndereco(endereco) == 0;
}

void RadioSi4713::resetFisico(bool registrar) {
  if (registrar) {
    Serial.printf(
        "[SI4713] reset fisico no GPIO%d: HIGH > LOW > HIGH\n",
        Configuracao::PIN_RESET_SI4713
    );
  }
  pinMode(Configuracao::PIN_RESET_SI4713, OUTPUT);
  digitalWrite(Configuracao::PIN_RESET_SI4713, HIGH);
  delay(10);
  digitalWrite(Configuracao::PIN_RESET_SI4713, LOW);
  delay(10);
  digitalWrite(Configuracao::PIN_RESET_SI4713, HIGH);
  delay(20);
}

void RadioSi4713::medirProximaFrequencia() {
  if (indiceVarredura_ >= QUANTIDADE_MEDICOES) {
    telemetria_.varreduraAtiva = false;
    telemetria_.varreduraConcluida = true;
    telemetria_.progressoVarredura = 100;
    restaurarAposVarredura();
    return;
  }

  const uint16_t frequencia = Configuracao::FREQUENCIA_MINIMA_KHZ
      + static_cast<uint16_t>(indiceVarredura_)
          * Configuracao::PASSO_FREQUENCIA_KHZ;
  if (!confirmarOperacao(
          radio_.readTuneMeasure(frequencia),
          "medicao de canal",
          true
      )
      || !confirmarOperacao(
          radio_.readTuneStatus(),
          "resultado da medicao",
          true
      )) {
    telemetria_.varreduraAtiva = false;
    return;
  }
  medicoes_[indiceVarredura_] = {frequencia, radio_.currNoiseLevel};
  indiceVarredura_++;
  telemetria_.progressoVarredura = static_cast<uint8_t>(
      (indiceVarredura_ * 100U) / QUANTIDADE_MEDICOES
  );
}

bool RadioSi4713::restaurarAposVarredura() {
  return aplicarEstadoRf(
      configuracaoAplicada_,
      "restauracao apos scan"
  );
}

bool RadioSi4713::aplicarEstadoRf(
    const ConfiguracaoTransmissor& configuracao,
    const char* contexto
) {
  telemetria_.transmitindo = false;
  recuperacaoRfPendente_ = false;

  if (!confirmarOperacao(
          radio_.setTXpower(0),
          "desligar potencia antes de restaurar RF",
          true
      )) {
    return false;
  }
  if (!confirmarOperacao(
          radio_.tuneFM(configuracao.frequenciaKhz),
          "restaurar sintonia RF",
          true
      )) {
    return false;
  }
  if (configuracao.transmissaoHabilitada) {
    if (!confirmarOperacao(
            radio_.setTXpower(
                configuracao.potenciaDbuv,
                configuracao.capacitanciaAntena
            ),
            "restaurar potencia RF",
            true
        )) {
      return false;
    }
  }
  if (!confirmarOperacao(
          radio_.readTuneStatus(),
          "confirmar estado RF restaurado",
          true
      )) {
    return false;
  }
  telemetria_.frequenciaEfetivaKhz = radio_.currFreq;
  telemetria_.potenciaEfetivaDbuv = radio_.currdBuV;
  telemetria_.capacitanciaEfetiva = radio_.currAntCap;
  return atualizarEstadoRfConfirmado(configuracao, contexto);
}

bool RadioSi4713::atualizarEstadoRfConfirmado(
    const ConfiguracaoTransmissor& configuracao,
    const char* contexto
) {
  const bool frequenciaConfere =
      radio_.currFreq == configuracao.frequenciaKhz;
  const bool potenciaConfere = configuracao.transmissaoHabilitada
      ? radio_.currdBuV > 0
      : radio_.currdBuV == 0;
  const bool estadoConfere = frequenciaConfere && potenciaConfere;

  if (!estadoConfere) {
    telemetria_.transmitindo = false;
    telemetria_.inconsistenciasRf++;
    recuperacaoRfPendente_ = true;
    ultimaTentativaRecuperacaoRfMs_ = millis();
    Serial.printf(
        "[RF] Estado incoerente em %s: desejado=%u/%s retornado=%u/%u\n",
        contexto,
        configuracao.frequenciaKhz,
        configuracao.transmissaoHabilitada ? "ON" : "OFF",
        radio_.currFreq,
        radio_.currdBuV
    );
    return false;
  }

  telemetria_.transmitindo = configuracao.transmissaoHabilitada;
  recuperacaoRfPendente_ = false;
  return true;
}

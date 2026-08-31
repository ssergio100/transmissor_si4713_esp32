#include "radio_si4713.h"

#include <Wire.h>
#include <string.h>

#include "configuracao.h"
#include "diagnostico_i2c.h"

namespace {

volatile bool interrupcaoSi4713Pendente = false;

void IRAM_ATTR tratarInterrupcaoSi4713() {
  interrupcaoSi4713Pendente = true;
}

bool consumirInterrupcaoSi4713() {
  noInterrupts();
  const bool pendente = interrupcaoSi4713Pendente;
  interrupcaoSi4713Pendente = false;
  interrupts();
  return pendente;
}

void limparInterrupcaoSi4713() {
  noInterrupts();
  interrupcaoSi4713Pendente = false;
  interrupts();
}

}  // namespace

RadioSi4713::RadioSi4713()
    : radio_(Configuracao::PIN_RESET_SI4713) {}

bool RadioSi4713::iniciar() {
  // Nao habilitar pull-up aqui: GP2 define o modo do barramento durante reset.
  pinMode(Configuracao::PIN_INTERRUPCAO_SI4713, INPUT);
  detachInterrupt(digitalPinToInterrupt(Configuracao::PIN_INTERRUPCAO_SI4713));
  limparInterrupcaoSi4713();
  attachInterrupt(
      digitalPinToInterrupt(Configuracao::PIN_INTERRUPCAO_SI4713),
      tratarInterrupcaoSi4713,
      FALLING
  );
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
      || sintoniaMudou
      || configuracao.potenciaDbuv != configuracaoAplicada_.potenciaDbuv
      || configuracao.transmissaoHabilitada
          != configuracaoAplicada_.transmissaoHabilitada
      || configuracao.capacitanciaAntena
          != configuracaoAplicada_.capacitanciaAntena
      || transmissaoPausadaParaAjuste_;
  const bool baseRdsMudou = primeiraAplicacao
      || configuracao.rdsHabilitado != configuracaoAplicada_.rdsHabilitado
      || configuracao.rdsPi != configuracaoAplicada_.rdsPi;
  const bool nomeRdsMudou = primeiraAplicacao
      || strncmp(configuracao.rdsPs, configuracaoAplicada_.rdsPs, 8) != 0;
  const bool textoRdsMudou = primeiraAplicacao
      || strncmp(configuracao.rdsText, configuracaoAplicada_.rdsText, 32) != 0;
  const bool estadoRfMudou = sintoniaMudou || potenciaMudou;

  // Durante uma inicializacao ou reaplicacao RF, NO AR so pode voltar depois
  // que frequencia e potencia terminarem e forem consultadas no Si4713.
  if (estadoRfMudou) telemetria_.transmitindo = false;

  // Uma mudanca direta (por exemplo, pela API ou pela varredura) tambem deve
  // retirar a portadora antes de ressintonizar. No ajuste interativo a potencia
  // ja foi zerada no primeiro passo e permanece assim ate a confirmacao.
  if (sintoniaMudou && !transmissaoPausadaParaAjuste_) {
    if (!confirmarOperacao(
            radio_.setTXpower(0),
            "pausar TX para alterar frequencia",
            true
        )) {
      return false;
    }
    transmissaoPausadaParaAjuste_ = true;
  }

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
    } else {
      if (!confirmarOperacao(
              radio_.setTXpower(0),
              "desligar TX",
              true
          )) {
        return false;
      }
    }
  }

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
  if (estadoRfMudou
      && !atualizarEstadoRfConfirmado(configuracao, "aplicacao")) {
    return false;
  }
  transmissaoPausadaParaAjuste_ = false;
  return true;
}

bool RadioSi4713::iniciarAjusteFrequencia() {
  if (!telemetria_.si4713Disponivel
      || telemetria_.varreduraAtiva
      || !configurado_) {
    return false;
  }
  if (transmissaoPausadaParaAjuste_) return true;

  if (!confirmarOperacao(
          radio_.setTXpower(0),
          "pausar TX para ajuste de frequencia",
          true
      )) {
    return false;
  }
  transmissaoPausadaParaAjuste_ = true;
  telemetria_.transmitindo = false;
  telemetria_.potenciaEfetivaDbuv = 0;
  Serial.println("[RF] TX pausado para ajuste de frequencia");
  return true;
}

bool RadioSi4713::previsualizarFrequencia(uint16_t frequenciaKhz) {
  if (frequenciaKhz < Configuracao::FREQUENCIA_MINIMA_KHZ
      || frequenciaKhz > Configuracao::FREQUENCIA_MAXIMA_KHZ
      || frequenciaKhz % Configuracao::PASSO_FREQUENCIA_KHZ != 0
      || !iniciarAjusteFrequencia()) {
    return false;
  }

  if (!confirmarOperacao(
          radio_.tuneFM(frequenciaKhz),
          "passo do ajuste de frequencia",
          true
      )
      || !confirmarOperacao(
          radio_.readTuneStatus(),
          "confirmar passo do ajuste de frequencia",
          true
      )) {
    return false;
  }

  telemetria_.frequenciaEfetivaKhz = radio_.currFreq;
  telemetria_.potenciaEfetivaDbuv = radio_.currdBuV;
  telemetria_.capacitanciaEfetiva = radio_.currAntCap;
  if (radio_.currFreq != frequenciaKhz || radio_.currdBuV != 0) {
    telemetria_.inconsistenciasRf++;
    Serial.printf(
        "[RF] Passo de ajuste incoerente: desejado=%u retornado=%u/%u\n",
        frequenciaKhz,
        radio_.currFreq,
        radio_.currdBuV
    );
    return false;
  }
  return true;
}

bool RadioSi4713::reiniciarRf() {
  if (!telemetria_.si4713Disponivel
      || telemetria_.varreduraAtiva
      || transmissaoPausadaParaAjuste_
      || !configurado_) {
    return false;
  }
  Serial.println("[RF] Reiniciando sintonia e estagio de potencia");
  return aplicarEstadoRf(configuracaoAplicada_, "reinicio RF");
}

void RadioSi4713::setLeituraAudio(bool habilitar) {
  if (leituraAudioHabilitada_ == habilitar) return;
  if (!habilitar) {
    telemetria_.nivelAudioDbfs = -70;
    telemetria_.asq = 0;
  }
  leituraAudioHabilitada_ = habilitar;
  ultimaLeituraAudioMs_ = millis();
  Serial.printf(
      "[AUDIO] monitoramento via I2C %s\n",
      habilitar ? "habilitado (solicitado pela interface)" : "desabilitado (sem polling ASQ)"
  );
}

bool RadioSi4713::leituraAudioHabilitada() const {
  return leituraAudioHabilitada_;
}

bool RadioSi4713::reconhecerInterrupcao() {
  if (!telemetria_.alarmeInterrupcaoSi4713Pendente) return true;
  if (!telemetria_.si4713Disponivel) return false;

  // Limpa primeiro apenas a marca do ESP. Se a condicao continuar ativa depois
  // do INTACK, um novo pulso sera preservado pela ISR e virara outro episodio.
  limparInterrupcaoSi4713();
  if (!confirmarOperacao(
          radio_.readASQ(true),
          "reconhecimento da interrupcao ASQ",
          false
      )) {
    return false;
  }

  telemetria_.nivelAudioDbfs = radio_.currInLevel;
  telemetria_.asq = radio_.currASQ;
  telemetria_.ultimoEventoAsq = radio_.currASQ & 0x07;
  telemetria_.ultimaInterrupcaoLida = true;
  telemetria_.alarmeInterrupcaoSi4713Pendente = false;
  telemetria_.sequenciaAudio++;
  Serial.printf(
      "[SI4713-INT] evento #%lu reconhecido; GP2 rearmado\n",
      static_cast<unsigned long>(telemetria_.interrupcoesSi4713)
  );
  return true;
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

  if (consumirInterrupcaoSi4713()) {
    if (!leituraAudioHabilitada_
        && !telemetria_.alarmeInterrupcaoSi4713Pendente) {
      processarInterrupcao();
      if (!telemetria_.si4713Disponivel) return;
    }
  }

  if (leituraAudioHabilitada_
      && telemetria_.alarmeInterrupcaoSi4713Pendente
      && !reconhecerInterrupcao()) {
    if (!telemetria_.si4713Disponivel) return;
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

  // O polling de ASQ e sempre dirigido pela interface (nunca autonomo).
  // Desligado, so uma interrupcao GP2 pode provocar uma leitura pontual.
  if (!leituraAudioHabilitada_) {
    ultimaLeituraAudioMs_ = agora;
  } else if (agora - ultimaLeituraAudioMs_
      >= Configuracao::INTERVALO_LEITURA_AUDIO_MS) {
    ultimaLeituraAudioMs_ = agora;
    if (confirmarOperacao(
            // Em monitoramento continuo, cada amostra reconhece o intervalo
            // anterior. O bit OVERMOD passa a representar corte recente.
            radio_.readASQ(true),
            "telemetria de audio",
            false
        )) {
      telemetria_.nivelAudioDbfs = radio_.currInLevel;
      telemetria_.asq = radio_.currASQ;
      if (telemetria_.alarmeInterrupcaoSi4713Pendente) {
        telemetria_.alarmeInterrupcaoSi4713Pendente = false;
        Serial.println(
            "[SI4713-INT] alerta reconhecido pelo monitoramento continuo"
        );
      }
      telemetria_.sequenciaAudio++;
    }
  }

}

void RadioSi4713::processarInterrupcao() {
  telemetria_.interrupcoesSi4713++;
  telemetria_.ultimaInterrupcaoSi4713Ms = millis();
  telemetria_.ultimoEventoAsq = 0;
  telemetria_.ultimaInterrupcaoLida = false;
  telemetria_.alarmeInterrupcaoSi4713Pendente = true;

  // Le sem INTACK: o Si4713 mantem a causa travada e nao gera uma tempestade
  // de novos pulsos enquanto a mesma ocorrencia aguarda reconhecimento.
  if (!confirmarOperacao(radio_.readASQ(false), "interrupcao ASQ", false)) {
    Serial.printf(
        "[SI4713-INT] evento #%lu; falha ao ler a causa\n",
        static_cast<unsigned long>(telemetria_.interrupcoesSi4713)
    );
    return;
  }

  telemetria_.nivelAudioDbfs = radio_.currInLevel;
  telemetria_.asq = radio_.currASQ;
  telemetria_.ultimoEventoAsq = radio_.currASQ & 0x07;
  telemetria_.ultimaInterrupcaoLida = true;
  telemetria_.sequenciaAudio++;
  Serial.printf(
      "[SI4713-INT] evento #%lu: %s (ASQ=0x%02X, nivel=%d dBFS)\n",
      static_cast<unsigned long>(telemetria_.interrupcoesSi4713),
      nomeEventoSi4713(telemetria_),
      telemetria_.ultimoEventoAsq,
      telemetria_.nivelAudioDbfs
  );
}

bool RadioSi4713::iniciarVarredura() {
  if (!telemetria_.si4713Disponivel
      || telemetria_.varreduraAtiva
      || transmissaoPausadaParaAjuste_) {
    return false;
  }

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
  telemetria_.alarmeInterrupcaoSi4713Pendente = false;
  limparInterrupcaoSi4713();
  Serial.printf(
      "[SI4713] GP2/INT ativo no GPIO%d (sobremodulacao)\n",
      Configuracao::PIN_INTERRUPCAO_SI4713
  );
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
      ? radio_.currdBuV == configuracao.potenciaDbuv
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
  Serial.printf(
      "[RF] Sequencia concluida em %s: frequencia=%u potencia=%u TX=%s\n",
      contexto,
      radio_.currFreq,
      radio_.currdBuV,
      telemetria_.transmitindo ? "ON" : "OFF"
  );
  return true;
}

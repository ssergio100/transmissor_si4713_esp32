#include "receptor_rda5807.h"

#include <Wire.h>
#include <ctype.h>
#include <string.h>

#include "configuracao.h"

bool RadioRda5807Monitorado::lerStatusCompleto(uint16_t& status0b) {
  constexpr uint8_t QUANTIDADE_REGISTRADORES = 6;
  constexpr uint8_t QUANTIDADE_BYTES = QUANTIDADE_REGISTRADORES * 2;
  const uint8_t recebidos = Wire.requestFrom(
      I2C_ADDR_FULL_ACCESS,
      QUANTIDADE_BYTES
  );
  if (recebidos != QUANTIDADE_BYTES) {
    while (Wire.available()) Wire.read();
    return false;
  }

  for (uint8_t indice = 0; indice < QUANTIDADE_REGISTRADORES; indice++) {
    uint16_t valor = static_cast<uint16_t>(Wire.read()) << 8;
    valor |= static_cast<uint16_t>(Wire.read());
    shadowStatusRegisters[indice] = valor;
  }
  status0b = shadowStatusRegisters[SH_REG0B];
  return true;
}

bool ReceptorRda5807::iniciar(uint16_t frequenciaKhz) {
  if (!enderecoResponde()) {
    Serial.println("[RDA5807] ausente no endereco sequencial 0x10");
    return false;
  }

  // A biblioteca usa a instancia global Wire. Ela encontra aqui o mesmo
  // barramento ja iniciado em setup() para LCD e Si4713.
  radio_.setup();
  const uint16_t identificador = radio_.getDeviceId();
  if ((identificador >> 8) != IDENTIFICADOR_CHIP) {
    Serial.printf(
        "[RDA5807] identificacao invalida em 0x11: 0x%04X\n",
        identificador
    );
    return false;
  }

  // Replica a inicializacao ja validada no projeto nixie-clock-with-DFPlayer.
  radio_.setBass(false);
  radio_.setBand(2);  // Faixa mundial: 76 a 108 MHz.
  radio_.setSoftmute(false);
  radio_.setAudioOutputHighImpedance(false);
  radio_.setVolume(7);
  radio_.setMute(false);
  radio_.setMono(true);
  radio_.setRDS(true);
  radio_.setRdsFifo(true);
  radio_.clearRdsBuffer();
  telemetria_.disponivel = true;

  if (!sintonizar(frequenciaKhz)) {
    telemetria_.disponivel = false;
    return false;
  }

  atualizarStatus();
  ultimaLeituraMs_ = millis();
  Serial.printf(
      "[RDA5807] iniciado: id=0x%04X frequencia=%u "
      "RSSI=%u reg0B=0x%04X\n",
      identificador,
      telemetria_.frequenciaKhz,
      telemetria_.rssi,
      telemetria_.status0bBruto
  );
  return true;
}

bool ReceptorRda5807::sintonizar(uint16_t frequenciaKhz) {
  if (!telemetria_.disponivel) return false;
  if (frequenciaKhz < Configuracao::FREQUENCIA_MINIMA_KHZ
      || frequenciaKhz > Configuracao::FREQUENCIA_MAXIMA_KHZ
      || frequenciaKhz % Configuracao::PASSO_FREQUENCIA_KHZ != 0) {
    return false;
  }
  if (telemetria_.frequenciaKhz == frequenciaKhz) return true;

  // O projeto e a biblioteca representam a frequencia em unidades de 10 kHz:
  // 10170 corresponde a 101,70 MHz.
  radio_.setFrequency(frequenciaKhz);
  telemetria_.frequenciaKhz = radio_.getRealFrequency();
  radio_.clearRdsBuffer();
  limparRdsRecebido();
  Serial.printf(
      "[RDA5807] sintonia solicitada=%u confirmada=%u\n",
      frequenciaKhz,
      telemetria_.frequenciaKhz
  );
  return true;
}

void ReceptorRda5807::processar() {
  if (!telemetria_.disponivel) return;
  const uint32_t agora = millis();
  if (agora - ultimaLeituraMs_
      < Configuracao::INTERVALO_LEITURA_RDA_MS) {
    return;
  }

  ultimaLeituraMs_ = agora;
  atualizarStatus();
}

const TelemetriaReceptorRda5807& ReceptorRda5807::telemetria() const {
  return telemetria_;
}

bool ReceptorRda5807::enderecoResponde() const {
  Wire.beginTransmission(ENDERECO_SEQUENCIAL);
  return Wire.endTransmission() == 0;
}

void ReceptorRda5807::atualizarStatus() {
  // A mesma transferencia atualiza RSSI e os seis registradores que formam um
  // grupo RDS. Isso mantem uma unica rajada I2C por ciclo de monitoramento.
  uint16_t status = 0;
  telemetria_.leituraStatusValida =
      radio_.lerStatusCompleto(status);
  if (!telemetria_.leituraStatusValida) {
    telemetria_.rdsSincronizado = false;
    return;
  }

  telemetria_.status0bBruto = status;
  telemetria_.rssi = static_cast<uint8_t>((status >> 9) & 0x7F);
  telemetria_.fmTrue = ((status >> 8) & 0x01) != 0;
  atualizarRds();
}

void ReceptorRda5807::atualizarRds() {
  telemetria_.rdsSincronizado = radio_.getRdsSync();
  if (!telemetria_.rdsSincronizado || !radio_.hasRdsInfoAB()) return;

  const uint16_t grupo = radio_.getRdsGroupType();
  if (grupo == 0) {
    char* ps = radio_.getRdsStationName();
    if (copiarTextoRecebido(
            telemetria_.rdsPs,
            sizeof(telemetria_.rdsPs),
            ps,
            8
        )) {
      telemetria_.rdsPsValido = true;
    }
    return;
  }

  // O Si4713 envia RadioText em grupos 2A (quatro caracteres por grupo).
  if (grupo != 2 || radio_.getRdsVersionCode() != 0) return;

  const uint8_t flagAtual = radio_.getRdsFlagAB();
  if (flagTextoAbConhecida_ && flagAtual != flagTextoAb_) {
    radio_.clearRdsBuffer();
    telemetria_.rdsTexto[0] = '\0';
    telemetria_.rdsTextoValido = false;
  }
  flagTextoAb_ = flagAtual;
  flagTextoAbConhecida_ = true;

  char* texto = radio_.getRdsProgramInformation();
  if (copiarTextoRecebido(
          telemetria_.rdsTexto,
          sizeof(telemetria_.rdsTexto),
          texto,
          64
      )) {
    telemetria_.rdsTextoValido = true;
  }
}

void ReceptorRda5807::limparRdsRecebido() {
  telemetria_.rdsSincronizado = false;
  telemetria_.rdsPsValido = false;
  telemetria_.rdsTextoValido = false;
  telemetria_.rdsPs[0] = '\0';
  telemetria_.rdsTexto[0] = '\0';
  flagTextoAbConhecida_ = false;
  flagTextoAb_ = 0;
}

bool ReceptorRda5807::copiarTextoRecebido(
    char* destino,
    size_t tamanhoDestino,
    const char* origem,
    size_t maximoOrigem
) {
  if (destino == nullptr
      || tamanhoDestino == 0
      || origem == nullptr) {
    return false;
  }

  char recebido[65] = {};
  const size_t limite = min(maximoOrigem, sizeof(recebido) - 1);
  size_t tamanho = 0;
  while (tamanho < limite
      && origem[tamanho] != '\0'
      && origem[tamanho] != '\r') {
    const unsigned char caractere = origem[tamanho];
    recebido[tamanho] = isprint(caractere)
        ? static_cast<char>(caractere)
        : ' ';
    tamanho++;
  }
  while (tamanho > 0 && recebido[tamanho - 1] == ' ') tamanho--;
  recebido[tamanho] = '\0';
  if (tamanho == 0) return false;

  snprintf(destino, tamanhoDestino, "%s", recebido);
  return true;
}

#include "receptor_rda5807.h"

#include <Wire.h>

#include "configuracao.h"

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
  radio_.setRDS(false);  // Receptor usado somente para sintonia e RSSI.
  telemetria_.disponivel = true;

  if (!sintonizar(frequenciaKhz)) {
    telemetria_.disponivel = false;
    return false;
  }

  atualizarLeituraRssi();
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
      < Configuracao::INTERVALO_ATUALIZACAO_DISPLAY_MS) {
    return;
  }

  ultimaLeituraMs_ = agora;
  atualizarLeituraRssi();
}

const TelemetriaReceptorRda5807& ReceptorRda5807::telemetria() const {
  return telemetria_;
}

bool ReceptorRda5807::enderecoResponde() const {
  Wire.beginTransmission(ENDERECO_SEQUENCIAL);
  return Wire.endTransmission() == 0;
}

void ReceptorRda5807::atualizarLeituraRssi() {
  // O diagnostico inicial comparava a biblioteca com uma leitura direta e,
  // por isso, consultava duas vezes o mesmo registrador 0x0B. A comparacao ja
  // provou que os valores coincidem. Manter apenas a leitura direta reduz pela
  // metade os pulsos I2C periodicos junto ao caminho de audio analogico.
  uint16_t status = 0;
  telemetria_.leituraDiretaValida =
      lerRegistradorDireto(REGISTRADOR_STATUS_0B, status);
  if (!telemetria_.leituraDiretaValida) return;

  telemetria_.status0bBruto = status;
  telemetria_.rssi = static_cast<uint8_t>((status >> 9) & 0x7F);
}

bool ReceptorRda5807::lerRegistradorDireto(
    uint8_t registrador,
    uint16_t& valor
) const {
  Wire.beginTransmission(ENDERECO_DIRETO);
  Wire.write(registrador);
  if (Wire.endTransmission(false) != 0) return false;

  if (Wire.requestFrom(ENDERECO_DIRETO, static_cast<uint8_t>(2)) != 2) {
    while (Wire.available()) Wire.read();
    return false;
  }

  valor = static_cast<uint16_t>(Wire.read()) << 8;
  valor |= static_cast<uint16_t>(Wire.read());
  return true;
}

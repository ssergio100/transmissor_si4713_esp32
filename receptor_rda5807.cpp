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

  radio_.setBand(2);  // Faixa mundial: 76 a 108 MHz.
  radio_.setStep(100);
  radio_.setVolume(0);
  telemetria_.disponivel = true;

  if (!sintonizar(frequenciaKhz)) {
    telemetria_.disponivel = false;
    return false;
  }

  telemetria_.rssi = static_cast<uint8_t>(radio_.getRssi());
  ultimaLeituraMs_ = millis();
  Serial.printf(
      "[RDA5807] iniciado: id=0x%04X frequencia=%u RSSI=%u\n",
      identificador,
      telemetria_.frequenciaKhz,
      telemetria_.rssi
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
  telemetria_.frequenciaKhz = frequenciaKhz;
  Serial.printf("[RDA5807] sintonizado em %u\n", frequenciaKhz);
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
  telemetria_.rssi = static_cast<uint8_t>(radio_.getRssi());
}

const TelemetriaReceptorRda5807& ReceptorRda5807::telemetria() const {
  return telemetria_;
}

bool ReceptorRda5807::enderecoResponde() const {
  Wire.beginTransmission(ENDERECO_SEQUENCIAL);
  return Wire.endTransmission() == 0;
}

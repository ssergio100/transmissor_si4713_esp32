#pragma once

#include <Arduino.h>
#include <RDA5807.h>

struct TelemetriaReceptorRda5807 {
  bool disponivel = false;
  uint16_t frequenciaKhz = 0;
  uint8_t rssi = 0;
};

class ReceptorRda5807 {
 public:
  bool iniciar(uint16_t frequenciaKhz);
  bool sintonizar(uint16_t frequenciaKhz);
  void processar();

  const TelemetriaReceptorRda5807& telemetria() const;

 private:
  static constexpr uint8_t ENDERECO_SEQUENCIAL = 0x10;
  static constexpr uint8_t IDENTIFICADOR_CHIP = 0x58;

  bool enderecoResponde() const;

  RDA5807 radio_;
  TelemetriaReceptorRda5807 telemetria_;
  uint32_t ultimaLeituraMs_ = 0;
};

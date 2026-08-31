#pragma once

#include <Arduino.h>
#include <RDA5807.h>

struct TelemetriaReceptorRda5807 {
  bool disponivel = false;
  uint16_t frequenciaKhz = 0;
  uint8_t rssi = 0;
  uint8_t rssiBiblioteca = 0;
  bool leituraDiretaValida = false;
  uint16_t status0bBruto = 0;
};

class ReceptorRda5807 {
 public:
  bool iniciar(uint16_t frequenciaKhz);
  bool sintonizar(uint16_t frequenciaKhz);
  void processar();

  const TelemetriaReceptorRda5807& telemetria() const;

 private:
  static constexpr uint8_t ENDERECO_SEQUENCIAL = 0x10;
  static constexpr uint8_t ENDERECO_DIRETO = 0x11;
  static constexpr uint8_t IDENTIFICADOR_CHIP = 0x58;
  static constexpr uint8_t REGISTRADOR_STATUS_0B = 0x0B;

  bool enderecoResponde() const;
  void atualizarLeituraRssi();
  bool lerRegistradorDireto(uint8_t registrador, uint16_t& valor) const;

  RDA5807 radio_;
  TelemetriaReceptorRda5807 telemetria_;
  uint32_t ultimaLeituraMs_ = 0;
};

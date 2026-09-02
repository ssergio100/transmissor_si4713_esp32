#pragma once

#include <Arduino.h>
#include <RDA5807.h>

class RadioRda5807Monitorado : public RDA5807 {
 public:
  // Le 0x0A..0x0F em uma unica transacao e atualiza o cache usado pelas
  // funcoes RDS da biblioteca. Retorna tambem 0x0B para extrair o RSSI sem
  // uma segunda consulta I2C.
  bool lerStatusCompleto(uint16_t& status0b);
};

struct TelemetriaReceptorRda5807 {
  bool disponivel = false;
  uint16_t frequenciaKhz = 0;
  uint8_t rssi = 0;
  bool leituraStatusValida = false;
  bool fmTrue = false;
  bool rdsSincronizado = false;
  bool rdsPsValido = false;
  bool rdsTextoValido = false;
  uint16_t status0bBruto = 0;
  char rdsPs[9] = {};
  char rdsTexto[65] = {};
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
  void atualizarStatus();
  void atualizarRds();
  void limparRdsRecebido();
  static bool copiarTextoRecebido(
      char* destino,
      size_t tamanhoDestino,
      const char* origem,
      size_t maximoOrigem
  );

  RadioRda5807Monitorado radio_;
  TelemetriaReceptorRda5807 telemetria_;
  uint32_t ultimaLeituraMs_ = 0;
  bool flagTextoAbConhecida_ = false;
  uint8_t flagTextoAb_ = 0;
};

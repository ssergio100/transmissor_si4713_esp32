#pragma once

#include <Arduino.h>
#include <Wire.h>

constexpr uint8_t SI4710_ADDR0 = 0x11;
constexpr uint8_t SI4710_ADDR1 = 0x63;

constexpr uint16_t SI4713_PROP_GPO_IEN = 0x0001;
constexpr uint16_t SI4713_PROP_REFCLK_FREQ = 0x0201;
constexpr uint16_t SI4713_PROP_TX_COMPONENT_ENABLE = 0x2100;
constexpr uint16_t SI4713_PROP_TX_AUDIO_DEVIATION = 0x2101;
constexpr uint16_t SI4713_PROP_TX_PILOT_DEVIATION = 0x2102;
constexpr uint16_t SI4713_PROP_TX_RDS_DEVIATION = 0x2103;
constexpr uint16_t SI4713_PROP_TX_LINE_INPUT_MUTE = 0x2105;
constexpr uint16_t SI4713_PROP_TX_PREEMPHASIS = 0x2106;
constexpr uint16_t SI4713_PROP_TX_PILOT_FREQUENCY = 0x2107;
constexpr uint16_t SI4713_PROP_TX_ACOMP_ENABLE = 0x2200;
constexpr uint16_t SI4713_PROP_TX_ACOMP_GAIN = 0x2204;
constexpr uint16_t SI4713_PROP_TX_ASQ_INTERRUPT_SOURCE = 0x2300;
constexpr uint16_t SI4713_PROP_TX_RDS_INTERRUPT_SOURCE = 0x2C00;
constexpr uint16_t SI4713_PROP_TX_RDS_PI = 0x2C01;
constexpr uint16_t SI4713_PROP_TX_RDS_PS_MIX = 0x2C02;
constexpr uint16_t SI4713_PROP_TX_RDS_PS_MISC = 0x2C03;
constexpr uint16_t SI4713_PROP_TX_RDS_PS_REPEAT_COUNT = 0x2C04;
constexpr uint16_t SI4713_PROP_TX_RDS_MESSAGE_COUNT = 0x2C05;
constexpr uint16_t SI4713_PROP_TX_RDS_PS_AF = 0x2C06;
constexpr uint16_t SI4713_PROP_TX_RDS_FIFO_SIZE = 0x2C07;

// Implementa somente os comandos usados pelo projeto. Diferente da biblioteca
// Adafruit original, toda espera por CTS/STC possui limite de tempo.
class Si4713Seguro {
 public:
  explicit Si4713Seguro(int8_t pinoReset = -1);

  bool begin(uint8_t endereco, TwoWire* wire = &Wire);
  bool tuneFM(uint16_t frequenciaKhz);
  bool setTXpower(uint8_t potenciaDbuv, uint8_t capacitanciaAntena = 0);
  bool readTuneMeasure(uint16_t frequenciaKhz);
  bool readTuneStatus();
  bool readASQ(bool reconhecerInterrupcao = false);
  bool setProperty(uint16_t propriedade, uint16_t valor);
  bool getProperty(uint16_t propriedade, uint16_t& valor);
  bool beginRDS(uint16_t pi);
  bool setRDSstation(const char* texto);
  bool setRDSbuffer(const char* texto);

  const char* ultimaFalha() const;

  uint16_t currFreq = 0;
  uint8_t currdBuV = 0;
  uint8_t currAntCap = 0;
  uint8_t currNoiseLevel = 0;
  uint8_t currASQ = 0;
  int8_t currInLevel = -70;

 private:
  static constexpr uint8_t STATUS_CTS = 0x80;
  static constexpr uint8_t STATUS_ERR = 0x40;
  static constexpr uint8_t STATUS_STC = 0x01;

  // Baseados nos limites do driver Si4713 do kernel Linux. O CTS recebe
  // margem para o escalonamento de 1 ms da plataforma Arduino.
  static constexpr uint32_t LIMITE_CTS_US = 5000;
  static constexpr uint32_t LIMITE_POWER_UP_US = 200000;
  static constexpr uint32_t LIMITE_TUNE_US = 110000;
  static constexpr uint32_t LIMITE_POWER_US = 30000;
  static constexpr uint32_t TEMPO_ESTABILIZAR_PROPRIEDADE_MS = 20;

  void reset();
  bool powerUp();
  bool getRev(uint8_t& revisao);
  bool getIntStatus(uint8_t& status);
  bool aguardarStc(uint32_t limiteUs);
  bool concluirOperacaoStc(uint32_t limiteUs);
  bool executarComando(
      const uint8_t* comando,
      size_t tamanhoComando,
      uint8_t* resposta,
      size_t tamanhoResposta,
      uint32_t limiteUs = LIMITE_CTS_US
  );
  bool escrever(const uint8_t* dados, size_t tamanho);
  bool ler(uint8_t* dados, size_t tamanho);
  bool falhar(const char* detalhe);

  bool rdsTextoAb_ = false;
  int8_t pinoReset_;
  uint8_t endereco_ = 0;
  TwoWire* wire_ = nullptr;
  const char* ultimaFalha_ = "nenhuma";
};

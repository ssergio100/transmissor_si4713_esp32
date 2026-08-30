#include "si4713_seguro.h"

#include <string.h>

namespace {

constexpr uint8_t CMD_POWER_UP = 0x01;
constexpr uint8_t CMD_GET_REV = 0x10;
constexpr uint8_t CMD_SET_PROPERTY = 0x12;
constexpr uint8_t CMD_GET_INT_STATUS = 0x14;
constexpr uint8_t CMD_TX_TUNE_FREQ = 0x30;
constexpr uint8_t CMD_TX_TUNE_POWER = 0x31;
constexpr uint8_t CMD_TX_TUNE_MEASURE = 0x32;
constexpr uint8_t CMD_TX_TUNE_STATUS = 0x33;
constexpr uint8_t CMD_TX_ASQ_STATUS = 0x34;
constexpr uint8_t CMD_TX_RDS_BUFF = 0x35;
constexpr uint8_t CMD_TX_RDS_PS = 0x36;
constexpr uint8_t NUMERO_PRODUTO_SI4713 = 13;

}  // namespace

Si4713Seguro::Si4713Seguro(int8_t pinoReset)
    : pinoReset_(pinoReset) {}

bool Si4713Seguro::begin(uint8_t endereco, TwoWire* wire) {
  endereco_ = endereco;
  wire_ = wire;
  ultimaFalha_ = "nenhuma";
  reset();

  if (!powerUp()) return false;
  uint8_t revisao = 0;
  if (!getRev(revisao)) return false;
  if (revisao != NUMERO_PRODUTO_SI4713) {
    return falhar("identificacao diferente de Si4713");
  }
  return true;
}

bool Si4713Seguro::tuneFM(uint16_t frequenciaKhz) {
  const uint8_t comando[] = {
      CMD_TX_TUNE_FREQ,
      0,
      static_cast<uint8_t>(frequenciaKhz >> 8),
      static_cast<uint8_t>(frequenciaKhz)
  };
  uint8_t status = 0;
  return executarComando(comando, sizeof(comando), &status, 1)
      && aguardarStc(LIMITE_TUNE_US);
}

bool Si4713Seguro::setTXpower(
    uint8_t potenciaDbuv,
    uint8_t capacitanciaAntena
) {
  const uint8_t comando[] = {
      CMD_TX_TUNE_POWER,
      0,
      0,
      potenciaDbuv,
      capacitanciaAntena
  };
  uint8_t status = 0;
  return executarComando(comando, sizeof(comando), &status, 1)
      && aguardarStc(LIMITE_POWER_US);
}

bool Si4713Seguro::readTuneMeasure(uint16_t frequenciaKhz) {
  if (frequenciaKhz % 5 != 0) frequenciaKhz -= frequenciaKhz % 5;
  const uint8_t comando[] = {
      CMD_TX_TUNE_MEASURE,
      0,
      static_cast<uint8_t>(frequenciaKhz >> 8),
      static_cast<uint8_t>(frequenciaKhz),
      0
  };
  uint8_t status = 0;
  return executarComando(comando, sizeof(comando), &status, 1)
      && aguardarStc(LIMITE_TUNE_US);
}

bool Si4713Seguro::readTuneStatus() {
  const uint8_t comando[] = {CMD_TX_TUNE_STATUS, 1};
  uint8_t resposta[8];
  if (!executarComando(comando, sizeof(comando), resposta, sizeof(resposta))) {
    return false;
  }
  currFreq = (static_cast<uint16_t>(resposta[2]) << 8) | resposta[3];
  currdBuV = resposta[5];
  currAntCap = resposta[6];
  currNoiseLevel = resposta[7];
  return true;
}

bool Si4713Seguro::readASQ() {
  const uint8_t comando[] = {CMD_TX_ASQ_STATUS, 1};
  uint8_t resposta[5];
  if (!executarComando(comando, sizeof(comando), resposta, sizeof(resposta))) {
    return false;
  }
  currASQ = resposta[1];
  currInLevel = static_cast<int8_t>(resposta[4]);
  return true;
}

bool Si4713Seguro::setProperty(uint16_t propriedade, uint16_t valor) {
  const uint8_t comando[] = {
      CMD_SET_PROPERTY,
      0,
      static_cast<uint8_t>(propriedade >> 8),
      static_cast<uint8_t>(propriedade),
      static_cast<uint8_t>(valor >> 8),
      static_cast<uint8_t>(valor)
  };
  uint8_t status = 0;
  if (!executarComando(comando, sizeof(comando), &status, 1)) return false;
  delay(TEMPO_ESTABILIZAR_PROPRIEDADE_MS);
  return true;
}

bool Si4713Seguro::beginRDS(uint16_t pi) {
  return setProperty(SI4713_PROP_TX_AUDIO_DEVIATION, 6625)
      && setProperty(SI4713_PROP_TX_RDS_DEVIATION, 200)
      && setProperty(SI4713_PROP_TX_RDS_INTERRUPT_SOURCE, 1)
      && setProperty(SI4713_PROP_TX_RDS_PI, pi)
      && setProperty(SI4713_PROP_TX_RDS_PS_MIX, 3)
      && setProperty(SI4713_PROP_TX_RDS_PS_MISC, 0x1008)
      && setProperty(SI4713_PROP_TX_RDS_PS_REPEAT_COUNT, 3)
      && setProperty(SI4713_PROP_TX_RDS_MESSAGE_COUNT, 1)
      && setProperty(SI4713_PROP_TX_RDS_PS_AF, 0xE0E0)
      && setProperty(SI4713_PROP_TX_RDS_FIFO_SIZE, 0)
      && setProperty(SI4713_PROP_TX_COMPONENT_ENABLE, 7);
}

bool Si4713Seguro::setRDSstation(const char* texto) {
  char preenchido[8];
  memset(preenchido, ' ', sizeof(preenchido));
  memcpy(preenchido, texto, min(strlen(texto), sizeof(preenchido)));
  for (uint8_t bloco = 0; bloco < 2; bloco++) {
    uint8_t comando[6] = {CMD_TX_RDS_PS, bloco, 0, 0, 0, 0};
    memcpy(comando + 2, preenchido + bloco * 4, 4);
    uint8_t status = 0;
    if (!executarComando(comando, sizeof(comando), &status, 1)) return false;
  }
  return true;
}

bool Si4713Seguro::setRDSbuffer(const char* texto) {
  char preenchido[32];
  memset(preenchido, ' ', sizeof(preenchido));
  memcpy(preenchido, texto, min(strlen(texto), sizeof(preenchido)));
  for (uint8_t bloco = 0; bloco < 8; bloco++) {
    uint8_t comando[8] = {
        CMD_TX_RDS_BUFF,
        static_cast<uint8_t>(bloco == 0 ? 0x06 : 0x04),
        0x20,
        bloco,
        0,
        0,
        0,
        0
    };
    memcpy(comando + 4, preenchido + bloco * 4, 4);
    uint8_t status = 0;
    if (!executarComando(comando, sizeof(comando), &status, 1)) return false;
  }
  return true;
}

const char* Si4713Seguro::ultimaFalha() const {
  return ultimaFalha_;
}

void Si4713Seguro::reset() {
  if (pinoReset_ < 0) return;
  pinMode(pinoReset_, OUTPUT);
  digitalWrite(pinoReset_, HIGH);
  delay(10);
  digitalWrite(pinoReset_, LOW);
  delay(10);
  digitalWrite(pinoReset_, HIGH);
  delay(20);
}

bool Si4713Seguro::powerUp() {
  const uint8_t comando[] = {CMD_POWER_UP, 0x12, 0x50};
  uint8_t status = 0;
  if (!executarComando(
          comando,
          sizeof(comando),
          &status,
          1,
          LIMITE_POWER_UP_US
      )) {
    return false;
  }
  return setProperty(SI4713_PROP_REFCLK_FREQ, 32768)
      && setProperty(SI4713_PROP_TX_PREEMPHASIS, 0)
      && setProperty(SI4713_PROP_TX_ACOMP_GAIN, 10)
      && setProperty(SI4713_PROP_TX_ACOMP_ENABLE, 0);
}

bool Si4713Seguro::getRev(uint8_t& revisao) {
  const uint8_t comando[] = {CMD_GET_REV, 0};
  uint8_t resposta[9];
  if (!executarComando(comando, sizeof(comando), resposta, sizeof(resposta))) {
    return false;
  }
  revisao = resposta[1];
  return true;
}

bool Si4713Seguro::getIntStatus(uint8_t& status) {
  const uint8_t comando[] = {CMD_GET_INT_STATUS};
  return executarComando(comando, sizeof(comando), &status, 1);
}

bool Si4713Seguro::aguardarStc(uint32_t limiteUs) {
  const uint32_t inicio = micros();
  do {
    uint8_t status = 0;
    if (!getIntStatus(status)) return false;
    if ((status & STATUS_STC) != 0) return true;
    delay(3);
  } while (static_cast<uint32_t>(micros() - inicio) <= limiteUs);
  return falhar("timeout aguardando STC");
}

bool Si4713Seguro::executarComando(
    const uint8_t* comando,
    size_t tamanhoComando,
    uint8_t* resposta,
    size_t tamanhoResposta,
    uint32_t limiteUs
) {
  if (wire_ == nullptr || endereco_ == 0) return falhar("I2C nao inicializado");
  if (!escrever(comando, tamanhoComando)) return false;

  const uint32_t inicio = micros();
  do {
    if (ler(resposta, tamanhoResposta)) {
      if ((resposta[0] & STATUS_ERR) != 0) {
        return falhar("Si4713 retornou ERR");
      }
      if ((resposta[0] & STATUS_CTS) != 0) {
        ultimaFalha_ = "nenhuma";
        return true;
      }
    }
    delay(1);
  } while (static_cast<uint32_t>(micros() - inicio) <= limiteUs);
  return falhar("timeout aguardando CTS");
}

bool Si4713Seguro::escrever(const uint8_t* dados, size_t tamanho) {
  wire_->beginTransmission(endereco_);
  if (wire_->write(dados, tamanho) != tamanho) {
    wire_->endTransmission();
    return falhar("buffer I2C insuficiente");
  }
  const uint8_t resultado = wire_->endTransmission();
  if (resultado != 0) {
    return falhar("falha ao escrever no I2C");
  }
  return true;
}

bool Si4713Seguro::ler(uint8_t* dados, size_t tamanho) {
  const size_t recebidos = wire_->requestFrom(endereco_, tamanho);
  if (recebidos != tamanho) {
    while (wire_->available()) wire_->read();
    ultimaFalha_ = "falha ao ler do I2C";
    return false;
  }
  for (size_t indice = 0; indice < tamanho; indice++) {
    dados[indice] = static_cast<uint8_t>(wire_->read());
  }
  return true;
}

bool Si4713Seguro::falhar(const char* detalhe) {
  ultimaFalha_ = detalhe;
  return false;
}

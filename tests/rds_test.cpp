#include <assert.h>
#include <stdio.h>
#include <string>
#include "si4713_seguro.h"

int main() {
  // Driver real com I2C simulado: verifica propriedades e comandos enviados.
  Si4713Seguro tx;
  assert(tx.begin(0x63));
  Wire.comandos.clear();
  assert(tx.beginRDS(0x4713));
  for (const auto& cmd : Wire.comandos) {
    assert(cmd[0] == 0x12);
    const unsigned prop = (cmd[2] << 8) | cmd[3];
    // A base RDS nao deve sobrescrever estereo/RDS ativo nem desvio de audio.
    assert(prop != SI4713_PROP_TX_COMPONENT_ENABLE);
    assert(prop != SI4713_PROP_TX_AUDIO_DEVIATION);
  }
  int flagAnterior = -1;
  uint16_t valor = 0;
  assert(tx.getProperty(SI4713_PROP_TX_RDS_PI, valor) && valor == 0x4713);
  assert(tx.getProperty(SI4713_PROP_TX_RDS_DEVIATION, valor) && valor == 200);
  assert(tx.setProperty(SI4713_PROP_TX_COMPONENT_ENABLE, 4));
  assert(tx.getProperty(SI4713_PROP_TX_COMPONENT_ENABLE, valor) && valor == 4);
  for (const std::string& texto : {std::string(32, 'X'), std::string("OLA"), std::string("")}) {
    Wire.comandos.clear();
    assert(tx.setRDSbuffer(texto.c_str()));
    assert(Wire.comandos.size() == (texto.size() + 4) / 4);
    const int flag = Wire.comandos[0][3] & 0x10;
    assert(flag != flagAnterior);
    flagAnterior = flag;
    std::string payload;
    unsigned segmento = 0;
    for (const auto& cmd : Wire.comandos) {
      assert(cmd.size() == 8 && cmd[0] == 0x35);
      assert(cmd[1] == (segmento == 0 ? 0x06 : 0x04));
      assert((cmd[3] & 0x0F) == segmento++);
      assert(cmd[2] == 0x20);
      for (unsigned i = 4; i < 8; ++i) payload += static_cast<char>(cmd[i]);
    }
    assert(payload.substr(0, texto.size()) == texto);
    assert(payload[texto.size()] == '\r');
    for (size_t i = texto.size() + 1; i < payload.size(); ++i) assert(payload[i] == ' ');
  }
  puts("RDS TX: propriedades, texto, terminador CR e alternancia A/B passaram");
}

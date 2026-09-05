#pragma once
#include <vector>
#include <map>
#include <stdint.h>
#include <stddef.h>
class TwoWire {
 public:
  std::vector<std::vector<uint8_t>> comandos;
  void beginTransmission(uint8_t) { atual.clear(); }
  size_t write(const uint8_t* dados, size_t n) {
    atual.assign(dados, dados + n); return n;
  }
  uint8_t endTransmission() {
    comandos.push_back(atual);
    if (atual.size() == 6 && atual[0] == 0x12) {
      propriedades[(atual[2] << 8) | atual[3]] = (atual[4] << 8) | atual[5];
    }
    return 0;
  }
  size_t requestFrom(uint8_t, size_t n) { indice = 0; restante = n; return n; }
  int available() { return restante; }
  int read() {
    --restante;
    const size_t pos = indice++;
    if (pos == 0) return 0x80;
    if (atual[0] == 0x10 && pos == 1) return 13; // GET_REV: Si4713
    if (atual[0] == 0x13 && pos >= 2) {
      const uint16_t valor = propriedades[(atual[2] << 8) | atual[3]];
      return pos == 2 ? valor >> 8 : valor & 0xFF;
    }
    return 0;
  }
 private:
  std::vector<uint8_t> atual;
  std::map<uint16_t, uint16_t> propriedades;
  size_t indice = 0, restante = 0;
};
inline TwoWire Wire;

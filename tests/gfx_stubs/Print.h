#pragma once
#include <stddef.h>
#include <stdint.h>
// Somente a saida de caracteres e simulada. O desenho usa Adafruit_GFX real.
class Print {
 public:
  virtual ~Print() = default;
  virtual size_t write(uint8_t caractere) = 0;
  size_t print(const char* texto) {
    size_t quantidade = 0;
    while (*texto) quantidade += write(static_cast<uint8_t>(*texto++));
    return quantidade;
  }
};

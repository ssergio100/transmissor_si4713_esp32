#pragma once

#include <Arduino.h>

class FrasesRds {
 public:
  static constexpr size_t MAXIMO_FRASES = 12;
  static constexpr size_t MAXIMO_CARACTERES = 32;

  bool carregar();
  bool substituir(const String* frases, size_t quantidade);
  bool salvar() const;
  size_t quantidade() const;
  const String& frase(size_t indice) const;

 private:
  String frases_[MAXIMO_FRASES];
  size_t quantidade_ = 0;
};

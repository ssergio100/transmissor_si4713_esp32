#pragma once

#include <Arduino.h>

namespace DiagnosticoI2c {

uint8_t consultarEndereco(uint8_t endereco);
const char* descreverResultado(uint8_t resultado);
void executarNoBoot();

}  // namespace DiagnosticoI2c

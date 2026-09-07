#pragma once
#include "../stubs/Arduino.h"
#include <cstdlib>
#include <cstring>
#include <string>
using String = std::string;
class __FlashStringHelper;
#define PROGMEM
#define pgm_read_byte(endereco) (*reinterpret_cast<const uint8_t*>(endereco))
#include <cmath>
inline float radians(float graus) { return graus * 3.14159265358979323846f / 180.0f; }

#pragma once
#include <algorithm>
#include <stddef.h>
#include <stdint.h>
using std::min;
constexpr int OUTPUT = 1, HIGH = 1, LOW = 0;
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline uint32_t tempoTeste = 0;
inline void delay(unsigned ms) { tempoTeste += ms * 1000; }
inline uint32_t micros() { return ++tempoTeste; }

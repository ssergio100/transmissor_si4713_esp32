#pragma once

#include "modelos.h"

namespace Persistencia {

bool carregar(ConfiguracaoTransmissor& configuracao);
bool salvar(const ConfiguracaoTransmissor& configuracao);
bool carregarFrequencia(uint16_t& frequenciaKhz);
bool salvarFrequencia(uint16_t frequenciaKhz);
bool salvarRepousoDisplay(uint16_t segundos);
bool salvarPassoFrequencia(uint8_t passoKhz);
bool salvarVolumeMonitor(uint8_t volume);

}  // namespace Persistencia

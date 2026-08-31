#pragma once

#include "modelos.h"

namespace Persistencia {

bool carregar(ConfiguracaoTransmissor& configuracao);
bool salvar(const ConfiguracaoTransmissor& configuracao);
bool carregarFrequencia(uint16_t& frequenciaKhz);
bool salvarFrequencia(uint16_t frequenciaKhz);
bool apagar();

}  // namespace Persistencia

#pragma once

#include "modelos.h"

namespace Persistencia {

bool carregar(ConfiguracaoTransmissor& configuracao);
bool salvar(const ConfiguracaoTransmissor& configuracao);
bool apagar();

}  // namespace Persistencia

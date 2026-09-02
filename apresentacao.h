#pragma once

#include "estado_painel.h"

class Menu;
class Transmissor;

namespace Apresentacao {

EstadoPainel gerar(const Menu& menu, const Transmissor& transmissor);

}  // namespace Apresentacao

#pragma once

#include <Arduino.h>

namespace Rede {

void iniciar();
void processar();
void abrirPortalConfiguracao();
bool conectada();
bool portalAtivo();
String enderecoIp();

}  // namespace Rede

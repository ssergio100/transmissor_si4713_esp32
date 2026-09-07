#include <cassert>
#include <cstdio>
#include "modelos.h"

int main() {
  const uint8_t isolados[] = {
      ConfiguracaoTransmissor::APENAS_L, ConfiguracaoTransmissor::APENAS_R};
  const uint8_t normais[] = {0, 3, 7, 1, 2};
  for (bool rds : {false, true}) {
    for (uint8_t isolado : isolados) {
      for (uint8_t normal : normais) {
        ConfiguracaoTransmissor c;
        c.rdsHabilitado = rds;
        c.audioMudo = true;
        const ConfiguracaoTransmissor antes = c;
        c.selecionarMultiplex(isolado);
        assert(c.valoresValidos());
        assert(c.modoAudio() == isolado);
        assert(c.componentesMultiplex() == 0x0003);
        assert(c.muteEntradas() == (isolado == ConfiguracaoTransmissor::APENAS_L ? 1 : 2));
        assert(c.rdsHabilitado == antes.rdsHabilitado);
        assert(c.rdsPi == antes.rdsPi);
        assert(c.frequenciaKhz == antes.frequenciaKhz);
        assert(c.potenciaDbuv == antes.potenciaDbuv);
        assert(c.capacitanciaAntena == antes.capacitanciaAntena);
        assert(c.preEnfaseUs == antes.preEnfaseUs);
        assert(c.desvioAudioKhz == antes.desvioAudioKhz);
        assert(c.volumeMonitor == antes.volumeMonitor);
        c.selecionarMultiplex(normal);
        assert(c.componentesMultiplex() == normal);
        assert(c.muteEntradas() == 0x0000);
        assert(!c.audioMudo);
      }
    }
  }
  ConfiguracaoTransmissor c;
  c.selecionarMultiplex(ConfiguracaoTransmissor::APENAS_L);
  c.selecionarMultiplex(ConfiguracaoTransmissor::APENAS_R);
  assert(c.componentesMultiplex() == 3 && c.muteEntradas() == 2);
  c.selecionarMultiplex(ConfiguracaoTransmissor::APENAS_L);
  assert(c.componentesMultiplex() == 3 && c.muteEntradas() == 1);
  puts("Multiplex: isolamento L/R, preservacao dos ajustes e restauracao passaram.");
}

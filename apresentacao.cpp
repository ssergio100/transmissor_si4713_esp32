#include "apresentacao.h"

#include <string.h>

#include "configuracao.h"
#include "menu.h"
#include "transmissor.h"

EstadoPainel Apresentacao::gerar(
    const Menu& menu,
    const Transmissor& transmissor
) {
  EstadoPainel estado;
  const ConfiguracaoTransmissor& configuracao = transmissor.configuracao();
  const TelemetriaTransmissor& telemetria = transmissor.telemetria();
  const TelemetriaReceptorRda5807& receptor =
      transmissor.telemetriaReceptor();

  estado.navegacao = menu.navegacao();

  estado.rf.frequenciaKhz = configuracao.frequenciaKhz;
  estado.rf.potenciaDbuv = configuracao.potenciaDbuv;
  estado.rf.capacitanciaAntena = configuracao.capacitanciaAntena;
  estado.rf.transmissaoHabilitada = configuracao.transmissaoHabilitada;
  estado.rf.transmitindo = telemetria.transmitindo;

  estado.audio.preEnfaseUs = configuracao.preEnfaseUs;
  estado.audio.desvioKhz = configuracao.desvioAudioKhz;
  estado.audio.nivelDbfs = telemetria.nivelAudioDbfs;
  estado.audio.asq = telemetria.asq;
  estado.audio.estereo = configuracao.estereo;
  estado.audio.componentesMultiplex = configuracao.componentesMultiplex();
  estado.audio.modoAudio = configuracao.modoAudio();
  estado.audio.mudo = configuracao.muteEntradas() == 3;

  estado.rds.pi = configuracao.rdsPi;
  estado.rds.habilitado = configuracao.rdsHabilitado;
  strncpy(estado.rds.textoAtual, transmissor.radioTextAtual(),
          sizeof(estado.rds.textoAtual) - 1);

  estado.receptor.rssi = receptor.rssi;
  estado.receptor.disponivel = receptor.disponivel;
  estado.receptor.leituraRssiValida = receptor.leituraDiretaValida;

  estado.varredura.melhorFrequenciaKhz = transmissor.melhorFrequencia();
  estado.varredura.progresso = telemetria.progressoVarredura;
  estado.varredura.ativa = telemetria.varreduraAtiva;

  estado.sistema.si4713Disponivel = telemetria.si4713Disponivel;
  estado.sistema.repousoDisplaySegundos = configuracao.repousoDisplaySegundos;
  estado.sistema.passoFrequenciaKhz = configuracao.passoFrequenciaKhz;
  estado.sistema.volumeMonitor = configuracao.volumeMonitor;
  return estado;
}

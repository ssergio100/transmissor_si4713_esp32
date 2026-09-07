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
  estado.rf.frequenciaEfetivaKhz = telemetria.frequenciaEfetivaKhz;
  estado.rf.potenciaDbuv = configuracao.potenciaDbuv;
  estado.rf.potenciaEfetivaDbuv = telemetria.potenciaEfetivaDbuv;
  estado.rf.capacitanciaAntena = configuracao.capacitanciaAntena;
  estado.rf.capacitanciaEfetiva = telemetria.capacitanciaEfetiva;
  estado.rf.transmissaoHabilitada = configuracao.transmissaoHabilitada;
  estado.rf.transmitindo = telemetria.transmitindo;

  estado.audio.preEnfaseUs = configuracao.preEnfaseUs;
  estado.audio.desvioKhz = configuracao.desvioAudioKhz;
  estado.audio.nivelDbfs = telemetria.nivelAudioDbfs;
  estado.audio.asq = telemetria.asq;
  estado.audio.estereo = configuracao.estereo;
  estado.audio.mudo = configuracao.audioMudo;

  estado.rds.pi = configuracao.rdsPi;
  estado.rds.habilitado = configuracao.rdsHabilitado;
  memcpy(estado.rds.ps, configuracao.rdsPs, sizeof(estado.rds.ps));
  memcpy(estado.rds.texto, configuracao.rdsText, sizeof(estado.rds.texto));
  strncpy(estado.rds.textoAtual, transmissor.radioTextAtual(),
          sizeof(estado.rds.textoAtual) - 1);

  estado.receptor.frequenciaKhz = receptor.frequenciaKhz;
  estado.receptor.rssi = receptor.rssi;
  estado.receptor.disponivel = receptor.disponivel;
  estado.receptor.leituraRssiValida = receptor.leituraDiretaValida;

  estado.varredura.melhorFrequenciaKhz = transmissor.melhorFrequencia();
  estado.varredura.melhorNivelRuido = transmissor.melhorNivelRuido();
  estado.varredura.progresso = telemetria.progressoVarredura;
  estado.varredura.ativa = telemetria.varreduraAtiva;
  estado.varredura.concluida = telemetria.varreduraConcluida;

  estado.sistema.versaoFirmware = Configuracao::VERSAO_FIRMWARE;
  estado.sistema.recuperacoes = telemetria.recuperacoes;
  estado.sistema.falhasComunicacao = telemetria.falhasComunicacao;
  estado.sistema.inconsistenciasRf = telemetria.inconsistenciasRf;
  estado.sistema.interrupcoesSi4713 = telemetria.interrupcoesSi4713;
  estado.sistema.si4713Disponivel = telemetria.si4713Disponivel;
  estado.sistema.recuperando = telemetria.recuperando;
  estado.sistema.alertaSi4713Pendente =
      telemetria.alarmeInterrupcaoSi4713Pendente;
  return estado;
}

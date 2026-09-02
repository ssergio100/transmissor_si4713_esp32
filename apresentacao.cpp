#include "apresentacao.h"

#include <string.h>

#include "configuracao.h"
#include "menu.h"
#include "transmissor.h"

namespace {

TelaPainel converterTela(Menu::Tela tela) {
  switch (tela) {
    case Menu::Tela::PRINCIPAL: return TelaPainel::PRINCIPAL;
    case Menu::Tela::RAIZ: return TelaPainel::RAIZ;
    case Menu::Tela::RF: return TelaPainel::RF;
    case Menu::Tela::AUDIO: return TelaPainel::AUDIO;
    case Menu::Tela::RDS: return TelaPainel::RDS;
    case Menu::Tela::MONITOR: return TelaPainel::MONITOR;
    case Menu::Tela::VARREDURA: return TelaPainel::VARREDURA;
    case Menu::Tela::SISTEMA: return TelaPainel::SISTEMA;
  }
  return TelaPainel::PRINCIPAL;
}

uint8_t quantidadeItens(Menu::Tela tela) {
  switch (tela) {
    case Menu::Tela::RAIZ: return Menu::QUANTIDADE_RAIZ;
    case Menu::Tela::RF: return Menu::QUANTIDADE_RF;
    case Menu::Tela::AUDIO: return Menu::QUANTIDADE_AUDIO;
    case Menu::Tela::RDS: return Menu::QUANTIDADE_RDS;
    case Menu::Tela::VARREDURA: return Menu::QUANTIDADE_VARREDURA;
    case Menu::Tela::SISTEMA: return Menu::QUANTIDADE_SISTEMA;
    default: return 1;
  }
}

ItemPainel converterItem(Menu::Tela tela, uint8_t item) {
  switch (tela) {
    case Menu::Tela::RAIZ:
      switch (item) {
        case Menu::RAIZ_RF: return ItemPainel::RAIZ_RF;
        case Menu::RAIZ_AUDIO: return ItemPainel::RAIZ_AUDIO;
        case Menu::RAIZ_RDS: return ItemPainel::RAIZ_RDS;
        case Menu::RAIZ_MONITOR: return ItemPainel::RAIZ_MONITOR;
        case Menu::RAIZ_VARREDURA: return ItemPainel::RAIZ_VARREDURA;
        case Menu::RAIZ_SISTEMA: return ItemPainel::RAIZ_SISTEMA;
        case Menu::RAIZ_VOLTAR: return ItemPainel::RAIZ_VOLTAR;
      }
      break;

    case Menu::Tela::RF:
      switch (item) {
        case Menu::RF_FREQUENCIA: return ItemPainel::RF_FREQUENCIA;
        case Menu::RF_POTENCIA: return ItemPainel::RF_POTENCIA;
        case Menu::RF_ANTENA: return ItemPainel::RF_ANTENA;
        case Menu::RF_TRANSMISSAO: return ItemPainel::RF_TRANSMISSAO;
        case Menu::RF_VOLTAR: return ItemPainel::RF_VOLTAR;
      }
      break;

    case Menu::Tela::AUDIO:
      switch (item) {
        case Menu::AUDIO_ESTEREO: return ItemPainel::AUDIO_ESTEREO;
        case Menu::AUDIO_PRE_ENFASE: return ItemPainel::AUDIO_PRE_ENFASE;
        case Menu::AUDIO_DESVIO: return ItemPainel::AUDIO_DESVIO;
        case Menu::AUDIO_MUDO: return ItemPainel::AUDIO_MUDO;
        case Menu::AUDIO_VOLTAR: return ItemPainel::AUDIO_VOLTAR;
      }
      break;

    case Menu::Tela::RDS:
      switch (item) {
        case Menu::RDS_HABILITADO: return ItemPainel::RDS_HABILITADO;
        case Menu::RDS_PS: return ItemPainel::RDS_PS;
        case Menu::RDS_TEXTO: return ItemPainel::RDS_TEXTO;
        case Menu::RDS_PI: return ItemPainel::RDS_PI;
        case Menu::RDS_VOLTAR: return ItemPainel::RDS_VOLTAR;
      }
      break;

    case Menu::Tela::VARREDURA:
      switch (item) {
        case Menu::VARREDURA_INICIAR: return ItemPainel::VARREDURA_INICIAR;
        case Menu::VARREDURA_USAR_MELHOR:
          return ItemPainel::VARREDURA_USAR_MELHOR;
        case Menu::VARREDURA_VOLTAR: return ItemPainel::VARREDURA_VOLTAR;
      }
      break;

    case Menu::Tela::SISTEMA:
      switch (item) {
        case Menu::SISTEMA_SALVAR: return ItemPainel::SISTEMA_SALVAR;
        case Menu::SISTEMA_PADROES: return ItemPainel::SISTEMA_PADROES;
        case Menu::SISTEMA_WIFI: return ItemPainel::SISTEMA_WIFI;
        case Menu::SISTEMA_RSSI_NO_AR: return ItemPainel::SISTEMA_RSSI_NO_AR;
        case Menu::SISTEMA_INFO: return ItemPainel::SISTEMA_INFO;
        case Menu::SISTEMA_VOLTAR: return ItemPainel::SISTEMA_VOLTAR;
      }
      break;

    default:
      break;
  }
  return ItemPainel::NENHUM;
}

}  // namespace

EstadoPainel Apresentacao::gerar(
    const Menu& menu,
    const Transmissor& transmissor
) {
  EstadoPainel estado;
  const ConfiguracaoTransmissor& configuracao = transmissor.configuracao();
  const TelemetriaTransmissor& telemetria = transmissor.telemetria();
  const TelemetriaReceptorRda5807& receptor =
      transmissor.telemetriaReceptor();

  estado.navegacao.tela = converterTela(menu.tela());
  estado.navegacao.item = converterItem(
      menu.tela(),
      menu.itemSelecionado()
  );
  estado.navegacao.indice = menu.itemSelecionado();
  estado.navegacao.quantidade = quantidadeItens(menu.tela());
  estado.navegacao.cursorTexto = menu.cursorTexto();
  estado.navegacao.editando = menu.editando();

  estado.rf.frequenciaKhz = configuracao.frequenciaKhz;
  estado.rf.frequenciaEfetivaKhz = telemetria.frequenciaEfetivaKhz;
  estado.rf.potenciaDbuv = configuracao.potenciaDbuv;
  estado.rf.potenciaEfetivaDbuv = telemetria.potenciaEfetivaDbuv;
  estado.rf.capacitanciaAntena = configuracao.capacitanciaAntena;
  estado.rf.capacitanciaEfetiva = telemetria.capacitanciaEfetiva;
  estado.rf.transmissaoHabilitada = configuracao.transmissaoHabilitada;
  estado.rf.transmitindo = telemetria.transmitindo;
  estado.rf.noArConfirmado = transmissor.noArConfirmado();

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

  estado.receptor.frequenciaKhz = receptor.frequenciaKhz;
  estado.receptor.rssi = receptor.rssi;
  estado.receptor.rssiMinimoNoAr = configuracao.rssiMinimoNoAr;
  estado.receptor.disponivel = receptor.disponivel;
  estado.receptor.leituraRssiValida = receptor.leituraStatusValida;
  estado.receptor.rdsSincronizado = receptor.rdsSincronizado;
  estado.receptor.rdsTextoValido = receptor.rdsTextoValido;
  memcpy(estado.receptor.rdsPs, receptor.rdsPs, sizeof(estado.receptor.rdsPs));
  memcpy(
      estado.receptor.rdsTexto,
      receptor.rdsTexto,
      sizeof(estado.receptor.rdsTexto)
  );

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

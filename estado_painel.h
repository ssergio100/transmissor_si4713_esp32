#pragma once

#include <stdint.h>

// Contrato semantico consumido por qualquer interface visual. Este arquivo nao
// conhece I2C, SPI, coordenadas, cores nem as classes internas do radio.

enum class TelaPainel : uint8_t {
  PRINCIPAL = 0,
  RAIZ,
  RF,
  AUDIO,
  RDS,
  MONITOR,
  VARREDURA,
  SISTEMA
};

enum class ItemPainel : uint8_t {
  NENHUM = 0,

  RAIZ_RF,
  RAIZ_AUDIO,
  RAIZ_RDS,
  RAIZ_MONITOR,
  RAIZ_VARREDURA,
  RAIZ_SISTEMA,
  RAIZ_VOLTAR,

  RF_FREQUENCIA,
  RF_POTENCIA,
  RF_ANTENA,
  RF_TRANSMISSAO,
  RF_VOLTAR,

  AUDIO_ESTEREO,
  AUDIO_PRE_ENFASE,
  AUDIO_DESVIO,
  AUDIO_MUDO,
  AUDIO_VOLTAR,

  RDS_HABILITADO,
  RDS_PS,
  RDS_TEXTO,
  RDS_PI,
  RDS_VOLTAR,

  VARREDURA_INICIAR,
  VARREDURA_USAR_MELHOR,
  VARREDURA_VOLTAR,

  SISTEMA_SALVAR,
  SISTEMA_PADROES,
  SISTEMA_WIFI,
  SISTEMA_INFO,
  SISTEMA_VOLTAR
};

struct NavegacaoPainel {
  TelaPainel tela = TelaPainel::PRINCIPAL;
  ItemPainel item = ItemPainel::NENHUM;
  uint8_t indice = 0;
  uint8_t quantidade = 1;
  uint8_t cursorTexto = 0;
  bool editando = false;
};

struct RfPainel {
  // Unidade legada: passos de 10 kHz (9950 = 99.50 MHz).
  uint16_t frequenciaKhz = 0;
  uint16_t frequenciaEfetivaKhz = 0;
  uint8_t potenciaDbuv = 0;
  uint8_t potenciaEfetivaDbuv = 0;
  uint8_t capacitanciaAntena = 0;
  uint8_t capacitanciaEfetiva = 0;
  bool transmissaoHabilitada = false;
  bool transmitindo = false;
};

struct AudioPainel {
  uint8_t preEnfaseUs = 0;
  uint8_t desvioKhz = 0;
  int8_t nivelDbfs = -70;
  uint8_t asq = 0;
  bool estereo = false;
  bool mudo = false;
};

struct RdsPainel {
  uint16_t pi = 0;
  bool habilitado = false;
  char ps[9] = {};
  char texto[33] = {};
  char textoAtual[33] = {};  // Frase manual ou ultimo RadioText dinamico aplicado.
};

struct ReceptorPainel {
  // Unidade legada: passos de 10 kHz (9950 = 99.50 MHz).
  uint16_t frequenciaKhz = 0;
  uint8_t rssi = 0;
  bool disponivel = false;
  bool leituraRssiValida = false;
};

struct VarreduraPainel {
  uint16_t melhorFrequenciaKhz = 0;
  uint8_t melhorNivelRuido = 0;
  uint8_t progresso = 0;
  bool ativa = false;
  bool concluida = false;
};

struct SistemaPainel {
  const char* versaoFirmware = "";
  uint16_t recuperacoes = 0;
  uint32_t falhasComunicacao = 0;
  uint32_t inconsistenciasRf = 0;
  uint32_t interrupcoesSi4713 = 0;
  bool si4713Disponivel = false;
  bool recuperando = false;
  bool alertaSi4713Pendente = false;
};

struct EstadoPainel {
  NavegacaoPainel navegacao;
  RfPainel rf;
  AudioPainel audio;
  RdsPainel rds;
  ReceptorPainel receptor;
  VarreduraPainel varredura;
  SistemaPainel sistema;
};

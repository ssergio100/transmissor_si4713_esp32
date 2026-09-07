#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

#include "configuracao.h"

enum class FonteRadioText : uint8_t {
  TEXTO_MANUAL = 0,
  FRASE,
  HORA,
  DATA,
  DATA_E_HORA,
  MODELO
};

struct ConfiguracaoTransmissor {
  static constexpr uint32_t MAGIC = 0x53493437;
  static constexpr uint8_t VERSAO = 1;

  uint32_t magic = MAGIC;
  uint8_t versao = VERSAO;
  // Unidade legada: passos de 10 kHz (9950 = 99.50 MHz).
  uint16_t frequenciaKhz = 9950;
  uint16_t rdsPi = 0x4713;
  uint8_t potenciaDbuv = 100;
  uint8_t preEnfaseUs = 50;
  uint8_t desvioAudioKhz = 66;
  uint8_t capacitanciaAntena = 0;
  FonteRadioText fonteRadioText = FonteRadioText::TEXTO_MANUAL;
  bool estereo = true;
  bool transmissaoHabilitada = false;
  bool rdsHabilitado = true;
  bool audioMudo = false;
  char rdsPs[9] = "SI4713";
  char rdsText[33] = "Transmissor FM Si4713";
  char rdsModelo[33] = "{data} {hora}";
  uint16_t repousoDisplaySegundos =
      Configuracao::TEMPO_REPOUSO_DISPLAY_PADRAO_SEGUNDOS;
  uint8_t passoFrequenciaKhz = Configuracao::PASSO_FREQUENCIA_PADRAO_KHZ;
  uint8_t volumeMonitor = Configuracao::VOLUME_MONITOR_PADRAO;

  // 0xFF preserva a combinacao das preferencias legadas Mono/Estereo e RDS.
  static constexpr uint8_t APENAS_L = 0x10;
  static constexpr uint8_t APENAS_R = 0x11;
  uint8_t modoMultiplex = 0xFF;

  uint8_t modoAudio() const;
  uint8_t componentesMultiplex() const;
  uint8_t muteEntradas() const;
  void selecionarMultiplex(uint8_t modo);
  void aplicarPadroes();
  void sanitizarTextos();
  bool valoresValidos() const;

  static void copiarTextoPreenchido(
      char* destino,
      size_t comprimento,
      const char* origem
  );
};

struct MedicaoCanal {
  // Unidade legada: passos de 10 kHz (9950 = 99.50 MHz).
  uint16_t frequenciaKhz = 0;
  uint8_t nivelRuido = 0;
};

struct TelemetriaTransmissor {
  bool si4713Disponivel = false;
  bool recuperando = false;
  bool transmitindo = false;
  bool varreduraAtiva = false;
  bool varreduraConcluida = false;
  uint8_t progressoVarredura = 0;
  uint16_t frequenciaEfetivaKhz = 0;
  uint8_t potenciaEfetivaDbuv = 0;
  uint8_t capacitanciaEfetiva = 0;
  int8_t nivelAudioDbfs = -70;
  uint8_t asq = 0;
  uint16_t recuperacoes = 0;
  uint32_t falhasComunicacao = 0;
  uint32_t inconsistenciasRf = 0;
  uint32_t sequenciaAudio = 0;
  uint32_t interrupcoesSi4713 = 0;
  uint32_t ultimaInterrupcaoSi4713Ms = 0;
  uint8_t ultimoEventoAsq = 0;
  bool ultimaInterrupcaoLida = false;
  bool alarmeInterrupcaoSi4713Pendente = false;
};

const char* nomeFonteRadioText(FonteRadioText fonte);
bool converterFonteRadioText(const char* texto, FonteRadioText& fonte);
const char* nomeEventoSi4713(const TelemetriaTransmissor& telemetria);

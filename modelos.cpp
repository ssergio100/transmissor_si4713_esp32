#include "modelos.h"

#include <ctype.h>

void ConfiguracaoTransmissor::aplicarPadroes() {
  magic = MAGIC;
  versao = VERSAO;
  frequenciaKhz = 9950;
  rdsPi = 0x4713;
  potenciaDbuv = 100;
  preEnfaseUs = 50;
  desvioAudioKhz = 66;
  capacitanciaAntena = 0;
  fonteRadioText = FonteRadioText::TEXTO_MANUAL;
  estereo = true;
  transmissaoHabilitada = false;
  rdsHabilitado = true;
  audioMudo = false;
  copiarTextoPreenchido(rdsPs, 8, "SI4713");
  copiarTextoPreenchido(rdsText, 32, "Transmissor FM Si4713");
  copiarTextoPreenchido(rdsModelo, 32, "{data} {hora}");
  rssiMinimoNoAr = Configuracao::RSSI_NO_AR_PADRAO;
  memset(reservado, 0, sizeof(reservado));
}

void ConfiguracaoTransmissor::sanitizarTextos() {
  rdsPs[8] = '\0';
  rdsText[32] = '\0';
  rdsModelo[32] = '\0';
  memset(reservado, 0, sizeof(reservado));

  char* textos[] = {rdsPs, rdsText, rdsModelo};
  const size_t comprimentos[] = {8, 32, 32};

  for (size_t texto = 0; texto < 3; texto++) {
    bool encontrouFim = false;
    for (size_t indice = 0; indice < comprimentos[texto]; indice++) {
      unsigned char atual = textos[texto][indice];
      if (atual == '\0') encontrouFim = true;
      if (encontrouFim) {
        textos[texto][indice] = ' ';
      } else if (!isprint(atual)) {
        textos[texto][indice] = ' ';
      }
    }
  }
}

bool ConfiguracaoTransmissor::valoresValidos() const {
  return magic == MAGIC
      && versao == VERSAO
      && frequenciaKhz >= Configuracao::FREQUENCIA_MINIMA_KHZ
      && frequenciaKhz <= Configuracao::FREQUENCIA_MAXIMA_KHZ
      && (frequenciaKhz % Configuracao::PASSO_FREQUENCIA_KHZ) == 0
      && potenciaDbuv >= Configuracao::POTENCIA_MINIMA_DBUV
      && potenciaDbuv <= Configuracao::POTENCIA_MAXIMA_DBUV
      && (preEnfaseUs == 50 || preEnfaseUs == 75)
      && desvioAudioKhz >= 50
      && desvioAudioKhz <= 66
      && capacitanciaAntena <= Configuracao::CAPACITANCIA_ANTENA_MAXIMA
      && rssiMinimoNoAr <= Configuracao::RSSI_NO_AR_MAXIMO
      && static_cast<uint8_t>(fonteRadioText)
          <= static_cast<uint8_t>(FonteRadioText::MODELO);
}

void ConfiguracaoTransmissor::copiarTextoPreenchido(
    char* destino,
    size_t comprimento,
    const char* origem
) {
  size_t indice = 0;
  while (indice < comprimento && origem[indice] != '\0') {
    destino[indice] = origem[indice];
    indice++;
  }
  while (indice < comprimento) destino[indice++] = ' ';
  destino[comprimento] = '\0';
}

const char* nomeFonteRadioText(FonteRadioText fonte) {
  switch (fonte) {
    case FonteRadioText::TEXTO_MANUAL: return "manual";
    case FonteRadioText::FRASE: return "frase";
    case FonteRadioText::HORA: return "hora";
    case FonteRadioText::DATA: return "data";
    case FonteRadioText::DATA_E_HORA: return "data_hora";
    case FonteRadioText::MODELO: return "modelo";
  }
  return "manual";
}

bool converterFonteRadioText(const char* texto, FonteRadioText& fonte) {
  if (strcmp(texto, "manual") == 0) {
    fonte = FonteRadioText::TEXTO_MANUAL;
  } else if (strcmp(texto, "frase") == 0) {
    fonte = FonteRadioText::FRASE;
  } else if (strcmp(texto, "hora") == 0) {
    fonte = FonteRadioText::HORA;
  } else if (strcmp(texto, "data") == 0) {
    fonte = FonteRadioText::DATA;
  } else if (strcmp(texto, "data_hora") == 0) {
    fonte = FonteRadioText::DATA_E_HORA;
  } else if (strcmp(texto, "modelo") == 0) {
    fonte = FonteRadioText::MODELO;
  } else {
    return false;
  }
  return true;
}

const char* nomeEventoSi4713(const TelemetriaTransmissor& telemetria) {
  if (telemetria.interrupcoesSi4713 == 0) return "none";
  if (!telemetria.ultimaInterrupcaoLida) return "read_error";
  if ((telemetria.ultimoEventoAsq & 0x04) != 0) return "overmodulation";
  if ((telemetria.ultimoEventoAsq & 0x02) != 0) return "audio_high";
  if ((telemetria.ultimoEventoAsq & 0x01) != 0) return "audio_low";
  return "asq";
}

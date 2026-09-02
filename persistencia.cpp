#include "persistencia.h"

#include <Preferences.h>

namespace {

constexpr char ESPACO_NVS[] = "si4713";
constexpr char CHAVE_CONFIGURACAO[] = "config";
constexpr char CHAVE_FREQUENCIA[] = "frequencia";

// Formato gravado pelas versoes de firmware anteriores a 0.1.23. A migracao
// preserva todos os ajustes existentes e acrescenta somente o limiar de RSSI.
struct ConfiguracaoTransmissorV1 {
  uint32_t magic;
  uint8_t versao;
  uint16_t frequenciaKhz;
  uint16_t rdsPi;
  uint8_t potenciaDbuv;
  uint8_t preEnfaseUs;
  uint8_t desvioAudioKhz;
  uint8_t capacitanciaAntena;
  FonteRadioText fonteRadioText;
  bool estereo;
  bool transmissaoHabilitada;
  bool rdsHabilitado;
  bool audioMudo;
  char rdsPs[9];
  char rdsText[33];
  char rdsModelo[33];
};

static_assert(
    sizeof(ConfiguracaoTransmissorV1) < sizeof(ConfiguracaoTransmissor),
    "A configuracao v2 deve ser distinguivel do bloco NVS v1"
);

bool migrarConfiguracaoV1(
    const ConfiguracaoTransmissorV1& antiga,
    ConfiguracaoTransmissor& atual
) {
  if (antiga.magic != ConfiguracaoTransmissor::MAGIC
      || antiga.versao != 1) {
    return false;
  }

  atual.aplicarPadroes();
  atual.frequenciaKhz = antiga.frequenciaKhz;
  atual.rdsPi = antiga.rdsPi;
  atual.potenciaDbuv = antiga.potenciaDbuv;
  atual.preEnfaseUs = antiga.preEnfaseUs;
  atual.desvioAudioKhz = antiga.desvioAudioKhz;
  atual.capacitanciaAntena = antiga.capacitanciaAntena;
  atual.fonteRadioText = antiga.fonteRadioText;
  atual.estereo = antiga.estereo;
  atual.transmissaoHabilitada = antiga.transmissaoHabilitada;
  atual.rdsHabilitado = antiga.rdsHabilitado;
  atual.audioMudo = antiga.audioMudo;
  memcpy(atual.rdsPs, antiga.rdsPs, sizeof(atual.rdsPs));
  memcpy(atual.rdsText, antiga.rdsText, sizeof(atual.rdsText));
  memcpy(atual.rdsModelo, antiga.rdsModelo, sizeof(atual.rdsModelo));
  atual.sanitizarTextos();
  return atual.valoresValidos();
}

}  // namespace

bool Persistencia::carregar(ConfiguracaoTransmissor& configuracao) {
  Preferences preferencias;
  if (!preferencias.begin(ESPACO_NVS, true)) return false;

  const size_t tamanho = preferencias.getBytesLength(CHAVE_CONFIGURACAO);
  if (tamanho == sizeof(ConfiguracaoTransmissor)) {
    ConfiguracaoTransmissor carregada;
    const size_t lidos = preferencias.getBytes(
        CHAVE_CONFIGURACAO,
        &carregada,
        sizeof(carregada)
    );
    preferencias.end();

    carregada.sanitizarTextos();
    if (lidos != sizeof(carregada) || !carregada.valoresValidos()) return false;
    configuracao = carregada;
    return true;
  }

  if (tamanho == sizeof(ConfiguracaoTransmissorV1)) {
    ConfiguracaoTransmissorV1 antiga{};
    const size_t lidos = preferencias.getBytes(
        CHAVE_CONFIGURACAO,
        &antiga,
        sizeof(antiga)
    );
    preferencias.end();
    if (lidos != sizeof(antiga)
        || !migrarConfiguracaoV1(antiga, configuracao)) {
      return false;
    }
    Serial.printf(
        "[NVS] Configuracao migrada: v1 -> v%u; limiar RSSI=%u\n",
        ConfiguracaoTransmissor::VERSAO,
        configuracao.rssiMinimoNoAr
    );
    return true;
  }

  preferencias.end();
  return false;
}

bool Persistencia::salvar(const ConfiguracaoTransmissor& configuracao) {
  if (!configuracao.valoresValidos()) return false;

  Preferences preferencias;
  if (!preferencias.begin(ESPACO_NVS, false)) return false;
  const size_t gravados = preferencias.putBytes(
      CHAVE_CONFIGURACAO,
      &configuracao,
      sizeof(configuracao)
  );
  const size_t frequenciaGravada = preferencias.putUShort(
      CHAVE_FREQUENCIA,
      configuracao.frequenciaKhz
  );
  preferencias.end();
  return gravados == sizeof(configuracao)
      && frequenciaGravada == sizeof(configuracao.frequenciaKhz);
}

bool Persistencia::carregarFrequencia(uint16_t& frequenciaKhz) {
  Preferences preferencias;
  if (!preferencias.begin(ESPACO_NVS, true)) return false;
  if (!preferencias.isKey(CHAVE_FREQUENCIA)) {
    preferencias.end();
    return false;
  }

  const uint16_t carregada = preferencias.getUShort(CHAVE_FREQUENCIA, 0);
  preferencias.end();
  if (carregada < Configuracao::FREQUENCIA_MINIMA_KHZ
      || carregada > Configuracao::FREQUENCIA_MAXIMA_KHZ
      || carregada % Configuracao::PASSO_FREQUENCIA_KHZ != 0) {
    return false;
  }
  frequenciaKhz = carregada;
  return true;
}

bool Persistencia::salvarFrequencia(uint16_t frequenciaKhz) {
  if (frequenciaKhz < Configuracao::FREQUENCIA_MINIMA_KHZ
      || frequenciaKhz > Configuracao::FREQUENCIA_MAXIMA_KHZ
      || frequenciaKhz % Configuracao::PASSO_FREQUENCIA_KHZ != 0) {
    return false;
  }

  Preferences preferencias;
  if (!preferencias.begin(ESPACO_NVS, false)) return false;
  const size_t gravados = preferencias.putUShort(
      CHAVE_FREQUENCIA,
      frequenciaKhz
  );
  preferencias.end();
  return gravados == sizeof(frequenciaKhz);
}

bool Persistencia::apagar() {
  Preferences preferencias;
  if (!preferencias.begin(ESPACO_NVS, false)) return false;
  const bool tinhaConfiguracao = preferencias.isKey(CHAVE_CONFIGURACAO);
  const bool tinhaFrequencia = preferencias.isKey(CHAVE_FREQUENCIA);
  const bool configuracaoRemovida = !tinhaConfiguracao
      || preferencias.remove(CHAVE_CONFIGURACAO);
  const bool frequenciaRemovida = !tinhaFrequencia
      || preferencias.remove(CHAVE_FREQUENCIA);
  preferencias.end();
  return configuracaoRemovida && frequenciaRemovida;
}

#include "persistencia.h"

#include <Preferences.h>

namespace {

constexpr char ESPACO_NVS[] = "si4713";
constexpr char CHAVE_CONFIGURACAO[] = "config";
constexpr char CHAVE_FREQUENCIA[] = "frequencia";

}  // namespace

bool Persistencia::carregar(ConfiguracaoTransmissor& configuracao) {
  Preferences preferencias;
  if (!preferencias.begin(ESPACO_NVS, true)) return false;

  const size_t tamanho = preferencias.getBytesLength(CHAVE_CONFIGURACAO);
  if (tamanho != sizeof(ConfiguracaoTransmissor)) {
    preferencias.end();
    return false;
  }

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

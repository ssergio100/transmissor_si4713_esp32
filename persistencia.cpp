#include "persistencia.h"

#include <Preferences.h>

namespace {

constexpr char ESPACO_NVS[] = "si4713";
constexpr char CHAVE_CONFIGURACAO[] = "config";

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
  preferencias.end();
  return gravados == sizeof(configuracao);
}

bool Persistencia::apagar() {
  Preferences preferencias;
  if (!preferencias.begin(ESPACO_NVS, false)) return false;
  const bool removida = preferencias.remove(CHAVE_CONFIGURACAO);
  preferencias.end();
  return removida;
}

#include "persistencia.h"

#include <Preferences.h>

namespace {

constexpr char ESPACO_NVS[] = "si4713";
constexpr char CHAVE_CONFIGURACAO[] = "config";
constexpr char CHAVE_FREQUENCIA[] = "frequencia";
// Chave antiga em minutos, mantida para migrar configuracoes ja gravadas.
constexpr char CHAVE_REPOUSO_DISPLAY_MINUTOS[] = "repouso_tft";
constexpr char CHAVE_REPOUSO_DISPLAY_SEGUNDOS[] = "repouso_seg";
constexpr char CHAVE_PASSO_FREQUENCIA[] = "passo_freq";
constexpr char CHAVE_VOLUME_MONITOR[] = "volume_rx";
constexpr size_t TAMANHO_CONFIGURACAO_ANTERIOR = 96;
constexpr char CHAVE_MULTIPLEX[] = "multiplex";

}  // namespace

bool Persistencia::carregar(ConfiguracaoTransmissor& configuracao) {
  Preferences preferencias;
  if (!preferencias.begin(ESPACO_NVS, true)) return false;

  const size_t tamanho = preferencias.getBytesLength(CHAVE_CONFIGURACAO);
  if (tamanho != sizeof(ConfiguracaoTransmissor)
      && tamanho != TAMANHO_CONFIGURACAO_ANTERIOR) {
    preferencias.end();
    return false;
  }

  ConfiguracaoTransmissor carregada;
  const size_t lidos = preferencias.getBytes(
      CHAVE_CONFIGURACAO,
      &carregada,
      tamanho
  );
  // Campo novo ocupa bytes que antes eram preenchimento da estrutura. A chave
  // separada evita interpretar o conteudo antigo desses bytes como um tempo.
  if (preferencias.isKey(CHAVE_REPOUSO_DISPLAY_SEGUNDOS)) {
    carregada.repousoDisplaySegundos = preferencias.getUShort(
        CHAVE_REPOUSO_DISPLAY_SEGUNDOS,
        Configuracao::TEMPO_REPOUSO_DISPLAY_PADRAO_SEGUNDOS
    );
  } else {
    const uint8_t minutosSalvos = preferencias.getUChar(
        CHAVE_REPOUSO_DISPLAY_MINUTOS,
        Configuracao::TEMPO_REPOUSO_DISPLAY_PADRAO_SEGUNDOS / 60
    );
    carregada.repousoDisplaySegundos =
        static_cast<uint16_t>(minutosSalvos) * 60;
  }
  carregada.passoFrequenciaKhz = preferencias.getUChar(
      CHAVE_PASSO_FREQUENCIA,
      Configuracao::PASSO_FREQUENCIA_PADRAO_KHZ
  );
  carregada.volumeMonitor = preferencias.getUChar(
      CHAVE_VOLUME_MONITOR,
      Configuracao::VOLUME_MONITOR_PADRAO
  );
  // O modo ocupa preenchimento do blob antigo; use a chave para migrar
  // sem interpretar bytes antigos como uma selecao de diagnostico.
  carregada.modoMultiplex = preferencias.getUChar(CHAVE_MULTIPLEX, 0xFF);
  preferencias.end();

  carregada.sanitizarTextos();
  if (lidos != tamanho || !carregada.valoresValidos()) return false;

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
  const size_t multiplexGravado = preferencias.putUChar(
      CHAVE_MULTIPLEX, configuracao.modoMultiplex);
  const size_t frequenciaGravada = preferencias.putUShort(
      CHAVE_FREQUENCIA,
      configuracao.frequenciaKhz
  );
  const size_t repousoGravado = preferencias.putUShort(
      CHAVE_REPOUSO_DISPLAY_SEGUNDOS,
      configuracao.repousoDisplaySegundos
  );
  const size_t passoGravado = preferencias.putUChar(
      CHAVE_PASSO_FREQUENCIA,
      configuracao.passoFrequenciaKhz
  );
  const size_t volumeGravado = preferencias.putUChar(
      CHAVE_VOLUME_MONITOR,
      configuracao.volumeMonitor
  );
  preferencias.end();
  return multiplexGravado == sizeof(configuracao.modoMultiplex)
      && gravados == sizeof(configuracao)
      && frequenciaGravada == sizeof(configuracao.frequenciaKhz)
      && repousoGravado == sizeof(configuracao.repousoDisplaySegundos)
      && passoGravado == sizeof(configuracao.passoFrequenciaKhz)
      && volumeGravado == sizeof(configuracao.volumeMonitor);
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

bool Persistencia::salvarRepousoDisplay(uint16_t segundos) {
  bool valorValido = false;
  for (uint16_t tempo : Configuracao::TEMPOS_REPOUSO_DISPLAY_SEGUNDOS) {
    if (segundos == tempo) valorValido = true;
  }
  if (!valorValido) return false;

  Preferences preferencias;
  if (!preferencias.begin(ESPACO_NVS, false)) return false;
  const size_t gravados = preferencias.putUShort(
      CHAVE_REPOUSO_DISPLAY_SEGUNDOS,
      segundos
  );
  preferencias.end();
  return gravados == sizeof(segundos);
}

bool Persistencia::salvarPassoFrequencia(uint8_t passoKhz) {
  bool valorValido = false;
  for (uint16_t passo : Configuracao::PASSOS_FREQUENCIA_KHZ) {
    if (passoKhz == passo) valorValido = true;
  }
  if (!valorValido) return false;

  Preferences preferencias;
  if (!preferencias.begin(ESPACO_NVS, false)) return false;
  const size_t gravados = preferencias.putUChar(
      CHAVE_PASSO_FREQUENCIA,
      passoKhz
  );
  preferencias.end();
  return gravados == sizeof(passoKhz);
}

bool Persistencia::salvarVolumeMonitor(uint8_t volume) {
  if (volume > Configuracao::VOLUME_MONITOR_MAXIMO) {
    return false;
  }
  Preferences preferencias;
  if (!preferencias.begin(ESPACO_NVS, false)) return false;
  const size_t gravados = preferencias.putUChar(CHAVE_VOLUME_MONITOR, volume);
  preferencias.end();
  return gravados == sizeof(volume);
}

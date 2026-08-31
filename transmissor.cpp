#include "transmissor.h"

#include <time.h>

#include "persistencia.h"

bool Transmissor::iniciar() {
  if (!Persistencia::carregar(configuracao_)) {
    configuracao_.aplicarPadroes();
  }
  uint16_t frequenciaSalva = 0;
  if (Persistencia::carregarFrequencia(frequenciaSalva)) {
    configuracao_.frequenciaKhz = frequenciaSalva;
  }

  configuracao_.sanitizarTextos();
  if (!radio_.iniciar()) return false;
  radioDisponivelNoCicloAnterior_ = true;
  return aplicarConfiguracao(configuracao_);
}

void Transmissor::processar() {
  radio_.processar();

  const bool disponivel = radio_.telemetria().si4713Disponivel;
  if (disponivel && !radioDisponivelNoCicloAnterior_) {
    aplicarConfiguracao(configuracao_);
  }
  radioDisponivelNoCicloAnterior_ = disponivel;
  atualizarRadioTextDinamico();
}

const ConfiguracaoTransmissor& Transmissor::configuracao() const {
  return configuracao_;
}

const TelemetriaTransmissor& Transmissor::telemetria() const {
  return radio_.telemetria();
}

ConfiguracaoTransmissor Transmissor::copiarConfiguracao() const {
  return configuracao();
}

bool Transmissor::aplicarConfiguracao(ConfiguracaoTransmissor configuracao) {
  configuracao.sanitizarTextos();
  if (!configuracao.valoresValidos()) return false;

  const ConfiguracaoTransmissor anterior = configuracao_;
  const bool deveSalvarFrequencia = ajusteFrequenciaAtivo_
      || configuracao.frequenciaKhz != anterior.frequenciaKhz;
  configuracao_ = configuracao;

  char textoEfetivo[33];
  bool aplicado = false;
  if (formatarRadioText(textoEfetivo, sizeof(textoEfetivo))) {
    ConfiguracaoTransmissor efetiva = configuracao_;
    ConfiguracaoTransmissor::copiarTextoPreenchido(
        efetiva.rdsText,
        32,
        textoEfetivo
    );
    aplicado = radio_.aplicar(efetiva);
    if (aplicado) {
      strncpy(ultimoRadioTextAplicado_, efetiva.rdsText, 32);
      ultimoRadioTextAplicado_[32] = '\0';
    }
  } else {
    aplicado = radio_.aplicar(configuracao_);
  }

  if (!aplicado) {
    configuracao_ = anterior;
    return false;
  }

  ajusteFrequenciaAtivo_ = false;
  if (deveSalvarFrequencia
      && !Persistencia::salvarFrequencia(configuracao_.frequenciaKhz)) {
    Serial.println("[ERRO] Frequencia aplicada, mas nao gravada na NVS");
    return false;
  }
  return true;
}

bool Transmissor::salvarConfiguracao() {
  if (ajusteFrequenciaAtivo_
      && !aplicarConfiguracao(configuracao_)) {
    return false;
  }
  return Persistencia::salvar(configuracao_);
}

bool Transmissor::restaurarPadroes() {
  ConfiguracaoTransmissor padroes;
  padroes.aplicarPadroes();
  if (!aplicarConfiguracao(padroes)) return false;
  return salvarConfiguracao();
}

bool Transmissor::reiniciarRf() {
  if (ajusteFrequenciaAtivo_) return false;
  return radio_.reiniciarRf();
}

bool Transmissor::setLeituraAudio(bool habilitar) {
  leituraAudioWebSolicitada_ = habilitar;
  radio_.setLeituraAudio(
      leituraAudioWebSolicitada_ || leituraAudioDisplaySolicitada_
  );
  return true;
}

bool Transmissor::setLeituraAudioDisplay(bool habilitar) {
  leituraAudioDisplaySolicitada_ = habilitar;
  radio_.setLeituraAudio(
      leituraAudioWebSolicitada_ || leituraAudioDisplaySolicitada_
  );
  return true;
}

bool Transmissor::leituraAudioHabilitada() const {
  return radio_.leituraAudioHabilitada();
}

bool Transmissor::reconhecerInterrupcaoSi4713() {
  return radio_.reconhecerInterrupcao();
}

bool Transmissor::iniciarVarredura() {
  if (ajusteFrequenciaAtivo_) return false;
  return radio_.iniciarVarredura();
}

bool Transmissor::cancelarVarredura() {
  return radio_.cancelarVarredura();
}

bool Transmissor::iniciarAjusteFrequencia() {
  if (ajusteFrequenciaAtivo_) return true;
  if (!radio_.iniciarAjusteFrequencia()) return false;
  ajusteFrequenciaAtivo_ = true;
  return true;
}

bool Transmissor::previsualizarFrequencia(uint16_t frequenciaKhz) {
  if (!iniciarAjusteFrequencia()
      || !radio_.previsualizarFrequencia(frequenciaKhz)) {
    return false;
  }
  configuracao_.frequenciaKhz = frequenciaKhz;
  return true;
}

bool Transmissor::aplicarFrequencia(uint16_t frequenciaKhz) {
  const bool persistenciaJaSolicitada = ajusteFrequenciaAtivo_
      || frequenciaKhz != configuracao_.frequenciaKhz;
  ConfiguracaoTransmissor alterada = configuracao();
  alterada.frequenciaKhz = frequenciaKhz;
  if (!aplicarConfiguracao(alterada)) return false;
  return persistenciaJaSolicitada
      || Persistencia::salvarFrequencia(frequenciaKhz);
}

size_t Transmissor::quantidadeMedicoes() const {
  return radio_.quantidadeMedicoes();
}

const MedicaoCanal& Transmissor::medicao(size_t indice) const {
  return radio_.medicao(indice);
}

uint16_t Transmissor::melhorFrequencia() const {
  return radio_.melhorFrequencia();
}

uint8_t Transmissor::melhorNivelRuido() const {
  return radio_.melhorNivelRuido();
}

uint8_t Transmissor::enderecoRadio() const {
  return radio_.endereco();
}

bool Transmissor::horaValida() const {
  struct tm horario;
  return getLocalTime(&horario, 0);
}

void Transmissor::atualizarRadioTextDinamico(bool forcar) {
  if (ajusteFrequenciaAtivo_) return;
  const bool habilitado = configuracao_.rdsHabilitado;
  const FonteRadioText fonte = configuracao_.fonteRadioText;
  const bool varreduraAtiva = radio_.telemetria().varreduraAtiva;
  const bool disponivel = radio_.telemetria().si4713Disponivel;
  if (!habilitado
      || fonte == FonteRadioText::TEXTO_MANUAL
      || fonte == FonteRadioText::FRASE
      || varreduraAtiva
      || !disponivel) {
    return;
  }

  const uint32_t segundo = millis() / 1000;
  if (!forcar && segundo == ultimoSegundoRadioText_) return;
  ultimoSegundoRadioText_ = segundo;

  char texto[33];
  if (!formatarRadioText(texto, sizeof(texto))) return;
  if (!forcar && strncmp(texto, ultimoRadioTextAplicado_, 32) == 0) return;

  ConfiguracaoTransmissor efetiva = configuracao_;
  ConfiguracaoTransmissor::copiarTextoPreenchido(efetiva.rdsText, 32, texto);
  const bool aplicado = radio_.aplicar(efetiva);
  if (aplicado) {
    strncpy(ultimoRadioTextAplicado_, efetiva.rdsText, 32);
    ultimoRadioTextAplicado_[32] = '\0';
  }
}

bool Transmissor::formatarRadioText(char* destino, size_t tamanho) const {
  if (tamanho < 33) return false;
  if (configuracao_.fonteRadioText == FonteRadioText::TEXTO_MANUAL
      || configuracao_.fonteRadioText == FonteRadioText::FRASE) {
    strncpy(destino, configuracao_.rdsText, 32);
    destino[32] = '\0';
    return true;
  }

  struct tm horario;
  if (!getLocalTime(&horario, 0)) return false;

  char data[11];
  char hora[6];
  strftime(data, sizeof(data), "%d/%m/%Y", &horario);
  strftime(hora, sizeof(hora), "%H:%M", &horario);

  switch (configuracao_.fonteRadioText) {
    case FonteRadioText::HORA:
      snprintf(destino, tamanho, "%s", hora);
      break;
    case FonteRadioText::DATA:
      snprintf(destino, tamanho, "%s", data);
      break;
    case FonteRadioText::DATA_E_HORA:
      snprintf(destino, tamanho, "%s %s", data, hora);
      break;
    case FonteRadioText::MODELO: {
      const String resultado = aplicarModelo(configuracao_.rdsModelo, data, hora);
      snprintf(destino, tamanho, "%.32s", resultado.c_str());
      break;
    }
    default:
      return false;
  }
  return true;
}

String Transmissor::aplicarModelo(
    const char* modelo,
    const char* data,
    const char* hora
) {
  String resultado(modelo);
  resultado.replace("{data}", data);
  resultado.replace("{hora}", hora);
  return resultado;
}

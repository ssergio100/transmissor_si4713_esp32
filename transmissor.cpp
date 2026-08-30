#include "transmissor.h"

#include <time.h>

#include "persistencia.h"

bool Transmissor::iniciar() {
  if (!Persistencia::carregar(configuracao_)) {
    configuracao_.aplicarPadroes();
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
  return configuracao_;
}

bool Transmissor::aplicarConfiguracao(ConfiguracaoTransmissor configuracao) {
  configuracao.sanitizarTextos();
  if (!configuracao.valoresValidos()) return false;

  char textoEfetivo[33];
  ConfiguracaoTransmissor anterior = configuracao_;
  configuracao_ = configuracao;
  if (formatarRadioText(textoEfetivo, sizeof(textoEfetivo))) {
    ConfiguracaoTransmissor efetiva = configuracao_;
    ConfiguracaoTransmissor::copiarTextoPreenchido(
        efetiva.rdsText,
        32,
        textoEfetivo
    );
    if (radio_.aplicar(efetiva)) {
      strncpy(ultimoRadioTextAplicado_, efetiva.rdsText, 32);
      ultimoRadioTextAplicado_[32] = '\0';
      return true;
    }
  } else if (radio_.aplicar(configuracao_)) {
    return true;
  }

  configuracao_ = anterior;
  return false;
}

bool Transmissor::salvarConfiguracao() {
  return Persistencia::salvar(configuracao_);
}

bool Transmissor::restaurarPadroes() {
  ConfiguracaoTransmissor padroes;
  padroes.aplicarPadroes();
  if (!aplicarConfiguracao(padroes)) return false;
  return salvarConfiguracao();
}

bool Transmissor::reiniciarRf() {
  return radio_.reiniciarRf();
}

bool Transmissor::iniciarVarredura() {
  return radio_.iniciarVarredura();
}

bool Transmissor::cancelarVarredura() {
  return radio_.cancelarVarredura();
}

bool Transmissor::aplicarFrequencia(uint16_t frequenciaKhz) {
  ConfiguracaoTransmissor alterada = configuracao_;
  alterada.frequenciaKhz = frequenciaKhz;
  return aplicarConfiguracao(alterada);
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
  if (!configuracao_.rdsHabilitado
      || configuracao_.fonteRadioText == FonteRadioText::TEXTO_MANUAL
      || configuracao_.fonteRadioText == FonteRadioText::FRASE
      || radio_.telemetria().varreduraAtiva
      || !radio_.telemetria().si4713Disponivel) {
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
  if (radio_.aplicar(efetiva)) {
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

#include "transmissor.h"

#include <time.h>

#include "persistencia.h"

bool Transmissor::iniciar() {
  mutexEstado_ = xSemaphoreCreateMutex();
  filaComandos_ = xQueueCreate(4, sizeof(ComandoTransmissor));

  if (!Persistencia::carregar(configuracao_)) {
    configuracao_.aplicarPadroes();
  }

  configuracao_.sanitizarTextos();
  if (!radio_.iniciar()) return false;
  radioDisponivelNoCicloAnterior_ = true;
  return aplicarConfiguracao(configuracao_);
}

void Transmissor::processar() {
  xSemaphoreTake(mutexEstado_, portMAX_DELAY);
  radio_.processar();
  xSemaphoreGive(mutexEstado_);

  const bool disponivel = telemetria().si4713Disponivel;
  if (disponivel && !radioDisponivelNoCicloAnterior_) {
    aplicarConfiguracao(configuracao_);
  }
  radioDisponivelNoCicloAnterior_ = disponivel;
  processarComandos();
  atualizarRadioTextDinamico();
}

ConfiguracaoTransmissor Transmissor::configuracao() const {
  xSemaphoreTake(mutexEstado_, portMAX_DELAY);
  const ConfiguracaoTransmissor copia = configuracao_;
  xSemaphoreGive(mutexEstado_);
  return copia;
}

TelemetriaTransmissor Transmissor::telemetria() const {
  xSemaphoreTake(mutexEstado_, portMAX_DELAY);
  const TelemetriaTransmissor copia = radio_.telemetria();
  xSemaphoreGive(mutexEstado_);
  return copia;
}

ConfiguracaoTransmissor Transmissor::copiarConfiguracao() const {
  return configuracao();
}

bool Transmissor::aplicarConfiguracao(ConfiguracaoTransmissor configuracao) {
  configuracao.sanitizarTextos();
  if (!configuracao.valoresValidos()) return false;

  xSemaphoreTake(mutexEstado_, portMAX_DELAY);
  const ConfiguracaoTransmissor anterior = configuracao_;
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

  if (!aplicado) configuracao_ = anterior;
  xSemaphoreGive(mutexEstado_);
  return aplicado;
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
  xSemaphoreTake(mutexEstado_, portMAX_DELAY);
  const bool ok = radio_.reiniciarRf();
  xSemaphoreGive(mutexEstado_);
  return ok;
}

bool Transmissor::iniciarVarredura() {
  xSemaphoreTake(mutexEstado_, portMAX_DELAY);
  const bool ok = radio_.iniciarVarredura();
  xSemaphoreGive(mutexEstado_);
  return ok;
}

bool Transmissor::cancelarVarredura() {
  xSemaphoreTake(mutexEstado_, portMAX_DELAY);
  const bool ok = radio_.cancelarVarredura();
  xSemaphoreGive(mutexEstado_);
  return ok;
}

bool Transmissor::aplicarFrequencia(uint16_t frequenciaKhz) {
  ConfiguracaoTransmissor alterada = configuracao();
  alterada.frequenciaKhz = frequenciaKhz;
  return aplicarConfiguracao(alterada);
}

bool Transmissor::enviarComando(ComandoTransmissor& comando) {
  if (filaComandos_ == nullptr) return false;

  auto* resposta = new RespostaComando();
  resposta->concluido = xSemaphoreCreateBinary();
  comando.resposta = resposta;
  if (resposta->concluido == nullptr) {
    delete resposta;
    comando.resposta = nullptr;
    return false;
  }

  if (xQueueSend(filaComandos_, &comando, pdMS_TO_TICKS(100)) != pdTRUE) {
    vSemaphoreDelete(resposta->concluido);
    delete resposta;
    comando.resposta = nullptr;
    return false;
  }

  const bool respondeu =
      xSemaphoreTake(resposta->concluido, pdMS_TO_TICKS(3000)) == pdTRUE;
  const bool resultado = respondeu && resposta->resultado;
  vSemaphoreDelete(resposta->concluido);
  delete resposta;
  comando.resposta = nullptr;
  return resultado;
}

void Transmissor::processarComandos() {
  ComandoTransmissor comando;
  while (xQueueReceive(filaComandos_, &comando, 0) == pdTRUE) {
    if (comando.resposta == nullptr) continue;
    comando.resposta->resultado = executarComando(comando);
    xSemaphoreGive(comando.resposta->concluido);
  }
}

bool Transmissor::executarComando(const ComandoTransmissor& comando) {
  switch (comando.tipo) {
    case ComandoTipo::APLICAR_CONFIGURACAO:
      return aplicarConfiguracao(comando.configuracao);
    case ComandoTipo::SALVAR_CONFIGURACAO:
      return salvarConfiguracao();
    case ComandoTipo::RESTAURAR_PADROES:
      return restaurarPadroes();
    case ComandoTipo::REINICIAR_RF:
      return reiniciarRf();
    case ComandoTipo::INICIAR_VARREDURA:
      return iniciarVarredura();
    case ComandoTipo::CANCELAR_VARREDURA:
      return cancelarVarredura();
    case ComandoTipo::APLICAR_FREQUENCIA:
      return aplicarFrequencia(comando.frequenciaKhz);
  }
  return false;
}

size_t Transmissor::quantidadeMedicoes() const {
  xSemaphoreTake(mutexEstado_, portMAX_DELAY);
  const size_t quantidade = radio_.quantidadeMedicoes();
  xSemaphoreGive(mutexEstado_);
  return quantidade;
}

MedicaoCanal Transmissor::medicao(size_t indice) const {
  xSemaphoreTake(mutexEstado_, portMAX_DELAY);
  const MedicaoCanal item = radio_.medicao(indice);
  xSemaphoreGive(mutexEstado_);
  return item;
}

uint16_t Transmissor::melhorFrequencia() const {
  xSemaphoreTake(mutexEstado_, portMAX_DELAY);
  const uint16_t melhor = radio_.melhorFrequencia();
  xSemaphoreGive(mutexEstado_);
  return melhor;
}

uint8_t Transmissor::melhorNivelRuido() const {
  xSemaphoreTake(mutexEstado_, portMAX_DELAY);
  const uint8_t nivel = radio_.melhorNivelRuido();
  xSemaphoreGive(mutexEstado_);
  return nivel;
}

uint8_t Transmissor::enderecoRadio() const {
  xSemaphoreTake(mutexEstado_, portMAX_DELAY);
  const uint8_t endereco = radio_.endereco();
  xSemaphoreGive(mutexEstado_);
  return endereco;
}

bool Transmissor::horaValida() const {
  struct tm horario;
  return getLocalTime(&horario, 0);
}

void Transmissor::atualizarRadioTextDinamico(bool forcar) {
  xSemaphoreTake(mutexEstado_, portMAX_DELAY);
  const bool habilitado = configuracao_.rdsHabilitado;
  const FonteRadioText fonte = configuracao_.fonteRadioText;
  const bool varreduraAtiva = radio_.telemetria().varreduraAtiva;
  const bool disponivel = radio_.telemetria().si4713Disponivel;
  xSemaphoreGive(mutexEstado_);

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

  xSemaphoreTake(mutexEstado_, portMAX_DELAY);
  ConfiguracaoTransmissor efetiva = configuracao_;
  ConfiguracaoTransmissor::copiarTextoPreenchido(efetiva.rdsText, 32, texto);
  const bool aplicado = radio_.aplicar(efetiva);
  if (aplicado) {
    strncpy(ultimoRadioTextAplicado_, efetiva.rdsText, 32);
    ultimoRadioTextAplicado_[32] = '\0';
  }
  xSemaphoreGive(mutexEstado_);
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
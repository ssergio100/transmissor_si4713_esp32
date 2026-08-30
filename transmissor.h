#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include "modelos.h"
#include "radio_si4713.h"

enum class ComandoTipo : uint8_t {
  APLICAR_CONFIGURACAO = 0,
  SALVAR_CONFIGURACAO,
  RESTAURAR_PADROES,
  REINICIAR_RF,
  INICIAR_VARREDURA,
  CANCELAR_VARREDURA,
  APLICAR_FREQUENCIA
};

struct RespostaComando {
  SemaphoreHandle_t concluido = nullptr;
  bool resultado = false;
};

struct ComandoTransmissor {
  ComandoTipo tipo = ComandoTipo::APLICAR_CONFIGURACAO;
  ConfiguracaoTransmissor configuracao;
  uint16_t frequenciaKhz = 0;
  RespostaComando* resposta = nullptr;
};

class Transmissor {
 public:
  bool iniciar();
  void processar();

  ConfiguracaoTransmissor configuracao() const;
  TelemetriaTransmissor telemetria() const;
  ConfiguracaoTransmissor copiarConfiguracao() const;

  bool aplicarConfiguracao(ConfiguracaoTransmissor configuracao);
  bool salvarConfiguracao();
  bool restaurarPadroes();
  bool reiniciarRf();
  bool iniciarVarredura();
  bool cancelarVarredura();
  bool aplicarFrequencia(uint16_t frequenciaKhz);

  bool enviarComando(ComandoTransmissor& comando);

  size_t quantidadeMedicoes() const;
  MedicaoCanal medicao(size_t indice) const;
  uint16_t melhorFrequencia() const;
  uint8_t melhorNivelRuido() const;
  uint8_t enderecoRadio() const;
  bool horaValida() const;

 private:
  bool executarComando(const ComandoTransmissor& comando);
  void processarComandos();
  void atualizarRadioTextDinamico(bool forcar = false);
  bool formatarRadioText(char* destino, size_t tamanho) const;
  static String aplicarModelo(
      const char* modelo,
      const char* data,
      const char* hora
  );

  SemaphoreHandle_t mutexEstado_ = nullptr;
  QueueHandle_t filaComandos_ = nullptr;
  ConfiguracaoTransmissor configuracao_;
  RadioSi4713 radio_;
  char ultimoRadioTextAplicado_[33] = "";
  uint32_t ultimoSegundoRadioText_ = UINT32_MAX;
  bool radioDisponivelNoCicloAnterior_ = false;
};
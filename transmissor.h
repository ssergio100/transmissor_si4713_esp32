#pragma once

#include "modelos.h"
#include "radio_si4713.h"
#include "receptor_rda5807.h"

class Transmissor {
 public:
  bool iniciar();
  void processar();

  const ConfiguracaoTransmissor& configuracao() const;
  const TelemetriaTransmissor& telemetria() const;
  const TelemetriaReceptorRda5807& telemetriaReceptor() const;
  ConfiguracaoTransmissor copiarConfiguracao() const;

  bool aplicarConfiguracao(ConfiguracaoTransmissor configuracao);
  bool salvarConfiguracao();
  bool restaurarPadroes();
  bool reiniciarRf();
  bool iniciarVarredura();
  bool cancelarVarredura();
  bool iniciarAjusteFrequencia();
  bool previsualizarFrequencia(uint16_t frequenciaKhz);
  bool aplicarFrequencia(uint16_t frequenciaKhz);
  bool setLeituraAudio(bool habilitar);
  bool setLeituraAudioDisplay(bool habilitar);
  bool leituraAudioHabilitada() const;
  bool reconhecerInterrupcaoSi4713();
  void registrarDiagnosticoReceptor(const char* fase) const;

  size_t quantidadeMedicoes() const;
  const MedicaoCanal& medicao(size_t indice) const;
  uint16_t melhorFrequencia() const;
  uint8_t melhorNivelRuido() const;
  uint8_t enderecoRadio() const;
  bool horaValida() const;

 private:
  void atualizarRadioTextDinamico(bool forcar = false);
  bool formatarRadioText(char* destino, size_t tamanho) const;
  static String aplicarModelo(
      const char* modelo,
      const char* data,
      const char* hora
  );

  ConfiguracaoTransmissor configuracao_;
  RadioSi4713 radio_;
  ReceptorRda5807 receptor_;
  char ultimoRadioTextAplicado_[33] = "";
  uint32_t ultimoSegundoRadioText_ = UINT32_MAX;
  bool radioDisponivelNoCicloAnterior_ = false;
  bool leituraAudioWebSolicitada_ = false;
  bool leituraAudioDisplaySolicitada_ = false;
  bool ajusteFrequenciaAtivo_ = false;
};

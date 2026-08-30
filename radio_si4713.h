#pragma once

#include "modelos.h"
#include "si4713_seguro.h"

class RadioSi4713 {
 public:
  static constexpr size_t QUANTIDADE_MEDICOES =
      ((Configuracao::FREQUENCIA_MAXIMA_KHZ
        - Configuracao::FREQUENCIA_MINIMA_KHZ)
       / Configuracao::PASSO_FREQUENCIA_KHZ) + 1;

  RadioSi4713();

  bool iniciar();
  bool aplicar(const ConfiguracaoTransmissor& configuracao);
  bool reiniciarRf();
  void processar();

  bool iniciarVarredura();
  bool cancelarVarredura();
  size_t quantidadeMedicoes() const;
  const MedicaoCanal& medicao(size_t indice) const;
  uint16_t melhorFrequencia() const;
  uint8_t melhorNivelRuido() const;

  const TelemetriaTransmissor& telemetria() const;
  uint8_t endereco() const;

 private:
  bool inicializarNoEndereco(uint8_t endereco);
  bool enderecoResponde(uint8_t endereco);
  bool recuperar();
  bool confirmarOperacao(bool resultado, const char* operacao, bool critica);
  void resetFisico(bool registrar);
  void medirProximaFrequencia();
  bool restaurarAposVarredura();
  bool aplicarEstadoRf(
      const ConfiguracaoTransmissor& configuracao,
      const char* contexto
  );
  bool atualizarEstadoRfConfirmado(
      const ConfiguracaoTransmissor& configuracao,
      const char* contexto
  );

  Si4713Seguro radio_;
  ConfiguracaoTransmissor configuracaoAplicada_;
  TelemetriaTransmissor telemetria_;
  MedicaoCanal medicoes_[QUANTIDADE_MEDICOES];
  bool configurado_ = false;
  bool recuperacaoPendente_ = false;
  bool recuperacaoRfPendente_ = false;
  uint8_t endereco_ = 0;
  uint8_t proximoEnderecoRecuperacao_ = SI4710_ADDR1;
  uint8_t tentativasRecuperacaoDesdeReset_ = 0;
  uint8_t falhasConsecutivas_ = 0;
  size_t indiceVarredura_ = 0;
  uint32_t ultimaLeituraAudioMs_ = 0;
  uint32_t ultimaLeituraStatusMs_ = 0;
  uint32_t ultimaTentativaRecuperacaoMs_ = 0;
  uint32_t ultimaTentativaRecuperacaoRfMs_ = 0;
};

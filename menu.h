#pragma once

#include <Arduino.h>

#include "modelos.h"

class Menu {
 public:
  enum class Tela : uint8_t {
    PRINCIPAL = 0,
    RAIZ,
    RF,
    AUDIO,
    RDS,
    MONITOR,
    VARREDURA,
    SISTEMA
  };

  enum class Acao : uint8_t {
    NENHUMA = 0,
    APLICAR_CONFIGURACAO,
    INICIAR_AJUSTE_FREQUENCIA,
    PREVISUALIZAR_FREQUENCIA,
    APLICAR_FREQUENCIA,
    SALVAR_CONFIGURACAO,
    RESTAURAR_PADROES,
    INICIAR_VARREDURA,
    USAR_MELHOR_FREQUENCIA,
    CONFIGURAR_WIFI
  };

  enum ItemRaiz : uint8_t {
    RAIZ_RF = 0,
    RAIZ_AUDIO,
    RAIZ_RDS,
    RAIZ_MONITOR,
    RAIZ_VARREDURA,
    RAIZ_SISTEMA,
    RAIZ_VOLTAR,
    QUANTIDADE_RAIZ
  };

  enum ItemRf : uint8_t {
    RF_FREQUENCIA = 0,
    RF_POTENCIA,
    RF_ANTENA,
    RF_TRANSMISSAO,
    RF_VOLTAR,
    QUANTIDADE_RF
  };

  enum ItemAudio : uint8_t {
    AUDIO_ESTEREO = 0,
    AUDIO_PRE_ENFASE,
    AUDIO_DESVIO,
    AUDIO_MUDO,
    AUDIO_VOLTAR,
    QUANTIDADE_AUDIO
  };

  enum ItemRds : uint8_t {
    RDS_HABILITADO = 0,
    RDS_PS,
    RDS_TEXTO,
    RDS_PI,
    RDS_VOLTAR,
    QUANTIDADE_RDS
  };

  enum ItemVarredura : uint8_t {
    VARREDURA_INICIAR = 0,
    VARREDURA_USAR_MELHOR,
    VARREDURA_VOLTAR,
    QUANTIDADE_VARREDURA
  };

  enum ItemSistema : uint8_t {
    SISTEMA_SALVAR = 0,
    SISTEMA_PADROES,
    SISTEMA_WIFI,
    SISTEMA_INFO,
    SISTEMA_VOLTAR,
    QUANTIDADE_SISTEMA
  };

  Acao selecionar(ConfiguracaoTransmissor& configuracao);
  Acao voltar();
  Acao girar(int8_t deslocamento, ConfiguracaoTransmissor& configuracao);

  Tela tela() const;
  uint8_t itemSelecionado() const;
  bool editando() const;
  uint8_t cursorTexto() const;

 private:
  Acao selecionarRaiz();
  Acao selecionarRf(ConfiguracaoTransmissor& configuracao);
  Acao selecionarAudio(ConfiguracaoTransmissor& configuracao);
  Acao selecionarRds(ConfiguracaoTransmissor& configuracao);
  Acao selecionarVarredura();
  Acao selecionarSistema();
  void entrar(Tela tela);
  uint8_t quantidadeItens() const;
  static char girarCaractere(char atual, int8_t deslocamento);

  Tela tela_ = Tela::PRINCIPAL;
  uint8_t item_ = 0;
  uint8_t cursorTexto_ = 0;
  bool editando_ = false;
};

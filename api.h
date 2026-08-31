#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#include "frases_rds.h"
#include "transmissor.h"

class Api {
 public:
  Api(Transmissor& transmissor, FrasesRds& frasesRds);

  void iniciar();
  void processar();

 private:
  void registrarRotas();
  void responderEstado();
  void atualizarConfiguracao();
  void salvarConfiguracao();
  void restaurarPadroes();
  void controlarTransmissao();
  void reiniciarRf();
  void iniciarVarredura();
  void responderMedicoes();
  void previsualizarFrequencia();
  void aplicarFrequencia();
  void atualizarMonitorAudio();
  void responderMonitorAudio();
  void reconhecerInterrupcaoSi4713();
  void responderFrases();
  void substituirFrases();
  void abrirPortalWifi();
  void responderSaude();
  void responderOpcoes();
  void responderErro(int codigo, const char* erro, const char* detalhe);
  void responderJson(int codigo, const String& conteudo);
  bool lerCorpoJson(JsonDocument& documento);
  bool preencherConfiguracao(
      const JsonDocument& documento,
      ConfiguracaoTransmissor& configuracao,
      String& erro
  );
  String criarJsonEstado() const;
  String serializarEstado(bool comTipo) const;
  uint32_t assinaturaEstado() const;
  void publicarTelemetriaAudio();
  void publicarEstadoSeMudou();
  void vigiarMonitorAudio();

  static void tratarEventoWebSocket(
      uint8_t cliente,
      WStype_t tipo,
      uint8_t* dados,
      size_t tamanho
  );

  Transmissor& transmissor_;
  FrasesRds& frasesRds_;
  WebServer servidor_{80};
  WebSocketsServer websocket_{81};
  uint32_t ultimaSequenciaAudio_ = 0;
  uint32_t ultimaAssinaturaEstado_ = 0;
  bool ultimoEstadoNoAr_ = false;
  bool audioMonitorSolicitado_ = false;
  uint32_t ultimaConexaoAudioWsMs_ = 0;
};

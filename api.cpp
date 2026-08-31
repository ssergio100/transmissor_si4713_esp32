#include "api.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "configuracao.h"
#include "rede.h"

namespace {

bool copiarCampoTexto(
    const JsonDocument& documento,
    const char* campo,
    char* destino,
    size_t maximo,
    String& erro
) {
  if (!documento[campo].is<const char*>()) return true;
  const char* texto = documento[campo].as<const char*>();
  if (strlen(texto) > maximo) {
    erro = String(campo) + " excede " + String(maximo) + " caracteres";
    return false;
  }
  ConfiguracaoTransmissor::copiarTextoPreenchido(destino, maximo, texto);
  return true;
}

}  // namespace

Api::Api(Transmissor& transmissor, FrasesRds& frasesRds)
    : transmissor_(transmissor), frasesRds_(frasesRds) {}

void Api::iniciar() {
  registrarRotas();
  servidor_.begin();
  websocket_.begin();
  websocket_.onEvent(tratarEventoWebSocket);
  Serial.println("[API] REST na porta 80; WebSocket na porta 81");
}

void Api::processar() {
  servidor_.handleClient();
  websocket_.loop();
  publicarTelemetriaAudio();
  publicarEstadoSeMudou();
  vigiarMonitorAudio();
}

void Api::registrarRotas() {
  servidor_.on("/api/v1/state", HTTP_GET, [this]() { responderEstado(); });
  servidor_.on("/api/v1/settings", HTTP_PUT, [this]() { atualizarConfiguracao(); });
  servidor_.on("/api/v1/settings/save", HTTP_POST, [this]() { salvarConfiguracao(); });
  servidor_.on("/api/v1/settings/defaults", HTTP_POST, [this]() { restaurarPadroes(); });
  servidor_.on("/api/v1/tx", HTTP_POST, [this]() { controlarTransmissao(); });
  servidor_.on("/api/v1/tx/restart", HTTP_POST, [this]() { reiniciarRf(); });
  servidor_.on("/api/v1/audio/monitor", HTTP_PUT, [this]() { atualizarMonitorAudio(); });
  servidor_.on("/api/v1/audio/monitor", HTTP_GET, [this]() { responderMonitorAudio(); });
  servidor_.on(
      "/api/v1/si4713/interrupt/ack",
      HTTP_POST,
      [this]() { reconhecerInterrupcaoSi4713(); }
  );
  servidor_.on("/api/v1/scan/start", HTTP_POST, [this]() { iniciarVarredura(); });
  servidor_.on("/api/v1/scan/results", HTTP_GET, [this]() { responderMedicoes(); });
  servidor_.on(
      "/api/v1/frequency/adjust",
      HTTP_PUT,
      [this]() { previsualizarFrequencia(); }
  );
  servidor_.on("/api/v1/scan/apply", HTTP_POST, [this]() { aplicarFrequencia(); });
  servidor_.on("/api/v1/rds/phrases", HTTP_GET, [this]() { responderFrases(); });
  servidor_.on("/api/v1/rds/phrases", HTTP_PUT, [this]() { substituirFrases(); });
  servidor_.on("/api/v1/wifi/portal", HTTP_POST, [this]() { abrirPortalWifi(); });
  servidor_.on("/api/v1/health", HTTP_GET, [this]() { responderSaude(); });

  servidor_.onNotFound([this]() {
    if (servidor_.method() == HTTP_OPTIONS) {
      responderOpcoes();
    } else {
      responderErro(404, "rota_nao_encontrada", "Endpoint inexistente");
    }
  });
}

void Api::responderEstado() {
  responderJson(200, criarJsonEstado());
}

void Api::atualizarConfiguracao() {
  JsonDocument documento;
  if (!lerCorpoJson(documento)) return;

  ConfiguracaoTransmissor configuracao = transmissor_.copiarConfiguracao();
  String erro;
  if (!preencherConfiguracao(documento, configuracao, erro)) {
    responderErro(422, "configuracao_invalida", erro.c_str());
    return;
  }

  if (!transmissor_.aplicarConfiguracao(configuracao)) {
    responderErro(409, "nao_aplicada", "Si4713 ocupado, ausente ou em varredura");
    return;
  }
  responderJson(200, criarJsonEstado());
}

void Api::salvarConfiguracao() {
  if (!transmissor_.salvarConfiguracao()) {
    responderErro(500, "falha_persistencia", "Nao foi possivel salvar os ajustes");
    return;
  }
  responderJson(200, "{\"salvo\":true}");
}

void Api::restaurarPadroes() {
  if (!transmissor_.restaurarPadroes()) {
    responderErro(500, "falha_restauracao", "Nao foi possivel restaurar os padroes");
    return;
  }
  responderJson(200, criarJsonEstado());
}

void Api::controlarTransmissao() {
  JsonDocument documento;
  if (!lerCorpoJson(documento)) return;
  if (!documento["enabled"].is<bool>()) {
    responderErro(422, "campo_invalido", "enabled deve ser booleano");
    return;
  }
  ConfiguracaoTransmissor configuracao = transmissor_.copiarConfiguracao();
  configuracao.transmissaoHabilitada = documento["enabled"].as<bool>();

  if (!transmissor_.aplicarConfiguracao(configuracao)) {
    responderErro(409, "nao_aplicada", "Nao foi possivel alterar a transmissao");
    return;
  }
  responderJson(200, criarJsonEstado());
}

void Api::reiniciarRf() {
  if (!transmissor_.reiniciarRf()) {
    responderErro(
        409,
        "rf_nao_restaurado",
        "O Si4713 nao confirmou a restauracao do estado RF"
    );
    return;
  }
  responderJson(200, criarJsonEstado());
}

void Api::atualizarMonitorAudio() {
  JsonDocument documento;
  if (!lerCorpoJson(documento)) return;
  if (!documento["enabled"].is<bool>()) {
    responderErro(422, "campo_invalido", "enabled deve ser booleano");
    return;
  }
  const bool habilitar = documento["enabled"].as<bool>();
  audioMonitorSolicitado_ = habilitar;
  if (habilitar) {
    ultimaConexaoAudioWsMs_ = millis();
  }
  transmissor_.setLeituraAudio(habilitar);
  responderMonitorAudio();
}

void Api::responderMonitorAudio() {
  const TelemetriaTransmissor& telemetria = transmissor_.telemetria();
  JsonDocument documento;
  documento["enabled"] = audioMonitorSolicitado_;
  documento["active"] = transmissor_.leituraAudioHabilitada();
  documento["requested"] = audioMonitorSolicitado_;
  documento["sequence"] = telemetria.sequenciaAudio;
  documento["levelDbfs"] = telemetria.nivelAudioDbfs;
  documento["asq"] = telemetria.asq;
  documento["overmodulation"] = (telemetria.asq & 0x04) != 0;
  documento["onAir"] = telemetria.transmitindo;
  String resposta;
  serializeJson(documento, resposta);
  responderJson(200, resposta);
}

void Api::reconhecerInterrupcaoSi4713() {
  if (!transmissor_.reconhecerInterrupcaoSi4713()) {
    responderErro(
        409,
        "interrupcao_nao_reconhecida",
        "O Si4713 esta indisponivel ou nao confirmou o INTACK"
    );
    return;
  }
  responderJson(200, criarJsonEstado());
}

void Api::iniciarVarredura() {
  if (!transmissor_.iniciarVarredura()) {
    responderErro(409, "varredura_indisponivel", "Si4713 ocupado ou ausente");
    return;
  }
  responderJson(202, criarJsonEstado());
}

void Api::responderMedicoes() {
  JsonDocument documento;
  documento["running"] = transmissor_.telemetria().varreduraAtiva;
  documento["finished"] = transmissor_.telemetria().varreduraConcluida;
  documento["progress"] = transmissor_.telemetria().progressoVarredura;
  documento["recommendedFrequencyKhz"] = transmissor_.melhorFrequencia();
  documento["recommendedNoiseLevel"] = transmissor_.melhorNivelRuido();
  JsonArray medicoes = documento["measurements"].to<JsonArray>();
  for (size_t indice = 0; indice < transmissor_.quantidadeMedicoes(); indice++) {
    JsonObject item = medicoes.add<JsonObject>();
    item["frequencyKhz"] = transmissor_.medicao(indice).frequenciaKhz;
    item["noiseLevel"] = transmissor_.medicao(indice).nivelRuido;
  }
  String resposta;
  serializeJson(documento, resposta);
  responderJson(200, resposta);
}

void Api::previsualizarFrequencia() {
  JsonDocument documento;
  if (!lerCorpoJson(documento)) return;
  if (!documento["frequencyKhz"].is<uint16_t>()) {
    responderErro(422, "campo_invalido", "frequencyKhz e obrigatorio");
    return;
  }

  const uint16_t frequenciaKhz = documento["frequencyKhz"].as<uint16_t>();
  if (frequenciaKhz < Configuracao::FREQUENCIA_MINIMA_KHZ
      || frequenciaKhz > Configuracao::FREQUENCIA_MAXIMA_KHZ
      || frequenciaKhz % Configuracao::PASSO_FREQUENCIA_KHZ != 0) {
    responderErro(422, "frequencia_invalida", "Frequencia fora da faixa ou do passo permitido");
    return;
  }

  if (!transmissor_.previsualizarFrequencia(frequenciaKhz)) {
    responderErro(
        409,
        "frequencia_nao_ajustada",
        "Nao foi possivel pausar o TX ou aplicar o passo de frequencia"
    );
    return;
  }
  responderJson(200, criarJsonEstado());
}

void Api::aplicarFrequencia() {
  JsonDocument documento;
  if (!lerCorpoJson(documento)) return;
  if (!documento["frequencyKhz"].is<uint16_t>()) {
    responderErro(422, "campo_invalido", "frequencyKhz e obrigatorio");
    return;
  }

  const uint16_t frequenciaKhz = documento["frequencyKhz"].as<uint16_t>();
  if (frequenciaKhz < Configuracao::FREQUENCIA_MINIMA_KHZ
      || frequenciaKhz > Configuracao::FREQUENCIA_MAXIMA_KHZ
      || frequenciaKhz % Configuracao::PASSO_FREQUENCIA_KHZ != 0) {
    char detalhe[64];
    snprintf(
        detalhe,
        sizeof(detalhe),
        "Use uma frequencia entre %.1f e %.1f MHz, em passos de %.1f MHz",
        Configuracao::FREQUENCIA_MINIMA_KHZ / 100.0f,
        Configuracao::FREQUENCIA_MAXIMA_KHZ / 100.0f,
        Configuracao::PASSO_FREQUENCIA_KHZ / 100.0f
    );
    responderErro(
        422,
        "frequencia_invalida",
        detalhe
    );
    return;
  }

  if (!transmissor_.aplicarFrequencia(frequenciaKhz)) {
    responderErro(
        409,
        "frequencia_nao_aplicada",
        "O Si4713 esta ocupado ou indisponivel e nao conseguiu aplicar a frequencia"
    );
    return;
  }
  responderJson(200, criarJsonEstado());
}

void Api::responderFrases() {
  JsonDocument documento;
  JsonArray frases = documento["phrases"].to<JsonArray>();
  for (size_t indice = 0; indice < frasesRds_.quantidade(); indice++) {
    frases.add(frasesRds_.frase(indice));
  }
  documento["maxPhrases"] = FrasesRds::MAXIMO_FRASES;
  documento["maxLength"] = FrasesRds::MAXIMO_CARACTERES;
  String resposta;
  serializeJson(documento, resposta);
  responderJson(200, resposta);
}

void Api::substituirFrases() {
  JsonDocument documento;
  if (!lerCorpoJson(documento)) return;
  if (!documento["phrases"].is<JsonArray>()) {
    responderErro(422, "campo_invalido", "phrases deve ser uma lista");
    return;
  }
  JsonArray lista = documento["phrases"].as<JsonArray>();
  if (lista.size() > FrasesRds::MAXIMO_FRASES) {
    responderErro(422, "limite_excedido", "Quantidade maxima de frases excedida");
    return;
  }

  String frases[FrasesRds::MAXIMO_FRASES];
  size_t quantidade = 0;
  for (JsonVariant valor : lista) {
    if (!valor.is<const char*>()) {
      responderErro(422, "frase_invalida", "Toda frase deve ser texto");
      return;
    }
    frases[quantidade++] = valor.as<String>();
  }
  if (!frasesRds_.substituir(frases, quantidade)) {
    responderErro(422, "frase_invalida", "Frases devem ter entre 1 e 32 caracteres");
    return;
  }
  responderFrases();
}

void Api::abrirPortalWifi() {
  Rede::abrirPortalConfiguracao();
  responderJson(202, "{\"portalActive\":true,\"ssid\":\"TRANSMISSOR-SI4713\",\"ip\":\"192.168.4.1\"}");
}

void Api::responderSaude() {
  const TelemetriaTransmissor& telemetria = transmissor_.telemetria();
  JsonDocument documento;
  documento["firmwareVersion"] = Configuracao::VERSAO_FIRMWARE;
  documento["uptimeMs"] = millis();
  documento["freeHeap"] = ESP.getFreeHeap();
  documento["freePsram"] = ESP.getFreePsram();
  documento["wifiConnected"] = Rede::conectada();
  documento["wifiPortalActive"] = Rede::portalAtivo();
  documento["ip"] = Rede::enderecoIp();
  documento["rssi"] = Rede::conectada() ? WiFi.RSSI() : 0;
  documento["timeValid"] = transmissor_.horaValida();
  documento["si4713Available"] = telemetria.si4713Disponivel;
  documento["recovering"] = telemetria.recuperando;
  documento["recoveries"] = telemetria.recuperacoes;
  documento["i2cCommunicationFailures"] = telemetria.falhasComunicacao;
  documento["rfStateMismatches"] = telemetria.inconsistenciasRf;
  documento["si4713InterruptPin"] = Configuracao::PIN_INTERRUPCAO_SI4713;
  documento["si4713InterruptCount"] = telemetria.interrupcoesSi4713;
  documento["si4713LastInterruptMs"] =
      telemetria.ultimaInterrupcaoSi4713Ms;
  documento["si4713LastInterrupt"] = nomeEventoSi4713(telemetria);
  documento["si4713InterruptPending"] =
      telemetria.alarmeInterrupcaoSi4713Pendente;
  String resposta;
  serializeJson(documento, resposta);
  responderJson(200, resposta);
}

void Api::responderOpcoes() {
  servidor_.sendHeader("Access-Control-Allow-Origin", "*");
  servidor_.sendHeader("Access-Control-Allow-Methods", "GET,POST,PUT,OPTIONS");
  servidor_.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  servidor_.send(204, "text/plain", "");
}

void Api::responderErro(int codigo, const char* erro, const char* detalhe) {
  JsonDocument documento;
  documento["error"] = erro;
  documento["detail"] = detalhe;
  String resposta;
  serializeJson(documento, resposta);
  responderJson(codigo, resposta);
}

void Api::responderJson(int codigo, const String& conteudo) {
  servidor_.sendHeader("Access-Control-Allow-Origin", "*");
  servidor_.sendHeader("Cache-Control", "no-store");
  servidor_.send(codigo, "application/json; charset=utf-8", conteudo);
}

bool Api::lerCorpoJson(JsonDocument& documento) {
  if (!servidor_.hasArg("plain")) {
    responderErro(400, "corpo_ausente", "Envie um corpo JSON");
    return false;
  }
  const DeserializationError resultado = deserializeJson(
      documento,
      servidor_.arg("plain")
  );
  if (resultado != DeserializationError::Ok) {
    responderErro(400, "json_invalido", resultado.c_str());
    return false;
  }
  return true;
}

bool Api::preencherConfiguracao(
    const JsonDocument& documento,
    ConfiguracaoTransmissor& configuracao,
    String& erro
) {
  if (documento["frequencyKhz"].is<uint16_t>()) {
    configuracao.frequenciaKhz = documento["frequencyKhz"];
  }
  if (documento["powerDbuv"].is<uint8_t>()) {
    configuracao.potenciaDbuv = documento["powerDbuv"];
  }
  if (documento["antennaCap"].is<uint8_t>()) {
    configuracao.capacitanciaAntena = documento["antennaCap"];
  }
  if (documento["stereo"].is<bool>()) configuracao.estereo = documento["stereo"];
  if (documento["preemphasisUs"].is<uint8_t>()) {
    configuracao.preEnfaseUs = documento["preemphasisUs"];
  }
  if (documento["audioDeviationKhz"].is<uint8_t>()) {
    configuracao.desvioAudioKhz = documento["audioDeviationKhz"];
  }
  if (documento["muted"].is<bool>()) configuracao.audioMudo = documento["muted"];
  if (documento["rdsEnabled"].is<bool>()) {
    configuracao.rdsHabilitado = documento["rdsEnabled"];
  }
  if (documento["rdsPi"].is<uint16_t>()) configuracao.rdsPi = documento["rdsPi"];

  if (!copiarCampoTexto(documento, "rdsPs", configuracao.rdsPs, 8, erro)
      || !copiarCampoTexto(documento, "rdsText", configuracao.rdsText, 32, erro)
      || !copiarCampoTexto(documento, "rdsTemplate", configuracao.rdsModelo, 32, erro)) {
    return false;
  }

  if (documento["rdsSource"].is<const char*>()) {
    if (!converterFonteRadioText(
            documento["rdsSource"].as<const char*>(),
            configuracao.fonteRadioText
        )) {
      erro = "rdsSource desconhecido";
      return false;
    }
  }

  configuracao.sanitizarTextos();
  if (!configuracao.valoresValidos()) {
    erro = "Um ou mais valores estao fora dos limites permitidos";
    return false;
  }
  return true;
}

String Api::criarJsonEstado() const {
  return serializarEstado(false);
}

String Api::serializarEstado(bool comTipo) const {
  const ConfiguracaoTransmissor& configuracao = transmissor_.configuracao();
  const TelemetriaTransmissor& telemetria = transmissor_.telemetria();
  JsonDocument documento;

  if (comTipo) documento["type"] = "state";

  JsonObject desejado = documento["desired"].to<JsonObject>();
  desejado["frequencyKhz"] = configuracao.frequenciaKhz;
  desejado["powerDbuv"] = configuracao.potenciaDbuv;
  desejado["antennaCap"] = configuracao.capacitanciaAntena;
  desejado["txEnabled"] = configuracao.transmissaoHabilitada;
  desejado["stereo"] = configuracao.estereo;
  desejado["preemphasisUs"] = configuracao.preEnfaseUs;
  desejado["audioDeviationKhz"] = configuracao.desvioAudioKhz;
  desejado["muted"] = configuracao.audioMudo;
  desejado["rdsEnabled"] = configuracao.rdsHabilitado;
  desejado["rdsPi"] = configuracao.rdsPi;
  desejado["rdsPs"] = configuracao.rdsPs;
  desejado["rdsText"] = configuracao.rdsText;
  desejado["rdsTemplate"] = configuracao.rdsModelo;
  desejado["rdsSource"] = nomeFonteRadioText(configuracao.fonteRadioText);

  JsonObject aplicado = documento["applied"].to<JsonObject>();
  aplicado["onAir"] = telemetria.transmitindo;
  aplicado["frequencyKhz"] = telemetria.frequenciaEfetivaKhz;
  aplicado["powerDbuv"] = telemetria.potenciaEfetivaDbuv;
  aplicado["antennaCap"] = telemetria.capacitanciaEfetiva;
  aplicado["audioLevelDbfs"] = telemetria.nivelAudioDbfs;
  aplicado["asq"] = telemetria.asq;

  JsonObject sistema = documento["system"].to<JsonObject>();
  sistema["si4713Available"] = telemetria.si4713Disponivel;
  sistema["recovering"] = telemetria.recuperando;
  sistema["recoveries"] = telemetria.recuperacoes;
  sistema["i2cCommunicationFailures"] = telemetria.falhasComunicacao;
  sistema["rfStateMismatches"] = telemetria.inconsistenciasRf;
  sistema["si4713InterruptPin"] = Configuracao::PIN_INTERRUPCAO_SI4713;
  sistema["si4713InterruptCount"] = telemetria.interrupcoesSi4713;
  sistema["si4713LastInterruptMs"] =
      telemetria.ultimaInterrupcaoSi4713Ms;
  sistema["si4713LastInterrupt"] = nomeEventoSi4713(telemetria);
  sistema["si4713InterruptPending"] =
      telemetria.alarmeInterrupcaoSi4713Pendente;
  sistema["scanRunning"] = telemetria.varreduraAtiva;
  sistema["scanFinished"] = telemetria.varreduraConcluida;
  sistema["scanProgress"] = telemetria.progressoVarredura;
  sistema["wifiConnected"] = Rede::conectada();
  sistema["wifiPortalActive"] = Rede::portalAtivo();
  sistema["ip"] = Rede::enderecoIp();
  sistema["timeValid"] = transmissor_.horaValida();
  sistema["uptimeMs"] = millis();
  sistema["firmwareVersion"] = Configuracao::VERSAO_FIRMWARE;
  sistema["frequencyMinKhz"] = Configuracao::FREQUENCIA_MINIMA_KHZ;
  sistema["frequencyMaxKhz"] = Configuracao::FREQUENCIA_MAXIMA_KHZ;
  sistema["frequencyStepKhz"] = Configuracao::PASSO_FREQUENCIA_KHZ;

  String resposta;
  serializeJson(documento, resposta);
  return resposta;
}

void Api::publicarTelemetriaAudio() {
  const TelemetriaTransmissor& telemetria = transmissor_.telemetria();
  const bool audioMudou =
      telemetria.sequenciaAudio != ultimaSequenciaAudio_;
  const bool estadoNoArMudou =
      telemetria.transmitindo != ultimoEstadoNoAr_;
  if (websocket_.connectedClients() == 0
      || (!audioMudou && !estadoNoArMudou)) {
    return;
  }
  ultimaSequenciaAudio_ = telemetria.sequenciaAudio;
  ultimoEstadoNoAr_ = telemetria.transmitindo;

  JsonDocument documento;
  documento["type"] = "audio";
  documento["sequence"] = telemetria.sequenciaAudio;
  documento["timestampMs"] = millis();
  documento["levelDbfs"] = telemetria.nivelAudioDbfs;
  documento["asq"] = telemetria.asq;
  documento["overmodulation"] = (telemetria.asq & 0x04) != 0;
  documento["onAir"] = telemetria.transmitindo;

  String mensagem;
  serializeJson(documento, mensagem);
  websocket_.broadcastTXT(mensagem);
}

void Api::publicarEstadoSeMudou() {
  if (websocket_.connectedClients() == 0) return;

  const uint32_t assinatura = assinaturaEstado();
  if (assinatura == ultimaAssinaturaEstado_) return;
  ultimaAssinaturaEstado_ = assinatura;

  String mensagem = serializarEstado(true);
  websocket_.broadcastTXT(mensagem);
}

void Api::vigiarMonitorAudio() {
  if (!audioMonitorSolicitado_) return;
  const uint32_t agora = millis();
  if (websocket_.connectedClients() > 0) {
    ultimaConexaoAudioWsMs_ = agora;
    return;
  }
  if (agora - ultimaConexaoAudioWsMs_
      >= Configuracao::TEMPO_AUTO_DESATIVAR_AUDIO_MS) {
    audioMonitorSolicitado_ = false;
    transmissor_.setLeituraAudio(false);
    Serial.println(
        "[AUDIO] solicitacao web encerrada: sem cliente WebSocket"
    );
  }
}

uint32_t Api::assinaturaEstado() const {
  const ConfiguracaoTransmissor& configuracao = transmissor_.configuracao();
  const TelemetriaTransmissor& telemetria = transmissor_.telemetria();

  uint32_t assinatura = 2166136261u;
  const auto dobra = [&assinatura](uint32_t valor) {
    assinatura ^= valor;
    assinatura *= 16777619u;
  };

  dobra(configuracao.frequenciaKhz);
  dobra(configuracao.potenciaDbuv);
  dobra(configuracao.capacitanciaAntena);
  dobra(configuracao.rdsPi);
  dobra(configuracao.preEnfaseUs);
  dobra(configuracao.desvioAudioKhz);
  dobra(configuracao.estereo);
  dobra(configuracao.transmissaoHabilitada);
  dobra(configuracao.rdsHabilitado);
  dobra(configuracao.audioMudo);
  dobra(static_cast<uint8_t>(configuracao.fonteRadioText));
  for (char caractere : configuracao.rdsPs) dobra(static_cast<uint8_t>(caractere));
  for (char caractere : configuracao.rdsText) {
    dobra(static_cast<uint8_t>(caractere));
  }
  for (char caractere : configuracao.rdsModelo) {
    dobra(static_cast<uint8_t>(caractere));
  }

  dobra(telemetria.si4713Disponivel);
  dobra(telemetria.recuperando);
  dobra(telemetria.frequenciaEfetivaKhz);
  dobra(telemetria.potenciaEfetivaDbuv);
  dobra(telemetria.capacitanciaEfetiva);
  dobra(telemetria.varreduraAtiva);
  dobra(telemetria.varreduraConcluida);
  dobra(telemetria.progressoVarredura);
  dobra(telemetria.recuperacoes);
  dobra(telemetria.interrupcoesSi4713);
  dobra(telemetria.ultimaInterrupcaoSi4713Ms);
  dobra(telemetria.ultimoEventoAsq);
  dobra(telemetria.ultimaInterrupcaoLida);
  dobra(telemetria.alarmeInterrupcaoSi4713Pendente);
  return assinatura;
}

void Api::tratarEventoWebSocket(
    uint8_t cliente,
    WStype_t tipo,
    uint8_t*,
    size_t
) {
  if (tipo == WStype_CONNECTED) {
    Serial.printf("[WS] Cliente %u conectado\n", cliente);
  } else if (tipo == WStype_DISCONNECTED) {
    Serial.printf("[WS] Cliente %u desconectado\n", cliente);
  }
}

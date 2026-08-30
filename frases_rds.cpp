#include "frases_rds.h"

#include <ArduinoJson.h>
#include <Preferences.h>

namespace {

constexpr char ESPACO_NVS[] = "si4713-rds";
constexpr char CHAVE_FRASES[] = "frases";
const String FRASE_VAZIA;

}  // namespace

bool FrasesRds::carregar() {
  Preferences preferencias;
  if (!preferencias.begin(ESPACO_NVS, true)) return false;
  const String json = preferencias.getString(CHAVE_FRASES, "[]");
  preferencias.end();

  JsonDocument documento;
  if (deserializeJson(documento, json) != DeserializationError::Ok
      || !documento.is<JsonArray>()) {
    quantidade_ = 0;
    return false;
  }

  quantidade_ = 0;
  for (JsonVariant valor : documento.as<JsonArray>()) {
    if (!valor.is<const char*>() || quantidade_ >= MAXIMO_FRASES) break;
    String texto = valor.as<String>();
    texto.trim();
    if (texto.isEmpty() || texto.length() > MAXIMO_CARACTERES) continue;
    frases_[quantidade_++] = texto;
  }
  return true;
}

bool FrasesRds::substituir(const String* frases, size_t quantidade) {
  if (quantidade > MAXIMO_FRASES) return false;
  for (size_t indice = 0; indice < quantidade; indice++) {
    String texto = frases[indice];
    texto.trim();
    if (texto.isEmpty() || texto.length() > MAXIMO_CARACTERES) return false;
  }

  for (size_t indice = 0; indice < quantidade; indice++) {
    frases_[indice] = frases[indice];
    frases_[indice].trim();
  }
  for (size_t indice = quantidade; indice < quantidade_; indice++) {
    frases_[indice] = "";
  }
  quantidade_ = quantidade;
  return salvar();
}

bool FrasesRds::salvar() const {
  JsonDocument documento;
  JsonArray lista = documento.to<JsonArray>();
  for (size_t indice = 0; indice < quantidade_; indice++) {
    lista.add(frases_[indice]);
  }

  String json;
  serializeJson(documento, json);

  Preferences preferencias;
  if (!preferencias.begin(ESPACO_NVS, false)) return false;
  const size_t gravados = preferencias.putString(CHAVE_FRASES, json);
  preferencias.end();
  return gravados == json.length();
}

size_t FrasesRds::quantidade() const { return quantidade_; }

const String& FrasesRds::frase(size_t indice) const {
  return indice < quantidade_ ? frases_[indice] : FRASE_VAZIA;
}

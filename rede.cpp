#include "rede.h"

#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "configuracao.h"

namespace {

WiFiManager gerenciadorWifi;
bool mdnsIniciado = false;
bool horarioConfigurado = false;
bool estavaConectada = false;
bool portalSolicitado = false;

void tratarPortal(WiFiManager* gerenciador) {
  portalSolicitado = true;
  Serial.println("[WIFI] Portal de configuracao iniciado");
  Serial.print("[WIFI] Rede: ");
  Serial.println(gerenciador->getConfigPortalSSID());
  Serial.println("[WIFI] Endereco: http://192.168.4.1");
}

void tratarConexaoEstabelecida() {
  Serial.print("[WIFI] Conectado a ");
  Serial.println(WiFi.SSID());
  Serial.print("[WIFI] IP: ");
  Serial.println(WiFi.localIP());

  if (!mdnsIniciado) {
    mdnsIniciado = MDNS.begin(Configuracao::NOME_HOST);
    if (mdnsIniciado) {
      MDNS.addService("http", "tcp", 80);
      MDNS.addService("ws", "tcp", 81);
      Serial.printf("[WIFI] mDNS: http://%s.local\n", Configuracao::NOME_HOST);
    }
  }

  if (!horarioConfigurado) {
    configTime(
        Configuracao::FUSO_HORARIO_SEGUNDOS,
        Configuracao::AJUSTE_HORARIO_VERAO_SEGUNDOS,
        Configuracao::SERVIDOR_NTP_PRIMARIO,
        Configuracao::SERVIDOR_NTP_SECUNDARIO
    );
    horarioConfigurado = true;
  }
}

}  // namespace

void Rede::iniciar() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  gerenciadorWifi.setAPCallback(tratarPortal);
  gerenciadorWifi.setConfigPortalBlocking(false);
  gerenciadorWifi.setConnectTimeout(20);
  gerenciadorWifi.setHostname(Configuracao::NOME_HOST);
  gerenciadorWifi.autoConnect(Configuracao::NOME_PORTAL_WIFI);

  estavaConectada = WiFi.status() == WL_CONNECTED;
  if (estavaConectada) tratarConexaoEstabelecida();
}

void Rede::processar() {
  gerenciadorWifi.process();
  const bool agoraConectada = WiFi.status() == WL_CONNECTED;

  if (agoraConectada && !estavaConectada) {
    portalSolicitado = false;
    tratarConexaoEstabelecida();
  } else if (!agoraConectada && estavaConectada) {
    Serial.println("[WIFI] Conexao perdida");
  }

  estavaConectada = agoraConectada;
}

void Rede::abrirPortalConfiguracao() {
  if (portalSolicitado) return;
  portalSolicitado = true;
  gerenciadorWifi.startConfigPortal(Configuracao::NOME_PORTAL_WIFI);
}

bool Rede::conectada() {
  return WiFi.status() == WL_CONNECTED;
}

bool Rede::portalAtivo() {
  return portalSolicitado;
}

String Rede::enderecoIp() {
  if (conectada()) return WiFi.localIP().toString();
  if (portalAtivo()) return WiFi.softAPIP().toString();
  return "0.0.0.0";
}

#include <Arduino.h>
#include <Wire.h>

#include "api.h"
#include "configuracao.h"
#include "controles.h"
#include "display.h"
#include "diagnostico_i2c.h"
#include "frases_rds.h"
#include "menu.h"
#include "rede.h"
#include "transmissor.h"

Controles controles;
Display display;
Menu menu;
Transmissor transmissor;
FrasesRds frasesRds;
Api api(transmissor, frasesRds);

namespace {

uint32_t ultimaAtualizacaoDisplayMs = 0;

void executarAcao(
    Menu::Acao acao,
    ConfiguracaoTransmissor configuracaoEditada
) {
  switch (acao) {
    case Menu::Acao::APLICAR_CONFIGURACAO:
      if (!transmissor.aplicarConfiguracao(configuracaoEditada)) {
        Serial.println("[AVISO] Configuracao nao aplicada");
      }
      break;

    case Menu::Acao::SALVAR_CONFIGURACAO:
      if (transmissor.salvarConfiguracao()) {
        Serial.println("[NVS] Configuracao salva");
        display.mostrarMensagem("Configuracao salva", "Memoria atualizada");
      } else {
        Serial.println("[ERRO] Falha ao salvar configuracao");
        display.mostrarMensagem("Falha ao salvar", "Verifique o log");
      }
      break;

    case Menu::Acao::RESTAURAR_PADROES:
      if (transmissor.restaurarPadroes()) {
        Serial.println("[SISTEMA] Padroes restaurados");
        display.mostrarMensagem("Padroes restaurados", "TX desligado");
      }
      break;

    case Menu::Acao::INICIAR_VARREDURA:
      if (transmissor.iniciarVarredura()) {
        Serial.println("[SCAN] Varredura iniciada; TX pausado");
      }
      break;

    case Menu::Acao::USAR_MELHOR_FREQUENCIA: {
      const uint16_t melhor = transmissor.melhorFrequencia();
      if (melhor != 0 && transmissor.aplicarFrequencia(melhor)) {
        Serial.printf("[SCAN] Frequencia aplicada: %u\n", melhor);
      }
      break;
    }

    case Menu::Acao::CONFIGURAR_WIFI:
      Rede::abrirPortalConfiguracao();
      display.mostrarMensagem("Portal Wi-Fi ativo", "192.168.4.1");
      break;

    default:
      break;
  }
}

void processarControles() {
  if (transmissor.telemetria().varreduraAtiva) {
    const bool clique = controles.consumirClique();
    const bool pressaoLonga = controles.consumirPressaoLonga();
    const int8_t giro = controles.consumirGiro();
    if (!clique && !pressaoLonga && giro == 0) return;

    if (transmissor.cancelarVarredura()) {
      Serial.println("[SCAN] Varredura cancelada pelo encoder; TX restaurado");
    } else {
      Serial.println("[AVISO] Falha ao restaurar TX apos cancelar varredura");
    }

    ConfiguracaoTransmissor editada = transmissor.copiarConfiguracao();
    if (pressaoLonga) {
      executarAcao(menu.voltar(), editada);
    } else if (giro != 0) {
      executarAcao(menu.girar(giro, editada), editada);
    }
    return;
  }

  ConfiguracaoTransmissor editada = transmissor.copiarConfiguracao();

  const bool clique = controles.consumirClique();
  if (clique) {
    if (Configuracao::LOG_EVENTOS_ENCODER) Serial.println("[ENCODER] clique");
    executarAcao(menu.selecionar(editada), editada);
    editada = transmissor.copiarConfiguracao();
  }

  const bool pressaoLonga = controles.consumirPressaoLonga();
  if (pressaoLonga) {
    if (Configuracao::LOG_EVENTOS_ENCODER) {
      Serial.println("[ENCODER] pressao longa");
    }
    executarAcao(menu.voltar(), editada);
    editada = transmissor.copiarConfiguracao();
  }

  const int8_t giro = controles.consumirGiro();
  if (giro != 0) {
    if (Configuracao::LOG_EVENTOS_ENCODER) {
      Serial.printf("[ENCODER] giro=%d\n", giro);
    }
    executarAcao(menu.girar(giro, editada), editada);
  }
}

void renderizarDisplay() {
  display.renderizar(
      menu,
      transmissor.configuracao(),
      transmissor.telemetria(),
      transmissor.melhorFrequencia(),
      transmissor.melhorNivelRuido()
  );
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.printf(
      "[BOOT] Transmissor Si4713 ESP32-S3 v%s\n",
      Configuracao::VERSAO_FIRMWARE
  );

  Wire.begin(Configuracao::PIN_I2C_SDA, Configuracao::PIN_I2C_SCL);
  Wire.setClock(Configuracao::FREQUENCIA_I2C_HZ);
  Wire.setTimeOut(25);
  Serial.printf(
      "[OK] I2C iniciado: SDA=%d SCL=%d %luHz\n",
      Configuracao::PIN_I2C_SDA,
      Configuracao::PIN_I2C_SCL,
      static_cast<unsigned long>(Configuracao::FREQUENCIA_I2C_HZ)
  );

  controles.iniciar();
  Serial.println("[OK] Encoder inicializado");
  Serial.printf(
      "[ENCODER] repouso inicial: DT=%d CLK=%d SW=%d; logs ativos\n",
      digitalRead(Configuracao::PIN_ENCODER_DT),
      digitalRead(Configuracao::PIN_ENCODER_CLK),
      digitalRead(Configuracao::PIN_ENCODER_BOTAO)
  );

  if (display.iniciar()) {
    display.mostrarInicializacao();
    Serial.println("[OK] LCD 20x4 encontrado em 0x27");
  } else {
    Serial.println("[AVISO] LCD nao respondeu em 0x27");
  }

  if (transmissor.iniciar()) {
    Serial.printf(
        "[OK] Si4713 iniciado em 0x%02X\n",
        transmissor.enderecoRadio()
    );
  } else {
    Serial.println("[AVISO] Si4713 indisponivel; recuperacao automatica ativa");
    display.mostrarMensagem("Si4713 ausente", "Tentando recuperar");
  }

  DiagnosticoI2c::executarNoBoot();

  if (!frasesRds.carregar()) {
    Serial.println("[AVISO] Frases RDS iniciadas com valores padrao");
  }

  Rede::iniciar();
  api.iniciar();

  renderizarDisplay();
  Serial.println("[BOOT] Sistema local, rede e API inicializados");
}

void loop() {
  controles.processar();
  processarControles();
  transmissor.processar();
  Rede::processar();

  const uint32_t agora = millis();
  if (agora - ultimaAtualizacaoDisplayMs
      >= Configuracao::INTERVALO_ATUALIZACAO_DISPLAY_MS) {
    ultimaAtualizacaoDisplayMs = agora;
    renderizarDisplay();
  }

  delay(2);
}

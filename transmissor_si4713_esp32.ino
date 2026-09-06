#include <Arduino.h>
#include <Wire.h>

#include "api.h"
#include "apresentacao.h"
#include "configuracao.h"
#include "controles.h"
#include "display_tft.h"
#include "diagnostico_i2c.h"
#include "frases_rds.h"
#include "menu.h"
#include "rede.h"
#include "transmissor.h"

Controles controles;
DisplayTft displayTft;
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

    case Menu::Acao::INICIAR_AJUSTE_FREQUENCIA:
      if (!transmissor.iniciarAjusteFrequencia()) {
        Serial.println("[AVISO] Nao foi possivel iniciar o ajuste de frequencia");
      }
      break;

    case Menu::Acao::PREVISUALIZAR_FREQUENCIA:
      if (!transmissor.previsualizarFrequencia(
              configuracaoEditada.frequenciaKhz
          )) {
        Serial.println("[AVISO] Passo de frequencia nao aplicado");
      }
      break;

    case Menu::Acao::APLICAR_FREQUENCIA:
      if (transmissor.aplicarFrequencia(configuracaoEditada.frequenciaKhz)) {
        Serial.printf(
            "[NVS] Frequencia aplicada e salva: %u\n",
            configuracaoEditada.frequenciaKhz
        );
      } else {
        Serial.println("[ERRO] Falha ao aplicar ou salvar frequencia");
      }
      break;

    case Menu::Acao::SALVAR_CONFIGURACAO:
      if (transmissor.salvarConfiguracao()) {
        Serial.println("[NVS] Configuracao salva");
      } else {
        Serial.println("[ERRO] Falha ao salvar configuracao");
      }
      break;

    case Menu::Acao::RESTAURAR_PADROES:
      if (transmissor.restaurarPadroes()) {
        Serial.println("[SISTEMA] Padroes restaurados");
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
        Serial.printf("[SCAN] Frequencia aplicada e salva: %u\n", melhor);
      }
      break;
    }

    case Menu::Acao::CONFIGURAR_WIFI:
      Rede::abrirPortalConfiguracao();
      break;

    default:
      break;
  }
}

bool processarControles() {
  const Controles::Evento evento = controles.consumirEvento();
  if (evento.tipo == Controles::TipoEvento::NENHUM) return false;

  if (transmissor.telemetria().varreduraAtiva) {
    if (transmissor.cancelarVarredura()) {
      Serial.println("[SCAN] Varredura cancelada pelo encoder; TX restaurado");
    } else {
      Serial.println("[AVISO] Falha ao restaurar TX apos cancelar varredura");
    }

    ConfiguracaoTransmissor editada = transmissor.copiarConfiguracao();
    if (evento.tipo == Controles::TipoEvento::PRESSAO_LONGA) {
      executarAcao(menu.voltar(), editada);
    } else if (evento.tipo == Controles::TipoEvento::GIRO) {
      executarAcao(menu.girar(evento.deslocamento, editada), editada);
    }
    return true;
  }

  ConfiguracaoTransmissor editada = transmissor.copiarConfiguracao();
  switch (evento.tipo) {
    case Controles::TipoEvento::CLIQUE:
      if (Configuracao::LOG_EVENTOS_ENCODER) {
        Serial.printf(
            "[ENCODER] clique curto: %lu ms\n",
            static_cast<unsigned long>(evento.duracaoPressaoMs)
        );
      }
      executarAcao(menu.selecionar(editada), editada);
      break;

    case Controles::TipoEvento::PRESSAO_LONGA:
      if (Configuracao::LOG_EVENTOS_ENCODER) {
        Serial.printf(
            "[ENCODER] pressao longa: %lu ms\n",
            static_cast<unsigned long>(evento.duracaoPressaoMs)
        );
      }
      executarAcao(menu.voltar(), editada);
      break;

    case Controles::TipoEvento::GIRO:
      if (Configuracao::LOG_EVENTOS_ENCODER) {
        Serial.printf("[ENCODER] giro=%d\n", evento.deslocamento);
      }
      executarAcao(menu.girar(evento.deslocamento, editada), editada);
      break;

    default:
      break;
  }
  return true;
}

void renderizarDisplay() {
  const EstadoPainel estado = Apresentacao::gerar(menu, transmissor);
  displayTft.renderizar(estado);
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

  // O RDA5807 tambem responde em 0x60 no modo compativel com TEA5767. Uma
  // varredura ampla depois da sintonia muda o canal fisico do receptor para
  // zero sem atualizar o registrador 0x03. Execute o scan antes de configurar
  // qualquer periferico; a inicializacao abaixo restabelece seus estados.
  DiagnosticoI2c::executarNoBoot();

  controles.iniciar();
  Serial.println("[OK] Encoder inicializado");
  Serial.printf(
      "[ENCODER] repouso inicial: DT=%d CLK=%d SW=%d; logs ativos\n",
      digitalRead(Configuracao::PIN_ENCODER_DT),
      digitalRead(Configuracao::PIN_ENCODER_CLK),
      digitalRead(Configuracao::PIN_ENCODER_BOTAO)
  );
  Serial.printf(
      "[ENCODER] contrato: %u transicoes/passo, curto >=%u ms, longo >=%lu ms (evento na soltura)\n",
      Configuracao::TRANSICOES_ENCODER_POR_DETENTE,
      Configuracao::TEMPO_PRESSIONAMENTO_CURTO_MINIMO_MS,
      static_cast<unsigned long>(Configuracao::TEMPO_PRESSIONAMENTO_LONGO_MS)
  );

  if (displayTft.iniciar()) {
    displayTft.mostrarInicializacao();
    Serial.printf(
        "[OK] TFT ST7789 inicializado: %ux%u SPI CS=%d DC=%d RST=%d MOSI=%d SCLK=%d\n",
        Configuracao::TFT_LARGURA,
        Configuracao::TFT_ALTURA,
        Configuracao::PIN_TFT_CS,
        Configuracao::PIN_TFT_DC,
        Configuracao::PIN_TFT_RESET,
        Configuracao::PIN_TFT_MOSI,
        Configuracao::PIN_TFT_SCLK
    );
    Serial.println("[TFT] Interface somente escrita; presenca nao confirmavel por software");
  } else {
    Serial.println("[ERRO] Nao foi possivel iniciar o barramento SPI do TFT");
  }

  if (transmissor.iniciar()) {
    Serial.printf(
        "[OK] Si4713 iniciado em 0x%02X\n",
        transmissor.enderecoRadio()
    );
  } else {
    Serial.println("[AVISO] Si4713 indisponivel; recuperacao automatica ativa");
  }

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
  const bool controleProcessado = processarControles();
  transmissor.setLeituraAudioDisplay(menu.tela() == Menu::Tela::MONITOR);
  transmissor.processar();
  Rede::processar();
  api.processar();

  const uint32_t agora = millis();
  if (controleProcessado
      || agora - ultimaAtualizacaoDisplayMs
      >= Configuracao::INTERVALO_ATUALIZACAO_DISPLAY_MS) {
    ultimaAtualizacaoDisplayMs = agora;
    renderizarDisplay();
  }

  delay(2);
}

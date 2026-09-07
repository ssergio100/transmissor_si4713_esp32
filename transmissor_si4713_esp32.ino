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

// acao: pedido do menu. configuracaoEditada: copia atual com o campo confirmado.
void executarAcao(Menu::Acao acao, const ConfiguracaoTransmissor& configuracaoEditada) {
  using Acao = Menu::Acao;
  switch (acao) {
    case Acao::APLICAR_CONFIGURACAO:
      menu.concluirAplicacao(transmissor.aplicarConfiguracao(configuracaoEditada));
      break;
    case Acao::APLICAR_FREQUENCIA:
      menu.concluirAplicacao(transmissor.aplicarFrequencia(configuracaoEditada.frequenciaKhz));
      break;
    case Acao::INICIAR_VARREDURA:
      menu.confirmarInicioVarredura(transmissor.iniciarVarredura());
      break;
    case Acao::CANCELAR_VARREDURA:
      if (!transmissor.cancelarVarredura()) menu.informarErro("Falha ao restaurar TX");
      break;
    case Acao::USAR_MELHOR_FREQUENCIA: {
      const uint16_t melhor = transmissor.melhorFrequencia();
      const bool resultadoValido = transmissor.telemetria().varreduraConcluida && melhor != 0;
      menu.concluirAplicacao(resultadoValido && transmissor.aplicarFrequencia(melhor));
      break;
    }
    default: break;
  }
}

bool processarControles() {
  const Controles::Evento evento = controles.consumirEvento();
  using Evento = Controles::TipoEvento;
  if (evento.tipo == Evento::NENHUM) return false;

  ConfiguracaoTransmissor editada = transmissor.copiarConfiguracao();
  Menu::Acao acao = Menu::NENHUMA;
  switch (evento.tipo) {
    case Evento::CLIQUE: acao = menu.selecionar(editada); break;
    case Evento::PRESSAO_LONGA: acao = menu.sairParaPrincipal(); break;
    case Evento::GIRO: acao = menu.girar(evento.deslocamento); break;
    default: break;
  }
  if (Configuracao::LOG_EVENTOS_ENCODER) {
    const char* nome = evento.tipo == Evento::GIRO ? "giro"
        : evento.tipo == Evento::CLIQUE ? "clique" : "saida direta";
    Serial.printf("[ENCODER] %s: passos=%d, duracao=%lu ms\n", nome,
                  evento.deslocamento, static_cast<unsigned long>(evento.duracaoPressaoMs));
  }
  // Primeiro o menu termina de alterar a copia; so depois ela vai ao hardware.
  executarAcao(acao, editada);
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
    Serial.println("[ERRO] Falha ao iniciar SPI ou alocar a janela do TFT");
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

  // Sincroniza depois do radio e da API para desenhar o resultado atual.
  const auto& telemetria = transmissor.telemetria();
  menu.atualizarVarredura(telemetria.varreduraAtiva, telemetria.varreduraConcluida,
                         transmissor.melhorFrequencia());

  const uint32_t agora = millis();
  if (controleProcessado
      || agora - ultimaAtualizacaoDisplayMs
      >= Configuracao::INTERVALO_ATUALIZACAO_DISPLAY_MS) {
    ultimaAtualizacaoDisplayMs = agora;
    renderizarDisplay();
  }

  delay(2);
}

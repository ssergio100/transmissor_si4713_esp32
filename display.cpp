#include "display.h"

#include <Wire.h>
#include <string.h>

#include "configuracao.h"

namespace {

constexpr uint8_t ASQ_SOBREMODULACAO = 0x04;

}  // namespace

Display::Display()
    : lcd_(
          Configuracao::LCD_ENDERECO,
          Configuracao::LCD_COLUNAS,
          Configuracao::LCD_LINHAS
      ) {}

bool Display::iniciar() {
  Wire.beginTransmission(Configuracao::LCD_ENDERECO);
  if (Wire.endTransmission() != 0) return false;

  lcd_.init();
  lcd_.backlight();
  lcd_.clear();
  invalidarCache();
  pronto_ = true;
  return true;
}

void Display::mostrarInicializacao() {
  if (!pronto_) return;
  lcd_.clear();
  invalidarCache();
  escreverLinha(0, "Transmissor FM");
  escreverLinha(1, "Si4713 + ESP32-S3");
  escreverLinha(2, "Inicializando...");
  escreverLinha(3, "");
}

void Display::mostrarMensagem(const char* linha1, const char* linha2) {
  if (!pronto_) return;
  mensagemAteMs_ = millis() + Configuracao::TEMPO_MENSAGEM_DISPLAY_MS;
  escreverLinha(0, linha1);
  escreverLinha(1, linha2 == nullptr ? "" : linha2);
  escreverLinha(2, "");
  escreverLinha(3, "");
}

void Display::cancelarMensagem() {
  mensagemAteMs_ = 0;
}

void Display::renderizar(
    const Menu& menu,
    const ConfiguracaoTransmissor& configuracao,
    const TelemetriaTransmissor& telemetria,
    const TelemetriaReceptorRda5807& telemetriaReceptor,
    uint16_t melhorFrequencia,
    uint8_t melhorRuido
) {
  if (!pronto_) return;
  if (mensagemAteMs_ != 0
      && static_cast<int32_t>(mensagemAteMs_ - millis()) > 0) {
    return;
  }
  mensagemAteMs_ = 0;

  if (telemetria.recuperando) {
    mostrarMensagem("Recuperando Si4713", "Aguarde...");
    return;
  }

  switch (menu.tela()) {
    case Menu::Tela::PRINCIPAL:
      mostrarPrincipal(configuracao, telemetria, telemetriaReceptor);
      break;
    case Menu::Tela::RAIZ:
      mostrarRaiz(menu.itemSelecionado());
      break;
    case Menu::Tela::RF:
      mostrarRf(configuracao, menu.itemSelecionado(), menu.editando());
      break;
    case Menu::Tela::AUDIO:
      mostrarAudio(configuracao, menu.itemSelecionado(), menu.editando());
      break;
    case Menu::Tela::RDS:
      mostrarRds(
          configuracao,
          menu.itemSelecionado(),
          menu.editando(),
          menu.cursorTexto()
      );
      break;
    case Menu::Tela::MONITOR:
      mostrarMonitor(telemetria);
      break;
    case Menu::Tela::VARREDURA:
      mostrarVarredura(
          menu.itemSelecionado(),
          telemetria,
          melhorFrequencia,
          melhorRuido
      );
      break;
    case Menu::Tela::SISTEMA:
      mostrarSistema(menu.itemSelecionado());
      break;
  }
}

void Display::mostrarPrincipal(
    const ConfiguracaoTransmissor& configuracao,
    const TelemetriaTransmissor& telemetria,
    const TelemetriaReceptorRda5807& telemetriaReceptor
) {
  char texto[21];
  snprintf(
      texto,
      sizeof(texto),
      "FM %u.%02u MHz %s",
      configuracao.frequenciaKhz / 100,
      configuracao.frequenciaKhz % 100,
      telemetria.transmitindo ? "NO AR" : "OFF"
  );
  escreverLinha(0, texto);
  snprintf(
      texto,
      sizeof(texto),
      "PWR:%3u %s RDS:%s",
      configuracao.potenciaDbuv,
      configuracao.estereo ? "ST" : "MO",
      configuracao.rdsHabilitado ? "ON" : "OFF"
  );
  escreverLinha(1, texto);

  escreverLinha(2, "Audio: abra Monitor");
  if (telemetriaReceptor.disponivel) {
    if (telemetriaReceptor.leituraDiretaValida) {
      snprintf(
          texto,
          sizeof(texto),
          "%u.%02u RSSI:%3u",
          telemetriaReceptor.frequenciaKhz / 100,
          telemetriaReceptor.frequenciaKhz % 100,
          telemetriaReceptor.rssi
      );
    } else {
      snprintf(
          texto,
          sizeof(texto),
          "%u.%02u RSSI:---",
          telemetriaReceptor.frequenciaKhz / 100,
          telemetriaReceptor.frequenciaKhz % 100
      );
    }
    escreverLinha(3, texto);
  } else {
    escreverLinha(3, "---.-- RSSI:---");
  }
}

void Display::mostrarRaiz(uint8_t item) {
  mostrarCabecalho("MENU PRINCIPAL", item, Menu::QUANTIDADE_RAIZ);
  static const char* itens[] = {
      "RF", "Audio", "RDS", "Monitor", "Canal livre", "Sistema", "Voltar"
  };
  escreverLinha(1, itens[min(item, static_cast<uint8_t>(Menu::RAIZ_VOLTAR))]);
  escreverLinha(2, "Clique para abrir");
  escreverLinha(3, "Gire  Segure: Volta");
}

void Display::mostrarRf(
    const ConfiguracaoTransmissor& configuracao,
    uint8_t item,
    bool editando
) {
  mostrarCabecalho("MENU RF", item, Menu::QUANTIDADE_RF);
  char texto[21];
  if (item == Menu::RF_FREQUENCIA) {
    escreverLinha(1, "Frequencia");
    snprintf(
        texto,
        sizeof(texto),
        "%u.%02u MHz",
        configuracao.frequenciaKhz / 100,
        configuracao.frequenciaKhz % 100
    );
  } else if (item == Menu::RF_POTENCIA) {
    escreverLinha(1, "Potencia RF");
    snprintf(texto, sizeof(texto), "%u dBuV", configuracao.potenciaDbuv);
  } else if (item == Menu::RF_ANTENA) {
    escreverLinha(1, "Capacitancia antena");
    if (configuracao.capacitanciaAntena == 0) {
      snprintf(texto, sizeof(texto), "AUTO");
    } else {
      snprintf(
          texto,
          sizeof(texto),
          "%u.%02u pF",
          configuracao.capacitanciaAntena / 4,
          (configuracao.capacitanciaAntena % 4) * 25
      );
    }
  } else if (item == Menu::RF_TRANSMISSAO) {
    escreverLinha(1, "Transmissao");
    snprintf(
        texto,
        sizeof(texto),
        "%s",
        configuracao.transmissaoHabilitada ? "TX LIGADO" : "TX DESLIGADO"
    );
  } else {
    escreverLinha(1, "Voltar");
    snprintf(texto, sizeof(texto), "Clique para voltar");
  }
  escreverLinha(2, texto);
  mostrarAjudaEdicao(editando);
}

void Display::mostrarAudio(
    const ConfiguracaoTransmissor& configuracao,
    uint8_t item,
    bool editando
) {
  mostrarCabecalho("MENU AUDIO", item, Menu::QUANTIDADE_AUDIO);
  char texto[21];
  if (item == Menu::AUDIO_ESTEREO) {
    escreverLinha(1, "Modo de audio");
    snprintf(texto, sizeof(texto), "%s", configuracao.estereo ? "ESTEREO" : "MONO");
  } else if (item == Menu::AUDIO_PRE_ENFASE) {
    escreverLinha(1, "Pre-enfase");
    snprintf(texto, sizeof(texto), "%u us", configuracao.preEnfaseUs);
  } else if (item == Menu::AUDIO_DESVIO) {
    escreverLinha(1, "Desvio de audio");
    snprintf(texto, sizeof(texto), "%u kHz", configuracao.desvioAudioKhz);
  } else if (item == Menu::AUDIO_MUDO) {
    escreverLinha(1, "Entrada de audio");
    snprintf(texto, sizeof(texto), "%s", configuracao.audioMudo ? "MUTE" : "ATIVA");
  } else {
    escreverLinha(1, "Voltar");
    snprintf(texto, sizeof(texto), "Clique para voltar");
  }
  escreverLinha(2, texto);
  mostrarAjudaEdicao(editando);
}

void Display::mostrarRds(
    const ConfiguracaoTransmissor& configuracao,
    uint8_t item,
    bool editando,
    uint8_t cursor
) {
  mostrarCabecalho("MENU RDS", item, Menu::QUANTIDADE_RDS);
  char texto[21];
  if (item == Menu::RDS_HABILITADO) {
    escreverLinha(1, "Transmissao RDS");
    snprintf(texto, sizeof(texto), "%s", configuracao.rdsHabilitado ? "RDS LIGADO" : "RDS DESLIGADO");
  } else if (item == Menu::RDS_PS) {
    escreverLinha(1, "Nome PS (8 chars)");
    snprintf(texto, sizeof(texto), "%.8s", configuracao.rdsPs);
  } else if (item == Menu::RDS_TEXTO) {
    snprintf(texto, sizeof(texto), "RadioText pos %u/32", cursor + 1);
    escreverLinha(1, texto);
    const uint8_t inicio = cursor < 20 ? 0 : 12;
    snprintf(texto, sizeof(texto), "%.20s", configuracao.rdsText + inicio);
  } else if (item == Menu::RDS_PI) {
    escreverLinha(1, "PI Code");
    snprintf(texto, sizeof(texto), "0x%04X", configuracao.rdsPi);
  } else {
    escreverLinha(1, "Voltar");
    snprintf(texto, sizeof(texto), "Clique para voltar");
  }
  escreverLinha(2, texto);

  if (editando && (item == Menu::RDS_PS || item == Menu::RDS_TEXTO)) {
    const char caractere = item == Menu::RDS_PS
        ? configuracao.rdsPs[cursor]
        : configuracao.rdsText[cursor];
    snprintf(texto, sizeof(texto), "Pos:%u Char:%c Clique>", cursor + 1, caractere);
    escreverLinha(3, texto);
  } else {
    mostrarAjudaEdicao(editando);
  }
}

void Display::mostrarMonitor(const TelemetriaTransmissor& telemetria) {
  char texto[21];
  escreverLinha(0, "MONITOR DE AUDIO");
  if (telemetria.transmitindo) {
    snprintf(
        texto,
        sizeof(texto),
        "Nivel: %4d dBFS",
        telemetria.nivelAudioDbfs
    );
    escreverLinha(1, texto);
    escreverLinha(
        2,
        (telemetria.asq & ASQ_SOBREMODULACAO) != 0
            ? "Estado: CORTE"
            : "Estado: OK"
    );
  } else {
    escreverLinha(1, "Nivel:  -- dBFS");
    escreverLinha(2, "Estado: TX desligado");
  }
  escreverLinha(3, "Clique/Segure: sair");
}

void Display::mostrarVarredura(
    uint8_t item,
    const TelemetriaTransmissor& telemetria,
    uint16_t melhorFrequencia,
    uint8_t melhorRuido
) {
  mostrarCabecalho("CANAL LIVRE", item, Menu::QUANTIDADE_VARREDURA);
  char texto[21];
  if (telemetria.varreduraAtiva) {
    escreverLinha(1, "Medindo faixa FM...");
    snprintf(texto, sizeof(texto), "Progresso: %u%%", telemetria.progressoVarredura);
    escreverLinha(2, texto);
    escreverLinha(3, "Encoder: cancelar");
    return;
  }
  if (item == Menu::VARREDURA_INICIAR) {
    escreverLinha(1, "Iniciar varredura");
    escreverLinha(2, "Clique para iniciar");
  } else if (item == Menu::VARREDURA_USAR_MELHOR) {
    escreverLinha(1, "Usar melhor canal");
    if (telemetria.varreduraConcluida && melhorFrequencia != 0) {
      snprintf(
          texto,
          sizeof(texto),
          "%u.%02u MHz N:%u",
          melhorFrequencia / 100,
          melhorFrequencia % 100,
          melhorRuido
      );
      escreverLinha(2, texto);
    } else {
      escreverLinha(2, "Execute o scanner");
    }
  } else {
    escreverLinha(1, "Voltar");
    escreverLinha(2, "Clique para voltar");
  }
  escreverLinha(3, "Gire  Clique: Sel");
}

void Display::mostrarSistema(uint8_t item) {
  mostrarCabecalho("SISTEMA", item, Menu::QUANTIDADE_SISTEMA);
  if (item == Menu::SISTEMA_SALVAR) {
    escreverLinha(1, "Salvar configuracao");
    escreverLinha(2, "Gravar na memoria");
  } else if (item == Menu::SISTEMA_PADROES) {
    escreverLinha(1, "Restaurar padroes");
    escreverLinha(2, "Clique para restaurar");
  } else if (item == Menu::SISTEMA_WIFI) {
    escreverLinha(1, "Configurar Wi-Fi");
    escreverLinha(2, "Abrir portal local");
  } else if (item == Menu::SISTEMA_INFO) {
    escreverLinha(1, "Firmware ESP32-S3");
    escreverLinha(2, Configuracao::VERSAO_FIRMWARE);
  } else {
    escreverLinha(1, "Voltar");
    escreverLinha(2, "Clique para voltar");
  }
  escreverLinha(3, "Gire  Clique: Sel");
}

void Display::mostrarCabecalho(
    const char* titulo,
    uint8_t item,
    uint8_t quantidade
) {
  char texto[21];
  snprintf(texto, sizeof(texto), "%-15s %u/%u", titulo, item + 1, quantidade);
  escreverLinha(0, texto);
}

void Display::mostrarAjudaEdicao(bool editando) {
  escreverLinha(3, editando ? "Gire  Clique: OK" : "Gire  Clique: Sel");
}

void Display::escreverLinha(uint8_t linha, const char* texto) {
  char completa[21];
  size_t indice = 0;
  while (indice < Configuracao::LCD_COLUNAS && texto[indice] != '\0') {
    completa[indice] = texto[indice];
    indice++;
  }
  while (indice < Configuracao::LCD_COLUNAS) completa[indice++] = ' ';
  completa[Configuracao::LCD_COLUNAS] = '\0';
  // O cache antigo evitava linhas identicas, mas reenviava os 20 caracteres
  // quando apenas um digito (normalmente o RSSI) mudava. Escreva somente cada
  // trecho diferente para encurtar ao minimo a rajada I2C sobre o LCD.
  size_t coluna = 0;
  while (coluna < Configuracao::LCD_COLUNAS) {
    if (linhasRenderizadas_[linha][coluna] == completa[coluna]) {
      coluna++;
      continue;
    }

    lcd_.setCursor(coluna, linha);
    do {
      lcd_.write(static_cast<uint8_t>(completa[coluna]));
      linhasRenderizadas_[linha][coluna] = completa[coluna];
      coluna++;
    } while (
        coluna < Configuracao::LCD_COLUNAS
        && linhasRenderizadas_[linha][coluna] != completa[coluna]
    );
  }
  linhasRenderizadas_[linha][Configuracao::LCD_COLUNAS] = '\0';
}

void Display::invalidarCache() {
  memset(linhasRenderizadas_, 0, sizeof(linhasRenderizadas_));
}

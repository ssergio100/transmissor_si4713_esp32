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

void Display::renderizar(const EstadoPainel& estado) {
  if (!pronto_) return;
  if (mensagemAteMs_ != 0
      && static_cast<int32_t>(mensagemAteMs_ - millis()) > 0) {
    return;
  }
  mensagemAteMs_ = 0;

  if (estado.sistema.recuperando) {
    mostrarMensagem("Recuperando Si4713", "Aguarde...");
    return;
  }

  switch (estado.navegacao.tela) {
    case TelaPainel::PRINCIPAL:
      mostrarPrincipal(estado);
      break;
    case TelaPainel::RAIZ:
      mostrarRaiz(estado.navegacao);
      break;
    case TelaPainel::RF:
      mostrarRf(estado);
      break;
    case TelaPainel::AUDIO:
      mostrarAudio(estado);
      break;
    case TelaPainel::RDS:
      mostrarRds(estado);
      break;
    case TelaPainel::MONITOR:
      mostrarMonitor(estado);
      break;
    case TelaPainel::VARREDURA:
      mostrarVarredura(estado);
      break;
    case TelaPainel::SISTEMA:
      mostrarSistema(estado);
      break;
  }
}

void Display::mostrarPrincipal(const EstadoPainel& estado) {
  char texto[21];
  snprintf(
      texto,
      sizeof(texto),
      "FM %u.%02u MHz %s",
      estado.rf.frequenciaKhz / 100,
      estado.rf.frequenciaKhz % 100,
      estado.rf.transmitindo ? "NO AR" : "OFF"
  );
  escreverLinha(0, texto);
  snprintf(
      texto,
      sizeof(texto),
      "PWR:%3u %s RDS:%s",
      estado.rf.potenciaDbuv,
      estado.audio.estereo ? "ST" : "MO",
      estado.rds.habilitado ? "ON" : "OFF"
  );
  escreverLinha(1, texto);

  escreverLinha(2, "Audio: abra Monitor");
  if (estado.receptor.disponivel) {
    if (estado.receptor.leituraRssiValida) {
      snprintf(
          texto,
          sizeof(texto),
          "%u.%02u RSSI:%3u",
          estado.receptor.frequenciaKhz / 100,
          estado.receptor.frequenciaKhz % 100,
          estado.receptor.rssi
      );
    } else {
      snprintf(
          texto,
          sizeof(texto),
          "%u.%02u RSSI:---",
          estado.receptor.frequenciaKhz / 100,
          estado.receptor.frequenciaKhz % 100
      );
    }
    escreverLinha(3, texto);
  } else {
    escreverLinha(3, "---.-- RSSI:---");
  }
}

void Display::mostrarRaiz(const NavegacaoPainel& navegacao) {
  mostrarCabecalho(
      "MENU PRINCIPAL",
      navegacao.indice,
      navegacao.quantidade
  );
  static const char* itens[] = {
      "RF", "Audio", "RDS", "Monitor", "Canal livre", "Sistema", "Voltar"
  };
  constexpr uint8_t ultimoItem =
      static_cast<uint8_t>((sizeof(itens) / sizeof(itens[0])) - 1);
  escreverLinha(
      1,
      itens[min(navegacao.indice, ultimoItem)]
  );
  escreverLinha(2, "Clique para abrir");
  escreverLinha(3, "Gire  Segure: Volta");
}

void Display::mostrarRf(const EstadoPainel& estado) {
  const NavegacaoPainel& navegacao = estado.navegacao;
  mostrarCabecalho("MENU RF", navegacao.indice, navegacao.quantidade);
  char texto[21];
  if (navegacao.item == ItemPainel::RF_FREQUENCIA) {
    escreverLinha(1, "Frequencia");
    snprintf(
        texto,
        sizeof(texto),
        "%u.%02u MHz",
        estado.rf.frequenciaKhz / 100,
        estado.rf.frequenciaKhz % 100
    );
  } else if (navegacao.item == ItemPainel::RF_POTENCIA) {
    escreverLinha(1, "Potencia RF");
    snprintf(texto, sizeof(texto), "%u dBuV", estado.rf.potenciaDbuv);
  } else if (navegacao.item == ItemPainel::RF_ANTENA) {
    escreverLinha(1, "Capacitancia antena");
    if (estado.rf.capacitanciaAntena == 0) {
      snprintf(texto, sizeof(texto), "AUTO");
    } else {
      snprintf(
          texto,
          sizeof(texto),
          "%u.%02u pF",
          estado.rf.capacitanciaAntena / 4,
          (estado.rf.capacitanciaAntena % 4) * 25
      );
    }
  } else if (navegacao.item == ItemPainel::RF_TRANSMISSAO) {
    escreverLinha(1, "Transmissao");
    snprintf(
        texto,
        sizeof(texto),
        "%s",
        estado.rf.transmissaoHabilitada ? "TX LIGADO" : "TX DESLIGADO"
    );
  } else {
    escreverLinha(1, "Voltar");
    snprintf(texto, sizeof(texto), "Clique para voltar");
  }
  escreverLinha(2, texto);
  mostrarAjudaEdicao(navegacao.editando);
}

void Display::mostrarAudio(const EstadoPainel& estado) {
  const NavegacaoPainel& navegacao = estado.navegacao;
  mostrarCabecalho("MENU AUDIO", navegacao.indice, navegacao.quantidade);
  char texto[21];
  if (navegacao.item == ItemPainel::AUDIO_ESTEREO) {
    escreverLinha(1, "Modo de audio");
    snprintf(texto, sizeof(texto), "%s", estado.audio.estereo ? "ESTEREO" : "MONO");
  } else if (navegacao.item == ItemPainel::AUDIO_PRE_ENFASE) {
    escreverLinha(1, "Pre-enfase");
    snprintf(texto, sizeof(texto), "%u us", estado.audio.preEnfaseUs);
  } else if (navegacao.item == ItemPainel::AUDIO_DESVIO) {
    escreverLinha(1, "Desvio de audio");
    snprintf(texto, sizeof(texto), "%u kHz", estado.audio.desvioKhz);
  } else if (navegacao.item == ItemPainel::AUDIO_MUDO) {
    escreverLinha(1, "Entrada de audio");
    snprintf(texto, sizeof(texto), "%s", estado.audio.mudo ? "MUTE" : "ATIVA");
  } else {
    escreverLinha(1, "Voltar");
    snprintf(texto, sizeof(texto), "Clique para voltar");
  }
  escreverLinha(2, texto);
  mostrarAjudaEdicao(navegacao.editando);
}

void Display::mostrarRds(const EstadoPainel& estado) {
  const NavegacaoPainel& navegacao = estado.navegacao;
  mostrarCabecalho("MENU RDS", navegacao.indice, navegacao.quantidade);
  char texto[21];
  if (navegacao.item == ItemPainel::RDS_HABILITADO) {
    escreverLinha(1, "Transmissao RDS");
    snprintf(texto, sizeof(texto), "%s", estado.rds.habilitado ? "RDS LIGADO" : "RDS DESLIGADO");
  } else if (navegacao.item == ItemPainel::RDS_PS) {
    escreverLinha(1, "Nome PS (8 chars)");
    snprintf(texto, sizeof(texto), "%.8s", estado.rds.ps);
  } else if (navegacao.item == ItemPainel::RDS_TEXTO) {
    snprintf(
        texto,
        sizeof(texto),
        "RadioText pos %u/32",
        navegacao.cursorTexto + 1
    );
    escreverLinha(1, texto);
    const uint8_t inicio = navegacao.cursorTexto < 20 ? 0 : 12;
    snprintf(texto, sizeof(texto), "%.20s", estado.rds.texto + inicio);
  } else if (navegacao.item == ItemPainel::RDS_PI) {
    escreverLinha(1, "PI Code");
    snprintf(texto, sizeof(texto), "0x%04X", estado.rds.pi);
  } else {
    escreverLinha(1, "Voltar");
    snprintf(texto, sizeof(texto), "Clique para voltar");
  }
  escreverLinha(2, texto);

  if (navegacao.editando
      && (navegacao.item == ItemPainel::RDS_PS
          || navegacao.item == ItemPainel::RDS_TEXTO)) {
    const char caractere = navegacao.item == ItemPainel::RDS_PS
        ? estado.rds.ps[navegacao.cursorTexto]
        : estado.rds.texto[navegacao.cursorTexto];
    snprintf(
        texto,
        sizeof(texto),
        "Pos:%u Char:%c Clique>",
        navegacao.cursorTexto + 1,
        caractere
    );
    escreverLinha(3, texto);
  } else {
    mostrarAjudaEdicao(navegacao.editando);
  }
}

void Display::mostrarMonitor(const EstadoPainel& estado) {
  char texto[21];
  escreverLinha(0, "MONITOR DE AUDIO");
  if (estado.rf.transmitindo) {
    snprintf(
        texto,
        sizeof(texto),
        "Nivel: %4d dBFS",
        estado.audio.nivelDbfs
    );
    escreverLinha(1, texto);
    escreverLinha(
        2,
        (estado.audio.asq & ASQ_SOBREMODULACAO) != 0
            ? "Estado: CORTE"
            : "Estado: OK"
    );
  } else {
    escreverLinha(1, "Nivel:  -- dBFS");
    escreverLinha(2, "Estado: TX desligado");
  }
  escreverLinha(3, "Clique/Segure: sair");
}

void Display::mostrarVarredura(const EstadoPainel& estado) {
  const NavegacaoPainel& navegacao = estado.navegacao;
  mostrarCabecalho(
      "CANAL LIVRE",
      navegacao.indice,
      navegacao.quantidade
  );
  char texto[21];
  if (estado.varredura.ativa) {
    escreverLinha(1, "Medindo faixa FM...");
    snprintf(
        texto,
        sizeof(texto),
        "Progresso: %u%%",
        estado.varredura.progresso
    );
    escreverLinha(2, texto);
    escreverLinha(3, "Encoder: cancelar");
    return;
  }
  if (navegacao.item == ItemPainel::VARREDURA_INICIAR) {
    escreverLinha(1, "Iniciar varredura");
    escreverLinha(2, "Clique para iniciar");
  } else if (navegacao.item == ItemPainel::VARREDURA_USAR_MELHOR) {
    escreverLinha(1, "Usar melhor canal");
    if (estado.varredura.concluida
        && estado.varredura.melhorFrequenciaKhz != 0) {
      snprintf(
          texto,
          sizeof(texto),
          "%u.%02u MHz N:%u",
          estado.varredura.melhorFrequenciaKhz / 100,
          estado.varredura.melhorFrequenciaKhz % 100,
          estado.varredura.melhorNivelRuido
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

void Display::mostrarSistema(const EstadoPainel& estado) {
  const NavegacaoPainel& navegacao = estado.navegacao;
  mostrarCabecalho("SISTEMA", navegacao.indice, navegacao.quantidade);
  if (navegacao.item == ItemPainel::SISTEMA_SALVAR) {
    escreverLinha(1, "Salvar configuracao");
    escreverLinha(2, "Gravar na memoria");
  } else if (navegacao.item == ItemPainel::SISTEMA_PADROES) {
    escreverLinha(1, "Restaurar padroes");
    escreverLinha(2, "Clique para restaurar");
  } else if (navegacao.item == ItemPainel::SISTEMA_WIFI) {
    escreverLinha(1, "Configurar Wi-Fi");
    escreverLinha(2, "Abrir portal local");
  } else if (navegacao.item == ItemPainel::SISTEMA_INFO) {
    escreverLinha(1, "Firmware ESP32-S3");
    escreverLinha(2, estado.sistema.versaoFirmware);
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

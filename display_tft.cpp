#include "display_tft.h"

#include <SPI.h>
#include <stdio.h>
#include <string.h>

#include "configuracao.h"

namespace {

constexpr uint8_t ASQ_SOBREMODULACAO = 0x04;
constexpr uint8_t QUANTIDADE_LINHAS = 5;
constexpr int16_t ALTURA_CABECALHO = 42;
constexpr int16_t Y_PRIMEIRA_LINHA = 50;
constexpr int16_t ALTURA_LINHA = 43;
constexpr int16_t Y_RODAPE = 278;

constexpr uint16_t COR_FUNDO = 0x0861;
constexpr uint16_t COR_CABECALHO = 0x10C3;
constexpr uint16_t COR_TEXTO = ST77XX_WHITE;
constexpr uint16_t COR_SUAVE = 0x9CF3;
constexpr uint16_t COR_DESTAQUE = ST77XX_CYAN;
constexpr uint16_t COR_OK = ST77XX_GREEN;
constexpr uint16_t COR_AVISO = ST77XX_YELLOW;
constexpr uint16_t COR_ERRO = ST77XX_RED;

const char* rotuloItem(ItemPainel item) {
  switch (item) {
    case ItemPainel::RAIZ_RF: return "RF";
    case ItemPainel::RAIZ_AUDIO: return "Audio";
    case ItemPainel::RAIZ_RDS: return "RDS";
    case ItemPainel::RAIZ_MONITOR: return "Monitor";
    case ItemPainel::RAIZ_VARREDURA: return "Canal livre";
    case ItemPainel::RAIZ_SISTEMA: return "Sistema";
    case ItemPainel::RAIZ_VOLTAR: return "Voltar";

    case ItemPainel::RF_FREQUENCIA: return "Frequencia";
    case ItemPainel::RF_POTENCIA: return "Potencia RF";
    case ItemPainel::RF_ANTENA: return "Antena";
    case ItemPainel::RF_TRANSMISSAO: return "Transmissao";
    case ItemPainel::RF_VOLTAR: return "Voltar";

    case ItemPainel::AUDIO_ESTEREO: return "Modo de audio";
    case ItemPainel::AUDIO_PRE_ENFASE: return "Pre-enfase";
    case ItemPainel::AUDIO_DESVIO: return "Desvio de audio";
    case ItemPainel::AUDIO_MUDO: return "Entrada de audio";
    case ItemPainel::AUDIO_VOLTAR: return "Voltar";

    case ItemPainel::RDS_HABILITADO: return "Transmissao RDS";
    case ItemPainel::RDS_PS: return "Nome PS";
    case ItemPainel::RDS_TEXTO: return "RadioText";
    case ItemPainel::RDS_PI: return "PI Code";
    case ItemPainel::RDS_VOLTAR: return "Voltar";

    case ItemPainel::VARREDURA_INICIAR: return "Iniciar varredura";
    case ItemPainel::VARREDURA_USAR_MELHOR: return "Usar melhor canal";
    case ItemPainel::VARREDURA_VOLTAR: return "Voltar";

    case ItemPainel::SISTEMA_SALVAR: return "Salvar configuracao";
    case ItemPainel::SISTEMA_PADROES: return "Restaurar padroes";
    case ItemPainel::SISTEMA_WIFI: return "Configurar Wi-Fi";
    case ItemPainel::SISTEMA_INFO: return "Informacoes";
    case ItemPainel::SISTEMA_VOLTAR: return "Voltar";

    default: return "";
  }
}

}  // namespace

DisplayTft::DisplayTft()
    : tft_(
          Configuracao::PIN_TFT_CS,
          Configuracao::PIN_TFT_DC,
          Configuracao::PIN_TFT_RESET
      ) {}

bool DisplayTft::iniciar() {
  // CS deve permanecer inativo enquanto o barramento e o reset sao preparados.
  pinMode(Configuracao::PIN_TFT_CS, OUTPUT);
  digitalWrite(Configuracao::PIN_TFT_CS, HIGH);
  pinMode(Configuracao::PIN_TFT_DC, OUTPUT);
  digitalWrite(Configuracao::PIN_TFT_DC, HIGH);
  pinMode(Configuracao::PIN_TFT_RESET, OUTPUT);
  digitalWrite(Configuracao::PIN_TFT_RESET, HIGH);

  if (!SPI.begin(
          Configuracao::PIN_TFT_SCLK,
          Configuracao::PIN_TFT_MISO,
          Configuracao::PIN_TFT_MOSI,
          Configuracao::PIN_TFT_CS
      )) {
    return false;
  }

  tft_.init(Configuracao::TFT_LARGURA, Configuracao::TFT_ALTURA);
  tft_.setRotation(Configuracao::TFT_ROTACAO);
  tft_.setTextWrap(false);
  tft_.fillScreen(COR_FUNDO);
  pronto_ = true;
  telaValida_ = false;
  invalidarCache();
  return true;
}

void DisplayTft::mostrarInicializacao() {
  if (!pronto_) return;
  tft_.fillScreen(COR_FUNDO);
  telaValida_ = false;
  invalidarCache();

  tft_.fillRect(0, 0, tft_.width(), ALTURA_CABECALHO, COR_CABECALHO);
  tft_.setTextColor(COR_DESTAQUE);
  tft_.setTextSize(2);
  tft_.setCursor(12, 13);
  tft_.print("TRANSMISSOR FM");

  escreverLinha(0, "ESP32-S3 + Si4713", COR_TEXTO, 2);
  escreverLinha(1, "TFT ST7789", COR_DESTAQUE, 3);
  escreverLinha(2, "Inicializando...", COR_SUAVE, 2);
  limparLinhasAPartir(3);
  escreverRodape("Interface local", COR_SUAVE);
}

void DisplayTft::renderizar(const EstadoPainel& estado) {
  if (!pronto_) return;

  if (estado.sistema.recuperando) {
    mostrarRecuperacao();
    return;
  }

  switch (estado.navegacao.tela) {
    case TelaPainel::PRINCIPAL:
      mostrarPrincipal(estado);
      break;
    case TelaPainel::RAIZ:
      mostrarRaiz(estado);
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

void DisplayTft::prepararTela(
    const char* titulo,
    TelaPainel tela,
    bool recuperando
) {
  if (telaValida_
      && telaRenderizada_ == tela
      && recuperacaoRenderizada_ == recuperando) {
    return;
  }

  tft_.fillScreen(COR_FUNDO);
  tft_.fillRect(0, 0, tft_.width(), ALTURA_CABECALHO, COR_CABECALHO);
  tft_.drawFastHLine(
      0,
      ALTURA_CABECALHO - 1,
      tft_.width(),
      COR_DESTAQUE
  );
  tft_.setTextColor(COR_TEXTO);
  tft_.setTextSize(2);
  tft_.setCursor(10, 13);
  tft_.print(titulo);

  telaRenderizada_ = tela;
  recuperacaoRenderizada_ = recuperando;
  telaValida_ = true;
  invalidarCache();
}

void DisplayTft::mostrarPrincipal(const EstadoPainel& estado) {
  prepararTela("TRANSMISSOR FM", TelaPainel::PRINCIPAL);
  char texto[36];

  snprintf(
      texto,
      sizeof(texto),
      "%u.%02u MHz",
      estado.rf.frequenciaKhz / 100,
      estado.rf.frequenciaKhz % 100
  );
  escreverLinha(0, texto, COR_DESTAQUE, 3);

  snprintf(
      texto,
      sizeof(texto),
      "%s   %u dBuV",
      estado.rf.transmitindo ? "NO AR" : "TX OFF",
      estado.rf.potenciaDbuv
  );
  escreverLinha(
      1,
      texto,
      estado.rf.transmitindo ? COR_OK : COR_ERRO,
      2
  );

  snprintf(
      texto,
      sizeof(texto),
      "Audio:%s  RDS:%s",
      estado.audio.estereo ? "ST" : "MO",
      estado.rds.habilitado ? "ON" : "OFF"
  );
  escreverLinha(2, texto, COR_TEXTO, 2);

  if (estado.receptor.disponivel) {
    snprintf(
        texto,
        sizeof(texto),
        "RX %u.%02u MHz",
        estado.receptor.frequenciaKhz / 100,
        estado.receptor.frequenciaKhz % 100
    );
    escreverLinha(3, texto, COR_SUAVE, 2);
    if (estado.receptor.leituraRssiValida) {
      snprintf(texto, sizeof(texto), "RSSI %u", estado.receptor.rssi);
    } else {
      snprintf(texto, sizeof(texto), "RSSI ---");
    }
    escreverLinha(4, texto, COR_AVISO, 2);
  } else {
    escreverLinha(3, "RDA indisponivel", COR_ERRO, 2);
    escreverLinha(4, "RSSI ---", COR_SUAVE, 2);
  }
  escreverRodape("Clique: menu", COR_SUAVE);
}

void DisplayTft::mostrarRaiz(const EstadoPainel& estado) {
  prepararTela("MENU PRINCIPAL", TelaPainel::RAIZ);
  char texto[36];
  escreverLinha(0, rotuloItem(estado.navegacao.item), COR_DESTAQUE, 3);
  snprintf(
      texto,
      sizeof(texto),
      "Item %u de %u",
      estado.navegacao.indice + 1,
      estado.navegacao.quantidade
  );
  escreverLinha(1, texto, COR_SUAVE, 2);
  escreverLinha(2, "Gire para navegar", COR_TEXTO, 2);
  escreverLinha(3, "Clique para abrir", COR_TEXTO, 2);
  limparLinhasAPartir(4);
  escreverRodape("Segure: voltar", COR_SUAVE);
}

void DisplayTft::mostrarRf(const EstadoPainel& estado) {
  prepararTela("AJUSTES RF", TelaPainel::RF);
  char texto[36];
  escreverLinha(0, rotuloItem(estado.navegacao.item), COR_SUAVE, 2);

  switch (estado.navegacao.item) {
    case ItemPainel::RF_FREQUENCIA:
      snprintf(
          texto,
          sizeof(texto),
          "%u.%02u MHz",
          estado.rf.frequenciaKhz / 100,
          estado.rf.frequenciaKhz % 100
      );
      escreverLinha(1, texto, COR_DESTAQUE, 3);
      snprintf(
          texto,
          sizeof(texto),
          "Efetiva %u.%02u",
          estado.rf.frequenciaEfetivaKhz / 100,
          estado.rf.frequenciaEfetivaKhz % 100
      );
      break;

    case ItemPainel::RF_POTENCIA:
      snprintf(texto, sizeof(texto), "%u dBuV", estado.rf.potenciaDbuv);
      escreverLinha(1, texto, COR_DESTAQUE, 3);
      snprintf(
          texto,
          sizeof(texto),
          "Efetiva %u dBuV",
          estado.rf.potenciaEfetivaDbuv
      );
      break;

    case ItemPainel::RF_ANTENA:
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
      escreverLinha(1, texto, COR_DESTAQUE, 3);
      snprintf(
          texto,
          sizeof(texto),
          "Efetiva %u.%02u pF",
          estado.rf.capacitanciaEfetiva / 4,
          (estado.rf.capacitanciaEfetiva % 4) * 25
      );
      break;

    case ItemPainel::RF_TRANSMISSAO:
      snprintf(
          texto,
          sizeof(texto),
          "%s",
          estado.rf.transmissaoHabilitada ? "TX LIGADO" : "TX DESLIGADO"
      );
      escreverLinha(
          1,
          texto,
          estado.rf.transmissaoHabilitada ? COR_OK : COR_ERRO,
          3
      );
      snprintf(
          texto,
          sizeof(texto),
          "Portadora %s",
          estado.rf.transmitindo ? "ativa" : "inativa"
      );
      break;

    default:
      escreverLinha(1, "Clique para voltar", COR_TEXTO, 2);
      snprintf(texto, sizeof(texto), "Menu principal");
      break;
  }

  escreverLinha(2, texto, COR_SUAVE, 2);
  limparLinhasAPartir(3);
  escreverRodape(
      estado.navegacao.editando
          ? "Gire: ajustar  Clique: OK"
          : "Gire: item  Clique: selecionar",
      estado.navegacao.editando ? COR_AVISO : COR_SUAVE
  );
}

void DisplayTft::mostrarAudio(const EstadoPainel& estado) {
  prepararTela("AJUSTES DE AUDIO", TelaPainel::AUDIO);
  char texto[36];
  escreverLinha(0, rotuloItem(estado.navegacao.item), COR_SUAVE, 2);

  switch (estado.navegacao.item) {
    case ItemPainel::AUDIO_ESTEREO:
      snprintf(texto, sizeof(texto), "%s", estado.audio.estereo ? "ESTEREO" : "MONO");
      break;
    case ItemPainel::AUDIO_PRE_ENFASE:
      snprintf(texto, sizeof(texto), "%u us", estado.audio.preEnfaseUs);
      break;
    case ItemPainel::AUDIO_DESVIO:
      snprintf(texto, sizeof(texto), "%u kHz", estado.audio.desvioKhz);
      break;
    case ItemPainel::AUDIO_MUDO:
      snprintf(texto, sizeof(texto), "%s", estado.audio.mudo ? "MUTE" : "ATIVA");
      break;
    default:
      snprintf(texto, sizeof(texto), "Voltar");
      break;
  }

  escreverLinha(1, texto, COR_DESTAQUE, 3);
  escreverLinha(
      2,
      estado.audio.mudo ? "Entrada silenciada" : "Entrada habilitada",
      estado.audio.mudo ? COR_AVISO : COR_OK,
      2
  );
  limparLinhasAPartir(3);
  escreverRodape(
      estado.navegacao.editando
          ? "Gire: ajustar  Clique: OK"
          : "Gire: item  Clique: selecionar",
      estado.navegacao.editando ? COR_AVISO : COR_SUAVE
  );
}

void DisplayTft::mostrarRds(const EstadoPainel& estado) {
  prepararTela("AJUSTES RDS", TelaPainel::RDS);
  char texto[36];
  escreverLinha(0, rotuloItem(estado.navegacao.item), COR_SUAVE, 2);

  switch (estado.navegacao.item) {
    case ItemPainel::RDS_HABILITADO:
      snprintf(texto, sizeof(texto), "%s", estado.rds.habilitado ? "RDS LIGADO" : "RDS DESLIGADO");
      break;
    case ItemPainel::RDS_PS:
      snprintf(texto, sizeof(texto), "%.8s", estado.rds.ps);
      break;
    case ItemPainel::RDS_TEXTO:
      snprintf(texto, sizeof(texto), "%.19s", estado.rds.texto);
      break;
    case ItemPainel::RDS_PI:
      snprintf(texto, sizeof(texto), "0x%04X", estado.rds.pi);
      break;
    default:
      snprintf(texto, sizeof(texto), "Voltar");
      break;
  }

  escreverLinha(
      1,
      texto,
      estado.rds.habilitado ? COR_DESTAQUE : COR_SUAVE,
      estado.navegacao.item == ItemPainel::RDS_TEXTO ? 2 : 3
  );
  if (estado.navegacao.editando
      && (estado.navegacao.item == ItemPainel::RDS_PS
          || estado.navegacao.item == ItemPainel::RDS_TEXTO)) {
    snprintf(
        texto,
        sizeof(texto),
        "Posicao %u",
        estado.navegacao.cursorTexto + 1
    );
    escreverLinha(2, texto, COR_AVISO, 2);
  } else {
    escreverLinha(
        2,
        estado.rds.habilitado ? "Servico ativo" : "Servico inativo",
        estado.rds.habilitado ? COR_OK : COR_SUAVE,
        2
    );
  }
  limparLinhasAPartir(3);
  escreverRodape(
      estado.navegacao.editando
          ? "Gire: editar  Clique: avancar"
          : "Gire: item  Clique: selecionar",
      estado.navegacao.editando ? COR_AVISO : COR_SUAVE
  );
}

void DisplayTft::mostrarMonitor(const EstadoPainel& estado) {
  prepararTela("MONITOR DE AUDIO", TelaPainel::MONITOR);
  char texto[36];
  escreverLinha(0, "Nivel de entrada", COR_SUAVE, 2);
  if (estado.rf.transmitindo) {
    snprintf(texto, sizeof(texto), "%d dBFS", estado.audio.nivelDbfs);
    escreverLinha(1, texto, COR_DESTAQUE, 3);
    const bool corte = (estado.audio.asq & ASQ_SOBREMODULACAO) != 0;
    escreverLinha(2, corte ? "CORTE" : "SINAL OK", corte ? COR_ERRO : COR_OK, 3);
    escreverLinha(3, "Transmissor ativo", COR_SUAVE, 2);
  } else {
    escreverLinha(1, "-- dBFS", COR_SUAVE, 3);
    escreverLinha(2, "TX DESLIGADO", COR_ERRO, 2);
    escreverLinha(3, "Sem leitura de audio", COR_SUAVE, 2);
  }
  limparLinhasAPartir(4);
  escreverRodape("Clique ou segure: sair", COR_SUAVE);
}

void DisplayTft::mostrarVarredura(const EstadoPainel& estado) {
  prepararTela("CANAL LIVRE", TelaPainel::VARREDURA);
  char texto[36];

  if (estado.varredura.ativa) {
    escreverLinha(0, "Medindo faixa FM", COR_DESTAQUE, 2);
    snprintf(texto, sizeof(texto), "%u%%", estado.varredura.progresso);
    escreverLinha(1, texto, COR_AVISO, 3);
    escreverLinha(2, "TX pausado", COR_SUAVE, 2);
    escreverLinha(3, "Gire para cancelar", COR_TEXTO, 2);
    limparLinhasAPartir(4);
    escreverRodape("Varredura em andamento", COR_AVISO);
    return;
  }

  escreverLinha(0, rotuloItem(estado.navegacao.item), COR_SUAVE, 2);
  if (estado.varredura.concluida
      && estado.varredura.melhorFrequenciaKhz != 0) {
    snprintf(
        texto,
        sizeof(texto),
        "%u.%02u MHz",
        estado.varredura.melhorFrequenciaKhz / 100,
        estado.varredura.melhorFrequenciaKhz % 100
    );
    escreverLinha(1, texto, COR_DESTAQUE, 3);
    snprintf(
        texto,
        sizeof(texto),
        "Ruido %u",
        estado.varredura.melhorNivelRuido
    );
    escreverLinha(2, texto, COR_SUAVE, 2);
  } else {
    escreverLinha(1, "Sem resultado", COR_SUAVE, 2);
    escreverLinha(2, "Inicie a varredura", COR_TEXTO, 2);
  }
  limparLinhasAPartir(3);
  escreverRodape("Gire: item  Clique: selecionar", COR_SUAVE);
}

void DisplayTft::mostrarSistema(const EstadoPainel& estado) {
  prepararTela("SISTEMA", TelaPainel::SISTEMA);
  char texto[36];
  escreverLinha(0, rotuloItem(estado.navegacao.item), COR_SUAVE, 2);

  if (estado.navegacao.item == ItemPainel::SISTEMA_INFO) {
    escreverLinha(1, estado.sistema.versaoFirmware, COR_DESTAQUE, 2);
    snprintf(
        texto,
        sizeof(texto),
        "Falhas I2C %lu",
        static_cast<unsigned long>(estado.sistema.falhasComunicacao)
    );
    escreverLinha(2, texto, COR_TEXTO, 2);
    snprintf(
        texto,
        sizeof(texto),
        "Recuperacoes %u",
        estado.sistema.recuperacoes
    );
    escreverLinha(3, texto, COR_TEXTO, 2);
  } else {
    escreverLinha(1, "Clique para executar", COR_DESTAQUE, 2);
    escreverLinha(
        2,
        estado.sistema.si4713Disponivel ? "Si4713 disponivel" : "Si4713 ausente",
        estado.sistema.si4713Disponivel ? COR_OK : COR_ERRO,
        2
    );
    limparLinhasAPartir(3);
  }
  limparLinhasAPartir(4);
  escreverRodape("Gire: item  Segure: voltar", COR_SUAVE);
}

void DisplayTft::mostrarRecuperacao() {
  prepararTela("SISTEMA", TelaPainel::SISTEMA, true);
  escreverLinha(0, "RECUPERANDO", COR_AVISO, 3);
  escreverLinha(1, "Si4713 indisponivel", COR_ERRO, 2);
  escreverLinha(2, "Tentando novamente", COR_TEXTO, 2);
  escreverLinha(3, "Aguarde...", COR_SUAVE, 2);
  limparLinhasAPartir(4);
  escreverRodape("Controle local permanece ativo", COR_SUAVE);
}

void DisplayTft::escreverLinha(
    uint8_t linha,
    const char* texto,
    uint16_t cor,
    uint8_t tamanho
) {
  if (linha >= QUANTIDADE_LINHAS) return;
  LinhaRenderizada& cache = linhasRenderizadas_[linha];
  if (cache.tamanho == tamanho
      && cache.cor == cor
      && strncmp(cache.texto, texto, sizeof(cache.texto)) == 0) {
    return;
  }

  const int16_t y = Y_PRIMEIRA_LINHA + linha * ALTURA_LINHA;
  tft_.fillRect(6, y, tft_.width() - 12, ALTURA_LINHA - 4, COR_FUNDO);
  tft_.setTextColor(cor);
  tft_.setTextSize(tamanho);

  int16_t x1;
  int16_t y1;
  uint16_t largura;
  uint16_t altura;
  tft_.getTextBounds(texto, 0, 0, &x1, &y1, &largura, &altura);
  int16_t x = (tft_.width() - static_cast<int16_t>(largura)) / 2;
  if (x < 8) x = 8;
  const int16_t textoY = y + (ALTURA_LINHA - altura) / 2 - y1 - 2;
  tft_.setCursor(x, textoY);
  tft_.print(texto);

  snprintf(cache.texto, sizeof(cache.texto), "%s", texto);
  cache.cor = cor;
  cache.tamanho = tamanho;
}

void DisplayTft::escreverRodape(const char* texto, uint16_t cor) {
  LinhaRenderizada& cache = linhasRenderizadas_[QUANTIDADE_LINHAS];
  if (cache.tamanho == 1
      && cache.cor == cor
      && strncmp(cache.texto, texto, sizeof(cache.texto)) == 0) {
    return;
  }

  tft_.fillRect(0, Y_RODAPE, tft_.width(), tft_.height() - Y_RODAPE, COR_CABECALHO);
  tft_.drawFastHLine(0, Y_RODAPE, tft_.width(), COR_DESTAQUE);
  tft_.setTextColor(cor);
  tft_.setTextSize(1);

  int16_t x1;
  int16_t y1;
  uint16_t largura;
  uint16_t altura;
  tft_.getTextBounds(texto, 0, 0, &x1, &y1, &largura, &altura);
  int16_t x = (tft_.width() - static_cast<int16_t>(largura)) / 2;
  if (x < 5) x = 5;
  tft_.setCursor(x, Y_RODAPE + 17);
  tft_.print(texto);

  snprintf(cache.texto, sizeof(cache.texto), "%s", texto);
  cache.cor = cor;
  cache.tamanho = 1;
}

void DisplayTft::limparLinhasAPartir(uint8_t primeira) {
  for (uint8_t linha = primeira; linha < QUANTIDADE_LINHAS; linha++) {
    escreverLinha(linha, "", COR_TEXTO, 2);
  }
}

void DisplayTft::invalidarCache() {
  memset(linhasRenderizadas_, 0, sizeof(linhasRenderizadas_));
}

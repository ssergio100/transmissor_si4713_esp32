#include "janela_menu_tft.h"
#include "menu.h"
#include "tema_tft.h"
#include <stdio.h>
#include <string.h>

namespace {
using namespace TemaTft;
using Tela = TelaPainel;

// Posicao em pixels na tela de 320 x 240. A faixa NO AR / RDS fica descoberta.
constexpr int16_t JANELA_LARGURA = 288;
constexpr int16_t JANELA_ALTURA = 176;
constexpr int16_t ALTURA_ITEM = 27;

// frequencia: unidade legada do projeto, passos de 10 kHz (9950 = 99.50 MHz).
void formatarFrequencia(char* texto, size_t capacidade, uint16_t frequencia) {
  snprintf(texto, capacidade, "%u.%02u MHz", frequencia / 100, frequencia % 100);
}

// antena: 0 representa AUTO; demais valores usam a conversao existente do projeto.
void formatarAntena(char* texto, size_t capacidade, uint8_t antena) {
  if (!antena) snprintf(texto, capacidade, "AUTO");
  else snprintf(texto, capacidade, "%u.%02u pF", antena / 4, (antena % 4) * 25);
}

// segundos: zero desativa o repouso automatico.
void formatarRepouso(char* texto, size_t capacidade, uint16_t segundos) {
  if (segundos == 0) snprintf(texto, capacidade, "NUNCA");
  else if (segundos < 60) snprintf(texto, capacidade, "%u S", segundos);
  else snprintf(texto, capacidade, "%u MIN", segundos / 60);
}

// passoKhz usa a unidade legada: 10 representa 100 kHz e 20 representa 200 kHz.
void formatarPassoFrequencia(char* texto, size_t capacidade, uint8_t passoKhz) {
  snprintf(texto, capacidade, "0.%u MHz%s", passoKhz / 10,
           passoKhz == Configuracao::PASSO_FREQUENCIA_PADRAO_KHZ ? " BR" : "");
}

void valorAtual(ItemPainel item, const EstadoPainel& estado, char* texto, size_t capacidade) {
  texto[0] = '\0';
  switch (item) {
    case RF_FREQUENCIA: formatarFrequencia(texto, capacidade, estado.rf.frequenciaKhz); break;
    case RF_POTENCIA: snprintf(texto, capacidade, "%u dBuV", estado.rf.potenciaDbuv); break;
    case RF_ANTENA: formatarAntena(texto, capacidade, estado.rf.capacitanciaAntena); break;
    case RF_TRANSMISSAO: snprintf(texto, capacidade, "%s", estado.rf.transmissaoHabilitada ? "LIGADO" : "DESLIGADO"); break;
    case AUDIO_ESTEREO: {
      const char* nome = "MONO + RDS"; // Preferencia legada.
      switch (estado.audio.modoAudio) {
        case 0: nome = "MONO"; break;
        case 3: nome = "ESTEREO"; break;
        case 7: nome = "ESTEREO + RDS"; break;
        case 1: nome = "SOMENTE PILOTO"; break;
        case 2: nome = "SOMENTE L-R"; break;
        case ConfiguracaoTransmissor::APENAS_L: nome = "APENAS L"; break;
        case ConfiguracaoTransmissor::APENAS_R: nome = "APENAS R"; break;
      }
      snprintf(texto, capacidade, "%s", nome);
      break;
    }
    case AUDIO_PRE_ENFASE: snprintf(texto, capacidade, "%u us", estado.audio.preEnfaseUs); break;
    case AUDIO_DESVIO: snprintf(texto, capacidade, "%u kHz", estado.audio.desvioKhz); break;
    case ItemPainel::AUDIO_MUDO: snprintf(texto, capacidade, "%s", estado.audio.mudo ? "MUDA" : "ATIVA"); break;
    case RDS_HABILITADO: snprintf(texto, capacidade, "%s", estado.rds.habilitado ? "LIGADO" : "DESLIGADO"); break;
    case RDS_PI: snprintf(texto, capacidade, "%04X", estado.rds.pi); break;
    case SISTEMA_REPOUSO:
      formatarRepouso(texto, capacidade, estado.sistema.repousoDisplaySegundos);
      break;
    case SISTEMA_PASSO_FREQUENCIA:
      formatarPassoFrequencia(texto, capacidade, estado.sistema.passoFrequenciaKhz);
      break;
    case SISTEMA_VOLUME_MONITOR:
      snprintf(texto, capacidade, "%u / 15", estado.sistema.volumeMonitor);
      break;
    default: break;
  }
}
}  // namespace

JanelaMenuTft::JanelaMenuTft() : janela_(JANELA_LARGURA, JANELA_ALTURA) {}
bool JanelaMenuTft::pronta() const { return janela_.getBuffer() != nullptr; }

const GFXcanvas16& JanelaMenuTft::imagem() const { return janela_; }

bool JanelaMenuTft::conteudoMudou(const EstadoPainel& estado) const {
  const auto& atual = estado.navegacao;
  const auto& antiga = anterior_.navegacao;
  // Compare campos, nao os bytes das structs: o preenchimento pode variar.
  if (atual.tela != antiga.tela || atual.indice != antiga.indice
      || atual.item != antiga.item || atual.editando != antiga.editando
      || atual.editandoCaractere != antiga.editandoCaractere
      || atual.cursorTexto != antiga.cursorTexto
      || atual.comprimentoTexto != antiga.comprimentoTexto
      || atual.valorEditado != antiga.valorEditado
      || atual.resultadoBuscaDisponivel != antiga.resultadoBuscaDisponivel
      || strcmp(atual.textoEditado, antiga.textoEditado)
      || strcmp(atual.erro, antiga.erro)) return true;
  // O editor e a mensagem ja foram comparados; a telemetria fica encoberta.
  if (atual.editando || atual.erro[0]) return false;
  switch (atual.tela) {
    case Tela::RF:
      return estado.rf.frequenciaKhz != anterior_.rf.frequenciaKhz
          || estado.rf.potenciaDbuv != anterior_.rf.potenciaDbuv
          || estado.rf.capacitanciaAntena != anterior_.rf.capacitanciaAntena
          || estado.rf.transmissaoHabilitada != anterior_.rf.transmissaoHabilitada;
    case Tela::AUDIO:
      return estado.audio.modoAudio != anterior_.audio.modoAudio
          || estado.audio.componentesMultiplex != anterior_.audio.componentesMultiplex
          || estado.audio.estereo != anterior_.audio.estereo
          || estado.audio.mudo != anterior_.audio.mudo
          || estado.audio.preEnfaseUs != anterior_.audio.preEnfaseUs
          || estado.audio.desvioKhz != anterior_.audio.desvioKhz;
    case Tela::RDS:
      return estado.rds.habilitado != anterior_.rds.habilitado
          || estado.rds.pi != anterior_.rds.pi;
    case Tela::SISTEMA:
      return estado.sistema.repousoDisplaySegundos
              != anterior_.sistema.repousoDisplaySegundos
          || estado.sistema.passoFrequenciaKhz
              != anterior_.sistema.passoFrequenciaKhz
          || estado.sistema.volumeMonitor != anterior_.sistema.volumeMonitor;
    case Tela::MONITOR:
      return estado.rf.transmitindo != anterior_.rf.transmitindo
          || (estado.rf.transmitindo
              && (estado.audio.nivelDbfs != anterior_.audio.nivelDbfs
                  || ((estado.audio.asq ^ anterior_.audio.asq) & 0x04)));
    case Tela::VARREDURA:
      if (estado.varredura.ativa != anterior_.varredura.ativa) return true;
      if (estado.varredura.ativa)
        return estado.varredura.progresso != anterior_.varredura.progresso;
      return atual.resultadoBuscaDisponivel
          && estado.varredura.melhorFrequenciaKhz != anterior_.varredura.melhorFrequenciaKhz;
    default: return false;
  }
}

bool JanelaMenuTft::preparar(const EstadoPainel& estado, bool forcar) {
  if (!pronta()) return false;
  if (imagemValida_ && !forcar && !conteudoMudou(estado)) return false;
  // Monta a janela em RAM antes de transferir: o TFT nao recebe etapas de limpeza.
  janela_.fillScreen(FUNDO_JANELA);
  janela_.setTextWrap(false);
  janela_.drawRoundRect(0, 0, JANELA_LARGURA, JANELA_ALTURA, 6, BORDA_JANELA);
  const NavegacaoPainel& navegacao = estado.navegacao;
  if (navegacao.erro[0]) {
    titulo("NAO CONCLUIDO");
    textoCentral(70, navegacao.erro, 1, VERMELHO);
    linha(132, "VOLTAR", "", true);
  } else if (navegacao.editando) {
    mostrarEdicao(navegacao);
  } else if (navegacao.tela == Tela::MONITOR) {
    mostrarMonitor(estado);
  } else if (navegacao.tela == Tela::VARREDURA) {
    mostrarVarredura(estado);
  } else {
    mostrarLista(estado);
  }
  anterior_ = estado;
  imagemValida_ = true;
  return true;
}

void JanelaMenuTft::mostrarLista(const EstadoPainel& estado) {
  const NavegacaoPainel& navegacao = estado.navegacao;
  const bool inicial = navegacao.tela == Tela::RAIZ;
  if (!inicial) {
    const char* nome = navegacao.tela == Tela::RF ? "RF"
        : navegacao.tela == Tela::AUDIO ? "AUDIO"
        : navegacao.tela == Tela::RDS ? "RDS" : "SISTEMA";
    titulo(nome);
  }
  uint8_t quantidade;
  const OpcaoMenu* opcoes = opcoesMenu(navegacao.tela, quantidade);
  // A raiz mostra seis itens; os submenus, cinco. Listas maiores rolam pelo foco.
  const uint8_t linhasVisiveis = inicial ? 6 : 5;
  const uint8_t primeira = navegacao.indice >= linhasVisiveis ? navegacao.indice - linhasVisiveis + 1 : 0;
  for (uint8_t i = primeira; i < quantidade && i < primeira + linhasVisiveis; i++) {
    char valor[24];
    valorAtual(opcoes[i].item, estado, valor, sizeof(valor));
    linha((inicial ? 7 : 34) + (i - primeira) * ALTURA_ITEM,
          opcoes[i].nome, valor, i == navegacao.indice);
  }
}

void JanelaMenuTft::mostrarEdicao(const NavegacaoPainel& edicao) {
  titulo(nomeItemMenu(edicao.item));
  if (edicao.comprimentoTexto) { mostrarTexto(edicao); return; }

  char valor[24];
  switch (edicao.item) {
    case RF_FREQUENCIA: formatarFrequencia(valor, sizeof(valor), edicao.valorEditado); break;
    case RF_POTENCIA: snprintf(valor, sizeof(valor), "%ld dBuV", static_cast<long>(edicao.valorEditado)); break;
    case RF_ANTENA: formatarAntena(valor, sizeof(valor), edicao.valorEditado); break;
    case AUDIO_DESVIO: snprintf(valor, sizeof(valor), "%ld kHz", static_cast<long>(edicao.valorEditado)); break;
    case SISTEMA_REPOUSO: {
      const auto& tempos = Configuracao::TEMPOS_REPOUSO_DISPLAY_SEGUNDOS;
      const uint8_t quantidade = sizeof(tempos) / sizeof(tempos[0]);
      constexpr uint8_t linhasVisiveis = 5;
      uint8_t selecionado = 0;
      while (selecionado < quantidade
             && tempos[selecionado] != edicao.valorEditado) selecionado++;
      uint8_t primeira = selecionado >= linhasVisiveis
          ? selecionado - linhasVisiveis + 1 : 0;
      for (uint8_t i = primeira;
           i < quantidade && i < primeira + linhasVisiveis; i++) {
        formatarRepouso(valor, sizeof(valor), tempos[i]);
        linha(34 + (i - primeira) * ALTURA_ITEM,
              valor, "", edicao.valorEditado == tempos[i]);
      }
      return;
    }
    case SISTEMA_VOLUME_MONITOR:
      snprintf(valor, sizeof(valor), "%ld / 15",
               static_cast<long>(edicao.valorEditado));
      break;
    default: valor[0] = '\0'; break;
  }
  textoCentral(83, valor, 3, VALOR_MENU);
}

void JanelaMenuTft::mostrarTexto(const NavegacaoPainel& edicao) {
  // Cada celula tem 16 pixels; RadioText usa duas linhas de 16 caracteres.
  // Borda marca a posicao; preenchimento marca o caractere sendo alterado.
  const uint8_t colunas = edicao.comprimentoTexto > 16 ? 16 : edicao.comprimentoTexto;
  const int16_t inicioX = (JANELA_LARGURA - colunas * 16) / 2;
  for (uint8_t i = 0; i < edicao.comprimentoTexto; i++) {
    const int16_t x = inicioX + (i % colunas) * 16;
    const int16_t y = 59 + (i / colunas) * 30;
    const bool foco = i == edicao.cursorTexto;
    const bool alterando = foco && edicao.editandoCaractere;
    if (alterando) janela_.fillRect(x, y, 16, 24, FUNDO_CARACTERE);
    else if (foco) janela_.drawRect(x, y, 16, 24, BORDA_JANELA);
    janela_.setTextSize(2);
    janela_.setTextColor(alterando ? TEXTO_CARACTERE : TEXTO_MENU);
    janela_.setCursor(x + 2, y + 4);
    janela_.write(edicao.textoEditado[i]);
  }
  linha(135, "CONCLUIR", "", edicao.cursorTexto == edicao.comprimentoTexto);
}

void JanelaMenuTft::mostrarMonitor(const EstadoPainel& estado) {
  titulo("MONITOR");
  char valor[24];
  if (estado.rf.transmitindo) {
    snprintf(valor, sizeof(valor), "%d dBFS", estado.audio.nivelDbfs);
    textoCentral(54, valor, 3, VALOR_MENU);
    const bool corte = (estado.audio.asq & 0x04) != 0; // Bit ASQ de sobremodulacao.
    textoCentral(98, corte ? "CORTE" : "SINAL OK", 2, corte ? VERMELHO : VERDE);
    textoCentral(139, "Transmissor ativo", 1, CINZA);
  } else {
    textoCentral(54, "-- dBFS", 3, CINZA);
    textoCentral(98, "TX SEM PORTADORA", 2, AMARELO);
    textoCentral(139, "Sem leitura de audio", 1, CINZA);
  }
}

void JanelaMenuTft::mostrarVarredura(const EstadoPainel& estado) {
  titulo("CANAL LIVRE");
  char valor[24];
  if (estado.varredura.ativa) {
    snprintf(valor, sizeof(valor), "%u%%", estado.varredura.progresso);
    textoCentral(55, valor, 3, VALOR_MENU);
    janela_.drawRect(24, 103, 240, 12, BORDA_JANELA);
    const uint8_t progresso = estado.varredura.progresso > 100 ? 100 : estado.varredura.progresso;
    janela_.fillRect(26, 105, 236 * progresso / 100, 8, VALOR_MENU);
    textoCentral(140, "TX PAUSADO", 2, CINZA);
    return;
  }
  if (estado.navegacao.resultadoBuscaDisponivel && estado.varredura.melhorFrequenciaKhz) {
    formatarFrequencia(valor, sizeof(valor), estado.varredura.melhorFrequenciaKhz);
    textoCentral(51, valor, 3, VALOR_MENU);
    uint8_t quantidade;
    const OpcaoMenu* opcoes = opcoesMenu(Tela::VARREDURA, quantidade);
    for (uint8_t i = 0; i < quantidade; i++) {
      linha(106 + i * 32, opcoes[i].nome, "", estado.navegacao.indice == i);
    }
  } else {
    textoCentral(64, "Sem resultado", 2, CINZA);
    linha(132, "VOLTAR", "", true);
  }
}

void JanelaMenuTft::titulo(const char* texto) {
  textoCentral(12, texto, 2, TEXTO_MENU);
}

void JanelaMenuTft::textoCentral(int16_t y, const char* texto, uint8_t tamanho, uint16_t cor) {
  int16_t x1, y1;
  uint16_t largura, altura;
  janela_.setTextSize(tamanho);
  janela_.setTextColor(cor);
  janela_.getTextBounds(texto, 0, 0, &x1, &y1, &largura, &altura);
  janela_.setCursor((JANELA_LARGURA - largura) / 2 - x1, y);
  janela_.print(texto);
}

void JanelaMenuTft::linha(int16_t y, const char* nome, const char* valor, bool selecionada) {
  if (selecionada) janela_.fillRoundRect(6, y, JANELA_LARGURA - 12, 25, 3, FUNDO_SELECAO);
  janela_.setTextSize(2);
  janela_.setTextColor(TEXTO_MENU);
  janela_.setCursor(14, y + 5);
  janela_.print(nome);
  // Valores secundarios usam fonte menor para manter o nome completo legivel.
  janela_.setTextSize(1);
  janela_.setTextColor(VALOR_MENU);
  janela_.setCursor(JANELA_LARGURA - 14 - strlen(valor) * 6, y + 9);
  janela_.print(valor);
}

#include "menu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
using Tela = TelaPainel;

// EDITE AQUI os nomes e a ordem. Desenho e navegacao usam estas mesmas listas.
// Cada linha: { funcao do item, "nome mostrado na tela" }.
const OpcaoMenu menuInicial[] = {
    {RAIZ_RF, "RF"},
    {RAIZ_AUDIO, "AUDIO"},
    {RAIZ_RDS, "RDS"},
    {RAIZ_MONITOR, "MONITOR"},
    {RAIZ_VARREDURA, "CANAL LIVRE"},
    {RAIZ_SISTEMA, "SISTEMA"},
    {RAIZ_VOLTAR, "VOLTAR"},
};
const OpcaoMenu menuRf[] = {
    {RF_TRANSMISSAO, "TRANSMISSAO"},
    {RF_FREQUENCIA, "FREQUENCIA"},
    {RF_POTENCIA, "POTENCIA"},
    {RF_ANTENA, "ANTENA"},
    {RF_VOLTAR, "VOLTAR"},
};
const OpcaoMenu menuAudio[] = {
    {AUDIO_ESTEREO, "MODO"},
    {AUDIO_PRE_ENFASE, "PRE-ENFASE"},
    {AUDIO_DESVIO, "DESVIO"},
    {AUDIO_MUDO, "ENTRADA DE AUDIO"},
    {AUDIO_VOLTAR, "VOLTAR"},
};
const OpcaoMenu menuRds[] = {
    {RDS_HABILITADO, "TRANSMISSAO RDS"},
    {RDS_PS, "NOME DA EMISSORA"},
    {RDS_TEXTO, "RADIOTEXT"},
    {RDS_PI, "CODIGO PI"},
    {RDS_VOLTAR, "VOLTAR"},
};
const OpcaoMenu menuResultado[] = {
    {VARREDURA_VOLTAR, "VOLTAR"},
    {VARREDURA_USAR_MELHOR, "APLICAR"},
};
const OpcaoMenu menuSistema[] = {
    {SISTEMA_REPOUSO, "REPOUSO"},
    {SISTEMA_PASSO_FREQUENCIA, "PASSO FREQUENCIA"},
    {SISTEMA_VOLUME_MONITOR, "VOLUME MONITOR"},
    {SISTEMA_VOLTAR, "VOLTAR"},
};

// atual e passos usam a unidade do parametro; minimo/maximo sao inclusivos.
int32_t ajustarNumero(int32_t atual, int8_t passos, int32_t minimo,
                      int32_t maximo, int32_t passo = 1) {
  const int32_t novo = atual + passos * passo;
  if (novo < minimo) return minimo;
  if (novo > maximo) return maximo;
  return novo;
}

// Move pela grade que comeca em 76,1 MHz. Se a frequencia atual estiver fora
// da grade escolhida, o primeiro giro entra no canal valido da mesma direcao.
int32_t ajustarFrequencia(int32_t atual, int8_t passos, int32_t passo) {
  const int32_t minimo = Configuracao::FREQUENCIA_MINIMA_KHZ;
  const int32_t maximo = Configuracao::FREQUENCIA_MAXIMA_KHZ;
  const int32_t deslocamento = atual - minimo;
  int32_t indice;
  if (passos > 0) {
    indice = deslocamento / passo + passos;
  } else {
    indice = (deslocamento + passo - 1) / passo + passos;
  }
  if (indice < 0) indice = 0;
  const int32_t ultimoIndice = (maximo - minimo) / passo;
  if (indice > ultimoIndice) indice = ultimoIndice;
  return minimo + indice * passo;
}

char ajustarCaractere(char atual, int8_t passos, bool hexadecimal) {
  const char* caracteres = hexadecimal ? "0123456789ABCDEF"
      : " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-{}:/";
  const char* encontrado = strchr(caracteres, atual);
  int posicao = encontrado ? encontrado - caracteres : 0;
  const int quantidade = strlen(caracteres);
  posicao = (posicao + passos) % quantidade;
  if (posicao < 0) posicao += quantidade;
  return caracteres[posicao];
}
}  // namespace

const OpcaoMenu* opcoesMenu(TelaPainel tela, uint8_t& quantidade) {
  switch (tela) {
    case Tela::RAIZ:
      quantidade = sizeof(menuInicial) / sizeof(menuInicial[0]); return menuInicial;
    case Tela::RF:
      quantidade = sizeof(menuRf) / sizeof(menuRf[0]); return menuRf;
    case Tela::AUDIO:
      quantidade = sizeof(menuAudio) / sizeof(menuAudio[0]); return menuAudio;
    case Tela::RDS:
      quantidade = sizeof(menuRds) / sizeof(menuRds[0]); return menuRds;
    case Tela::VARREDURA:
      quantidade = sizeof(menuResultado) / sizeof(menuResultado[0]); return menuResultado;
    case Tela::SISTEMA:
      quantidade = sizeof(menuSistema) / sizeof(menuSistema[0]); return menuSistema;
    default: quantidade = 0; return nullptr;
  }
}

const char* nomeItemMenu(ItemPainel item) {
  const Tela telas[] = {
      Tela::RAIZ, Tela::RF, Tela::AUDIO, Tela::RDS, Tela::VARREDURA,
      Tela::SISTEMA
  };
  for (Tela tela : telas) {
    uint8_t quantidade;
    const OpcaoMenu* opcoes = opcoesMenu(tela, quantidade);
    for (uint8_t i = 0; i < quantidade; i++) {
      if (opcoes[i].item == item) return opcoes[i].nome;
    }
  }
  return "";
}

Menu::Acao Menu::selecionar(ConfiguracaoTransmissor& configuracao) {
  if (erro_[0]) { erro_[0] = '\0'; return NENHUMA; }
  if (tela_ == Tela::PRINCIPAL) { entrar(Tela::RAIZ); return NENHUMA; }
  if (tela_ == Tela::MONITOR) { voltarAoMenu(); return NENHUMA; }
  if (tela_ == Tela::VARREDURA) {
    if (varreduraAtiva_) { voltarAoMenu(); return CANCELAR_VARREDURA; }
    if (itemAtual() == VARREDURA_USAR_MELHOR && resultadoDisponivel_) {
      return USAR_MELHOR_FREQUENCIA;
    }
    voltarAoMenu();
    return NENHUMA;
  }
  if (editando_) {
    if (comprimentoTexto_ && cursorTexto_ < comprimentoTexto_) {
      editandoCaractere_ = !editandoCaractere_;
      return NENHUMA;
    }
    copiarCampoConfirmado(configuracao);
    if (itemAtual() == RF_FREQUENCIA) return APLICAR_FREQUENCIA;
    if (itemAtual() == SISTEMA_REPOUSO) return SALVAR_REPOUSO_DISPLAY;
    if (itemAtual() == SISTEMA_VOLUME_MONITOR) return SALVAR_VOLUME_MONITOR;
    return APLICAR_CONFIGURACAO;
  }
  if (tela_ == Tela::RAIZ) {
    indiceRaiz_ = indice_;
    switch (itemAtual()) {
      case RAIZ_RF: entrar(Tela::RF); break;
      case RAIZ_AUDIO: entrar(Tela::AUDIO); break;
      case RAIZ_RDS: entrar(Tela::RDS); break;
      case RAIZ_MONITOR: entrar(Tela::MONITOR); break;
      case RAIZ_VARREDURA:
        entrar(Tela::VARREDURA);
        buscaIniciada_ = false;
        varreduraAtiva_ = true;
        resultadoDisponivel_ = false;
        return INICIAR_VARREDURA;
      case RAIZ_SISTEMA: entrar(Tela::SISTEMA); break;
      default: entrar(Tela::PRINCIPAL); break;
    }
    return NENHUMA;
  }
  // Opcoes binarias: o proprio clique alterna e aplica, sem abrir um editor.
  switch (itemAtual()) {
    case RF_TRANSMISSAO:
      configuracao.transmissaoHabilitada = !configuracao.transmissaoHabilitada;
      return APLICAR_CONFIGURACAO;
    case AUDIO_ESTEREO: {
      const uint8_t modos[] = {0, 3, 7, 1, 2,
          ConfiguracaoTransmissor::APENAS_L, ConfiguracaoTransmissor::APENAS_R};
      uint8_t proximo = 0;
      for (unsigned i = 0; i < sizeof(modos); ++i) {
        if (configuracao.modoAudio() == modos[i]) {
          proximo = modos[(i + 1) % sizeof(modos)];
          break;
        }
      }
      configuracao.selecionarMultiplex(proximo);
      return APLICAR_CONFIGURACAO;
    }
    case AUDIO_PRE_ENFASE:
      configuracao.preEnfaseUs = configuracao.preEnfaseUs == 50 ? 75 : 50;
      return APLICAR_CONFIGURACAO;
    case AUDIO_MUDO:
      configuracao.audioMudo = !configuracao.audioMudo;
      return APLICAR_CONFIGURACAO;
    case RDS_HABILITADO:
      configuracao.rdsHabilitado = !configuracao.rdsHabilitado;
      configuracao.modoMultiplex = 0xFF;
      return APLICAR_CONFIGURACAO;
    case SISTEMA_PASSO_FREQUENCIA:
      configuracao.passoFrequenciaKhz =
          configuracao.passoFrequenciaKhz == 20 ? 10 : 20;
      return SALVAR_PASSO_FREQUENCIA;
    case RF_VOLTAR: case AUDIO_VOLTAR: case RDS_VOLTAR:
    case SISTEMA_VOLTAR: voltarAoMenu(); break;
    default: iniciarEdicao(configuracao); break;
  }
  return NENHUMA;
}

void Menu::iniciarEdicao(const ConfiguracaoTransmissor& configuracao) {
  editando_ = true;
  editandoCaractere_ = false;
  comprimentoTexto_ = 0;
  cursorTexto_ = 0;
  switch (itemAtual()) {
    case RF_FREQUENCIA:
      valorEditado_ = configuracao.frequenciaKhz;
      passoFrequenciaEdicao_ = configuracao.passoFrequenciaKhz;
      break;
    case RF_POTENCIA: valorEditado_ = configuracao.potenciaDbuv; break;
    case RF_ANTENA: valorEditado_ = configuracao.capacitanciaAntena; break;
    case AUDIO_DESVIO: valorEditado_ = configuracao.desvioAudioKhz; break;
    case SISTEMA_REPOUSO:
      valorEditado_ = configuracao.repousoDisplaySegundos;
      break;
    case SISTEMA_VOLUME_MONITOR:
      valorEditado_ = configuracao.volumeMonitor;
      break;
    case RDS_PS:
      comprimentoTexto_ = 8;
      ConfiguracaoTransmissor::copiarTextoPreenchido(textoEditado_, 8, configuracao.rdsPs);
      break;
    case RDS_TEXTO:
      comprimentoTexto_ = 32;
      ConfiguracaoTransmissor::copiarTextoPreenchido(textoEditado_, 32, configuracao.rdsText);
      break;
    case RDS_PI:
      comprimentoTexto_ = 4;
      snprintf(textoEditado_, sizeof(textoEditado_), "%04X", configuracao.rdsPi);
      break;
    default: editando_ = false; break;
  }
}

Menu::Acao Menu::girar(int8_t deslocamento) {
  if (!deslocamento || erro_[0] || tela_ == Tela::PRINCIPAL
      || tela_ == Tela::MONITOR || (tela_ == Tela::VARREDURA && varreduraAtiva_)) {
    return NENHUMA;
  }
  if (!editando_) {
    const uint8_t quantidade = quantidadeItens();
    if (quantidade) {
      // Lista circular: antes do primeiro vem o ultimo, e depois dele o primeiro.
      int16_t proximo = (static_cast<int16_t>(indice_) + deslocamento) % quantidade;
      if (proximo < 0) proximo += quantidade;
      indice_ = static_cast<uint8_t>(proximo);
    }
    return NENHUMA;
  }
  if (comprimentoTexto_) {
    if (editandoCaractere_) {
      textoEditado_[cursorTexto_] = ajustarCaractere(
          textoEditado_[cursorTexto_], deslocamento, itemAtual() == RDS_PI);
    } else {
      cursorTexto_ = ajustarNumero(cursorTexto_, deslocamento, 0, comprimentoTexto_);
    }
    return NENHUMA;
  }
  switch (itemAtual()) {
    case RF_FREQUENCIA:
      valorEditado_ = ajustarFrequencia(
          valorEditado_, deslocamento, passoFrequenciaEdicao_);
      break;
    case RF_POTENCIA:
      valorEditado_ = ajustarNumero(valorEditado_, deslocamento,
          Configuracao::POTENCIA_MINIMA_DBUV, Configuracao::POTENCIA_MAXIMA_DBUV);
      break;
    case RF_ANTENA:
      valorEditado_ = ajustarNumero(valorEditado_, deslocamento, 0,
          Configuracao::CAPACITANCIA_ANTENA_MAXIMA);
      break;
    case AUDIO_DESVIO: valorEditado_ = ajustarNumero(valorEditado_, deslocamento, 50, 66); break;
    case SISTEMA_REPOUSO: {
      const auto& tempos = Configuracao::TEMPOS_REPOUSO_DISPLAY_SEGUNDOS;
      const int quantidade = sizeof(tempos) / sizeof(tempos[0]);
      int indice = 0;
      while (indice < quantidade && tempos[indice] != valorEditado_) indice++;
      if (indice == quantidade) indice = 0;
      indice = (indice + deslocamento) % quantidade;
      if (indice < 0) indice += quantidade;
      valorEditado_ = tempos[indice];
      break;
    }
    case SISTEMA_VOLUME_MONITOR:
      valorEditado_ = ajustarNumero(
          valorEditado_, deslocamento,
          Configuracao::VOLUME_MONITOR_MINIMO,
          Configuracao::VOLUME_MONITOR_MAXIMO);
      break;
    default: break;
  }
  return NENHUMA;
}

void Menu::copiarCampoConfirmado(ConfiguracaoTransmissor& configuracao) {
  // Preserva alteracoes feitas pela interface web enquanto o editor estava aberto.
  switch (itemAtual()) {
    case RF_FREQUENCIA: configuracao.frequenciaKhz = valorEditado_; break;
    case RF_POTENCIA: configuracao.potenciaDbuv = valorEditado_; break;
    case RF_ANTENA: configuracao.capacitanciaAntena = valorEditado_; break;
    case AUDIO_DESVIO: configuracao.desvioAudioKhz = valorEditado_; break;
    case SISTEMA_REPOUSO:
      configuracao.repousoDisplaySegundos = valorEditado_;
      break;
    case SISTEMA_VOLUME_MONITOR:
      configuracao.volumeMonitor = valorEditado_;
      break;
    case RDS_PS: memcpy(configuracao.rdsPs, textoEditado_, sizeof(configuracao.rdsPs)); break;
    case RDS_TEXTO: memcpy(configuracao.rdsText, textoEditado_, sizeof(configuracao.rdsText)); break;
    case RDS_PI: configuracao.rdsPi = strtoul(textoEditado_, nullptr, 16); break;
    default: break;
  }
}

Menu::Acao Menu::sairParaPrincipal() {
  const bool cancelar = tela_ == Tela::VARREDURA && varreduraAtiva_;
  entrar(Tela::PRINCIPAL);
  return cancelar ? CANCELAR_VARREDURA : NENHUMA;
}

void Menu::concluirAplicacao(bool sucesso) {
  if (!sucesso) { informarErro("Falha ao aplicar ou gravar"); return; }
  editando_ = false;
  editandoCaractere_ = false;
  comprimentoTexto_ = 0;
  if (tela_ == Tela::VARREDURA) voltarAoMenu();
}

void Menu::informarErro(const char* mensagem) {
  snprintf(erro_, sizeof(erro_), "%s", mensagem);
}

void Menu::confirmarInicioVarredura(bool sucesso) {
  buscaIniciada_ = sucesso;
  varreduraAtiva_ = sucesso;
  resultadoDisponivel_ = false;
  if (!sucesso) informarErro("Falha ao iniciar busca");
}

void Menu::atualizarVarredura(bool ativa, bool concluida, uint16_t melhorFrequencia) {
  varreduraAtiva_ = buscaIniciada_ && ativa;
  resultadoDisponivel_ = buscaIniciada_ && concluida && melhorFrequencia != 0;
  if (tela_ == Tela::VARREDURA && !resultadoDisponivel_) indice_ = 0;
}

void Menu::entrar(Tela tela) {
  tela_ = tela;
  indice_ = 0;
  cursorTexto_ = 0;
  comprimentoTexto_ = 0;
  editando_ = false;
  editandoCaractere_ = false;
  erro_[0] = '\0';
}

void Menu::voltarAoMenu() {
  entrar(Tela::RAIZ);
  indice_ = indiceRaiz_;
}

Menu::Tela Menu::tela() const { return tela_; }

uint8_t Menu::quantidadeItens() const {
  uint8_t quantidade;
  opcoesMenu(tela_, quantidade);
  if (tela_ == Tela::VARREDURA && !resultadoDisponivel_) return 1;
  return quantidade;
}

ItemPainel Menu::itemAtual() const {
  uint8_t quantidade;
  const OpcaoMenu* opcoes = opcoesMenu(tela_, quantidade);
  return indice_ < quantidade ? opcoes[indice_].item : NENHUM;
}

NavegacaoPainel Menu::navegacao() const {
  NavegacaoPainel n;
  n.tela = tela_;
  n.item = itemAtual();
  n.indice = indice_;
  n.quantidade = quantidadeItens();
  n.editando = editando_;
  n.resultadoBuscaDisponivel = resultadoDisponivel_;
  n.editandoCaractere = editandoCaractere_;
  n.valorEditado = valorEditado_;
  n.cursorTexto = cursorTexto_;
  n.comprimentoTexto = comprimentoTexto_;
  memcpy(n.textoEditado, textoEditado_, sizeof(n.textoEditado));
  memcpy(n.erro, erro_, sizeof(n.erro));
  return n;
}

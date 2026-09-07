#pragma once

#include "estado_painel.h"
#include "modelos.h"

// A ordem e os nomes dos itens ficam nas listas no inicio de menu.cpp.
struct OpcaoMenu {
  ItemPainel item;     // Identifica a funcao executada ao clicar.
  const char* nome;    // Texto que aparece na janela.
};

// tela: menu desejado. quantidade: recebe o tamanho da lista retornada.
const OpcaoMenu* opcoesMenu(TelaPainel tela, uint8_t& quantidade);
const char* nomeItemMenu(ItemPainel item);

class Menu {
 public:
  using Tela = TelaPainel;
  enum Acao : uint8_t {
    NENHUMA, APLICAR_CONFIGURACAO, APLICAR_FREQUENCIA,
    INICIAR_VARREDURA, CANCELAR_VARREDURA, USAR_MELHOR_FREQUENCIA
  };

  // configuracao: estado atual; recebe SOMENTE o campo confirmado pelo clique.
  Acao selecionar(ConfiguracaoTransmissor& configuracao);
  // deslocamento: passos do encoder; positivo avanca/aumenta, negativo recua.
  Acao girar(int8_t deslocamento);
  // Pressao longa: descarta o rascunho e fecha todas as janelas.
  Acao sairParaPrincipal();
  // Chamado depois da tentativa no hardware. Falha conserva o editor aberto.
  void concluirAplicacao(bool sucesso);
  void informarErro(const char* mensagem);
  // Rejeita resultados antigos se a tentativa de iniciar uma nova busca falhar.
  void confirmarInicioVarredura(bool sucesso);
  void atualizarVarredura(bool ativa, bool concluida, uint16_t melhorFrequencia);
  Tela tela() const;
  NavegacaoPainel navegacao() const;

 private:
  void entrar(Tela tela);
  void voltarAoMenu();
  void iniciarEdicao(const ConfiguracaoTransmissor& configuracao);
  void copiarCampoConfirmado(ConfiguracaoTransmissor& configuracao);
  ItemPainel itemAtual() const;
  uint8_t quantidadeItens() const;

  Tela tela_ = Tela::PRINCIPAL;
  uint8_t indice_ = 0;
  uint8_t indiceRaiz_ = 0;
  bool editando_ = false;
  bool editandoCaractere_ = false;
  int32_t valorEditado_ = 0;
  char textoEditado_[33] = {};
  uint8_t cursorTexto_ = 0;
  uint8_t comprimentoTexto_ = 0;
  char erro_[40] = {};
  bool buscaIniciada_ = false;
  bool varreduraAtiva_ = false;
  bool resultadoDisponivel_ = false;
};

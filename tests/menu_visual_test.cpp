#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "menu.h"
#include "janela_menu_tft.h"
#include "tela_principal_tft.h"

// Captura pixels do renderizador real, sem simular as funcoes de desenho.
void salvarImagem(const char* nome, GFXcanvas16& tela) {
  char caminho[120];
  snprintf(caminho, sizeof(caminho), "build/previas-menu/%s.ppm", nome);
  FILE* arquivo = fopen(caminho, "wb");
  assert(arquivo);
  fprintf(arquivo, "P6\n320 240\n255\n");
  for (int y = 0; y < 240; y++) {
    for (int x = 0; x < 320; x++) {
      const uint16_t cor = tela.getPixel(x, y);
      const unsigned char rgb[] = {
          static_cast<unsigned char>(((cor >> 11) & 31) * 255 / 31),
          static_cast<unsigned char>(((cor >> 5) & 63) * 255 / 63),
          static_cast<unsigned char>((cor & 31) * 255 / 31)};
      fwrite(rgb, 1, 3, arquivo);
    }
  }
  fclose(arquivo);
}

// No computador o destino e um canvas. Na placa, DisplayTft envia pelo driver SPI.
void desenharJanela(JanelaMenuTft& janela, GFXcanvas16& tela, const EstadoPainel& estado) {
  if (!janela.preparar(estado)) return;
  const auto& imagem = janela.imagem();
  tela.drawRGBBitmap(JanelaMenuTft::X, JanelaMenuTft::Y, imagem.getBuffer(),
                     imagem.width(), imagem.height());
}

int main() {
  GFXcanvas16 tela(320, 240);
  TelaPrincipalTft principal;
  JanelaMenuTft janela;
  assert(janela.pronta());
  EstadoPainel e;
  e.sistema.si4713Disponivel = true;
  e.rf.transmitindo = true;
  e.rf.transmissaoHabilitada = true;
  e.rf.frequenciaKhz = 9950;
  e.rf.potenciaDbuv = 100;
  e.rds.habilitado = true;
  e.rds.pi = 0x4713;
  e.audio.estereo = true;
  e.audio.componentesMultiplex = 7;
  e.audio.modoAudio = 7;
  e.audio.preEnfaseUs = 50;
  e.audio.desvioKhz = 66;
  strcpy(e.rds.textoAtual, "Transmissor FM Si4713");
  principal.renderizar(tela, e, true);
  const std::vector<uint16_t> fundo(tela.getBuffer(), tela.getBuffer() + 320 * 240);
  salvarImagem("principal", tela);

  const TelaPainel menus[] = {
      TelaPainel::RAIZ, TelaPainel::RF, TelaPainel::AUDIO, TelaPainel::RDS,
      TelaPainel::SISTEMA
  };
  const char* arquivos[] = {"raiz", "rf", "audio", "rds", "sistema"};
  for (unsigned i = 0; i < 5; i++) {
    e.navegacao.tela = menus[i];
    desenharJanela(janela, tela, e);
    salvarImagem(arquivos[i], tela);
  }
  e.navegacao.tela = TelaPainel::AUDIO;
  e.navegacao.indice = 0;
  e.navegacao.item = AUDIO_ESTEREO;
  const uint8_t multiplex[] = {0, 3, 7, 1, 2,
      ConfiguracaoTransmissor::APENAS_L, ConfiguracaoTransmissor::APENAS_R};
  const char* previasMultiplex[] = {"modo_mono", "modo_estereo", "modo_rds", "modo_piloto", "modo_lmr", "modo_apenas_l", "modo_apenas_r"};
  for (unsigned i = 0; i < sizeof(multiplex); ++i) {
    e.audio.modoAudio = multiplex[i];
    e.audio.componentesMultiplex = i < 5 ? multiplex[i] : 3;
    desenharJanela(janela, tela, e);
    assert(!janela.preparar(e));
    salvarImagem(previasMultiplex[i], tela);
  }
  // Atualizacoes periodicas e telemetria encoberta nao refazem a janela RDS.
  e.navegacao.tela = TelaPainel::RDS;
  e.navegacao.indice = 0;
  desenharJanela(janela, tela, e);
  assert(!janela.preparar(e));
  e.audio.nivelDbfs = -20;
  e.receptor.rssi = 50;
  assert(!janela.preparar(e));
  e.navegacao.indice = 1;
  assert(janela.preparar(e));
  assert(!janela.preparar(e));
  e.rds.pi++;
  assert(janela.preparar(e));
  assert(janela.preparar(e, true)); // Reabrir exige envio mesmo sem mudanca.
  e.rds.pi--;
  e.navegacao.indice = 0;
  desenharJanela(janela, tela, e);
  // O desenho respeita a posicao configurada, inclusive quando a janela sobe.
  const int esquerda = JanelaMenuTft::X;
  const int topo = JanelaMenuTft::Y;
  const int direita = esquerda + janela.imagem().width();
  const int base = topo + janela.imagem().height();
  // Verifica somente pixels externos a janela.
  for (int y = 0; y < 240; y++) {
    for (int x = 0; x < 320; x++) {
      if (y < topo || y >= base || x < esquerda || x >= direita) {
        assert(tela.getPixel(x, y) == fundo[y * 320 + x]);
      }
    }
  }
  // O cabecalho ocupa ate y=56. Verifica a parte da janela abaixo dessa faixa;
  // se Y for menor que 56, a janela cobre parte do cabecalho.
  const std::vector<uint16_t> comJanela(tela.getBuffer(), tela.getBuffer() + 320 * 240);
  e.rf.transmitindo = false;
  e.rf.frequenciaKhz = 10790;
  principal.renderizar(tela, e, false, true);
  for (int y = topo > 56 ? topo : 56; y < base; y++) {
    for (int x = esquerda; x < direita; x++) assert(tela.getPixel(x, y) == comJanela[y * 320 + x]);
  }
  e.rf.transmitindo = true;
  principal.renderizar(tela, e, false, true);

  e.navegacao.editando = true;
  e.navegacao.item = RF_FREQUENCIA;
  e.navegacao.valorEditado = 10800;
  desenharJanela(janela, tela, e); salvarImagem("frequencia", tela);
  e.navegacao.tela = TelaPainel::SISTEMA;
  e.navegacao.item = SISTEMA_REPOUSO;
  e.navegacao.valorEditado = 30;
  desenharJanela(janela, tela, e); salvarImagem("repouso", tela);
  e.navegacao.editando = false;
  e.navegacao.tela = TelaPainel::RF;
  e.navegacao.item = RF_TRANSMISSAO;
  e.navegacao.indice = 0;
  desenharJanela(janela, tela, e); salvarImagem("transmissao", tela);
  e.rf.transmissaoHabilitada = false;
  assert(janela.preparar(e)); // Alternancia deve invalidar o valor na lista.
  e.rf.transmissaoHabilitada = true;
  e.navegacao.editando = true;
  e.navegacao.item = RDS_TEXTO;
  e.navegacao.comprimentoTexto = 32;
  e.navegacao.cursorTexto = 31;
  e.navegacao.editandoCaractere = true;
  strcpy(e.navegacao.textoEditado, "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345");
  desenharJanela(janela, tela, e); salvarImagem("radiotext", tela);
  e.navegacao.editandoCaractere = false;
  e.navegacao.cursorTexto = 32;
  desenharJanela(janela, tela, e); salvarImagem("concluir_texto", tela);
  e.navegacao.item = RDS_PI;
  e.navegacao.comprimentoTexto = 4;
  e.navegacao.cursorTexto = 0;
  strcpy(e.navegacao.textoEditado, "4713");
  desenharJanela(janela, tela, e); salvarImagem("pi", tela);
  e.navegacao.editando = false;
  e.navegacao.tela = TelaPainel::MONITOR;
  e.audio.nivelDbfs = -12;
  desenharJanela(janela, tela, e); salvarImagem("monitor", tela);
  e.navegacao.tela = TelaPainel::VARREDURA;
  e.varredura.ativa = true;
  e.varredura.progresso = 43;
  e.rf.transmitindo = false;
  principal.renderizar(tela, e, false, true);
  desenharJanela(janela, tela, e); salvarImagem("busca", tela);
  e.varredura.ativa = false;
  e.navegacao.resultadoBuscaDisponivel = true;
  e.rf.transmitindo = true;
  principal.renderizar(tela, e, false, true);
  e.varredura.melhorFrequenciaKhz = 10100;
  e.navegacao.indice = 1;
  desenharJanela(janela, tela, e); salvarImagem("resultado", tela);
  strcpy(e.navegacao.erro, "Falha ao aplicar ou gravar");
  desenharJanela(janela, tela, e); salvarImagem("erro", tela);

  // Retorno restaura a tela inteira, inclusive pixels que estavam sob a janela.
  e.rf.frequenciaKhz = 9950;
  principal.renderizar(tela, e, true);
  assert(memcmp(tela.getBuffer(), fundo.data(), fundo.size() * sizeof(uint16_t)) == 0);
  // As telas dinamicas continuam atualizando apenas seus dados visiveis.
  e.navegacao.erro[0] = '\0';
  e.navegacao.tela = TelaPainel::MONITOR;
  assert(janela.preparar(e));
  assert(!janela.preparar(e));
  e.audio.nivelDbfs++;
  assert(janela.preparar(e));
  e.audio.asq ^= 0x04;
  assert(janela.preparar(e));
  e.rf.transmitindo = false;
  assert(janela.preparar(e));
  e.audio.nivelDbfs++;
  assert(!janela.preparar(e)); // Sem portadora, o nivel nao aparece.
  e.navegacao.tela = TelaPainel::VARREDURA;
  e.varredura.ativa = true;
  assert(janela.preparar(e));
  assert(!janela.preparar(e));
  e.varredura.progresso++;
  assert(janela.preparar(e));
  e.varredura.ativa = false;
  assert(janela.preparar(e));
  e.varredura.melhorFrequenciaKhz += 10;
  assert(janela.preparar(e));
  e.navegacao.editando = true;
  e.navegacao.item = RF_FREQUENCIA;
  assert(janela.preparar(e));
  e.rf.frequenciaKhz += 10;
  assert(!janela.preparar(e)); // Edicao mostra o rascunho, nao a telemetria.
  e.navegacao.valorEditado += 10;
  assert(janela.preparar(e));
  puts("TFT: sobreposicao, atualizacao do cabecalho e restauracao e cache de atualizacoes passaram.");
}

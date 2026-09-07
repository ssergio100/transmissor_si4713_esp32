#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "menu.h"

using Tela = TelaPainel;

// Percorre a lista pelo encoder ate o item pedido, sem depender da ordem visual.
void escolher(Menu& menu, ItemPainel item) {
  for (unsigned i = 0; i < 20 && menu.navegacao().item != item; i++) menu.girar(1);
  assert(menu.navegacao().item == item);
}

void abrir(Menu& menu, ConfiguracaoTransmissor& c, ItemPainel categoria, ItemPainel item) {
  menu.sairParaPrincipal();
  assert(menu.selecionar(c) == Menu::NENHUMA);
  escolher(menu, categoria);
  assert(menu.selecionar(c) == Menu::NENHUMA);
  escolher(menu, item);
  assert(menu.selecionar(c) == Menu::NENHUMA);
  assert(menu.navegacao().editando);
}

// Chega ao item sem clicar nele; o clique seguinte ja deve aplicar a alternancia.
void posicionarBinario(Menu& menu, ConfiguracaoTransmissor& c, ItemPainel categoria, ItemPainel item) {
  menu.sairParaPrincipal();
  menu.selecionar(c);
  escolher(menu, categoria);
  menu.selecionar(c);
  escolher(menu, item);
  assert(!menu.navegacao().editando);
}

void clicarBinario(Menu& menu, ConfiguracaoTransmissor& c) {
  const auto antes = menu.navegacao();
  assert(menu.selecionar(c) == Menu::APLICAR_CONFIGURACAO);
  assert(!menu.navegacao().editando);
  menu.concluirAplicacao(true);
  assert(menu.navegacao().tela == antes.tela);
  assert(menu.navegacao().item == antes.item);
  assert(menu.navegacao().indice == antes.indice);
}

int main() {
  ConfiguracaoTransmissor c;
  Menu menu;
  assert(menu.sairParaPrincipal() == Menu::NENHUMA);
  assert(menu.tela() == Tela::PRINCIPAL);

  // Um passo para tras no primeiro item chega a VOLTAR; um para frente retorna.
  menu.selecionar(c);
  assert(menu.navegacao().item == RAIZ_RF);
  menu.girar(-1);
  assert(menu.navegacao().item == RAIZ_VOLTAR);
  menu.girar(1);
  assert(menu.navegacao().item == RAIZ_RF);
  menu.selecionar(c);
  assert(menu.navegacao().item == RF_TRANSMISSAO);
  menu.girar(-1);
  assert(menu.navegacao().item == RF_VOLTAR);
  menu.girar(1);
  assert(menu.navegacao().item == RF_TRANSMISSAO);

  // A frequencia so muda no clique. Pressao longa descarta o valor provisório.
  abrir(menu, c, RAIZ_RF, RF_FREQUENCIA);
  const uint16_t original = c.frequenciaKhz;
  assert(menu.girar(3) == Menu::NENHUMA);
  assert(c.frequenciaKhz == original);
  assert(menu.navegacao().valorEditado == original + 3 * c.passoFrequenciaKhz);
  assert(menu.sairParaPrincipal() == Menu::NENHUMA);
  assert(c.frequenciaKhz == original && menu.tela() == Tela::PRINCIPAL);

  abrir(menu, c, RAIZ_RF, RF_FREQUENCIA);
  menu.girar(1);
  c.potenciaDbuv = 110; // Simula uma alteracao web durante o ajuste local.
  assert(menu.selecionar(c) == Menu::APLICAR_FREQUENCIA);
  assert(c.frequenciaKhz == original + c.passoFrequenciaKhz);
  assert(c.potenciaDbuv == 110);
  menu.concluirAplicacao(true);
  assert(!menu.navegacao().editando && menu.navegacao().item == RF_FREQUENCIA);
  menu.sairParaPrincipal();
  assert(c.frequenciaKhz != original); // Saida nao desfaz o que ja foi confirmado.

  // Limites numericos nao circulam do maximo para o minimo.
  abrir(menu, c, RAIZ_RF, RF_FREQUENCIA);
  for (int i = 0; i < 4; i++) menu.girar(100);
  assert(menu.navegacao().valorEditado == 10790);
  for (int i = 0; i < 4; i++) menu.girar(-100);
  assert(menu.navegacao().valorEditado == Configuracao::FREQUENCIA_MINIMA_KHZ);

  abrir(menu, c, RAIZ_RF, RF_POTENCIA);
  menu.girar(100);
  assert(menu.navegacao().valorEditado == Configuracao::POTENCIA_MAXIMA_DBUV);
  assert(c.potenciaDbuv == 110);
  menu.girar(-100);
  assert(menu.navegacao().valorEditado == Configuracao::POTENCIA_MINIMA_DBUV);

  abrir(menu, c, RAIZ_RF, RF_ANTENA);
  menu.girar(100); menu.girar(100);
  assert(menu.navegacao().valorEditado == Configuracao::CAPACITANCIA_ANTENA_MAXIMA);
  menu.girar(-100); menu.girar(-100);
  assert(menu.navegacao().valorEditado == 0);

  // Cada clique alterna, nos dois sentidos, e permanece na mesma linha.
  posicionarBinario(menu, c, RAIZ_RF, RF_TRANSMISSAO);
  assert(!c.transmissaoHabilitada);
  clicarBinario(menu, c); assert(c.transmissaoHabilitada);
  clicarBinario(menu, c); assert(!c.transmissaoHabilitada);
  posicionarBinario(menu, c, RAIZ_AUDIO, AUDIO_ESTEREO);
  const uint8_t modosEsperados[] = {1, 2,
      ConfiguracaoTransmissor::APENAS_L, ConfiguracaoTransmissor::APENAS_R, 0, 3, 7};
  const ConfiguracaoTransmissor antesMultiplex = c;
  for (uint8_t esperado : modosEsperados) {
    clicarBinario(menu, c);
    assert(c.modoAudio() == esperado);
    const bool isolado = esperado == ConfiguracaoTransmissor::APENAS_L
        || esperado == ConfiguracaoTransmissor::APENAS_R;
    assert(c.componentesMultiplex() == (isolado ? 3 : esperado));
    assert(c.estereo == ((c.componentesMultiplex() & 3) == 3));
    assert(c.rdsHabilitado == (!isolado && (esperado & 4) != 0));
    assert(c.valoresValidos());
    assert(c.frequenciaKhz == antesMultiplex.frequenciaKhz);
    assert(c.potenciaDbuv == antesMultiplex.potenciaDbuv);
    assert(c.preEnfaseUs == antesMultiplex.preEnfaseUs);
    assert(c.desvioAudioKhz == antesMultiplex.desvioAudioKhz);
    assert(c.audioMudo == antesMultiplex.audioMudo);
  }
  posicionarBinario(menu, c, RAIZ_AUDIO, AUDIO_PRE_ENFASE);
  clicarBinario(menu, c); assert(c.preEnfaseUs == 75);
  clicarBinario(menu, c); assert(c.preEnfaseUs == 50);
  posicionarBinario(menu, c, RAIZ_AUDIO, AUDIO_MUDO);
  clicarBinario(menu, c); assert(c.audioMudo);
  clicarBinario(menu, c); assert(!c.audioMudo);
  // Falha: dispensar a mensagem nao aplica outra alternancia nem abre editor.
  ConfiguracaoTransmissor tentativa = c;
  assert(menu.selecionar(tentativa) == Menu::APLICAR_CONFIGURACAO);
  menu.concluirAplicacao(false);
  assert(menu.navegacao().erro[0] && !menu.navegacao().editando);
  assert(menu.selecionar(c) == Menu::NENHUMA);
  assert(!c.audioMudo && menu.navegacao().item == AUDIO_MUDO);
  abrir(menu, c, RAIZ_AUDIO, AUDIO_DESVIO);
  menu.girar(-100);
  assert(menu.navegacao().valorEditado == 50);
  menu.girar(100);
  assert(menu.navegacao().valorEditado == 66);

  // Falha conserva o editor; o clique na mensagem apenas a dispensa.
  menu.concluirAplicacao(false);
  assert(menu.navegacao().editando && menu.navegacao().erro[0]);
  assert(menu.selecionar(c) == Menu::NENHUMA);
  assert(menu.navegacao().editando && !menu.navegacao().erro[0]);
  menu.sairParaPrincipal();

  // Texto: escolher posicao, editar caractere, retornar e concluir antes do fim.
  abrir(menu, c, RAIZ_RDS, RDS_PS);
  assert(menu.navegacao().comprimentoTexto == 8);
  const char primeiro = c.rdsPs[0];
  menu.selecionar(c);
  assert(menu.navegacao().editandoCaractere);
  menu.girar(1);
  assert(c.rdsPs[0] == primeiro);
  menu.selecionar(c);
  assert(!menu.navegacao().editandoCaractere);
  menu.girar(100); // CONCLUIR e uma posicao selecionavel.
  assert(menu.navegacao().cursorTexto == 8);
  assert(menu.selecionar(c) == Menu::APLICAR_CONFIGURACAO);
  menu.concluirAplicacao(true);
  assert(c.rdsPs[0] != primeiro && c.rdsPs[8] == '\0');

  abrir(menu, c, RAIZ_RDS, RDS_TEXTO);
  char textoOriginal[33]; memcpy(textoOriginal, c.rdsText, sizeof(textoOriginal));
  menu.girar(31); menu.selecionar(c); menu.girar(1);
  menu.sairParaPrincipal();
  assert(memcmp(textoOriginal, c.rdsText, sizeof(textoOriginal)) == 0);

  abrir(menu, c, RAIZ_RDS, RDS_PI);
  menu.selecionar(c); menu.girar(1); menu.selecionar(c); menu.girar(4);
  assert(menu.selecionar(c) == Menu::APLICAR_CONFIGURACAO);
  menu.concluirAplicacao(true);
  assert(c.rdsPi == 0x5713);

  posicionarBinario(menu, c, RAIZ_RDS, RDS_HABILITADO);
  clicarBinario(menu, c); assert(!c.rdsHabilitado);
  clicarBinario(menu, c); assert(c.rdsHabilitado);
  escolher(menu, RDS_VOLTAR); menu.selecionar(c);
  assert(menu.tela() == Tela::RAIZ && menu.navegacao().item == RAIZ_RDS);

  escolher(menu, RAIZ_MONITOR); menu.selecionar(c);
  assert(menu.tela() == Tela::MONITOR);
  menu.girar(100);
  assert(menu.selecionar(c) == Menu::NENHUMA);
  assert(menu.navegacao().item == RAIZ_MONITOR);

  // Canal livre inicia ao entrar. Giro nao cancela; clique e pressao longa sim.
  escolher(menu, RAIZ_VARREDURA);
  assert(menu.selecionar(c) == Menu::INICIAR_VARREDURA);
  assert(menu.girar(1) == Menu::NENHUMA);
  assert(menu.tela() == Tela::VARREDURA);
  assert(menu.selecionar(c) == Menu::CANCELAR_VARREDURA);
  assert(menu.tela() == Tela::RAIZ && menu.navegacao().item == RAIZ_VARREDURA);
  assert(menu.selecionar(c) == Menu::INICIAR_VARREDURA);
  assert(menu.sairParaPrincipal() == Menu::CANCELAR_VARREDURA);
  assert(menu.tela() == Tela::PRINCIPAL);

  menu.selecionar(c); escolher(menu, RAIZ_VARREDURA); menu.selecionar(c);
  menu.confirmarInicioVarredura(true);
  menu.atualizarVarredura(false, true, 10100);
  assert(menu.navegacao().quantidade == 2);
  escolher(menu, VARREDURA_USAR_MELHOR);
  assert(menu.selecionar(c) == Menu::USAR_MELHOR_FREQUENCIA);
  menu.concluirAplicacao(true);
  assert(menu.tela() == Tela::RAIZ);

  menu.selecionar(c); // Nova busca nao reaproveita resultado antigo.
  menu.confirmarInicioVarredura(false);
  menu.atualizarVarredura(false, true, 10100); // Resultado antigo do hardware.
  assert(!menu.navegacao().resultadoBuscaDisponivel);
  assert(menu.selecionar(c) == Menu::NENHUMA); // Fecha somente a mensagem de erro.
  menu.girar(100);
  assert(menu.navegacao().quantidade == 1);
  assert(menu.selecionar(c) == Menu::NENHUMA && menu.tela() == Tela::RAIZ);
  escolher(menu, RAIZ_VOLTAR); menu.selecionar(c);
  assert(menu.tela() == Tela::PRINCIPAL);

  // SISTEMA permite escolher os tempos definidos na configuracao do projeto.
  menu.selecionar(c);
  escolher(menu, RAIZ_SISTEMA);
  menu.selecionar(c);
  assert(menu.tela() == Tela::SISTEMA);
  assert(menu.navegacao().item == SISTEMA_REPOUSO);
  menu.selecionar(c);
  assert(menu.navegacao().editando);
  assert(menu.navegacao().valorEditado == 300);
  menu.girar(-3);
  assert(menu.navegacao().valorEditado == 13);
  menu.girar(1);
  assert(menu.navegacao().valorEditado == 30);
  assert(menu.selecionar(c) == Menu::SALVAR_REPOUSO_DISPLAY);
  assert(c.repousoDisplaySegundos == 30);
  menu.concluirAplicacao(true);
  assert(!menu.navegacao().editando);
  menu.selecionar(c);
  menu.girar(-1);
  assert(menu.navegacao().valorEditado == 13);
  menu.sairParaPrincipal();
  assert(c.repousoDisplaySegundos == 30);

  // O clique alterna o passo local entre 200 kHz (Brasil) e 100 kHz.
  posicionarBinario(menu, c, RAIZ_SISTEMA, SISTEMA_PASSO_FREQUENCIA);
  assert(c.passoFrequenciaKhz == 20);
  assert(menu.selecionar(c) == Menu::SALVAR_PASSO_FREQUENCIA);
  assert(c.passoFrequenciaKhz == 10);
  menu.concluirAplicacao(true);
  assert(menu.selecionar(c) == Menu::SALVAR_PASSO_FREQUENCIA);
  assert(c.passoFrequenciaKhz == 20);
  menu.concluirAplicacao(true);

  // Volume do receptor interno usa a escala completa oferecida pelo RDA5807.
  abrir(menu, c, RAIZ_SISTEMA, SISTEMA_VOLUME_MONITOR);
  assert(menu.navegacao().valorEditado == Configuracao::VOLUME_MONITOR_PADRAO);
  menu.girar(100);
  assert(menu.navegacao().valorEditado == Configuracao::VOLUME_MONITOR_MAXIMO);
  assert(menu.selecionar(c) == Menu::SALVAR_VOLUME_MONITOR);
  assert(c.volumeMonitor == Configuracao::VOLUME_MONITOR_MAXIMO);
  menu.concluirAplicacao(true);

  // Uma frequencia antiga fora da grade brasileira entra nela no primeiro giro.
  c.frequenciaKhz = 9960;
  abrir(menu, c, RAIZ_RF, RF_FREQUENCIA);
  menu.girar(1);
  assert(menu.navegacao().valorEditado == 9970);
  puts("Menus: confirmacao, cancelamento, limites, textos, monitor e busca passaram.");
}

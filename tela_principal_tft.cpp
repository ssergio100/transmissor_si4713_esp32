#include "tela_principal_tft.h"
#include "no_ar_suave.h"
#include "no_ar_suave_imagem.h"
#include <stdio.h>
#include <string.h>

// ======================================================
// 1. AJUSTE RAPIDO: SUBIR / DESCER SOMENTE OS VALORES
// ======================================================
// Unidade: pixels, contados a partir do topo do respectivo bloco.
// Maior numero = desce. Menor numero = sobe. Titulo e fundo ficam no lugar.
// Exemplo: FREQUENCIA_DISTANCIA_DO_TOPO de 27 para 28 desce o numero 1 pixel.
// Para mover o bloco inteiro ou mudar a fonte, use AREAS na secao 2.
constexpr int16_t FREQUENCIA_DISTANCIA_DO_TOPO = 27;
constexpr int16_t RDS_DISTANCIA_DO_TOPO = 23;
constexpr int16_t POTENCIA_DISTANCIA_DO_TOPO = 21;
constexpr int16_t RSSI_DISTANCIA_DO_TOPO = 21;
constexpr int16_t AUDIO_DISTANCIA_DO_TOPO = 23;
constexpr int16_t MODO_DISTANCIA_DO_TOPO = 23;
constexpr int16_t PRE_DESVIO_DISTANCIA_DO_TOPO = 21;

// Excecao: o texto da transmissao parte do alinhamento central do bloco.
// 0 = posicao atual; +1 = desce 1 pixel; -1 = sobe 1 pixel.
// Afeta NO AR, TX OFF, TX PAUSADO e FALHA TX; nao move a antena.
constexpr int16_t TRANSMISSAO_AJUSTE_VERTICAL = 0;
// Teste visual: false volta ao texto original, sem trocar o driver.
// Tamanho da imagem: ajuste TAMANHO em scripts/gerar_no_ar.py e execute o script.
constexpr bool NO_AR_SUAVIZADO = true;

// ======================================================
// 2. LAYOUT: POSICAO, DIMENSOES, TITULO E FONTE DOS BLOCOS
// ======================================================
// AREAS e uma tabela de configuracao, nao uma funcao.
// Cada linha descreve um bloco, nesta ordem de campos:
//
//   { Bloco::Nome, x, y, largura, altura, "titulo", fonte, distanciaDoValorAoTopo }
//   Campo                  O que altera
//   Bloco::Nome            Identifica o indicador, independente da ordem.
//   x / y                  Esquerda / topo: move o bloco inteiro.
//   largura / altura       Dimensoes do retangulo, em pixels.
//   titulo                 Legenda; "" deixa o bloco sem titulo.
//   fonte                  Escala do valor: 1, 2, 3, 4, 5...
//   distanciaDoValorAoTopo  Posicao vertical somente do valor, em pixels.
//
// Exemplos na linha Frequencia:
//   y:      62 -> 63  = desce fundo, titulo e numero juntos em 1 pixel.
//   altura: 88 -> 90  = aumenta o retangulo; nao aumenta a fonte.
//   fonte:   5 -> 4   = diminui apenas o numero; titulo continua tamanho 1.
//   Para descer SO o numero, edite FREQUENCIA_DISTANCIA_DO_TOPO acima.
//
// Tela: 320 x 240 pixels. Verifique espaco entre blocos ao aumentar dimensoes.
// A fonte dos valores nao diminui automaticamente, exceto na transmissao.
// Na transmissao, a ultima coluna e ignorada: use TRANSMISSAO_AJUSTE_VERTICAL.
// As linhas podem ser reordenadas: cada uma identifica seu Bloco explicitamente.
// Use uma unica linha por bloco.
//
//    bloco                 x    y    larg alt  titulo                 fonte distancia do valor
const TelaPrincipalTft::Area TelaPrincipalTft::AREAS[] = {
    {Bloco::Transmissao    ,   6,   6, 198, 50, ""                    , 4, 0 },  // Transmissao: centralizada
    {Bloco::Rds            , 210,   6, 104, 50, "RDS TX CONFIG."      , 2, RDS_DISTANCIA_DO_TOPO },
    {Bloco::Frequencia     ,   6,  62, 198, 88, "FREQUENCIA FM / MHz" , 5, FREQUENCIA_DISTANCIA_DO_TOPO },
    {Bloco::Potencia       , 210,  62, 104, 42, "POTENCIA dBuV"       , 2, POTENCIA_DISTANCIA_DO_TOPO },
    {Bloco::Rssi           , 210, 110, 104, 40, "RSSI RX"             , 2, RSSI_DISTANCIA_DO_TOPO },
    {Bloco::Audio          ,   6, 156,  98, 50, "AUDIO"               , 2, AUDIO_DISTANCIA_DO_TOPO },
    {Bloco::Modo           , 110, 156,  94, 50, "MODO"                , 2, MODO_DISTANCIA_DO_TOPO },
    {Bloco::PreEnfaseDesvio, 210, 156, 104, 50, "PRE us/DEV kHz"      , 2, PRE_DESVIO_DISTANCIA_DO_TOPO },
};

// ======================================================
// 3. COMPOSICAO: QUAIS BLOCOS SAO DESENHADOS
// ======================================================
// tft = display ja inicializado; estado = valores atuais do equipamento.
// entrada = true ao abrir/retornar a esta tela: redesenha todos os blocos.
// Nas atualizacoes seguintes, false permite reutilizar o desenho sem mudancas.
// Posicao e tamanho ficam em AREAS; a ordem das chamadas nao muda a posicao.
void TelaPrincipalTft::renderizar(Adafruit_GFX& tft,
    const EstadoPainel& estado, bool entrada) {
  tft_ = &tft;
  entrada_ = entrada;
  tft_->setTextWrap(false);
  if (entrada_) tft_->fillScreen(TemaTft::FUNDO_TELA);

  desenharTransmissao(estado);
  desenharRds(estado);
  desenharFrequencia(estado);
  desenharPotencia(estado);
  desenharRssi(estado);
  desenharAudio(estado);
  desenharModo(estado);
  desenharPreEnfaseDesvio(estado);
  desenharRodape(estado);
}

// ======================================================
// 4. CONTEUDO: O QUE CADA BLOCO MOSTRA
// ======================================================
// Cada funcao escolhe o texto/numero e as cores conforme o estado recebido.
// desenharBloco(bloco, valor, estilo):
//   bloco  = qual linha de AREAS usar;
//   valor  = texto principal, por exemplo "NO AR" ou "107.90";
//   estilo = conjunto de cores definido em tema_tft.h.
// Para mudar a legenda, use AREAS. Para mudar as cores, use tema_tft.h.
void TelaPrincipalTft::desenharTransmissao(const EstadoPainel& estado) {
  if (!estado.sistema.si4713Disponivel) {
    desenharBloco(Bloco::Transmissao, "FALHA TX", TemaTft::TX_FALHA);
  } else if (estado.rf.transmitindo) {
    desenharBloco(Bloco::Transmissao, "NO AR", TemaTft::TX_NO_AR);
  } else if (estado.rf.transmissaoHabilitada) {
    desenharBloco(Bloco::Transmissao, "TX PAUSADO", TemaTft::TX_PAUSADO);
  } else {
    desenharBloco(Bloco::Transmissao, "TX OFF", TemaTft::TX_DESLIGADO);
  }
}

void TelaPrincipalTft::desenharRds(const EstadoPainel& estado) {
  if (estado.rds.habilitado) {
    desenharBloco(Bloco::Rds, "ON", TemaTft::RDS_LIGADO);
  } else {
    desenharBloco(Bloco::Rds, "OFF", TemaTft::RDS_DESLIGADO);
  }
}

void TelaPrincipalTft::desenharFrequencia(const EstadoPainel& estado) {
  // Nome legado: frequenciaKhz armazena passos de 10 kHz, nao kHz literais.
  // Exemplo: 9950 passos = 99500 kHz = 99.50 MHz.
  const uint16_t frequenciaEm10Khz = estado.rf.frequenciaKhz;
  char valor[16];
  snprintf(valor, sizeof(valor), "%u.%02u",
           frequenciaEm10Khz / 100, frequenciaEm10Khz % 100);
  desenharBloco(Bloco::Frequencia, valor, TemaTft::FREQUENCIA);
}

void TelaPrincipalTft::desenharPotencia(const EstadoPainel& estado) {
  char valor[16];
  snprintf(valor, sizeof(valor), "%u", estado.rf.potenciaDbuv);
  desenharBloco(Bloco::Potencia, valor, TemaTft::POTENCIA);
}

void TelaPrincipalTft::desenharRssi(const EstadoPainel& estado) {
  if (!estado.receptor.disponivel || !estado.receptor.leituraRssiValida) {
    desenharBloco(Bloco::Rssi, "---", TemaTft::RSSI_INDISPONIVEL);
    return;
  }
  char valor[8];
  snprintf(valor, sizeof(valor), "%u", estado.receptor.rssi);
  desenharBloco(Bloco::Rssi, valor, TemaTft::RSSI_VALIDO);
}

void TelaPrincipalTft::desenharAudio(const EstadoPainel& estado) {
  if (estado.audio.mudo) {
    desenharBloco(Bloco::Audio, "MUDO", TemaTft::AUDIO_MUDO);
  } else {
    desenharBloco(Bloco::Audio, "ON", TemaTft::AUDIO_LIGADO);
  }
}

void TelaPrincipalTft::desenharModo(const EstadoPainel& estado) {
  if (estado.audio.estereo) {
    desenharBloco(Bloco::Modo, "ST", TemaTft::MODO_ESTEREO);
  } else {
    desenharBloco(Bloco::Modo, "MONO", TemaTft::MODO_MONO);
  }
}

void TelaPrincipalTft::desenharPreEnfaseDesvio(const EstadoPainel& estado) {
  char valor[16];
  snprintf(valor, sizeof(valor), "%u / %u",
           estado.audio.preEnfaseUs, estado.audio.desvioKhz);
  desenharBloco(Bloco::PreEnfaseDesvio, valor, TemaTft::PRE_ENFASE_DESVIO);
}

// Rodape RDS: frase inteira (ate 32 caracteres), centralizada em fonte 1.
// Atualiza somente quando o texto muda; a faixa fica em y=214, altura=26.
void TelaPrincipalTft::desenharRodape(const EstadoPainel& estado) {
  char mensagem[40];
  snprintf(mensagem, sizeof(mensagem), "RDS: %.32s", estado.rds.textoAtual);
  // Remove o preenchimento do RDS para centralizar apenas a frase visivel.
  size_t tamanho = strlen(mensagem);
  while (tamanho > 0 && mensagem[tamanho - 1] == ' ') mensagem[--tamanho] = '\0';
  if (tamanho <= 4) snprintf(mensagem, sizeof(mensagem), "RDS: sem frase");
  if (!entrada_ && strcmp(textoRodapeAnterior_, mensagem) == 0) return;

  tft_->fillRect(0, 214, 320, 26, TemaTft::FUNDO_RODAPE);
  int16_t deslocamentoX, deslocamentoY;
  uint16_t largura, altura;
  tft_->setTextSize(1);
  tft_->getTextBounds(mensagem, 0, 0, &deslocamentoX, &deslocamentoY,
                      &largura, &altura);
  const int x = (320 - largura) / 2 - deslocamentoX;
  const int y = 214 + (26 - altura) / 2 - deslocamentoY;
  escreverTexto(x, y, mensagem, 1, TemaTft::TEXTO_RODAPE);
  snprintf(textoRodapeAnterior_, sizeof(textoRodapeAnterior_), "%s", mensagem);
}

// ======================================================
// 5. AUXILIARES: COMO OS ELEMENTOS SAO DESENHADOS
// ======================================================
// Ajustes individuais ficam nas secoes 1 e 2; estas funcoes sao compartilhadas.
// escreverTexto(x, y, texto, tamanho, cor): x/y sao coordenadas da TELA,
// nao deslocamentos do bloco. Esta funcao nao centraliza nem limpa o fundo.
void TelaPrincipalTft::escreverTexto(int16_t x, int16_t y,
    const char* texto, uint8_t tamanho, uint16_t cor) {
  tft_->setTextSize(tamanho);
  tft_->setTextColor(cor);
  tft_->setCursor(x, y);
  tft_->print(texto);
}

// Fundo e borda usam raio 6. Titulo centralizado, 8 pixels abaixo do topo.
// O tamanho 1 abaixo e a fonte de TODOS os titulos; nao e a fonte do valor.
void TelaPrincipalTft::desenharPainel(const Area& area,
    const TemaTft::EstiloBloco& estilo) {
  tft_->fillRoundRect(area.x, area.y, area.largura, area.altura, 6, estilo.fundo);
  tft_->drawRoundRect(area.x, area.y, area.largura, area.altura, 6, estilo.borda);
  int16_t deslocamentoX, deslocamentoY;
  uint16_t larguraTitulo, alturaTitulo;
  tft_->setTextSize(1);
  tft_->getTextBounds(area.titulo, 0, 0, &deslocamentoX, &deslocamentoY,
                      &larguraTitulo, &alturaTitulo);
  const int xDoTitulo = area.x + (area.largura - larguraTitulo) / 2 - deslocamentoX;
  const int yDoTitulo = area.y + 8 - deslocamentoY;
  escreverTexto(xDoTitulo, yDoTitulo, area.titulo, 1, estilo.titulo);
}

// Antena: cx/cy posicionam somente o desenho em relacao ao bloco.
// Aumentar +25 move para a direita; aumentar +29 move para baixo.
// noAr habilita as ondas; estilo fornece a cor da antena e do fundo.
void TelaPrincipalTft::desenharIconeTransmissao(const Area& area,
    bool noAr, const TemaTft::EstiloBloco& estilo) {
  const int cx = area.x + 25;
  const int cy = area.y + 29;
  const uint16_t cor = estilo.valor;

  // Ondas da antena; a mascara usa o fundo do estado atual.
  if (noAr) {
    tft_->drawCircle(cx, cy - 5, 10, cor);
    tft_->drawCircle(cx, cy - 5, 15, cor);
    tft_->fillRect(cx - 17, cy + 1, 34, 12, estilo.fundo);
  }

  // Torre desenhada depois da mascara, para permanecer inteira.
  tft_->fillCircle(cx, cy - 5, 3, cor);
  tft_->drawLine(cx, cy - 2, cx - 8, cy + 14, cor);
  tft_->drawLine(cx, cy - 2, cx + 8, cy + 14, cor);
  tft_->drawLine(cx - 8, cy + 14, cx + 8, cy + 14, cor);
}

// Procura pelo nome do bloco, sem depender da posicao da linha na tabela.
const TelaPrincipalTft::Area* TelaPrincipalTft::localizarArea(Bloco bloco) {
  for (const Area& area : AREAS) {
    if (area.bloco == bloco) return &area;
  }
  return nullptr;
}

// Desenha somente quando texto/cores mudam ou quando a tela e reaberta.
// Centraliza os valores na horizontal. A transmissao reserva espaco a esquerda
// para a antena; os outros blocos usam a distancia vertical da tabela AREAS.
void TelaPrincipalTft::desenharBloco(Bloco bloco, const char* valor,
    const TemaTft::EstiloBloco& estilo) {
  const unsigned indice = static_cast<unsigned>(bloco);
  const Area* encontrada = localizarArea(bloco);
  if (encontrada == nullptr) return;
  const Area& area = *encontrada;
  CacheBloco& cache = cache_[indice];
  const bool mesmoEstilo = cache.estilo.fundo == estilo.fundo
      && cache.estilo.borda == estilo.borda
      && cache.estilo.titulo == estilo.titulo
      && cache.estilo.valor == estilo.valor;
  if (!entrada_ && mesmoEstilo && strcmp(cache.texto, valor) == 0) return;

  desenharPainel(area, estilo);
  if (bloco == Bloco::Transmissao) {
    desenharIconeTransmissao(area, strcmp(valor, "NO AR") == 0, estilo);
    const int inicioTexto = area.x + 48;
    const int larguraDisponivel = area.largura - 54;
    if (NO_AR_SUAVIZADO && strcmp(valor, "NO AR") == 0
        && NoArSuave::LARGURA <= larguraDisponivel
        && NoArSuave::ALTURA <= area.altura - 4) {
      const int x = inicioTexto + (larguraDisponivel - NoArSuave::LARGURA) / 2;
      const int y = area.y + (area.altura - NoArSuave::ALTURA) / 2
          + TRANSMISSAO_AJUSTE_VERTICAL;
      NoArSuave::desenhar(*tft_, x, y, estilo.valor, estilo.fundo);
      snprintf(cache.texto, sizeof(cache.texto), "%s", valor);
      cache.estilo = estilo;
      return;
    }
    uint8_t tamanho = area.tamanhoTexto;
    int16_t deslocamentoX, deslocamentoY;
    uint16_t larguraTexto, alturaTexto;
    // Mede novamente a cada reducao; respeita a fonte selecionada no display.
    do {
      tft_->setTextSize(tamanho);
      tft_->getTextBounds(valor, 0, 0, &deslocamentoX, &deslocamentoY,
                          &larguraTexto, &alturaTexto);
      if (tamanho <= 1 || (larguraTexto <= larguraDisponivel
                          && alturaTexto <= area.altura - 4)) break;
      --tamanho;
    } while (true);
    const int x = inicioTexto + (larguraDisponivel - larguraTexto) / 2
        - deslocamentoX;
    const int y = area.y + (area.altura - alturaTexto) / 2 - deslocamentoY
        + TRANSMISSAO_AJUSTE_VERTICAL;
    escreverTexto(x, y, valor, tamanho, estilo.valor);
  } else {
    int16_t deslocamentoX, deslocamentoY;
    uint16_t larguraTexto, alturaTexto;
    tft_->setTextSize(area.tamanhoTexto);
    tft_->getTextBounds(valor, 0, 0, &deslocamentoX, &deslocamentoY,
                        &larguraTexto, &alturaTexto);
    const int x = area.x + (area.largura - larguraTexto) / 2 - deslocamentoX;
    // Mantem o ajuste manual do topo, compensando a origem da fonte.
    const int yDoValor = area.y + area.distanciaValorAoTopo - deslocamentoY;
    escreverTexto(x, yDoValor, valor, area.tamanhoTexto, estilo.valor);
  }
  snprintf(cache.texto, sizeof(cache.texto), "%s", valor);
  cache.estilo = estilo;
}

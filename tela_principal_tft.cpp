#include "tela_principal_tft.h"

#include "no_ar_suave_imagem.h"
#include "tx_off_suave_imagem.h"
#include "on_suave_imagem.h"
#include "off_suave_imagem.h"
#include "stereo_suave_imagem.h"
#include "mono_suave_imagem.h"
#include "mute_suave_imagem.h"

#include <stdio.h>
#include <string.h>


// ======================================================
// 1. AJUSTE RAPIDO: SUBIR / DESCER SOMENTE OS VALORES
// ======================================================
//
// Unidade: pixels, contados a partir do topo do respectivo bloco.
//
// Maior numero = desce.
// Menor numero = sobe.
//
// Titulo e fundo permanecem no lugar.
//
// Para textos normais, estes valores continuam funcionando
// como antes.
//
// Para textos suaves, a posicao base usa estes valores e ainda
// pode receber um ajuste individual na tabela VALORES_SUAVES.
//
constexpr int16_t FREQUENCIA_DISTANCIA_DO_TOPO = 27;
constexpr int16_t RDS_DISTANCIA_DO_TOPO = 23;
constexpr int16_t POTENCIA_DISTANCIA_DO_TOPO = 21;
constexpr int16_t RSSI_DISTANCIA_DO_TOPO = 21;
constexpr int16_t AUDIO_DISTANCIA_DO_TOPO = 23;
constexpr int16_t MODO_DISTANCIA_DO_TOPO = 23;
constexpr int16_t PRE_DESVIO_DISTANCIA_DO_TOPO = 21;


// Excecao da transmissao:
//
// O valor da transmissao e centralizado verticalmente.
// Este ajuste movimenta somente o texto.
//
//   0  = posicao central
//   +1 = desce 1 pixel
//   -1 = sobe 1 pixel
//
constexpr int16_t TRANSMISSAO_AJUSTE_VERTICAL = 2;


// ======================================================
// 2. LAYOUT: POSICAO, DIMENSOES, TITULO E FONTE
// ======================================================
//
// Cada linha:
//
// { bloco, x, y, largura, altura, titulo, fonte, distanciaValorAoTopo }
//
// Para valores suaves:
//
// - x/y/largura/altura continuam valendo normalmente;
// - distanciaValorAoTopo continua sendo a referencia vertical;
// - a tabela VALORES_SUAVES permite ainda ajuste fino X/Y.
//
const TelaPrincipalTft::Area TelaPrincipalTft::AREAS[] = {

    {Bloco::Transmissao,
     6, 6, 198, 50,
     "",
     4,
     0},

    {Bloco::Rds,
     210, 6, 104, 50,
     "RDS TX CONFIG.",
     2,
     RDS_DISTANCIA_DO_TOPO},

    {Bloco::Frequencia,
     6, 62, 198, 88,
     "FREQUENCIA FM / MHz",
     5,
     FREQUENCIA_DISTANCIA_DO_TOPO},

    {Bloco::Potencia,
     210, 62, 104, 42,
     "POTENCIA dBuV",
     2,
     POTENCIA_DISTANCIA_DO_TOPO},

    {Bloco::Rssi,
     210, 110, 104, 40,
     "RSSI RX",
     2,
     RSSI_DISTANCIA_DO_TOPO},

    {Bloco::Audio,
     6, 156, 98, 50,
     "AUDIO",
     2,
     AUDIO_DISTANCIA_DO_TOPO},

    {Bloco::Modo,
     110, 156, 94, 50,
     "MODO",
     2,
     MODO_DISTANCIA_DO_TOPO},

    {Bloco::PreEnfaseDesvio,
     210, 156, 104, 50,
     "PRE us/DEV kHz",
     2,
     PRE_DESVIO_DISTANCIA_DO_TOPO},
};


// ======================================================
// 3. FUNCOES INTERNAS PARA TEXTO SUAVE
// ======================================================
//
// Os arquivos gerados por gerar_texto_suave.py contem:
//
//   LARGURA
//   ALTURA
//   COBERTURA[]
//
// COBERTURA possui 4 bits por pixel:
//
//   0  = somente fundo
//   15 = somente texto
//
// Os niveis intermediarios sao usados para antialiasing.
//
// As cores NAO ficam gravadas na imagem.
// Elas sao recebidas do TemaTft em tempo de execucao.
//
// Isso permite usar a mesma mascara com outras cores.
//
namespace {


// ------------------------------------------------------
// Mistura duas cores RGB565
// ------------------------------------------------------
//
// alpha:
//
//   0  -> fundo
//   15 -> texto
//
uint16_t misturarRgb565(
    uint16_t corTexto,
    uint16_t corFundo,
    uint8_t alpha) {

  if (alpha == 0) {
    return corFundo;
  }

  if (alpha >= 15) {
    return corTexto;
  }


  // Componentes da cor do texto.
  const uint8_t textoR =
      corTexto >> 11;

  const uint8_t textoG =
      (corTexto >> 5) & 0x3F;

  const uint8_t textoB =
      corTexto & 0x1F;


  // Componentes da cor do fundo.
  const uint8_t fundoR =
      corFundo >> 11;

  const uint8_t fundoG =
      (corFundo >> 5) & 0x3F;

  const uint8_t fundoB =
      corFundo & 0x1F;


  // Mistura diretamente no espaco RGB565.
  const uint8_t r =
      (
          textoR * alpha
          + fundoR * (15 - alpha)
          + 7
      ) / 15;

  const uint8_t g =
      (
          textoG * alpha
          + fundoG * (15 - alpha)
          + 7
      ) / 15;

  const uint8_t b =
      (
          textoB * alpha
          + fundoB * (15 - alpha)
          + 7
      ) / 15;


  return static_cast<uint16_t>(
      (r << 11)
      | (g << 5)
      | b);
}


// ------------------------------------------------------
// Desenha uma mascara suave genérica
// ------------------------------------------------------
//
// Funciona com QUALQUER arquivo produzido por:
//
//   gerar_texto_suave.py
//
// Cada byte possui dois pixels:
//
//   bits 7..4 = primeiro pixel
//   bits 3..0 = segundo pixel
//
void desenharMascaraSuave(
    Adafruit_GFX& tft,
    int16_t x,
    int16_t y,
    const uint8_t* cobertura,
    uint16_t largura,
    uint16_t altura,
    uint16_t corTexto,
    uint16_t corFundo) {

  // Existem somente 16 niveis de cobertura.
  //
  // Calculamos as 16 cores uma unica vez antes de
  // desenhar os pixels.
  uint16_t paleta[16];

  for (uint8_t alpha = 0; alpha < 16; ++alpha) {

    paleta[alpha] =
        misturarRgb565(
            corTexto,
            corFundo,
            alpha);
  }


  uint32_t indicePixel = 0;


  // Permite que drivers que suportam transacao de escrita
  // façam o desenho de maneira mais eficiente.
  tft.startWrite();


  for (uint16_t py = 0; py < altura; ++py) {

    for (uint16_t px = 0; px < largura; ++px) {

      // Dois pixels por byte.
      const uint8_t dado =
          pgm_read_byte(
              cobertura + (indicePixel >> 1));


      const uint8_t alpha =
          (indicePixel & 1)
              ? (dado & 0x0F)
              : (dado >> 4);


      // Alpha zero representa exatamente o fundo.
      //
      // O painel ja foi preenchido antes, portanto nao
      // precisamos reescrever esses pixels.
      if (alpha != 0) {

        tft.writePixel(
            x + px,
            y + py,
            paleta[alpha]);
      }


      ++indicePixel;
    }
  }


  tft.endWrite();
}


} // namespace


// ======================================================
// 4. COMPOSICAO: QUAIS BLOCOS SAO DESENHADOS
// ======================================================

void TelaPrincipalTft::renderizar(
    Adafruit_GFX& tft,
    const EstadoPainel& estado,
    bool entrada,
    bool somenteCabecalho) {

  tft_ = &tft;
  entrada_ = entrada;

  tft_->setTextWrap(false);


  if (entrada_) {
    tft_->fillScreen(
        TemaTft::FUNDO_TELA);
  }


  desenharTransmissao(estado);
  desenharRds(estado);


  // A janela cobre a parte inferior.
  // Nao desenhar os demais blocos sobre ela.
  if (somenteCabecalho) {
    return;
  }


  desenharFrequencia(estado);
  desenharPotencia(estado);
  desenharRssi(estado);
  desenharAudio(estado);
  desenharModo(estado);
  desenharPreEnfaseDesvio(estado);
  desenharRodape(estado);
}


// ======================================================
// 5. CONTEUDO: O QUE CADA BLOCO MOSTRA
// ======================================================

void TelaPrincipalTft::desenharTransmissao(
    const EstadoPainel& estado) {

  if (!estado.sistema.si4713Disponivel) {

    desenharBloco(
        Bloco::Transmissao,
        "FALHA TX",
        TemaTft::TX_FALHA);

  } else if (estado.rf.transmitindo) {

    desenharBloco(
        Bloco::Transmissao,
        "NO AR",
        TemaTft::TX_NO_AR);

  } else if (estado.rf.transmissaoHabilitada) {

    desenharBloco(
        Bloco::Transmissao,
        "TX PAUSADO",
        TemaTft::TX_PAUSADO);

  } else {

    desenharBloco(
        Bloco::Transmissao,
        "TX OFF",
        TemaTft::TX_DESLIGADO);
  }
}


void TelaPrincipalTft::desenharRds(
    const EstadoPainel& estado) {

  if (estado.rds.habilitado) {

    desenharBloco(
        Bloco::Rds,
        "ON",
        TemaTft::RDS_LIGADO);

  } else {

    desenharBloco(
        Bloco::Rds,
        "OFF",
        TemaTft::RDS_DESLIGADO);
  }
}


void TelaPrincipalTft::desenharFrequencia(
    const EstadoPainel& estado) {

  // Nome legado:
  // frequenciaKhz armazena passos de 10 kHz.
  //
  // Exemplo:
  //
  //   9950 -> 99.50 MHz
  //
  const uint16_t frequenciaEm10Khz =
      estado.rf.frequenciaKhz;


  char valor[16];


  snprintf(
      valor,
      sizeof(valor),
      "%u.%02u",
      frequenciaEm10Khz / 100,
      frequenciaEm10Khz % 100);


  desenharBloco(
      Bloco::Frequencia,
      valor,
      TemaTft::FREQUENCIA);
}


void TelaPrincipalTft::desenharPotencia(
    const EstadoPainel& estado) {

  char valor[16];


  snprintf(
      valor,
      sizeof(valor),
      "%u",
      estado.rf.potenciaDbuv);


  desenharBloco(
      Bloco::Potencia,
      valor,
      TemaTft::POTENCIA);
}


void TelaPrincipalTft::desenharRssi(
    const EstadoPainel& estado) {

  if (
      !estado.receptor.disponivel
      || !estado.receptor.leituraRssiValida
  ) {

    desenharBloco(
        Bloco::Rssi,
        "---",
        TemaTft::RSSI_INDISPONIVEL);

    return;
  }


  char valor[8];


  snprintf(
      valor,
      sizeof(valor),
      "%u",
      estado.receptor.rssi);


  desenharBloco(
      Bloco::Rssi,
      valor,
      TemaTft::RSSI_VALIDO);
}


void TelaPrincipalTft::desenharAudio(
    const EstadoPainel& estado) {

  if (estado.audio.mudo) {

    desenharBloco(
        Bloco::Audio,
        "MUDO",
        TemaTft::AUDIO_MUDO);

  } else {

    desenharBloco(
        Bloco::Audio,
        "ON",
        TemaTft::AUDIO_LIGADO);
  }
}


void TelaPrincipalTft::desenharModo(
    const EstadoPainel& estado) {

  if (estado.audio.estereo) {

    desenharBloco(
        Bloco::Modo,
        "ST",
        TemaTft::MODO_ESTEREO);

  } else {

    desenharBloco(
        Bloco::Modo,
        "MONO",
        TemaTft::MODO_MONO);
  }
}


void TelaPrincipalTft::desenharPreEnfaseDesvio(
    const EstadoPainel& estado) {

  char valor[16];


  snprintf(
      valor,
      sizeof(valor),
      "%u / %u",
      estado.audio.preEnfaseUs,
      estado.audio.desvioKhz);


  desenharBloco(
      Bloco::PreEnfaseDesvio,
      valor,
      TemaTft::PRE_ENFASE_DESVIO);
}


// ======================================================
// 6. RODAPE RDS
// ======================================================
//
// Frase inteira, ate 32 caracteres.
// Centralizada em fonte 1.
//
void TelaPrincipalTft::desenharRodape(
    const EstadoPainel& estado) {

  char mensagem[40];


  snprintf(
      mensagem,
      sizeof(mensagem),
      "RDS: %.32s",
      estado.rds.textoAtual);


  // Remove espacos no final.
  size_t tamanho =
      strlen(mensagem);


  while (
      tamanho > 0
      && mensagem[tamanho - 1] == ' '
  ) {

    mensagem[--tamanho] = '\0';
  }


  if (tamanho <= 4) {

    snprintf(
        mensagem,
        sizeof(mensagem),
        "RDS: sem frase");
  }


  // Evita redesenho desnecessario.
  if (
      !entrada_
      && strcmp(
          textoRodapeAnterior_,
          mensagem) == 0
  ) {

    return;
  }


  tft_->fillRect(
      0,
      214,
      320,
      26,
      TemaTft::FUNDO_RODAPE);


  int16_t deslocamentoX;
  int16_t deslocamentoY;

  uint16_t largura;
  uint16_t altura;


  tft_->setTextSize(1);


  tft_->getTextBounds(
      mensagem,
      0,
      0,
      &deslocamentoX,
      &deslocamentoY,
      &largura,
      &altura);


  const int x =
      (320 - largura) / 2
      - deslocamentoX;


  const int y =
      214
      + (26 - altura) / 2
      - deslocamentoY;


  escreverTexto(
      x,
      y,
      mensagem,
      1,
      TemaTft::TEXTO_RODAPE);


  snprintf(
      textoRodapeAnterior_,
      sizeof(textoRodapeAnterior_),
      "%s",
      mensagem);
}


// ======================================================
// 7. TEXTO NORMAL
// ======================================================

void TelaPrincipalTft::escreverTexto(
    int16_t x,
    int16_t y,
    const char* texto,
    uint8_t tamanho,
    uint16_t cor) {

  tft_->setTextSize(tamanho);
  tft_->setTextColor(cor);
  tft_->setCursor(x, y);
  tft_->print(texto);
}


// ======================================================
// 8. PAINEL
// ======================================================

void TelaPrincipalTft::desenharPainel(
    const Area& area,
    const TemaTft::EstiloBloco& estilo) {

  // Fundo.
  tft_->fillRoundRect(
      area.x,
      area.y,
      area.largura,
      area.altura,
      6,
      estilo.fundo);


  // Borda.
  tft_->drawRoundRect(
      area.x,
      area.y,
      area.largura,
      area.altura,
      6,
      estilo.borda);


  // Bloco sem titulo.
  if (
      area.titulo == nullptr
      || area.titulo[0] == '\0'
  ) {

    return;
  }


  int16_t deslocamentoX;
  int16_t deslocamentoY;

  uint16_t larguraTitulo;
  uint16_t alturaTitulo;


  tft_->setTextSize(1);


  tft_->getTextBounds(
      area.titulo,
      0,
      0,
      &deslocamentoX,
      &deslocamentoY,
      &larguraTitulo,
      &alturaTitulo);


  const int xDoTitulo =
      area.x
      + (area.largura - larguraTitulo) / 2
      - deslocamentoX;


  const int yDoTitulo =
      area.y
      + 8
      - deslocamentoY;


  escreverTexto(
      xDoTitulo,
      yDoTitulo,
      area.titulo,
      1,
      estilo.titulo);
}


// ======================================================
// 9. ICONE DE TRANSMISSAO
// ======================================================

void TelaPrincipalTft::desenharIconeTransmissao(
    const Area& area,
    bool noAr,
    const TemaTft::EstiloBloco& estilo) {

  const int cx =
      area.x + 25;

  const int cy =
      area.y + 29;

  const uint16_t cor =
      estilo.valor;


  // Ondas de RF.
  if (noAr) {

    tft_->drawCircle(
        cx,
        cy - 5,
        10,
        cor);

    tft_->drawCircle(
        cx,
        cy - 5,
        15,
        cor);


    // Apaga a parte inferior das circunferencias.
    tft_->fillRect(
        cx - 17,
        cy + 1,
        34,
        12,
        estilo.fundo);
  }


  // Ponto superior da antena.
  tft_->fillCircle(
      cx,
      cy - 5,
      3,
      cor);


  // Torre.
  tft_->drawLine(
      cx,
      cy - 2,
      cx - 8,
      cy + 14,
      cor);

  tft_->drawLine(
      cx,
      cy - 2,
      cx + 8,
      cy + 14,
      cor);

  tft_->drawLine(
      cx - 8,
      cy + 14,
      cx + 8,
      cy + 14,
      cor);
}


// ======================================================
// 10. LOCALIZACAO DE AREAS
// ======================================================

const TelaPrincipalTft::Area*
TelaPrincipalTft::localizarArea(
    Bloco bloco) {

  for (const Area& area : AREAS) {

    if (area.bloco == bloco) {
      return &area;
    }
  }


  return nullptr;
}


// ======================================================
// 11. DESENHO DOS VALORES DOS BLOCOS
// ======================================================
//
// Aqui fica o cadastro dos textos suaves.
//
// Para adicionar um novo:
//
// 1. Gere:
//
//    ./gerar_texto_suave.py \
//        "OFF" \
//        "90,190,255" \
//        "8,12,8" \
//        off_suave_imagem
//
// 2. No topo:
//
//    #include "off_suave_imagem.h"
//
// 3. Acrescente UMA entrada em VALORES_SUAVES.
//
// Nao precisa alterar desenharRds(), desenharAudio(),
// desenharModo(), etc.
//
// O campo texto e a CHAVE usada para decidir quando
// aquela mascara sera utilizada.
//
// Portanto:
//
//   "ON"
//
// pode apontar para uma mascara que visualmente contenha:
//
//   "On"
//
// que e justamente o seu caso atual.
//
// ajusteX:
//
//   +1 = direita
//   -1 = esquerda
//
// ajusteY:
//
//   +1 = baixo
//   -1 = cima
//
void TelaPrincipalTft::desenharBloco(
    Bloco bloco,
    const char* valor,
    const TemaTft::EstiloBloco& estilo) {


  // ----------------------------------------------------
  // Descricao de um valor suave
  // ----------------------------------------------------

  struct ValorSuave {

    Bloco bloco;

    const char* texto;

    const uint8_t* cobertura;

    uint16_t largura;
    uint16_t altura;

    int16_t ajusteX;
    int16_t ajusteY;
  };


  // ----------------------------------------------------
  // CADASTRO DE TEXTOS SUAVES
  // ----------------------------------------------------
  //
  // ESTE E O LUGAR QUE VOCE EDITARA DAQUI PARA FRENTE.
  //
  static const ValorSuave VALORES_SUAVES[] = {

      // ================================================
      // TRANSMISSAO: NO AR
      // ================================================
      {
          Bloco::Transmissao,

          "NO AR",

          NoArSuave::COBERTURA,
          NoArSuave::LARGURA,
          NoArSuave::ALTURA,

          0,
          TRANSMISSAO_AJUSTE_VERTICAL
      },

       {
          Bloco::Transmissao,

          "TX OFF",

          TextoSuave_tx_off_suave_imagem::COBERTURA,
          TextoSuave_tx_off_suave_imagem::LARGURA,
          TextoSuave_tx_off_suave_imagem::ALTURA,

          0,
          TRANSMISSAO_AJUSTE_VERTICAL
      },


      // ================================================
      // RDS: ON
      //
      // A chave continua sendo "ON", pois e isso que
      // desenharRds() envia.
      //
      // A imagem pode mostrar visualmente "On".
      // ================================================
      {
          Bloco::Rds,

          "ON",

          TextoSuave_on_suave_imagem::COBERTURA,
          TextoSuave_on_suave_imagem::LARGURA,
          TextoSuave_on_suave_imagem::ALTURA,

          0,
          0
      },

      {
          Bloco::Rds,

          "OFF",

          TextoSuave_off_suave_imagem::COBERTURA,
          TextoSuave_off_suave_imagem::LARGURA,
          TextoSuave_off_suave_imagem::ALTURA,

          0,
          0
      },

       {
          Bloco::Modo,

          "ST",

          TextoSuave_stereo_suave_imagem::COBERTURA,
          TextoSuave_stereo_suave_imagem::LARGURA,
          TextoSuave_stereo_suave_imagem::ALTURA,

          0,
          0
      },

      {
          Bloco::Modo,

          "MONO",

          TextoSuave_mono_suave_imagem::COBERTURA,
          TextoSuave_mono_suave_imagem::LARGURA,
          TextoSuave_mono_suave_imagem::ALTURA,

          0,
          0
      },

       {
          Bloco::Audio,

          "ON",

          TextoSuave_on_suave_imagem::COBERTURA,
          TextoSuave_on_suave_imagem::LARGURA,
          TextoSuave_on_suave_imagem::ALTURA,

          0,
          0
      },

      {
          Bloco::Audio,

          "MUTE",

          TextoSuave_mute_suave_imagem::COBERTURA,
          TextoSuave_mute_suave_imagem::LARGURA,
          TextoSuave_mute_suave_imagem::ALTURA,

          0,
          0
      },

  };


  // Quantidade de itens cadastrados.
  constexpr size_t TOTAL_VALORES_SUAVES =
      sizeof(VALORES_SUAVES)
      / sizeof(VALORES_SUAVES[0]);


  // ----------------------------------------------------
  // LOCALIZA O BLOCO
  // ----------------------------------------------------

  const Area* encontrada =
      localizarArea(bloco);


  if (encontrada == nullptr) {
    return;
  }


  const Area& area =
      *encontrada;


  // ----------------------------------------------------
  // CACHE
  // ----------------------------------------------------

  const unsigned indice =
      static_cast<unsigned>(bloco);


  CacheBloco& cache =
      cache_[indice];


  const bool mesmoEstilo =
      cache.estilo.fundo == estilo.fundo
      &&
      cache.estilo.borda == estilo.borda
      &&
      cache.estilo.titulo == estilo.titulo
      &&
      cache.estilo.valor == estilo.valor;


  const bool mesmoValor =
      strcmp(
          cache.texto,
          valor) == 0;


  // Nada mudou.
  if (
      !entrada_
      &&
      mesmoEstilo
      &&
      mesmoValor
  ) {

    return;
  }


  // ----------------------------------------------------
  // Funcao local para atualizar cache
  // ----------------------------------------------------

  auto atualizarCache = [&]() {

    snprintf(
        cache.texto,
        sizeof(cache.texto),
        "%s",
        valor);

    cache.estilo =
        estilo;
  };


  // ----------------------------------------------------
  // PROCURA UMA IMAGEM SUAVE PARA O VALOR
  // ----------------------------------------------------

  const ValorSuave* suave =
      nullptr;


  for (
      size_t i = 0;
      i < TOTAL_VALORES_SUAVES;
      ++i
  ) {

    const ValorSuave& candidato =
        VALORES_SUAVES[i];


    if (
        candidato.bloco == bloco
        &&
        strcmp(
            candidato.texto,
            valor) == 0
    ) {

      suave =
          &candidato;

      break;
    }
  }


  // ----------------------------------------------------
  // REDESENHA FUNDO, BORDA E TITULO
  // ----------------------------------------------------

  desenharPainel(
      area,
      estilo);


  // ====================================================
  // TRANSMISSAO
  // ====================================================

  if (bloco == Bloco::Transmissao) {


    // --------------------------------------------------
    // ICONE
    // --------------------------------------------------

    const bool noAr =
        strcmp(
            valor,
            "NO AR") == 0;


    desenharIconeTransmissao(
        area,
        noAr,
        estilo);


    // --------------------------------------------------
    // AREA DISPONIVEL PARA O VALOR
    // --------------------------------------------------

    constexpr int16_t ESPACO_ANTENA =
        48;

    constexpr int16_t MARGEM_DIREITA =
        6;


    const int16_t inicioTexto =
        area.x
        + ESPACO_ANTENA;


    const int16_t larguraDisponivel =
        area.largura
        - ESPACO_ANTENA
        - MARGEM_DIREITA;


    // --------------------------------------------------
    // IMAGEM SUAVE
    // --------------------------------------------------

    if (suave != nullptr) {


      const bool cabe =
          suave->largura <= larguraDisponivel
          &&
          suave->altura <= area.altura - 4;


      if (cabe) {


        const int16_t x =
            inicioTexto
            +
            (
                larguraDisponivel
                - suave->largura
            ) / 2
            +
            suave->ajusteX;


        const int16_t y =
            area.y
            +
            (
                area.altura
                - suave->altura
            ) / 2
            +
            suave->ajusteY;


        desenharMascaraSuave(
            *tft_,
            x,
            y,
            suave->cobertura,
            suave->largura,
            suave->altura,
            estilo.valor,
            estilo.fundo);


        atualizarCache();

        return;
      }
    }


    // --------------------------------------------------
    // FALLBACK: FONTE NORMAL
    // --------------------------------------------------
    //
    // Se nao existir mascara suave ou ela nao couber,
    // continua usando a fonte normal.
    //

    uint8_t tamanho =
        area.tamanhoTexto;


    int16_t deslocamentoX = 0;
    int16_t deslocamentoY = 0;

    uint16_t larguraTexto = 0;
    uint16_t alturaTexto = 0;


    while (true) {


      tft_->setTextSize(
          tamanho);


      tft_->getTextBounds(
          valor,
          0,
          0,
          &deslocamentoX,
          &deslocamentoY,
          &larguraTexto,
          &alturaTexto);


      const bool cabe =
          larguraTexto <= larguraDisponivel
          &&
          alturaTexto <= area.altura - 4;


      if (
          cabe
          ||
          tamanho <= 1
      ) {

        break;
      }


      --tamanho;
    }


    const int16_t x =
        inicioTexto
        +
        (
            larguraDisponivel
            - larguraTexto
        ) / 2
        -
        deslocamentoX;


    const int16_t y =
        area.y
        +
        (
            area.altura
            - alturaTexto
        ) / 2
        -
        deslocamentoY
        +
        TRANSMISSAO_AJUSTE_VERTICAL;


    escreverTexto(
        x,
        y,
        valor,
        tamanho,
        estilo.valor);


    atualizarCache();

    return;
  }


  // ====================================================
  // DEMAIS BLOCOS
  // ====================================================


  // ----------------------------------------------------
  // TENTA PRIMEIRO A VERSAO SUAVE
  // ----------------------------------------------------

  if (suave != nullptr) {


    constexpr int16_t MARGEM_HORIZONTAL =
        2;

    constexpr int16_t MARGEM_INFERIOR =
        2;

    // Evita que o valor invada a legenda do painel.
    constexpr int16_t TOPO_MINIMO_VALOR =
        17;


    const int16_t larguraDisponivel =
        area.largura
        - MARGEM_HORIZONTAL * 2;


    const int16_t alturaDisponivel =
        area.altura
        - TOPO_MINIMO_VALOR
        - MARGEM_INFERIOR;


    const bool cabe =
        suave->largura <= larguraDisponivel
        &&
        suave->altura <= alturaDisponivel;


    if (cabe) {


      // ----------------------------------------------
      // CENTRALIZACAO HORIZONTAL
      // ----------------------------------------------

      const int16_t x =
          area.x
          +
          (
              area.largura
              - suave->largura
          ) / 2
          +
          suave->ajusteX;


      // ----------------------------------------------
      // POSICAO VERTICAL
      // ----------------------------------------------
      //
      // Comeca pela mesma distancia configurada em AREAS
      // para o texto tradicional.
      //
      int16_t y =
          area.y
          +
          area.distanciaValorAoTopo
          +
          suave->ajusteY;


      // Limite superior:
      //
      // impede sobreposicao com o titulo.
      //
      const int16_t yMinimo =
          area.y
          +
          TOPO_MINIMO_VALOR;


      // Limite inferior:
      //
      // impede que a imagem saia do painel.
      //
      const int16_t yMaximo =
          area.y
          +
          area.altura
          -
          suave->altura
          -
          MARGEM_INFERIOR;


      if (y < yMinimo) {
        y = yMinimo;
      }


      if (y > yMaximo) {
        y = yMaximo;
      }


      // ----------------------------------------------
      // DESENHA
      // ----------------------------------------------

      desenharMascaraSuave(
          *tft_,
          x,
          y,
          suave->cobertura,
          suave->largura,
          suave->altura,
          estilo.valor,
          estilo.fundo);


      atualizarCache();

      return;
    }
  }


  // ----------------------------------------------------
  // FALLBACK: TEXTO NORMAL
  // ----------------------------------------------------
  //
  // Frequencia, potencia, RSSI e qualquer texto ainda
  // nao convertido continuam passando por aqui.
  //

  int16_t deslocamentoX = 0;
  int16_t deslocamentoY = 0;

  uint16_t larguraTexto = 0;
  uint16_t alturaTexto = 0;


  tft_->setTextSize(
      area.tamanhoTexto);


  tft_->getTextBounds(
      valor,
      0,
      0,
      &deslocamentoX,
      &deslocamentoY,
      &larguraTexto,
      &alturaTexto);


  const int16_t x =
      area.x
      +
      (
          area.largura
          - larguraTexto
      ) / 2
      -
      deslocamentoX;


  const int16_t y =
      area.y
      +
      area.distanciaValorAoTopo
      -
      deslocamentoY;


  escreverTexto(
      x,
      y,
      valor,
      area.tamanhoTexto,
      estilo.valor);


  atualizarCache();
}
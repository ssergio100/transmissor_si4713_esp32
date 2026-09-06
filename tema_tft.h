#pragma once
#include <stdint.h>

namespace TemaTft {

// ======================================================
// 1. PALETA: NOME E CODIGO DE CADA COR (RGB565)
// ======================================================
// Alterar um codigo muda todos os lugares que usam aquele nome.
// Para mudar apenas um bloco, troque o nome da cor na secao 3.

constexpr uint16_t RGB(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) |
           ((g & 0xFC) << 3) |
           (b >> 3);
}

constexpr uint16_t PRETO            = RGB(0, 0, 0);
constexpr uint16_t PETROLEO_ESCURO  = RGB(8, 12, 8);
constexpr uint16_t AZUL_ARDOSIA     = RGB(16, 24, 24);
constexpr uint16_t BRANCO           = RGB(239, 239, 239);
constexpr uint16_t CINZA            = RGB(156, 158, 156);
constexpr uint16_t LARANJA          = RGB(255, 166, 24);
constexpr uint16_t VERDE            = RGB(74, 215, 123);
constexpr uint16_t AMARELO          = RGB(255, 207, 66);
constexpr uint16_t VERMELHO         = RGB(255, 89, 82);
constexpr uint16_t AZUL             = RGB(90, 190, 255);
constexpr uint16_t ROSA             = RGB(255, 40, 160);

struct EstiloBloco {
  uint16_t fundo;
  uint16_t borda;
  uint16_t titulo;
  uint16_t valor;
};

// ======================================================
// 2. FUNDO GERAL E RODAPE
// ======================================================
// FUNDO_TELA colore os espacos entre blocos; cada bloco tem seu proprio fundo.
constexpr uint16_t FUNDO_TELA = PETROLEO_ESCURO;
constexpr uint16_t FUNDO_RODAPE = PETROLEO_ESCURO;
constexpr uint16_t TEXTO_RODAPE = CINZA;
constexpr uint16_t ALERTA_RODAPE = AMARELO;

// ======================================================
// 3. CORES POR BLOCO E ESTADO
// ======================================================
// Cada linha: { fundo do retangulo, borda, texto do titulo, texto do valor }.
// Exemplo: TX_NO_AR = { ROSA, VERDE, BRANCO, BRANCO } significa:
//   fundo rosa, contorno verde e textos brancos.
// Para mudar SO o fundo de NO AR, troque apenas a primeira cor dessa linha.
// A antena usa a cor do valor. A transmissao nao exibe titulo.
// Ligue/desligue RDS ou TX para conferir as respectivas cores na tela.
//
//                        Fundo          Borda     Titulo  Valor
constexpr EstiloBloco
    TX_NO_AR          = { ROSA         , ROSA   , BRANCO, BRANCO   },
    TX_DESLIGADO      = { PRETO , CINZA   , CINZA , CINZA    },
    TX_PAUSADO        = { AZUL_ARDOSIA , CINZA   , CINZA , CINZA    },
    TX_FALHA          = { AZUL_ARDOSIA , VERMELHO, CINZA , VERMELHO },
    RDS_LIGADO        = { AZUL_ARDOSIA , AZUL    , CINZA , AZUL     },
    RDS_DESLIGADO     = { AZUL_ARDOSIA , CINZA   , CINZA , CINZA    },
    FREQUENCIA        = { AZUL_ARDOSIA , LARANJA , CINZA , LARANJA  },
    POTENCIA          = { AZUL_ARDOSIA , BRANCO  , CINZA , BRANCO   },
    RSSI_VALIDO       = { AZUL_ARDOSIA , AZUL    , CINZA , AZUL     },
    RSSI_INDISPONIVEL = { AZUL_ARDOSIA , CINZA   , CINZA , CINZA    },
    AUDIO_LIGADO      = { AZUL_ARDOSIA , VERDE   , CINZA , VERDE    },
    AUDIO_MUDO        = { AZUL_ARDOSIA , AMARELO , CINZA , AMARELO  },
    MODO_ESTEREO      = { AZUL_ARDOSIA , BRANCO  , CINZA , BRANCO   },
    MODO_MONO         = { AZUL_ARDOSIA , BRANCO  , CINZA , BRANCO   },
    PRE_ENFASE_DESVIO = { AZUL_ARDOSIA , CINZA   , CINZA , CINZA    };

}  // namespace TemaTft

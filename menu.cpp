#include "menu.h"

Menu::Acao Menu::selecionar(ConfiguracaoTransmissor& configuracao) {
  if (tela_ == Tela::PRINCIPAL) {
    entrar(Tela::RAIZ);
    return Acao::NENHUMA;
  }

  if (editando_) {
    if (tela_ == Tela::RDS && (item_ == RDS_PS || item_ == RDS_TEXTO)) {
      const uint8_t ultimo = item_ == RDS_PS ? 7 : 31;
      if (cursorTexto_ < ultimo) {
        cursorTexto_++;
        return Acao::NENHUMA;
      }
    }
    editando_ = false;
    if (tela_ == Tela::RF && item_ == RF_FREQUENCIA) {
      return Acao::APLICAR_FREQUENCIA;
    }
    if (tela_ == Tela::SISTEMA && item_ == SISTEMA_RSSI_NO_AR) {
      return Acao::ATUALIZAR_LIMIAR_RSSI;
    }
    return Acao::APLICAR_CONFIGURACAO;
  }

  switch (tela_) {
    case Tela::RAIZ: return selecionarRaiz();
    case Tela::RF: return selecionarRf(configuracao);
    case Tela::AUDIO: return selecionarAudio(configuracao);
    case Tela::RDS: return selecionarRds(configuracao);
    case Tela::MONITOR: entrar(Tela::RAIZ); return Acao::NENHUMA;
    case Tela::VARREDURA: return selecionarVarredura();
    case Tela::SISTEMA: return selecionarSistema();
    default: return Acao::NENHUMA;
  }
}

Menu::Acao Menu::voltar() {
  if (editando_) {
    editando_ = false;
    if (tela_ == Tela::RF && item_ == RF_FREQUENCIA) {
      return Acao::APLICAR_FREQUENCIA;
    }
    if (tela_ == Tela::SISTEMA && item_ == SISTEMA_RSSI_NO_AR) {
      return Acao::ATUALIZAR_LIMIAR_RSSI;
    }
    return Acao::APLICAR_CONFIGURACAO;
  }
  if (tela_ == Tela::PRINCIPAL) entrar(Tela::RAIZ);
  else if (tela_ == Tela::RAIZ) entrar(Tela::PRINCIPAL);
  else entrar(Tela::RAIZ);
  return Acao::NENHUMA;
}

Menu::Acao Menu::girar(
    int8_t deslocamento,
    ConfiguracaoTransmissor& configuracao
) {
  if (deslocamento == 0 || tela_ == Tela::PRINCIPAL || tela_ == Tela::MONITOR) {
    return Acao::NENHUMA;
  }

  if (!editando_) {
    int16_t proximo = static_cast<int16_t>(item_) + deslocamento;
    const uint8_t quantidade = quantidadeItens();
    while (proximo < 0) proximo += quantidade;
    while (proximo >= quantidade) proximo -= quantidade;
    item_ = static_cast<uint8_t>(proximo);
    return Acao::NENHUMA;
  }

  if (tela_ == Tela::RF) {
    if (item_ == RF_FREQUENCIA) {
      const int32_t valor = static_cast<int32_t>(configuracao.frequenciaKhz)
          + static_cast<int32_t>(deslocamento)
              * Configuracao::PASSO_FREQUENCIA_KHZ;
      configuracao.frequenciaKhz = constrain(
          valor,
          Configuracao::FREQUENCIA_MINIMA_KHZ,
          Configuracao::FREQUENCIA_MAXIMA_KHZ
      );
      return Acao::PREVISUALIZAR_FREQUENCIA;
    }
    if (item_ == RF_POTENCIA) {
      configuracao.potenciaDbuv = constrain(
          static_cast<int16_t>(configuracao.potenciaDbuv) + deslocamento,
          Configuracao::POTENCIA_MINIMA_DBUV,
          Configuracao::POTENCIA_MAXIMA_DBUV
      );
      return Acao::APLICAR_CONFIGURACAO;
    }
    if (item_ == RF_ANTENA) {
      configuracao.capacitanciaAntena = constrain(
          static_cast<int16_t>(configuracao.capacitanciaAntena) + deslocamento,
          0,
          Configuracao::CAPACITANCIA_ANTENA_MAXIMA
      );
      return Acao::APLICAR_CONFIGURACAO;
    }
  }

  if (tela_ == Tela::AUDIO && item_ == AUDIO_DESVIO) {
    configuracao.desvioAudioKhz = constrain(
        static_cast<int16_t>(configuracao.desvioAudioKhz) + deslocamento,
        50,
        66
    );
    return Acao::APLICAR_CONFIGURACAO;
  }

  if (tela_ == Tela::RDS) {
    if (item_ == RDS_PS) {
      configuracao.rdsPs[cursorTexto_] = girarCaractere(
          configuracao.rdsPs[cursorTexto_],
          deslocamento
      );
    } else if (item_ == RDS_TEXTO) {
      configuracao.rdsText[cursorTexto_] = girarCaractere(
          configuracao.rdsText[cursorTexto_],
          deslocamento
      );
    } else if (item_ == RDS_PI) {
      configuracao.rdsPi = static_cast<uint16_t>(
          configuracao.rdsPi + deslocamento
      );
      return Acao::APLICAR_CONFIGURACAO;
    }
  }

  if (tela_ == Tela::SISTEMA && item_ == SISTEMA_RSSI_NO_AR) {
    configuracao.rssiMinimoNoAr = constrain(
        static_cast<int16_t>(configuracao.rssiMinimoNoAr) + deslocamento,
        Configuracao::RSSI_NO_AR_MINIMO,
        Configuracao::RSSI_NO_AR_MAXIMO
    );
    return Acao::ATUALIZAR_LIMIAR_RSSI;
  }

  return Acao::NENHUMA;
}

Menu::Tela Menu::tela() const { return tela_; }
uint8_t Menu::itemSelecionado() const { return item_; }
bool Menu::editando() const { return editando_; }
uint8_t Menu::cursorTexto() const { return cursorTexto_; }

Menu::Acao Menu::selecionarRaiz() {
  switch (item_) {
    case RAIZ_RF: entrar(Tela::RF); break;
    case RAIZ_AUDIO: entrar(Tela::AUDIO); break;
    case RAIZ_RDS: entrar(Tela::RDS); break;
    case RAIZ_MONITOR: entrar(Tela::MONITOR); break;
    case RAIZ_VARREDURA: entrar(Tela::VARREDURA); break;
    case RAIZ_SISTEMA: entrar(Tela::SISTEMA); break;
    default: entrar(Tela::PRINCIPAL); break;
  }
  return Acao::NENHUMA;
}

Menu::Acao Menu::selecionarRf(ConfiguracaoTransmissor& configuracao) {
  if (item_ == RF_FREQUENCIA) {
    editando_ = true;
    return Acao::INICIAR_AJUSTE_FREQUENCIA;
  }
  if (item_ == RF_POTENCIA || item_ == RF_ANTENA) {
    editando_ = true;
  } else if (item_ == RF_TRANSMISSAO) {
    configuracao.transmissaoHabilitada = !configuracao.transmissaoHabilitada;
    return Acao::APLICAR_CONFIGURACAO;
  } else {
    entrar(Tela::RAIZ);
  }
  return Acao::NENHUMA;
}

Menu::Acao Menu::selecionarAudio(ConfiguracaoTransmissor& configuracao) {
  if (item_ == AUDIO_ESTEREO) {
    configuracao.estereo = !configuracao.estereo;
    return Acao::APLICAR_CONFIGURACAO;
  }
  if (item_ == AUDIO_PRE_ENFASE) {
    configuracao.preEnfaseUs = configuracao.preEnfaseUs == 50 ? 75 : 50;
    return Acao::APLICAR_CONFIGURACAO;
  }
  if (item_ == AUDIO_DESVIO) {
    editando_ = true;
  } else if (item_ == AUDIO_MUDO) {
    configuracao.audioMudo = !configuracao.audioMudo;
    return Acao::APLICAR_CONFIGURACAO;
  } else if (item_ == AUDIO_VOLTAR) {
    entrar(Tela::RAIZ);
  }
  return Acao::NENHUMA;
}

Menu::Acao Menu::selecionarRds(ConfiguracaoTransmissor& configuracao) {
  if (item_ == RDS_HABILITADO) {
    configuracao.rdsHabilitado = !configuracao.rdsHabilitado;
    return Acao::APLICAR_CONFIGURACAO;
  }
  if (item_ == RDS_PS || item_ == RDS_TEXTO) {
    configuracao.sanitizarTextos();
    cursorTexto_ = 0;
    editando_ = true;
  } else if (item_ == RDS_PI) {
    editando_ = true;
  } else if (item_ == RDS_VOLTAR) {
    entrar(Tela::RAIZ);
  }
  return Acao::NENHUMA;
}

Menu::Acao Menu::selecionarVarredura() {
  if (item_ == VARREDURA_INICIAR) return Acao::INICIAR_VARREDURA;
  if (item_ == VARREDURA_USAR_MELHOR) return Acao::USAR_MELHOR_FREQUENCIA;
  entrar(Tela::RAIZ);
  return Acao::NENHUMA;
}

Menu::Acao Menu::selecionarSistema() {
  if (item_ == SISTEMA_SALVAR) return Acao::SALVAR_CONFIGURACAO;
  if (item_ == SISTEMA_PADROES) return Acao::RESTAURAR_PADROES;
  if (item_ == SISTEMA_WIFI) return Acao::CONFIGURAR_WIFI;
  if (item_ == SISTEMA_RSSI_NO_AR) editando_ = true;
  if (item_ == SISTEMA_VOLTAR) entrar(Tela::RAIZ);
  return Acao::NENHUMA;
}

void Menu::entrar(Tela tela) {
  tela_ = tela;
  item_ = 0;
  cursorTexto_ = 0;
  editando_ = false;
}

uint8_t Menu::quantidadeItens() const {
  switch (tela_) {
    case Tela::RAIZ: return QUANTIDADE_RAIZ;
    case Tela::RF: return QUANTIDADE_RF;
    case Tela::AUDIO: return QUANTIDADE_AUDIO;
    case Tela::RDS: return QUANTIDADE_RDS;
    case Tela::VARREDURA: return QUANTIDADE_VARREDURA;
    case Tela::SISTEMA: return QUANTIDADE_SISTEMA;
    default: return 1;
  }
}

char Menu::girarCaractere(char atual, int8_t deslocamento) {
  static const char caracteres[] =
      " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-{}:/";
  const int16_t quantidade = sizeof(caracteres) - 1;
  int16_t posicao = 0;
  while (posicao < quantidade && caracteres[posicao] != atual) posicao++;
  if (posicao >= quantidade) posicao = 0;
  posicao += deslocamento;
  while (posicao < 0) posicao += quantidade;
  while (posicao >= quantidade) posicao -= quantidade;
  return caracteres[posicao];
}

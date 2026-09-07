#include "display_tft.h"
#include "configuracao.h"
#include "tema_tft.h"
#include <SPI.h>

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
  tft_.fillScreen(TemaTft::FUNDO_TELA);
  pronto_ = janela_.pronta();
  painelValido_ = false;
  return pronto_;
}

void DisplayTft::mostrarInicializacao() {
  if (!pronto_) return;
  tft_.fillScreen(TemaTft::FUNDO_TELA);
  tft_.setTextColor(TemaTft::BRANCO);
  tft_.setTextSize(2);
  tft_.setCursor(64, 110);
  tft_.print("Inicializando...");
  painelValido_ = false;
}

void DisplayTft::renderizar(const EstadoPainel& estado) {
  if (!pronto_ || emRepouso_) return;
  const bool abrirJanela = estado.navegacao.tela != TelaPainel::PRINCIPAL
      || estado.navegacao.erro[0];
  const bool redesenharPainel = !painelValido_ || (janelaAberta_ && !abrirJanela);
  // Ao fechar, restaura inclusive os blocos cujo valor nao mudou sob a janela.
  principal_.renderizar(tft_, estado, redesenharPainel,
                        abrirJanela && !redesenharPainel);
  if (abrirJanela && janela_.preparar(estado, !janelaAberta_ || redesenharPainel)) {
    const auto& imagem = janela_.imagem();
    // Chame no driver concreto: a versao Adafruit_GFX envia pixel por pixel.
    tft_.drawRGBBitmap(JanelaMenuTft::X, JanelaMenuTft::Y, imagem.getBuffer(),
                       imagem.width(), imagem.height());
  }
  painelValido_ = true;
  janelaAberta_ = abrirJanela;
}

void DisplayTft::entrarRepouso() {
  if (!pronto_ || emRepouso_) return;
  // Display Off insere uma tela vazia; Sleep In para a varredura do painel.
  tft_.enableDisplay(false);
  tft_.enableSleep(true);
  emRepouso_ = true;
}

void DisplayTft::sairRepouso() {
  if (!pronto_ || !emRepouso_) return;
  tft_.enableSleep(false);
  // O ST7789 exige aguardar a estabilizacao depois de Sleep Out.
  delay(120);
  tft_.enableDisplay(true);
  emRepouso_ = false;
  painelValido_ = false;
  janelaAberta_ = false;
}

bool DisplayTft::emRepouso() const { return emRepouso_; }

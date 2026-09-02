#pragma once

#include <Arduino.h>

namespace Configuracao {

constexpr char VERSAO_FIRMWARE[] = "0.1.23-rda-rds";
constexpr char NOME_HOST[] = "transmissor-si4713";
constexpr char NOME_PORTAL_WIFI[] = "TRANSMISSOR-SI4713";

constexpr int PIN_I2C_SDA = 17;
constexpr int PIN_I2C_SCL = 18;
constexpr uint32_t FREQUENCIA_I2C_HZ = 100000;

// Display GMT020-02M(7P), controlador ST7789 e barramento SPI somente escrita.
// No conector do modulo, SDA significa MOSI e SCL significa clock SPI; nao se
// trata de um segundo barramento I2C. GPIO13 ocupa aqui a posicao MISO padrao
// do perfil Arduino, que este display de sete pinos nao utiliza.
constexpr int PIN_TFT_CS = 10;
constexpr int PIN_TFT_MOSI = 11;
constexpr int PIN_TFT_SCLK = 12;
constexpr int PIN_TFT_MISO = -1;
constexpr int PIN_TFT_DC = 13;
constexpr int PIN_TFT_RESET = 14;
constexpr uint16_t TFT_LARGURA = 240;
constexpr uint16_t TFT_ALTURA = 320;
// Orientacao confirmada na bancada para o conector ficar na posicao de montagem
// escolhida: retrato girado em 180 graus em relacao ao padrao do controlador.
constexpr uint8_t TFT_ROTACAO = 2;

constexpr int PIN_ENCODER_DT = 16;
constexpr int PIN_ENCODER_CLK = 15;
constexpr int PIN_ENCODER_BOTAO = 7;
constexpr int PIN_ENCODER_VCC = -1;
// Este e o mesmo encoder principal do radio_web_1. No componente instalado,
// quatro transicoes eletricas correspondem a um unico detente mecanico.
constexpr uint8_t TRANSICOES_ENCODER_POR_DETENTE = 4;
constexpr bool LOG_EVENTOS_ENCODER = true;
constexpr uint16_t TEMPO_DEBOUNCE_BOTAO_MS = 10;
// O exemplo nao bloqueante da biblioteca anteriormente usada recomenda 50 ms
// como duracao minima para rejeitar falsos cliques por contato mecanico.
constexpr uint16_t TEMPO_PRESSIONAMENTO_CURTO_MINIMO_MS = 50;

constexpr int PIN_RESET_SI4713 = 5;
// GP2/INT do Si4713. Mantido como INPUT sem pull durante o reset, pois esse
// sinal tambem participa da selecao do modo de barramento do CI.
constexpr int PIN_INTERRUPCAO_SI4713 = 4;

constexpr uint8_t LCD_ENDERECO = 0x27;
constexpr uint8_t LCD_COLUNAS = 20;
constexpr uint8_t LCD_LINHAS = 4;

constexpr uint32_t TEMPO_PRESSIONAMENTO_LONGO_MS = 700;
constexpr uint32_t INTERVALO_ATUALIZACAO_DISPLAY_MS = 250;
// Uma leitura sequencial traz RSSI e os blocos RDS no mesmo acesso. A cadencia
// permanece igual a antiga leitura isolada de RSSI para nao criar mais rajadas
// periodicas no barramento compartilhado com o caminho de audio.
constexpr uint32_t INTERVALO_LEITURA_RDA_MS = 250;
// Confirmacoes ficam visiveis por um segundo completo, mas qualquer novo
// evento do encoder as encerra imediatamente para manter o painel responsivo.
constexpr uint32_t TEMPO_MENSAGEM_DISPLAY_MS = 1000;
constexpr uint32_t INTERVALO_LEITURA_AUDIO_MS = 250;
// A leitura de audio (ASQ) na I2C e feita apenas quando a interface solicita
// explicitamente. Se nenhum cliente WebSocket sinalizar interesse dentro deste
// tempo, o ESP desativa sozinho o polling de ASQ (nunca e autonomo).
constexpr uint32_t TEMPO_AUTO_DESATIVAR_AUDIO_MS = 5000;
// Uma unica tentativa I2C (timeout maximo de 25 ms) a cada 5 s limita o tempo
// bloqueado pela ausencia do CI a no maximo 0,5% e prioriza os controles.
constexpr uint32_t INTERVALO_RECUPERACAO_SI4713_MS = 5000;

constexpr uint16_t FREQUENCIA_MINIMA_KHZ = 7610;
constexpr uint16_t FREQUENCIA_MAXIMA_KHZ = 10800;
constexpr uint16_t PASSO_FREQUENCIA_KHZ = 10;
constexpr uint8_t POTENCIA_MINIMA_DBUV = 88;
constexpr uint8_t POTENCIA_MAXIMA_DBUV = 118;
constexpr uint8_t CAPACITANCIA_ANTENA_MAXIMA = 191;
constexpr uint8_t RSSI_NO_AR_MINIMO = 0;
constexpr uint8_t RSSI_NO_AR_MAXIMO = 127;
// Ponto inicial usado pelos exemplos da biblioteca PU2CLR. Deve ser calibrado
// no conjunto real pela tela Sistema; nao representa dBm.
constexpr uint8_t RSSI_NO_AR_PADRAO = 50;

constexpr long FUSO_HORARIO_SEGUNDOS = -3L * 60L * 60L;
constexpr int AJUSTE_HORARIO_VERAO_SEGUNDOS = 0;
constexpr char SERVIDOR_NTP_PRIMARIO[] = "pool.ntp.org";
constexpr char SERVIDOR_NTP_SECUNDARIO[] = "time.nist.gov";

}  // namespace Configuracao

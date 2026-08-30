#pragma once

#include <Arduino.h>

namespace Configuracao {

constexpr char VERSAO_FIRMWARE[] = "0.1.9";
constexpr char NOME_HOST[] = "transmissor-si4713";
constexpr char NOME_PORTAL_WIFI[] = "TRANSMISSOR-SI4713";

constexpr int PIN_I2C_SDA = 17;
constexpr int PIN_I2C_SCL = 18;
constexpr uint32_t FREQUENCIA_I2C_HZ = 100000;

constexpr int PIN_ENCODER_DT = 16;
constexpr int PIN_ENCODER_CLK = 15;
constexpr int PIN_ENCODER_BOTAO = 7;
constexpr int PIN_ENCODER_VCC = -1;
constexpr uint8_t TRANSICOES_ENCODER_POR_DETENTE = 2;
constexpr bool LOG_EVENTOS_ENCODER = true;

constexpr int PIN_RESET_SI4713 = 5;

constexpr uint8_t LCD_ENDERECO = 0x27;
constexpr uint8_t LCD_COLUNAS = 20;
constexpr uint8_t LCD_LINHAS = 4;

constexpr uint32_t TEMPO_PRESSIONAMENTO_LONGO_MS = 700;
constexpr uint32_t INTERVALO_ATUALIZACAO_DISPLAY_MS = 250;
constexpr uint32_t INTERVALO_LEITURA_AUDIO_MS = 250;
constexpr uint32_t INTERVALO_STATUS_SI4713_MS = 2000;
// Uma unica tentativa I2C (timeout maximo de 25 ms) a cada 5 s limita o tempo
// bloqueado pela ausencia do CI a no maximo 0,5% e prioriza os controles.
constexpr uint32_t INTERVALO_RECUPERACAO_SI4713_MS = 5000;

constexpr uint16_t FREQUENCIA_MINIMA_KHZ = 8750;
constexpr uint16_t FREQUENCIA_MAXIMA_KHZ = 10800;
constexpr uint16_t PASSO_FREQUENCIA_KHZ = 10;
constexpr uint8_t POTENCIA_MINIMA_DBUV = 88;
constexpr uint8_t POTENCIA_MAXIMA_DBUV = 115;
constexpr uint8_t CAPACITANCIA_ANTENA_MAXIMA = 191;

constexpr long FUSO_HORARIO_SEGUNDOS = -3L * 60L * 60L;
constexpr int AJUSTE_HORARIO_VERAO_SEGUNDOS = 0;
constexpr char SERVIDOR_NTP_PRIMARIO[] = "pool.ntp.org";
constexpr char SERVIDOR_NTP_SECUNDARIO[] = "time.nist.gov";

}  // namespace Configuracao

#include "diagnostico_i2c.h"

#include <Wire.h>
#include <string.h>

#include "configuracao.h"
#include "si4713_seguro.h"

namespace {

constexpr uint8_t ENDERECO_INICIAL_SCAN = 0x08;
constexpr uint8_t ENDERECO_FINAL_SCAN = 0x77;

void imprimirResultadoConhecido(
    const char* nome,
    uint8_t endereco,
    uint8_t resultado
) {
  Serial.printf(
      "[I2C-DIAG] %-9s 0x%02X: %s (codigo=%u)\n",
      nome,
      endereco,
      DiagnosticoI2c::descreverResultado(resultado),
      resultado
  );
}

}  // namespace

uint8_t DiagnosticoI2c::consultarEndereco(uint8_t endereco) {
  Wire.beginTransmission(endereco);
  return Wire.endTransmission();
}

const char* DiagnosticoI2c::descreverResultado(uint8_t resultado) {
  switch (resultado) {
    case 0: return "ACK/dispositivo presente";
    case 1: return "buffer de transmissao excedido";
    case 2: return "NACK no endereco";
    case 3: return "NACK nos dados";
    case 4: return "outro erro I2C";
    case 5: return "timeout/barramento bloqueado";
    default: return "resultado desconhecido";
  }
}

void DiagnosticoI2c::executarNoBoot() {
  Serial.println("[I2C-DIAG] --- inicio ---");
  const int nivelSda = digitalRead(Configuracao::PIN_I2C_SDA);
  const int nivelScl = digitalRead(Configuracao::PIN_I2C_SCL);
  Serial.printf(
      "[I2C-DIAG] SDA GPIO%d=%s; SCL GPIO%d=%s; clock=%luHz; timeout=%ums\n",
      Configuracao::PIN_I2C_SDA,
      nivelSda == HIGH ? "HIGH" : "LOW",
      Configuracao::PIN_I2C_SCL,
      nivelScl == HIGH ? "HIGH" : "LOW",
      static_cast<unsigned long>(Wire.getClock()),
      Wire.getTimeOut()
  );

  if (nivelSda != HIGH || nivelScl != HIGH) {
    Serial.println(
        "[I2C-DIAG] ERRO: linha presa em LOW; scan cancelado para nao bloquear"
    );
    Serial.println(
        "[I2C-DIAG] Verifique curto, GND, pull-ups, level shifter e alimentacao"
    );
    Serial.println("[I2C-DIAG] --- fim ---");
    return;
  }

  uint8_t resultados[ENDERECO_FINAL_SCAN + 1];
  memset(resultados, 0xFF, sizeof(resultados));
  uint8_t encontrados = 0;
  uint8_t erros = 0;

  for (uint8_t endereco = ENDERECO_INICIAL_SCAN;
       endereco <= ENDERECO_FINAL_SCAN;
       endereco++) {
    const uint8_t resultado = consultarEndereco(endereco);
    resultados[endereco] = resultado;
    if (resultado == 0) {
      encontrados++;
      Serial.printf("[I2C-DIAG] ACK encontrado em 0x%02X\n", endereco);
    } else if (resultado != 2) {
      erros++;
      Serial.printf(
          "[I2C-DIAG] 0x%02X: %s (codigo=%u)\n",
          endereco,
          descreverResultado(resultado),
          resultado
      );
      if (resultado == 5) {
        Serial.println("[I2C-DIAG] Scan interrompido apos timeout");
        break;
      }
    }
  }

  imprimirResultadoConhecido(
      "Si4713-A",
      SI4710_ADDR1,
      resultados[SI4710_ADDR1]
  );
  imprimirResultadoConhecido(
      "Si4713-B",
      SI4710_ADDR0,
      resultados[SI4710_ADDR0]
  );
  Serial.printf(
      "[I2C-DIAG] resumo: %u dispositivo(s), %u erro(s) de barramento\n",
      encontrados,
      erros
  );
  Serial.println("[I2C-DIAG] --- fim ---");
}

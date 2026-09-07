#include <Arduino.h>
#include <Wire.h>
#include "si4713_seguro.h"

// Mesma placa ESP32-S3 e ligacoes do projeto principal.
constexpr int SDA_SI = 17;
constexpr int SCL_SI = 18;
constexpr int RESET_SI = 5;
constexpr int GP2_SI = 4;
constexpr uint8_t ENDERECO_SI = 0x63;
constexpr uint16_t FREQUENCIA = 10240; // Unidade do Si4713: 10 kHz = 101,7 MHz.
constexpr uint8_t POTENCIA = 118;     // dBuV.
constexpr uint8_t CAPACITOR_AUTO = 0;

Si4713Seguro si(RESET_SI);
bool pronto = false;

void lerPropriedade(uint16_t propriedade, const char* nome, uint8_t unidadeHz = 0) {
  uint16_t valor;
  if (!si.getProperty(propriedade, valor)) {
    Serial.printf("%s: FALHA (%s)\n", nome, si.ultimaFalha());
    return;
  }
  Serial.printf("%s: 0x%04X (%u)", nome,
                static_cast<unsigned>(valor), static_cast<unsigned>(valor));
  if (unidadeHz) Serial.printf(" = %lu Hz", static_cast<unsigned long>(valor) * unidadeHz);
  Serial.println();
}

bool selecionarModo(bool estereo) {
  const uint16_t componentes = estereo ? 0x0003 : 0x0000;
  if (!si.setProperty(SI4713_PROP_TX_COMPONENT_ENABLE, componentes)) {
    Serial.printf("Falha ao selecionar modo: %s\n", si.ultimaFalha());
    return false;
  }
  Serial.printf("Modo solicitado: %s\n", estereo ? "Estereo" : "Mono");
  lerPropriedade(SI4713_PROP_TX_COMPONENT_ENABLE, "TX_COMPONENT_ENABLE");
  lerPropriedade(SI4713_PROP_TX_PILOT_DEVIATION, "TX_PILOT_DEVIATION", 10);
  lerPropriedade(SI4713_PROP_TX_PILOT_FREQUENCY, "TX_PILOT_FREQUENCY", 1);
  lerPropriedade(SI4713_PROP_TX_LINE_INPUT_MUTE, "TX_LINE_INPUT_MUTE");
  return true;
}

void setup() {
  Serial.begin(115200);
  // GP2 participa da selecao do barramento: entrada sem pull-up durante reset.
  pinMode(GP2_SI, INPUT);
  Wire.begin(SDA_SI, SCL_SI);
  Wire.setClock(100000);
  Wire.setTimeOut(25);

  Serial.println("Si4713 minimo: 101,7 MHz / 118 dBuV / capacitor AUTO");
  if (!si.begin(ENDERECO_SI)
      || !si.setProperty(SI4713_PROP_TX_LINE_INPUT_MUTE, 0x0000)
      || !selecionarModo(false)
      || !si.tuneFM(FREQUENCIA)
      || !si.setTXpower(POTENCIA, CAPACITOR_AUTO)
      || !si.readTuneStatus()) {
    Serial.printf("Falha na inicializacao: %s. Reinicie para tentar novamente.\n",
                  si.ultimaFalha());
    return;
  }
  pronto = si.currFreq == FREQUENCIA && si.currdBuV == POTENCIA;
  Serial.printf("Lido do chip: frequencia=%u (10 kHz), potencia=%u dBuV, capacitor=%u\n",
                si.currFreq, si.currdBuV, si.currAntCap);
  Serial.println(pronto ? "Transmitindo. Console: 0 = Mono; 1 = Estereo."
                       : "Falha: frequencia/potencia lidas diferem do solicitado.");
}

void loop() {
  while (Serial.available()) {
    const char comando = Serial.read();
    if (comando == '\r' || comando == '\n' || comando == ' ' || comando == '\t') continue;
    if (comando != '0' && comando != '1') {
      Serial.println("Console: 0 = Mono; 1 = Estereo.");
    } else if (!pronto) {
      Serial.println("Si4713 nao inicializado. Reinicie e confira as ligacoes.");
    } else {
      selecionarModo(comando == '1');
    }
  }
  delay(1);
}

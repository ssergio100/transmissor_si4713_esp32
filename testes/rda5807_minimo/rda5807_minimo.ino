#include <Arduino.h>
#include <Wire.h>
#include <RDA5807.h>

// Mesmos pinos do RDA no projeto principal; nenhuma inicializacao do Si4713.
constexpr int SDA_RDA = 17;
constexpr int SCL_RDA = 18;
constexpr uint8_t ENDERECO_RDA = 0x11; // Acesso aleatorio aos registradores.
constexpr uint16_t FREQUENCIA = 10170; // 101,7 MHz, em unidades de 10 kHz.
constexpr uint8_t VOLUME = 7;          // Mesmo volume padrao do monitor (0..15).
RDA5807 rda;
bool pronto = false;
char linhaSerial[32];
size_t tamanhoLinha = 0;
bool linhaLonga = false;

bool lerRegistrador(uint8_t reg, uint16_t& valor) {
  Wire.beginTransmission(ENDERECO_RDA);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(ENDERECO_RDA, static_cast<uint8_t>(2)) != 2) {
    while (Wire.available()) Wire.read();
    return false;
  }
  valor = static_cast<uint16_t>(Wire.read()) << 8;
  valor |= static_cast<uint16_t>(Wire.read());
  return true;
}

void diagnostico() {
  uint16_t reg02 = 0, reg03 = 0, reg04 = 0, reg07 = 0, reg0a = 0, reg0b = 0;
  const bool ok02 = lerRegistrador(0x02, reg02);
  const bool ok03 = lerRegistrador(0x03, reg03);
  const bool ok04 = lerRegistrador(0x04, reg04);
  const bool ok07 = lerRegistrador(0x07, reg07);
  const bool ok0a = lerRegistrador(0x0A, reg0a);
  const bool ok0b = lerRegistrador(0x0B, reg0b);
  Serial.println("[RDA] Leitura real do chip:");
  if (ok02) Serial.printf("reg02=0x%04X MONO=%u DMUTE=%u DHIZ=%u\n",
      reg02, (reg02 >> 13) & 1, (reg02 >> 14) & 1, (reg02 >> 15) & 1);
  else Serial.println("reg02: FALHA");
  if (ok03) Serial.printf("reg03=0x%04X CHAN=%u BAND=%u SPACE=%u\n",
      reg03, reg03 >> 6, (reg03 >> 2) & 3, reg03 & 3);
  else Serial.println("reg03: FALHA");
  if (ok04) Serial.printf("reg04=0x%04X I2S=%u SOFTMUTE=%u\n",
      reg04, (reg04 >> 6) & 1, (reg04 >> 9) & 1);
  else Serial.println("reg04: FALHA");
  if (ok07) Serial.printf("reg07=0x%04X SOFTBLEND=%u\n", reg07, (reg07 >> 1) & 1);
  else Serial.println("reg07: FALHA");
  if (ok0a) Serial.printf("reg0A=0x%04X ST=%u READCHAN=%u\n",
      reg0a, (reg0a >> 10) & 1, reg0a & 0x03FF);
  else Serial.println("reg0A: FALHA");
  if (ok0b) Serial.printf("reg0B=0x%04X RSSI=%u\n", reg0b, (reg0b >> 9) & 0x7F);
  else Serial.println("reg0B: FALHA");
  if (!ok02 || !ok0a) return;
  if (reg02 & 0x2000) Serial.println("Resultado: MONO FORCADO no chip.");
  else if (reg0a & 0x0400) Serial.println("Resultado: chip indica RECEPCAO ESTEREO.");
  else Serial.println("Resultado: estereo permitido, mas chip indica MONO neste instante.");
}

// Le MHz com ponto ou virgula, sem arredondar para outra emissora.
bool converterFrequencia(const char* texto, uint16_t& frequencia) {
  unsigned mhz = 0;
  if (*texto < '0' || *texto > '9') return false;
  while (*texto >= '0' && *texto <= '9') {
    mhz = mhz * 10 + (*texto++ - '0');
    if (mhz > 108) return false;
  }
  unsigned decimal = 0;
  if (*texto == '.' || *texto == ',') {
    ++texto;
    if (*texto < '0' || *texto > '9') return false;
    decimal = *texto++ - '0';
    // Aceita tambem 101.70, mas rejeita passos menores que 100 kHz.
    while (*texto == '0') ++texto;
  }
  if (*texto != '\0') return false;
  const unsigned valor = mhz * 100 + decimal * 10;
  if (valor < 7600 || valor > 10800) return false;
  frequencia = static_cast<uint16_t>(valor);
  return true;
}

void processarLinha() {
  linhaSerial[tamanhoLinha] = '\0';
  String comando(linhaSerial);
  comando.trim();
  if (comando.length() == 0) return;
  if (!pronto) {
    Serial.println("RDA nao inicializado. Confira as ligacoes e reinicie.");
    return;
  }
  if (comando == "r" || comando == "R") {
    diagnostico();
    return;
  }
  uint16_t frequencia = 0;
  if (!converterFrequencia(comando.c_str(), frequencia)) {
    Serial.println("Frequencia invalida. Envie MHz entre 76.0 e 108.0, passo 0.1, e Enter. Ex.: 101.7");
    return;
  }
  rda.setFrequency(frequencia);
  Serial.printf("Sintonia solicitada: %u.%u MHz\n", frequencia / 100, (frequencia % 100) / 10);
  diagnostico();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_RDA, SCL_RDA);
  Wire.setClock(100000);
  Wire.setTimeOut(25);
  Serial.println("RDA5807 minimo: 101,7 MHz, estereo permitido, volume 7.");
  uint16_t id = 0;
  if (!lerRegistrador(0x00, id) || (id >> 8) != 0x58) {
    Serial.printf("RDA5807 nao identificado em 0x11 (id=0x%04X).\n", id);
    return;
  }
  rda.setup();
  rda.setBass(false);
  rda.setBand(2); // 76..108 MHz.
  rda.setSpace(0); // Canalizacao de 100 kHz.
  rda.setSoftmute(false);
  rda.setSoftBlendEnable(false); // Evita mistura automatica para mono neste teste.
  rda.setI2SOn(false); // Saidas analogicas LOUT/ROUT.
  rda.setAudioOutputHighImpedance(false);
  rda.setVolume(VOLUME);
  rda.setMute(false);
  rda.setMono(false);
  rda.setRDS(false);
  rda.setFrequency(FREQUENCIA);
  pronto = true;
  diagnostico();
  Serial.println("Console: frequencia em MHz + Enter (ex.: 101.7 ou 99,5); r = diagnostico.");
}

void loop() {
  while (Serial.available()) {
    const char recebido = Serial.read();
    if (recebido == '\r' || recebido == '\n') {
      if (linhaLonga) Serial.println("Comando longo demais; descartado.");
      else processarLinha();
      tamanhoLinha = 0;
      linhaLonga = false;
    } else if (!linhaLonga) {
      // Preserva o comando r imediato, inclusive sem quebra de linha.
      if (tamanhoLinha == 0 && (recebido == 'r' || recebido == 'R')) {
        linhaSerial[tamanhoLinha++] = recebido;
        processarLinha();
        tamanhoLinha = 0;
      } else if (tamanhoLinha < sizeof(linhaSerial) - 1) {
        linhaSerial[tamanhoLinha++] = recebido;
      } else {
        linhaLonga = true;
      }
    }
  }
  delay(1); // Sem consultas I2C periodicas durante a escuta.
}

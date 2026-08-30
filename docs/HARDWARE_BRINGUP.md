# Contrato inicial de hardware e bring-up

## Identidade do alvo

- Placa: ESP32-S3-DevKitC-1, mesma unidade/revisão do `radio_web_1`;
- módulo: ESP32-S3 N16R8, flash de 16 MB e PSRAM OPI de 8 MB;
- framework: Arduino core ESP32 3.3.10;
- USB/porta: ainda não conectado nesta sessão;
- módulo transmissor: mesmo CJMCU-4713/Si4713 do projeto original;
- display: mesmo LCD I2C 20x4 do projeto original, endereço esperado `0x27`;
- encoder: mesmo encoder incremental com botão do projeto original.

## Fontes locais

| Decisão | Fonte |
| --- | --- |
| Perfil ESP32-S3 e memória | `../radio_web_1/sketch.yaml` |
| I2C GPIO17/18 e encoder GPIO16/15/7 | `../radio_web_1/configuracao.h` |
| Ligações e níveis do conjunto Si4713/LCD | `../README.md` |
| Comportamento do Si4713 e endereços | `../transmissor_si4713/RadioController.h` |
| Comandos e métricas | AN332 e datasheet do Si4713 |

## Mapa inicial

| Função | GPIO | Direção | Observação antes do primeiro teste |
| --- | ---: | --- | --- |
| I2C SDA | 17 | bidirecional | confirmar pull-ups e níveis dos dois lados |
| I2C SCL | 18 | saída open-drain | iniciar em 100 kHz e executar scanner |
| Encoder DT | 16 | entrada | confirmar direção e quatro transições por detente |
| Encoder CLK | 15 | entrada | confirmar estado durante boot |
| Encoder SW | 7 | entrada pull-up | clique curto e pressão longa |
| Reset Si4713 | 5 | saída | confirmar nível no módulo antes de conectar |
| Detector físico de RF | 4 | entrada, pull-down | reservado; HIGH somente com portadora detectada |

O LCD de 5 V e o Si4713 de 3,3 V permanecem separados pelo mesmo conversor de
nível lógico BSS138 do transmissor original. O ESP32 usa lógica de 3,3 V; a
ligação final deve reproduzir os lados corretos do conversor e o GND comum.

## Verificação em camadas

- [ ] Placa sozinha inicia e produz log serial;
- [ ] perfil, flash e PSRAM conferidos no boot/build;
- [ ] níveis ociosos do I2C medidos antes de conectar sinais;
- [ ] scanner encontra LCD e Si4713 nos endereços esperados;
- [ ] LCD exibe a tela inicial;
- [ ] encoder confirma direção, detentes, clique e pressão longa;
- [ ] reset e identificação do Si4713 passam;
- [ ] transmissão permanece desligada nos padrões e após restaurar;
- [ ] nível de áudio bruto e ASQ são observados;
- [ ] reset e ciclo de energia recuperam o estado esperado;
- [ ] detector físico permanece LOW com TX desligado e muda para HIGH com TX ligado;
- [ ] detector físico não responde ao tráfego Wi-Fi com TX desligado;
- [ ] detector não altera a sintonia automática nem o alcance do Si4713;
- [ ] perda e retorno de rede não interrompem o painel físico.

## Diagnóstico I2C no boot

O firmware `0.1.2` registra o nível lógico de SDA/SCL, frequência, timeout,
endereços encontrados e o resultado específico de `0x27`, `0x63` e `0x11`.
Os códigos seguem o retorno do `Wire.endTransmission()` do Arduino ESP32:

- `0`: endereço respondeu (ACK);
- `2`: endereço não respondeu (NACK);
- `5`: timeout ou barramento bloqueado.

Se SDA ou SCL estiver em LOW antes do scan, o firmware cancela a varredura para
não bloquear os controles. O diagnóstico digital não mede a tensão: com o LCD
alimentado em 5 V, confirme fisicamente que o lado do ESP32/Si4713 permanece em
3,3 V e que o BSS138 separa corretamente os dois domínios.

## Lacunas que dependem da bancada

- identidade estável da porta USB;
- tensões medidas e resistência efetiva dos pull-ups I2C;
- comportamento do GPIO5 no reset do módulo conectado;
- cadência estável de leitura ASQ com LCD, Wi-Fi e telemetria ativos;
- calibração de `VDET`, `VREF` e do capacitor de amostragem do detector físico;
- validação RF, áudio e RDS no equipamento de medição/recepção.

O projeto do detector está em
[`../../hardware/detector_rf_si4713.md`](../../hardware/detector_rf_si4713.md).

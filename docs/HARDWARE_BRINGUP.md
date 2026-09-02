# Contrato inicial de hardware e bring-up

## Identidade do alvo

- Placa: ESP32-S3-DevKitC-1, mesma unidade/revisão do `radio_web_1`;
- módulo: ESP32-S3 N16R8, flash de 16 MB e PSRAM OPI de 8 MB;
- framework: Arduino core ESP32 3.3.10;
- USB/porta: ainda não conectado nesta sessão;
- módulo transmissor: mesmo CJMCU-4713/Si4713 do projeto original;
- display: mesmo LCD I2C 20x4 do projeto original, endereço esperado `0x27`;
- display TFT: GMT020-02M(7P) v1.1, 2 polegadas, controlador ST7789,
  240x320, interface SPI de quatro fios e lógica/alimentação de 3,3 V;
- encoder: mesmo encoder incremental com botão do projeto original.

## Fontes locais

| Decisão | Fonte |
| --- | --- |
| Perfil ESP32-S3 e memória | `../radio_web_1/sketch.yaml` |
| I2C GPIO17/18 e encoder GPIO16/15/7 | `../radio_web_1/configuracao.h` |
| Ligações e níveis do conjunto Si4713/LCD | `../README.md` |
| Comportamento do Si4713 e endereços | `../transmissor_si4713/RadioController.h` |
| Comandos e métricas | AN332 e datasheet do Si4713 |
| Controlador, resolução, interface e tensão do TFT | especificação GMT020-02 da GoldenMorning e identificação visual do módulo GMT020-02M(7P) v1.1 |

## Mapa inicial

| Função | GPIO | Direção | Observação antes do primeiro teste |
| --- | ---: | --- | --- |
| I2C SDA | 17 | bidirecional | confirmar pull-ups e níveis dos dois lados |
| I2C SCL | 18 | saída open-drain | iniciar em 100 kHz e executar scanner |
| Encoder DT | 16 | entrada | confirmar direção e quatro transições por detente |
| Encoder CLK | 15 | entrada | confirmar estado durante boot |
| Encoder SW | 7 | entrada pull-up | clique curto e pressão longa |
| Reset Si4713 | 5 | saída | confirmar nível no módulo antes de conectar |
| Si4713 GP2/INT | 4 | entrada, sem pull | pulso ativo em LOW; não usar pull-up durante o reset |
| TFT CS | 10 | saída | seleção do ST7789, ativa em LOW |
| TFT SDA/MOSI | 11 | saída | dados do ESP32 para o TFT; não é SDA de I2C |
| TFT SCL/SCLK | 12 | saída | clock SPI; não é SCL de I2C |
| TFT DC | 13 | saída | seleção entre comando e dados |
| TFT RST | 14 | saída | reset do módulo, ativo em LOW |

O TFT deve ser ligado assim:

| Pino no TFT | Ligação no ESP32-S3 |
| --- | --- |
| `CS` | GPIO10 |
| `DC` | GPIO13 |
| `RST` | GPIO14 |
| `SDA` | GPIO11 (MOSI) |
| `SCL` | GPIO12 (SCLK) |
| `VCC` | 3V3 |
| `GND` | GND comum |

O módulo de sete pinos não expõe MISO nem controle separado do backlight. O
GPIO13, que o perfil Arduino da placa nomeia como MISO padrão, pode portanto ser
usado como `DC`. Se outro periférico SPI que exija leitura for acrescentado, o
`DC` deverá ser remapeado. GPIO8 e GPIO9 permanecem livres para a possível
separação futura do RDA5807 em outro controlador I2C.

Não alimentar o TFT pelo pino de 5 V. A especificação elétrica do painel limita
VCC e os sinais lógicos a 3,3 V em operação; a placa adaptadora da foto não
indica conversão de nível.

O LCD de 5 V e o Si4713 de 3,3 V permanecem separados pelo mesmo conversor de
nível lógico BSS138 do transmissor original. O ESP32 usa lógica de 3,3 V; a
ligação final deve reproduzir os lados corretos do conversor e o GND comum.

## Verificação em camadas

- [ ] Placa sozinha inicia e produz log serial;
- [ ] antes de conectar o TFT, confirmar 3,3 V entre `VCC` e `GND`;
- [ ] conferir continuidade de `SDA` para GPIO11 e de `SCL` para GPIO12;
- [ ] a tela de inicialização do TFT aparece antes da inicialização dos rádios;
- [ ] perfil, flash e PSRAM conferidos no boot/build;
- [ ] níveis ociosos do I2C medidos antes de conectar sinais;
- [ ] scanner encontra LCD e Si4713 nos endereços esperados;
- [ ] LCD exibe a tela inicial;
- [ ] encoder confirma direção e exatamente um evento por detente;
- [ ] log mostra clique curto entre 50 e 699 ms somente na soltura;
- [ ] log mostra pressão longa a partir de 700 ms somente na soltura;
- [ ] pressionar ou soltar o eixo sem girar não muda o item do menu;
- [ ] reset e identificação do Si4713 passam;
- [ ] GP2 do módulo está ligado ao GPIO4, sem pull-up externo adicional;
- [ ] fora do monitoramento contínuo, sobremodulação gera um único log `[SI4713-INT]` e trava o alerta;
- [ ] o contador não continua subindo enquanto o alerta aguarda reconhecimento;
- [ ] a tela `Monitor` do LCD ativa a leitura somente enquanto está aberta;
- [ ] o LCD mostra apenas nível em dBFS e `OK`/`CORTE`, sem ASQ ou contador;
- [ ] botão web envia `INTACK` e rearma GP2 quando houver alerta travado;
- [ ] transmissão permanece desligada nos padrões e após restaurar;
- [ ] salvar TX ligado, cortar a alimentação e confirmar no novo boot o log
      `[RF] Sequencia concluida em aplicacao` antes de o LCD mostrar `NO AR`;
- [ ] repetir o ciclo com TX salvo desligado e confirmar potência retornada zero;
- [ ] com TX ligado, entrar no ajuste de frequência pelo LCD e confirmar que a
      potência cai para zero antes do primeiro giro;
- [ ] girar vários passos e confirmar cada frequência no LCD/API, enquanto o
      receptor de teste permanece sem portadora; clicar e confirmar que o TX volta;
- [ ] repetir o ajuste pela web, cortar a alimentação e confirmar no boot que a
      última frequência aplicada foi restaurada (não a frequência anterior);
- [ ] nível de áudio e estado `OK`/`CORTE` são observados no LCD;
- [ ] reset e ciclo de energia recuperam o estado esperado;
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
- detector físico de portadora adiado; o GPIO4 está ocupado por GP2/INT;
- validação RF, áudio e RDS no equipamento de medição/recepção.
- frequência SPI padrão da biblioteca Adafruit estável no cabeamento real; se
  necessário, fixar um valor medido sem alterar a pinagem acima.
- orientação física do TFT; a rotação inicial do driver é `0` (retrato).

O projeto do detector permanece documentado em
[`../../hardware/detector_rf_si4713.md`](../../hardware/detector_rf_si4713.md),
mas não faz parte desta etapa.

# RDA5807 minimo — recepcao estereo em 101,7 MHz

Abra `rda5807_minimo.ino`. Projeto independente para a ESP32-S3 N16R8 da
montagem existente, com Arduino-ESP32 3.3.10 e PU2CLR RDA5807 1.1.9.
Nao modifica nem utiliza o firmware principal ou o sketch minimo do Si4713.

- SDA: GPIO17; SCL: GPIO18; I2C: 100 kHz, acesso direto `0x11`.
- Sintonia inicial: **101,7 MHz**, alteravel pelo Serial; volume **7/15**; audio analogico LOUT/ROUT ativo.
- `MONO=0`: estereo permitido. Soft blend, soft mute, bass, I2S e RDS desligados.
- Sem Wi-Fi, display, preferencias, scanner I2C ou controle do Si4713.
- Alimentacao, terra e conexoes LOUT/ROUT seguem a montagem existente.

O Si4713 precisa continuar transmitindo por outro controlador/firmware.
Este sketch so controla o RDA: nao inicia uma transmissao. Grava-lo no mesmo
ESP32 substitui o programa que anteriormente controlava os dois radios.

## Verificar a recepcao

Console Serial a **115200 baud**. Para mudar a sintonia, envie a frequencia em
MHz e pressione Enter: `101.7`, `99,5` ou `100`. Selecione **Nova linha** ou
**CR+LF** no monitor serial. Faixa aceita: **76,0 a 108,0 MHz**, passo de
**0,1 MHz**. Entradas invalidas nao alteram a sintonia. Reset retorna a 101,7 MHz.

O estado e lido no boot, depois da sintonia e ao enviar **r**. O comando r
tambem funciona sem quebra de linha. A mudanca de frequencia preserva os
ajustes de estereo e volume.
Nao ha leituras periodicas: use r depois de estabilizar a recepcao e depois de
alternar o transmissor entre mono e estereo no controlador dele.

- `MONO=1`: o RDA esta forcado a mono.
- `MONO=0, ST=0`: estereo permitido, mas o receptor indica mono naquele instante.
- `MONO=0, ST=1`: o receptor indica recepcao estereo.
- `DMUTE=1`, `DHIZ=1`: audio e saidas habilitados.
- `SOFTBLEND=0`, `SOFTMUTE=0`, `I2S=0`: configuracao pretendida neste teste.

Os registradores, o canal e o RSSI sao leituras reais do chip. Falhas de leitura
aparecem como FALHA. O bit ST informa o estado do receptor; nao mede separacao
fisica nas saidas. Com ST=1, compare LOUT/ROUT usando os testes Apenas L/R do
transmissor para investigar tambem o caminho analogico.

## Compilar

A partir da raiz do repositorio:

```sh
arduino-cli compile --profile esp32s3 --output-dir build/rda5807_minimo testes/rda5807_minimo
```

O perfil esta em `sketch.yaml`. Compilar nao grava a placa. O teste ainda requer
validacao na bancada; nao ha um resultado de ST presumido ou simulado.

# Si4713 minimo

Abra `si4713_minimo.ino` no Arduino IDE ou compile com o perfil `esp32s3`.
Projeto independente para a mesma ESP32-S3 N16R8 do firmware principal.
Depende somente do core Arduino-ESP32 3.3.10 e de Wire; o driver local e uma
copia de `si4713_seguro.h/.cpp` do projeto principal, com os mesmos timeouts,
reset e inicializacao. Nao inicializa RDA5807, display, Wi-Fi ou preferencias.

Ligacoes reutilizadas: SDA GPIO17, SCL GPIO18, RESET GPIO5, GP2 GPIO4
(entrada sem pull). Si4713 no endereco I2C `0x63`; nao faz varredura do barramento.
A alimentacao e as entradas analogicas LIN/RIN continuam conforme a montagem
existente. O sketch nao gera audio: injete a fonte de audio nas entradas do Si.

Ao iniciar, transmite automaticamente em **101,7 MHz**, **118 dBuV**, capacitor
**AUTO (0)**, inicialmente em **mono**, com ambas as entradas ativas e RDS desligado.
O valor de capacitor lido depois da sintonia pode ser diferente de zero: e o
resultado do ajuste automatico.

Console Serial a **115200 baud**, com ou sem quebra de linha:

- `0`: Mono, `TX_COMPONENT_ENABLE = 0x0000`.
- `1`: Estereo, `TX_COMPONENT_ENABLE = 0x0003` (piloto + L-R).

Cada comando altera somente TX_COMPONENT_ENABLE e mostra leituras reais de
componentes, desvio/frequencia do piloto e mute. Os valores do piloto nao sao
reescritos. Nao ha leituras I2C periodicas. A selecao nao e persistida: reset
retorna a mono.

Compilar, a partir da raiz do repositorio:

```sh
arduino-cli compile --profile esp32s3 --output-dir build/si4713_minimo testes/si4713_minimo
```

A compilacao nao grava a placa. Artefatos ficam separados do firmware principal.

# Transmissor Si4713 com ESP32-S3

Projeto independente para controlar o módulo Si4713, TFT SPI ST7789 e encoder com
uma ESP32-S3-DevKitC-1 N16R8. O painel físico funciona sem rede; a interface
React é uma segunda forma de operação sobre o mesmo estado do transmissor.

## Implementado

- controle do Si4713 com frequência, potência, antena, estéreo, pré-ênfase,
  desvio, mute, transmissão e recuperação automática;
- TFT ST7789 com menus sobrepostos à tela principal; giro seleciona/ajusta,
  clique abre/confirma e pressão longa descarta a edição e fecha as janelas;
- giro decodificado por `AiEsp32RotaryEncoder`, com um evento por detente;
  clique curto e pressão longa são classificados uma única vez na soltura;
- configuração e frases RDS de transmissão persistidas em NVS;
- RDA5807 para sintonia e RSSI, com recepção RDS desabilitada;
- RadioText manual, frase salva, hora, data, data/hora e modelo com tokens;
- varredura completa de 76,1 a 108,0 MHz e aplicação direta de qualquer
  frequência medida;
- provisionamento pelo WiFiManager, sem SSID ou senha no código;
- API REST na porta 80 e estado/telemetria por WebSocket na porta 81;
- GP2/INT do Si4713 no GPIO4, inicialmente para alertar sobremodulação sem
  polling contínuo, com alerta travado e reconhecimento pela web;
- monitor local por demanda: a leitura ASQ funciona somente enquanto a tela
  `Monitor` está aberta e exibe apenas nível e `OK`/`CORTE`;
- restauração de transmissão no boot com conclusão e reconhecimento separados
  para sintonia e potência; `NO AR` só aparece depois da sequência completa;
- ajuste local de frequência com rascunho: o giro não altera o TX; o clique
  aplica e grava a frequência. A prévia de sintonia da interface web permanece
  disponível pelos endpoints existentes;
- interface React/Vite responsiva, com modo de simulação para desenvolvimento.

O frontend permanece separado e não é gravado no ESP32 nesta fase.

## Estrutura

```text
transmissor_si4713_esp32/
├── transmissor_si4713_esp32.ino  # inicialização e loop cooperativo
├── radio_si4713.*                # acesso exclusivo ao Si4713
├── transmissor.*                 # regras e estado compartilhado
├── estado_painel.h               # snapshot semântico independente de hardware
├── apresentacao.*                # traduz domínio e menu para o snapshot
├── rede.* / api.*                # Wi-Fi, REST e WebSocket
├── display_tft.* / menu.*        # composição do TFT e navegação física
├── janela_menu_tft.*             # listas, ajustes, monitor e busca sobrepostos
├── interface-web/                # React + TypeScript + Vite
├── docs/API.md                   # contrato de comunicação
└── docs/HARDWARE_BRINGUP.md      # ligações e testes de bancada
```

## Interface para testes

```sh
cd interface-web
npm install
npm run dev
```

Por padrão, `npm run dev` abre o painel em modo de simulação. Isso permite
ajustar e demonstrar toda a tela sem uma placa conectada.

Para usar o ESP32 real, crie `interface-web/.env.local`:

```dotenv
VITE_USE_MOCK=false
VITE_DEVICE_URL=http://transmissor-si4713.local
```

Também é possível substituir o nome local pelo IP mostrado no TFT ou no log
serial. Reinicie o Vite depois de alterar o arquivo.

## Provisionamento Wi-Fi

Sem credenciais válidas, o equipamento cria a rede `TRANSMISSOR-SI4713`.
Conecte-se a ela e abra `http://192.168.4.1` para escolher a rede local. O mesmo
portal pode ser solicitado pela interface web. O menu físico SISTEMA foi removido.

As credenciais ficam na área persistente administrada pela pilha Wi-Fi do ESP32.
Elas não são armazenadas em `configuracao.h`, no React ou em arquivos de projeto.

## Compilar o firmware

```sh
./scripts/compilar.sh
```

Se `arduino-cli` não estiver no `PATH`, defina explicitamente:

```sh
ARDUINO_CLI_BIN=/caminho/para/arduino-cli ./scripts/compilar.sh
```

O script compila para ESP32-S3 N16R8, mas não grava a placa. O mapa inicial de
pinos e o roteiro seguro de primeira energização estão em
[`docs/HARDWARE_BRINGUP.md`](docs/HARDWARE_BRINGUP.md).

## Documentação

- [`ANALISE_INICIAL.md`](ANALISE_INICIAL.md): decisões, referências e limites;
- [`docs/API.md`](docs/API.md): endpoints, JSON, WebSocket e modo de erro;
- [`docs/MENUS_TFT.md`](docs/MENUS_TFT.md): como alterar os menus, cores e ajustes;
- [`docs/ARQUITETURA_DISPLAYS.md`](docs/ARQUITETURA_DISPLAYS.md): contrato
  independente usado pelos renderizadores;
- [`docs/referencias-visuais/painel-desktop.png`](docs/referencias-visuais/painel-desktop.png): conceito visual usado na implementação.

## Validação atual

- firmware: compila para `esp32:esp32:esp32s3`; a restauração após queda de
  energia e da última frequência aplicada ainda depende do teste de bancada;
- menus locais: testes de navegação e renderização em computador concluídos;
  resposta do encoder e fluidez no TFT ainda dependem da bancada;
- frontend: TypeScript e build de produção concluídos;
- navegador: tela desktop verificada e início de transmissão simulado validado;
- hardware: ainda depende da conferência das ligações e do teste com a placa real.

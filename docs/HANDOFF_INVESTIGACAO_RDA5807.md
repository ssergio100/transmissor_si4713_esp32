# Handoff — investigação do RDA5807M

Data do registro: 31/08/2026  
Branch: `feature/teste-rda5807-rssi`  
HEAD: `fb85537 test: valida sintonia e RSSI bruto do RDA5807`

## Objetivo original

Integrar um RDA5807M ao mesmo barramento I2C do ESP32 para que o receptor:

- acompanhe sempre a frequência aplicada ao transmissor Si4713;
- forneça o nível de recepção;
- posteriormente receba o RDS transmitido pelo próprio equipamento;
- permita usar a recepção/RDS para compor o estado `No AR`;
- tenha um nível mínimo de recepção configurável.

O teste inicial ficou deliberadamente limitado a sintonizar o RDA, ler o RSSI e
mostrar RSSI e frequência na última linha do LCD. A implementação completa de
RDS e `No AR` não deve começar antes de provarmos a recepção correta.

## Estado do firmware

Commits existentes nesta branch:

- `d237781 test: integra RSSI do receptor RDA5807`
- `dd8ffa1 fix: exibe frequencia junto ao RSSI do receptor`
- `2d456d4 fix: evita truncar RSSI de tres digitos`
- `fb85537 test: valida sintonia e RSSI bruto do RDA5807`

Implementação atual:

- biblioteca PU2CLR RDA5807 1.1.9;
- RDA no endereço sequencial `0x10` e acesso direto em `0x11`;
- Si4713 operando em `0x63`;
- I2C em GPIO17/GPIO18, 100 kHz;
- banda RDA `2`, correspondente a 76–108 MHz;
- espaçamento padrão da biblioteca: 100 kHz;
- frequência representada no projeto em unidades de 10 kHz: `10170` significa
  101,70 MHz;
- sintonia feita por `radio_.setFrequency(frequenciaKhz)`;
- frequência mostrada no LCD obtida por `radio_.getRealFrequency()`;
- RSSI lido pela biblioteca e também diretamente do registrador `0x0B`.

Importante: `getRealFrequency()` lê `READCHAN` do registrador `0x0A` e converte
o canal para frequência. Isso comprova qual canal o CI registrou como
sintonizado, mas **não mede a frequência física do oscilador local**. Uma
configuração incorreta de clock, banda ou espaçamento ainda poderia produzir no
LCD o número esperado e deixar a recepção fisicamente fora da estação.

## Observações feitas na bancada

- O receptor do projeto Nixie mostrou RSSI aproximadamente `88` inicialmente.
- O mesmo receptor Nixie, a cerca de 5 metros e com antena telescópica recolhida,
  mostrou aproximadamente `54`.
- No novo circuito foram observados valores aproximadamente `26`, `32` e `38`;
  a antena telescópica elevou a leitura para aproximadamente `38`.
- Os módulos RDA5807 foram trocados entre os dois projetos e o comportamento
  permaneceu associado ao projeto/montagem, não ao módulo.
- RSSI obtido pela biblioteca e RSSI extraído diretamente de `0x0B` deram o
  mesmo valor. Portanto não há evidência de truncamento ou deslocamento errado
  dos bits RSSI no nosso código.
- Com o transmissor ligado e desligado, o novo RDA continuou indicando cerca de
  `32`.
- Um rádio externo ao lado confirmou de forma independente que a portadora do
  Si4713 realmente desaparece quando o firmware indica RF desligado. Não voltar
  a tratar portadora residual do Si4713 como hipótese para esse `32`.
- A saída de áudio do RDA estava originalmente em alta impedância e volume zero,
  pois a configuração havia sido copiada do projeto Nixie para o teste de RSSI.
- A saída foi habilitada localmente. Depois da gravação/teste, foi ouvido apenas
  QRM, sem a transmissão esperada.
- A recepção de apenas QRM torna a sintonia física a principal questão ainda não
  resolvida. O valor de frequência exibido no LCD, sozinho, não encerra essa
  questão.

## Alterações locais ainda não commitadas

O diretório está intencionalmente com duas alterações:

- `receptor_rda5807.cpp`
  - `setAudioOutputHighImpedance(false)`;
  - `setVolume(7)`;
  - `setMute(false)`.
- `configuracao.h`
  - versão alterada de `0.1.16-rda-diag` para `0.1.17-rda-audio`.

Essas alterações compilam corretamente:

- programa: 1.164.647 bytes, 37%;
- variáveis globais: 55.820 bytes, 17%.

Não foi criado commit para elas até este handoff.

## Comparação já realizada com o projeto Nixie

Projeto de referência:
`/home/sergio/projetos/nixie-clock-with-DFPlayer`

A sequência visível de inicialização do RDA é equivalente nos dois projetos:

1. `setup()`;
2. bass desligado;
3. banda `2`;
4. soft mute desligado;
5. mono ativado;
6. `setFrequency()` recebendo valores em unidades de 10 kHz.

No Nixie, por exemplo, `9470` representa 94,7 MHz. Não foi encontrada até aqui
uma conversão adicional de frequência ou uma escala diferente de RSSI naquele
projeto.

A biblioteca chama `Wire.begin()` dentro de `RDA5807::setup()`. No Arduino-ESP32
3.3.10, uma segunda chamada encontra o barramento mestre já iniciado e não troca
os GPIO configurados anteriormente. Portanto isso foi verificado e, por ora,
não explica a falha.

## Investigação planejada para amanhã

### 1. Separar receptor e transmissor no teste

Com o Si4713 realmente desligado, sintonizar pelo ajuste de frequência uma
estação FM comercial conhecida e forte, confirmada no rádio externo.

- Se o RDA continuar produzindo somente QRM, o problema está na configuração do
  receptor, referência de clock, caminho de RF ou caminho de áudio.
- Se a estação for recebida corretamente, o RDA está sintonizando e o próximo
  foco passa a ser a correspondência física com a portadora do Si4713 e possível
  sobrecarga por proximidade.

Esse é o primeiro teste porque não exige alteração de firmware e separa duas
metades do sistema.

### 2. Comparar registradores completos nos dois projetos

Criar temporariamente um dump dos registradores relevantes do RDA após a
inicialização e após a sintonia, especialmente:

- `0x02`: enable, mute, saída e configuração do clock;
- `0x03`: canal solicitado, tune, banda e espaçamento;
- `0x05`: LNA, limiar e volume;
- `0x0A`: STC e `READCHAN`;
- `0x0B`: RSSI, `FM_TRUE` e `FM_READY`.

Executar o mesmo dump no projeto ESP32 e no Nixie usando o mesmo módulo e a
mesma frequência. Comparar os valores brutos, não apenas campos interpretados
pela biblioteca.

### 3. Validar clock, banda e espaçamento

Confirmar no módulo físico qual referência de clock ele utiliza e confrontar
com `setup()` da biblioteca, atualmente configurado para cristal passivo de
32,768 kHz. Confirmar também nos valores brutos que banda `2` e espaçamento de
100 kHz chegaram ao registrador `0x03`.

### 4. Validar a cadeia numérica de frequência

Registrar simultaneamente:

- frequência desejada do modelo;
- bytes enviados por `TX_TUNE_FREQ` ao Si4713;
- frequência devolvida por `TX_TUNE_STATUS` do Si4713;
- canal escrito no `0x03` do RDA;
- `READCHAN` devolvido no `0x0A` do RDA;
- frequência calculada a partir desse canal.

Isso permitirá provar a unidade e a frequência nos dois caminhos sem inferência
baseada apenas no LCD.

### 5. Só depois retomar RSSI e `No AR`

Quando a recepção estiver comprovada:

- repetir o teste ligado/desligado e registrar RSSI, `FM_TRUE` e `FM_READY`;
- verificar recepção do RDS próprio;
- decidir se `No AR` pode usar sinal + identidade RDS.

O RSSI isolado não deve ser adotado como critério de `No AR` enquanto continuar
igual com a portadora ligada e desligada.

## Comandos úteis para retomada

```sh
cd /home/sergio/projetos/transmissor_Si4713/transmissor_si4713_esp32
git status --short --branch
git diff -- configuracao.h receptor_rda5807.cpp
./scripts/compilar.sh
```

## Ponto exato de retomada

Começar pelo teste de uma estação FM conhecida com o Si4713 desligado. Não
implementar ainda RDS, limiar configurável ou lógica definitiva de `No AR`.
Primeiro provar que o RDA está fisicamente sintonizado na frequência indicada.

## Resultado posterior — firmware completo sem Wi-Fi

Na branch `test/rda5807-sem-wifi`, o firmware completo foi executado sem
inicializar ou processar Wi-Fi, WiFiManager, API HTTP e WebSocket. O restante do
firmware local foi preservado. O RDA5807 continuou recebendo apenas QRM.

Resultado: a ativação da rede não é necessária para reproduzir a falha e deixa
de ser a linha principal desta investigação.

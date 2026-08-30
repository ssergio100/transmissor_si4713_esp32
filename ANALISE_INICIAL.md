# Transmissor Si4713 com ESP32-S3 — análise inicial

## Escopo confirmado

Este é um projeto novo e independente, na pasta `transmissor_si4713_esp32`.
Os projetos existentes não serão modificados e servirão apenas como referência:

- `transmissor_si4713`: comportamento atual do Si4713, LCD 20x4, encoder e menus;
- `../radio_web_1`: configuração do ESP32-S3 e padrões já usados para Wi-Fi, API e interface web.

O equipamento deverá continuar operando pelo painel físico mesmo sem rede ou
interface web. A interface web será um segundo meio de controle, não uma segunda
implementação das regras do transmissor.

## Funções que devem ser preservadas

O código atual possui os seguintes ajustes e estados, que formam o escopo
funcional inicial:

- RF: frequência, potência, capacitância de antena e transmissão ligada/desligada;
- áudio: estéreo/mono, pré-ênfase, desvio e mute da entrada;
- RDS: ligado/desligado, PS, RadioText, código PI e conteúdo dinâmico selecionável;
- monitor: frequência e potência efetivas, nível de áudio, capacitância e ASQ;
- busca de canal: varredura, progresso, melhor frequência e nível de ruído;
- sistema: salvar configuração, restaurar padrões e informações do firmware;
- recuperação automática do Si4713 quando ele deixa de responder.

O encoder manterá a mesma semântica: giro para navegar/alterar, clique curto para
selecionar/confirmar e pressão longa para voltar. O LCD continuará usando a mesma
hierarquia de telas e menus. A web exibirá todos os ajustes na mesma tela, sem
reproduzir a navegação limitada do LCD.

## Hardware confirmado

As placas e módulos serão exatamente os mesmos dos projetos de referência. A
base será a ESP32-S3-DevKitC-1 N16R8 do `radio_web_1`, usando o perfil
`ESP32S3 Dev Module`, flash de 16 MB, PSRAM OPI de 8 MB e Arduino core ESP32
3.3.10. Do transmissor atual serão mantidos o mesmo módulo Si4713, LCD I2C 20x4
no endereço `0x27` e encoder com botão.

O mapa inicial reutilizará o I2C `SDA=GPIO17` e `SCL=GPIO18` e o encoder
`DT=GPIO16`, `CLK=GPIO15` e `SW=GPIO7` já usados na mesma ESP32-S3. LCD e Si4713
compartilharão o I2C. O reset do Si4713 será avaliado inicialmente no `GPIO5`,
usado para essa função no transmissor atual e disponível neste projeto, mas o
mapa só será fechado após conferir as ligações e executar os testes isolados de
I2C, display, encoder e reset. As conexões elétricas existentes serão
reproduzidas; nenhuma tensão será inferida apenas pelo nome do módulo.

## Arquitetura proposta

Uma única camada de aplicação será a fonte de verdade. Ela receberá comandos do
encoder e da API, validará os valores, acionará o Si4713, atualizará o estado
efetivo, persistirá quando solicitado e notificará LCD e web. O acesso ao rádio
ficará encapsulado para impedir que servidor, display e controles manipulem a
biblioteca Si4713 diretamente.

Estrutura inicial prevista:

```text
transmissor_si4713_esp32/
├── transmissor_si4713_esp32.ino  # boot e coordenação do loop
├── configuracao.h                # placa, pinos e tempos
├── modelos.*                     # ajustes, telemetria e comandos
├── radio_si4713.*                # único proprietário do CI
├── transmissor.*                 # regras e estado compartilhados
├── controles.*                   # encoder e botão
├── menu.*                        # navegação física
├── display.*                     # LCD 20x4
├── persistencia.*                # configuração local do equipamento
├── wifi.*                        # conexão e provisionamento
├── api.*                         # API HTTP versionada
├── frontend/                     # aplicação React + Vite independente
├── docs/                         # arquitetura, API e validações
└── tests/                        # testes possíveis sem o hardware
```

O frontend será uma aplicação React criada com Vite e, na fase atual, será usada
exclusivamente pelo servidor de desenvolvimento iniciado com `npm run dev`. Ela
consumirá diretamente a API do ESP32 na rede local. Não haverá build da interface
armazenado no ESP32 durante essa fase. O endereço do equipamento poderá ser
resolvido por nome local ou informado no ambiente local do Vite, sem credenciais
no repositório.

## Rede e provisionamento Wi-Fi

Será reutilizado o comportamento do `radio_web_1`: quando não houver uma rede
válida, ou quando a reconfiguração for solicitada pelo menu físico, o ESP32 abrirá
seu próprio ponto de acesso e o portal do WiFiManager. O usuário conecta o celular
ou computador à rede do transmissor, escolhe a rede local e informa a senha.

As credenciais serão administradas pelo WiFiManager e pela pilha Wi-Fi do ESP32
na memória persistente interna. SSID e senha não aparecerão no código-fonte, em
`configuracao.h`, nos arquivos do React ou em arquivos versionados. A interface
React só passa a operar depois que computador e transmissor conseguem se alcançar
na rede local; o portal de provisionamento é independente dela.

## RDS na interface

A área RDS da tela única terá, além de habilitação, PI e PS, uma seleção explícita
da fonte do RadioText:

- texto manual;
- lista de frases cadastradas, com edição e ordenação;
- hora atual;
- data atual;
- data e hora;
- modelo personalizado que combine texto, data e hora.

O PS continuará limitado ao formato suportado pelo projeto atual. Frases e texto
dinâmico serão formatados no firmware antes de serem enviados ao buffer RDS; a
interface mostrará uma prévia exata do conteúdo aplicado. A atualização de hora
ou data só será enviada ao Si4713 quando o texto resultante mudar, evitando
reescritas contínuas sem efeito visível.

A fonte inicial de data e hora será o relógio do sistema sincronizado por SNTP
depois da conexão Wi-Fi. Sem uma hora válida, o firmware não deve transmitir uma
data inventada e a interface indicará que a sincronização está pendente. Um RTC
externo não faz parte do escopo confirmado deste transmissor; poderá ser avaliado
se for necessário preservar data e hora após reinicializar sem rede.

## Contrato inicial da API

A API deverá ser versionada e trabalhar com o mesmo modelo usado pelo painel:

- leitura do estado completo: ajustes desejados, valores efetivos e telemetria;
- atualização validada dos ajustes do transmissor;
- ligar e desligar a transmissão;
- iniciar a varredura, consultar progresso e aplicar o melhor canal;
- salvar configuração e restaurar padrões;
- informar saúde do Si4713, recuperação, Wi-Fi e versão do firmware.

Ao terminar a varredura, a interface apresentará os resultados medidos, ordenados
pelo menor nível de ruído, destacará a recomendação e oferecerá a ação
**Aplicar**. Essa ação altera a frequência do estado compartilhado e configura
imediatamente o Si4713; salvar a escolha de forma permanente continuará sendo
uma decisão explícita. Durante a varredura a interface deverá mostrar que o
transmissor está fora do ar, pois a medição de ruído do Si4713 desliga a saída de
transmissão. A classificação não usará um limite arbitrário para declarar um
canal livre: o critério e a apresentação serão validados com as medições reais.

O indicador **NO AR** será derivado do estado do rádio, não apenas do
valor solicitado na configuração. Estado em recuperação, falha de comunicação
ou varredura não poderá aparecer como transmissão normal. A resposta da API
deverá distinguir claramente `solicitado`, `aplicado` e `falha/ocupado` para que
LCD e web nunca mostrem confirmações falsas.

O contrato JSON será fechado antes de implementar a interface. Isso permitirá
desenvolver e testar o frontend com respostas simuladas, sem depender do ESP32
conectado e sem duplicar lógica de validação no navegador.

### Telemetria rápida de áudio

É possível medir o áudio sem acrescentar um ADC externo. O comando
`TX_ASQ_STATUS` do Si4713 devolve o nível atual da entrada em dBFS e os estados
de nível alto, nível baixo e sobremodulação. A biblioteca Adafruit instalada já
expõe esses dados por `readASQ()`, `currInLevel` e `currASQ`; o transmissor atual
faz essa leitura a cada 100 ms.

Requisições REST repetidas não serão usadas para animar o medidor. O firmware
fará a amostragem local e publicará nível, pico/sobremodulação, sequência e
instante por WebSocket para o React. REST continuará responsável por leitura de
estado, configurações e ações. O navegador fará somente a suavização visual e a
retenção de pico; o valor bruto e os alarmes continuarão vindo do equipamento.

Os 100 ms existentes serão a referência inicial do primeiro teste, não uma
decisão final. A maior cadência estável será determinada no mesmo hardware,
observando comunicação I2C, atualização do LCD, servidor, RDS e recuperação do
Si4713. Dessa forma o medidor poderá ser responsivo sem escolher antecipadamente
uma taxa que prejudique as funções principais.

## Roteiro direto de desenvolvimento

1. **Fechar o contrato de hardware e comportamento.** Registrar placa, módulo,
   alimentação, mapa de pinos e a equivalência de cada tela/ação antiga. O
   resultado será a configuração usada pelo porte, evitando correções espalhadas.
2. **Portar e validar o núcleo local.** Implementar persistência, Si4713,
   encoder, menu e LCD sobre o estado único; compilar e testar primeiro sem web.
   Só avançar quando o painel reproduzir o comportamento atual e se recuperar de
   reinicializações e falhas do rádio.
3. **Definir e implementar a comunicação.** Documentar REST, WebSocket, exemplos,
   validações e erros; testar comandos, scanner, aplicação de frequência e fluxo
   de áudio por simulador. O contrato estabilizado passa a ser a base do frontend.
4. **Criar e aprovar o conceito visual antes do código da interface.** Produzir
   uma tela completa para operação profissional, com RF, áudio, RDS, monitor,
   scanner, sistema e indicação NO AR. Após aprovação, implementar o frontend
   fiel ao conceito e conectá-lo à API.
5. **Integrar e validar o equipamento completo.** Testar alterações concorrentes
   entre web e encoder, perda de rede, reinício, persistência, varredura,
   recuperação do Si4713, responsividade da interface e operação prolongada.

## Testes que dependem do usuário e do equipamento

Podem ser repassados para execução física, com instruções e resultados esperados:

- conferir a ligação e executar um scanner I2C antes de conectar todos os módulos;
- validar sentido e quantidade de passos do encoder, clique e pressão longa;
- gravar o firmware e devolver o log serial de boot quando necessário;
- confirmar no LCD cada menu e alteração após o porte;
- medir a saída RF com equipamento apropriado e confirmar recepção, áudio e RDS;
- comparar diferentes cadências do medidor de áudio e devolver o log de qualquer
  travamento, atraso de tela ou falha I2C;
- testar a interface em computador, tablet e celular na rede real;
- simular queda de Wi-Fi, reinício e desconexão do Si4713, registrando o resultado.

Compilação, análise estática, testes da API simulada e testes automatizados do
frontend ficam a cargo do desenvolvimento e não precisam consumir tempo de teste
manual.

## Decisões ainda não fechadas

- confirmação em bancada do mapa de pinos e das ligações elétricas reproduzidas;
- requisito de autenticação da API depois do provisionamento na rede local;
- hospedagem final do frontend: servidor externo, estação de operação ou cópia
  opcional no ESP32;
- mecanismo de persistência dos ajustes e frases RDS e política de salvamento;
- necessidade futura de RTC para data/hora sem rede após reinicialização;
- nome visual do produto e identidade da interface.

Essas decisões devem ser resolvidas na ordem em que bloqueiam o roteiro, sem
antecipar detalhes visuais ou de implantação que ainda não afetam o porte do
hardware.

## Referências técnicas usadas nesta análise

- [AN332 — Si47xx Programming Guide](https://www.silabs.com/documents/public/application-notes/AN332.pdf): comandos de medição de ruído, ASQ, nível em dBFS e comportamento durante varredura;
- [biblioteca Adafruit Si4713](https://github.com/adafruit/Adafruit-Si4713-Library): API Arduino usada pelo transmissor de referência;
- [WiFiManager](https://github.com/tzapu/WiFiManager): portal cativo, conexão automática e reconfiguração sob demanda.

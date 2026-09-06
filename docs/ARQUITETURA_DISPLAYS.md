# Arquitetura para múltiplos displays

## Objetivo

Permitir que o painel atual e dispositivos futuros apresentem o mesmo estado do
transmissor sem acessar diretamente `Menu`, `RadioSi4713`, `ReceptorRda5807`,
persistência ou rede.

```text
Menu + Transmissor
        |
        v
Apresentacao::gerar()
        |
        v
   EstadoPainel
      /     \
     v       v
TFT ST7789  display futuro
  SPI        driver próprio
```

## Contrato semântico

`estado_painel.h` contém apenas tipos e valores copiáveis, agrupados por área:

- navegação e item selecionado;
- RF desejado e efetivamente aplicado;
- configuração e telemetria de áudio;
- configuração RDS;
- frequência e RSSI do receptor;
- progresso e resultado da varredura;
- saúde do Si4713 e versão do firmware.

O contrato não contém endereço de barramento, pino, coordenada, resolução, cor,
fonte, texto limitado a 20 colunas ou chamadas de biblioteca. Um renderizador
pode decidir livremente como representar cada valor.

`apresentacao.cpp` é o único adaptador entre as classes internas e esse contrato.
O TFT ST7789 recebe somente um `EstadoPainel`; portanto, ele serve como prova
de que o estado pode ser consumido sem dependência direta do domínio.

## Display atual

O módulo foi identificado como GMT020-02M(7P) v1.1: TFT IPS de 2 polegadas,
240x320, controlador ST7789 e interface SPI de quatro fios. Apesar das inscrições
`SDA` e `SCL` no conector, esses sinais são respectivamente MOSI e clock SPI; o
módulo não é I2C.

A pinagem reservada está em `configuracao.h`: GPIO10 (`CS`), GPIO11
(`SDA/MOSI`), GPIO12 (`SCL/SCLK`), GPIO13 (`DC`) e GPIO14 (`RST`). A alimentação
é 3,3 V. GPIO8 e GPIO9 ficam disponíveis para uma futura separação do barramento
I2C do RDA5807.

O driver inicial está isolado em `display_tft.h/.cpp`, usa Adafruit ST7789 e
recebe o `EstadoPainel` gerado pela apresentação. Ele inicializa o hardware SPI em
modo somente escrita, apresenta todas as áreas do menu e mantém cache por linha:
uma mudança de RSSI, nível de áudio ou progresso atualiza apenas a região visual
correspondente, sem reenviar o quadro inteiro.

O firmware usa paisagem 320×240 com rotação `1`; a orientação física precisa
ser conferida na montagem. A frequência SPI permanece no padrão da biblioteca.
Cores e estabilidade elétrica ainda dependem dos testes de bancada. A ausência
de MISO impede leitura de identificação; portanto, o software consegue confirmar
a inicialização do controlador SPI, mas a presença do painel somente pode ser
confirmada visualmente.

## Limite atual

O suporte ao LCD 20×4 foi removido. Confirmações de salvar, restaurar padrões
e falhas continuam no log serial. O TFT apresenta o estado e os menus;
notificações transitórias ainda não fazem parte do contrato de apresentação.

## Tela principal em paisagem

`tela_principal_tft.*` recebe o snapshot e desenha os blocos com cache de texto
e cor. `tema_tft.h` concentra a paleta RGB565 para ajustes visuais.
A tela não mostra relógio nem capacitor de antena. Frequência configurada,
TX efetivo, RDS configurado, potência configurada, RSSI válido, mute, modo,
pré-ênfase e desvio são apresentados. RSSI indisponível aparece como `---`.
O status NO AR usa `rf.transmitindo`; não representa medição RF externa.
As telas de configuração mantêm a navegação e usam linhas de 33 pixels,
com rodapé em y=211. A recuperação mantém sua tela dedicada.

Validação em bancada: conferir rotação, contraste, leitura, ON/OFF, mono/ST,
mute, alerta, retorno dos menus e ausência de cintilação com RSSI variável.

### Onde editar o visual

- `tema_tft.h`: paleta com nomes de cores e estilos por bloco/estado. Cada
  estilo lista `fundo`, `borda`, `titulo` e `valor`, nessa ordem. Todos os fundos
  de blocos começam em `AZUL_ARDOSIA`, preservando o visual inicial.
- `tela_principal_tft.cpp`, seção LAYOUT: posição, dimensões e tamanho do texto.
- Seção INTERFACE: chamadas curtas que compõem a tela.
- Seção CONTEUDO DOS BLOCOS: função própria para cada indicador e seus estados.
- Seção FUNCOES DE DESENHO: painel, texto, ícone e atualização com cache.

Por exemplo, para mudar apenas o fundo de TX desligado, altere a primeira cor
em `TemaTft::TX_DESLIGADO`. Para mudar todos os fundos padrão de uma vez,
altere `AZUL_ARDOSIA`. O cache compara as quatro cores, além do texto.

### Identificação e alinhamento dos blocos

Cada linha de `AREAS` informa seu `Bloco::Nome`; as linhas podem mudar de ordem
sem trocar os indicadores. `localizarArea()` procura pelo identificador.
A transmissão é centralizada com `getTextBounds()`, incluindo os deslocamentos
da fonte. Os demais valores preservam as distâncias ao topo ajustadas manualmente;
a medição compensa a origem da fonte e centraliza na horizontal.
O capacitor de antena permanece somente nas configurações RF.

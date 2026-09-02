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
LCD 20x4   display futuro
  I2C       driver próprio
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
O LCD 20x4 recebe agora somente um `EstadoPainel`; portanto, ele serve como prova
de que o estado pode ser consumido sem dependência direta do domínio.

## Próximo display

O módulo foi identificado como GMT020-02M(7P) v1.1: TFT IPS de 2 polegadas,
240x320, controlador ST7789 e interface SPI de quatro fios. Apesar das inscrições
`SDA` e `SCL` no conector, esses sinais são respectivamente MOSI e clock SPI; o
módulo não é I2C.

A pinagem reservada está em `configuracao.h`: GPIO10 (`CS`), GPIO11
(`SDA/MOSI`), GPIO12 (`SCL/SCLK`), GPIO13 (`DC`) e GPIO14 (`RST`). A alimentação
é 3,3 V. GPIO8 e GPIO9 ficam disponíveis para uma futura separação do barramento
I2C do RDA5807.

O driver inicial está isolado em `display_tft.h/.cpp`, usa Adafruit ST7789 e
recebe o mesmo `EstadoPainel` entregue ao LCD. Ele inicializa o hardware SPI em
modo somente escrita, apresenta todas as áreas do menu e mantém cache por linha:
uma mudança de RSSI, nível de áudio ou progresso atualiza apenas a região visual
correspondente, sem reenviar o quadro inteiro.

Na tela principal, `NO AR` não é mais inferido apenas do estado interno do
Si4713. Ele exige TX efetivamente ativo e uma leitura válida do RDA5807 maior ou
igual ao limiar configurado. O mesmo snapshot leva ao TFT o RadioText decodificado
pelo RDA, e o driver percorre esse texto em uma janela rolante de 18 caracteres.
Assim, a linha mostra o conteúdo recebido pelo enlace RF, não uma cópia direta do
texto solicitado ao transmissor.

A orientação confirmada na montagem é retrato com rotação `2` (180° em relação
ao padrão do controlador), e a frequência SPI permanece no padrão da biblioteca.
Cores e estabilidade elétrica ainda dependem dos testes de bancada. A ausência
de MISO impede leitura de identificação; portanto, o software consegue confirmar
a inicialização do controlador SPI, mas a presença do painel somente pode ser
confirmada visualmente.

## Limite atual

As mensagens transitórias (`mostrarMensagem`) ainda pertencem ao LCD 20x4. Para
que confirmações como “Configuração salva” também apareçam no TFT, essas
notificações devem migrar para o contrato de apresentação.

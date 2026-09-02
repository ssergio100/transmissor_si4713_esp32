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

A integração física do novo display não faz parte desta etapa. Quando o módulo
estiver disponível, devem ser confirmados no próprio hardware e na documentação
correspondente:

- controlador e revisão;
- tensão de alimentação e de lógica;
- endereço e pinagem I2C;
- sequência de inicialização;
- biblioteca compatível com Arduino ESP32 3.3.10.

Depois dessa confirmação, o novo driver deve ficar em uma classe separada e
implementar apenas a renderização de `EstadoPainel`. Nenhuma regra de rádio ou
menu deve ser duplicada no driver.

## Limite atual

As mensagens transitórias (`mostrarMensagem`) ainda pertencem ao LCD 20x4. Antes
de manter dois displays ativos ao mesmo tempo, essas notificações devem migrar
para o contrato de apresentação para que ambos recebam o mesmo aviso.

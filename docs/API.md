# API do transmissor

Contrato da versão `v1`. A API REST usa a porta 80 e responde JSON. A telemetria
rápida usa WebSocket na porta 81. Todas as respostas REST incluem CORS e
`Cache-Control: no-store`.

## Endereços

```text
REST:      http://transmissor-si4713.local/api/v1
WebSocket: ws://transmissor-si4713.local:81
```

## Estado completo

`GET /api/v1/state`

```json
{
  "desired": {
    "frequencyKhz": 9950,
    "powerDbuv": 100,
    "antennaCap": 0,
    "txEnabled": false,
    "stereo": true,
    "preemphasisUs": 50,
    "audioDeviationKhz": 66,
    "muted": false,
    "rdsEnabled": true,
    "rdsPi": 18195,
    "rdsPs": "SI4713  ",
    "rdsText": "Transmissor FM Si4713           ",
    "rdsTemplate": "{data} {hora}                   ",
    "rdsSource": "manual"
  },
  "applied": {
    "onAir": false,
    "frequencyKhz": 9950,
    "powerDbuv": 0,
    "antennaCap": 0,
    "audioLevelDbfs": -70,
    "asq": 0
  },
  "system": {
    "si4713Available": true,
    "recovering": false,
    "recoveries": 0,
    "rfStateMismatches": 0,
    "si4713InterruptPin": 4,
    "si4713InterruptCount": 0,
    "si4713LastInterruptMs": 0,
    "si4713LastInterrupt": "none",
    "si4713InterruptPending": false,
    "scanRunning": false,
    "scanFinished": false,
    "scanProgress": 0,
    "wifiConnected": true,
    "wifiPortalActive": false,
    "ip": "192.168.1.50",
    "timeValid": true,
    "uptimeMs": 120000,
    "firmwareVersion": "0.1.15",
    "frequencyMinKhz": 7610,
    "frequencyMaxKhz": 10800,
    "frequencyStepKhz": 10
  }
}
```

`desired` é a configuração solicitada; `applied` contém o estado confirmado
pelo rádio. Todas as interfaces usam `applied.onAir` para mostrar **NO AR** ou
**FORA DO AR**.
`system.uptimeMs` permite que a interface reconheça um reinício do ESP32 sem
confundir o estado anterior armazenado no navegador com o estado atual.
`system.frequencyMinKhz`, `frequencyMaxKhz` e `frequencyStepKhz` informam a
faixa e o passo de sintonia válidos — a interface deve usá-los para limitar os
campos em vez de valores fixos.
Os campos `si4713Interrupt*` registram os episódios recebidos em GP2/INT, a
causa confirmada por uma leitura `TX_ASQ_STATUS` e se o alerta aguarda
reconhecimento. Os valores possíveis de
`si4713LastInterrupt` são `none`, `overmodulation`, `audio_high`, `audio_low`,
`asq` e `read_error`.

## Configuração e transmissão

`PUT /api/v1/settings` aceita qualquer subconjunto dos campos de `desired` e
aplica imediatamente. Quando a frequência muda, a saída RF é pausada antes da
ressintonia e a nova frequência é persistida depois da confirmação do Si4713.
Limites principais:

- `frequencyKhz`: de `system.frequencyMinKhz` (76,1 MHz = 7610) a
  `system.frequencyMaxKhz` (108,0 MHz = 10800), no passo
  `system.frequencyStepKhz` (10 kHz);
- `powerDbuv`: 88 a 115;
- `antennaCap`: `0` seleciona AUTO; de `1` a `191`, cada passo representa
  `0,25 pF` (ajuste manual de `0,25` a `47,75 pF`). Em `applied`, o Si4713
  devolve o valor efetivo escolhido, inclusive quando `desired` está em AUTO;
- `preemphasisUs`: 50 ou 75;
- `audioDeviationKhz`: 50 a 66;
- `rdsPs`: até 8 caracteres;
- `rdsText` e `rdsTemplate`: até 32 caracteres;
- `rdsSource`: `manual`, `frase`, `hora`, `data`, `data_hora` ou `modelo`.

Exemplo:

```json
{
  "frequencyKhz": 9570,
  "powerDbuv": 96,
  "stereo": true,
  "rdsPs": "RADIO957",
  "rdsSource": "modelo",
  "rdsTemplate": "No ar {data} {hora}"
}
```

Outras ações:

| Método | Rota | Corpo | Resultado |
|---|---|---|---|
| `PUT` | `/api/v1/frequency/adjust` | `{"frequencyKhz":9570}` | Pausa o TX no primeiro passo e ressintoniza sem persistir |
| `POST` | `/api/v1/tx` | `{"enabled":true}` | Liga/desliga a transmissão |
| `POST` | `/api/v1/tx/restart` | — | Desliga potência, ressintoniza e reaplica o estágio RF |
| `POST` | `/api/v1/settings/save` | — | Persiste os ajustes atuais |
| `POST` | `/api/v1/settings/defaults` | — | Restaura e salva os padrões, com TX desligado |

## Monitoramento de áudio (por demanda)

A leitura **periódica** do nível de áudio nunca é autônoma: o ESP só inicia
essa consulta quando a interface solicita. Sem solicitação, não há polling de
ASQ nem de sintonia. Uma interrupção em GP2 ainda provoca uma única consulta
para identificar o evento, mas não o reconhece automaticamente.

| Método | Rota | Corpo | Resultado |
|---|---|---|---|
| `PUT` | `/api/v1/audio/monitor` | `{"enabled":true}` | Liga/desliga a leitura de áudio |
| `GET` | `/api/v1/audio/monitor` | — | Estado atual e último valor lido |

```json
{
  "enabled": true,
  "active": true,
  "requested": true,
  "sequence": 128,
  "levelDbfs": -12,
  "asq": 87,
  "overmodulation": false,
  "onAir": true
}
```

`enabled`/`requested` representam a solicitação web. `active` informa se a
leitura está efetivamente ativa, inclusive quando foi solicitada localmente
pela tela `Monitor` do LCD.

Segurança contra autonomia: mesmo com `PUT .../monitor {"enabled":true}` ativo,
se não houver cliente WebSocket conectado à porta 81 por 5 segundos, o ESP
desliga o monitoramento sozinho. O WebSocket permanece conectado mesmo com o
medidor desligado, porque também transporta estado e eventos do Si4713.

## Interrupção GP2/INT

O GP2 do Si4713 deve ser ligado ao `GPIO4` do ESP32-S3. O pino do ESP fica como
entrada, sem pull-up interno, inclusive durante o reset do rádio. O firmware
habilita inicialmente apenas a fonte ASQ de sobremodulação. A ISR não acessa a
I2C: ela somente marca o pulso e o `loop()` consulta `TX_ASQ_STATUS` sem
`INTACK`, atualiza os campos `si4713Interrupt*` e publica um quadro `state`.
Sem monitoramento contínuo, o alerta permanece travado, evitando novos pulsos
e leituras da mesma condição. Enquanto o medidor web ou a tela `Monitor` local
estiverem ativos, cada amostra reconhece o intervalo anterior; assim o bit
`OVERMOD` representa corte ocorrido nos últimos 250 ms e não fica congelado.

Um alerta que foi travado fora do monitoramento pode ser reconhecido pela API
ou ao entrar na tela `Monitor` local:

| Método | Rota | Resultado |
|---|---|---|
| `POST` | `/api/v1/si4713/interrupt/ack` | Envia `INTACK` e rearma GP2 |

Se a sobremodulação continuar presente, o Si4713 gera um novo episódio depois
do rearme, que volta a ficar travado até o próximo reconhecimento.

Essa interrupção informa condição do áudio interno do Si4713. Ela não confirma
fisicamente a presença da portadora RF.

## Varredura

1. `POST /api/v1/scan/start` pausa o TX e inicia a leitura cooperativa.
2. `GET /api/v1/scan/results` informa progresso e medições.
3. `POST /api/v1/scan/apply` com `{"frequencyKhz":9570}` aplica uma frequência
   válida diretamente. A varredura é apenas uma sugestão: não precisa existir e
   a frequência escolhida não precisa pertencer ao último resultado.

Resposta resumida de resultados:

```json
{
  "running": false,
  "finished": true,
  "progress": 100,
  "recommendedFrequencyKhz": 9570,
  "recommendedNoiseLevel": 2,
  "measurements": [
    { "frequencyKhz": 8750, "noiseLevel": 31 },
    { "frequencyKhz": 8760, "noiseLevel": 27 }
  ]
}
```

A transmissão anterior é restaurada quando a varredura termina. Aplicar uma
frequência pela rota `scan/apply` pausa a saída durante a ressintonia, confirma
o valor efetivo, persiste a escolha e então restaura o estado anterior do TX.

Na edição interativa, LCD e web usam `frequency/adjust` para cada passo. A
primeira chamada zera a potência RF e as seguintes mantêm o TX pausado. A
confirmação por `PUT /settings` aplica o valor final, grava somente a frequência
na chave dedicada da NVS e restaura o estado de transmissão que já estava
configurado.

## Frases RDS

- `GET /api/v1/rds/phrases`: lista frases e limites;
- `PUT /api/v1/rds/phrases`: substitui a lista inteira.

```json
{
  "phrases": [
    "Clássicos e sucessos",
    "Música e informação"
  ]
}
```

São aceitas até 12 frases, cada uma com 1 a 32 caracteres. A lista fica em um
namespace NVS próprio.

## Rede e saúde

| Método | Rota | Função |
|---|---|---|
| `POST` | `/api/v1/wifi/portal` | Abre o portal `TRANSMISSOR-SI4713` em `192.168.4.1` |
| `GET` | `/api/v1/health` | Versão, uptime, memória, rede, hora, recuperação e falhas I²C do rádio |

O endpoint do portal não recebe SSID nem senha. Essas informações são inseridas
diretamente no portal cativo e administradas pelo WiFiManager.

O campo `i2cCommunicationFailures` acumula falhas reais de comandos do
Si4713 e ajuda a distinguir instabilidade do barramento de uma queda de Wi-Fi.
O endpoint de saúde também inclui os quatro campos `si4713Interrupt*`.

## WebSocket de telemetria

O servidor publica um quadro quando a sequência ASQ muda (nominalmente a cada
250 ms) **enquanto o monitoramento de áudio está habilitado** por `PUT
/api/v1/audio/monitor` e há um cliente WebSocket conectado. Com o monitoramento
desligado, nenhum quadro periódico de áudio é emitido. Uma interrupção pode
atualizar a amostra ASQ uma vez e também provoca um quadro de estado:

```json
{
  "type": "audio",
  "sequence": 128,
  "timestampMs": 43120,
  "levelDbfs": -12,
  "asq": 87,
  "overmodulation": false,
  "onAir": true
}
```

`levelDbfs` é o `INLEVEL` instantâneo e `overmodulation` é o bit `OVERMOD` do
Si4713. A posição da barra representa o nível de entrada e a faixa vermelha
começa em −12 dBFS, conforme o ponto observado na bancada. Quando `OVERMOD`
está ativo, o contorno, o marcador e o aviso de corte ficam vermelhos. ASQ é
exibido como bitfield hexadecimal, não como porcentagem.

## WebSocket de estado

O mesmo canal permanece ativo e publica um quadro `state` sempre que a configuração
desejada, o estado RF aplicado, a transmissão, a varredura ou a disponibilidade
do rádio mudarem, ou quando GP2/INT registrar um evento (o nível periódico de
áudio segue no quadro `audio`):

```json
{
  "type": "state",
  "desired": { ... },
  "applied": { ... },
  "system": { ... }
}
```

O conteúdo reutiliza a mesma serialização de `GET /api/v1/state`; a interface
usa esse quadro para refletir imediatamente alterações feitas pelo painel
físico. O polling REST permanece como leitura inicial e como fallback quando o
WebSocket está fora do ar.

## Erros

Erros usam um código HTTP coerente e o mesmo formato:

```json
{
  "error": "configuracao_invalida",
  "detail": "Um ou mais valores estao fora dos limites permitidos"
}
```

Respostas esperadas incluem `400` para JSON inválido, `404` para rota ausente,
`409` quando o rádio está indisponível/ocupado e `422` para valores inválidos.

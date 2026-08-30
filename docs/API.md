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
    "scanRunning": false,
    "scanFinished": false,
    "scanProgress": 0,
    "wifiConnected": true,
    "wifiPortalActive": false,
    "ip": "192.168.1.50",
    "timeValid": true,
    "uptimeMs": 120000,
    "firmwareVersion": "0.1.9"
  }
}
```

`desired` é a configuração solicitada; `applied` contém o estado confirmado
pelo rádio. Todas as interfaces usam `applied.onAir` para mostrar **NO AR** ou
**FORA DO AR**.
`system.uptimeMs` permite que a interface reconheça um reinício do ESP32 sem
confundir o estado anterior armazenado no navegador com o estado atual.

## Configuração e transmissão

`PUT /api/v1/settings` aceita qualquer subconjunto dos campos de `desired` e
aplica imediatamente. Limites principais:

- `frequencyKhz`: 8750 a 10800, passo 10;
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
| `POST` | `/api/v1/tx` | `{"enabled":true}` | Liga/desliga a transmissão |
| `POST` | `/api/v1/tx/restart` | — | Desliga potência, ressintoniza e reaplica o estágio RF |
| `POST` | `/api/v1/settings/save` | — | Persiste os ajustes atuais |
| `POST` | `/api/v1/settings/defaults` | — | Restaura e salva os padrões, com TX desligado |

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
frequência altera o estado em uso, mas não persiste a escolha; o salvamento é
uma ação separada.

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

## WebSocket de áudio

O servidor publica um quadro quando a sequência ASQ muda, nominalmente a cada
250 ms enquanto o TX está ativo:

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

`levelDbfs` e `overmodulation` vêm do detector ASQ do Si4713. São adequados para
um indicador rápido de presença/pico e não substituem um medidor PCM calibrado.

## WebSocket de estado

O mesmo canal também publica um quadro `state` sempre que a configuração
desejada, o estado RF aplicado, a transmissão, a varredura ou a disponibilidade
do rádio mudarem (o nível de áudio segue no quadro `audio`):

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

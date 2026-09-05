# RDS de transmissão

O RDS é usado somente na transmissão pelo Si4713. O RDA5807 mantém sintonia
e RSSI, com RDS desabilitado e sem consulta ou decodificação de grupos recebidos.

Os logs `[SI4713][RDS]`, `[PS]` e `[RT]` registram as propriedades e textos
configurados no transmissor. Não comprovam recepção pela antena.

O driver mantém a seleção de estéreo/RDS e o desvio de áudio sob controle da
configuração do aplicativo. O RadioText inclui CR e alterna Text A/B a cada carga.

## Verificação local

```sh
g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Itests/stubs -I. tests/rds_test.cpp si4713_seguro.cpp \
  -o /tmp/transmissor-rds-test
ASAN_OPTIONS=detect_leaks=0 /tmp/transmissor-rds-test
./scripts/compilar.sh
```

O teste usa o driver TX real com I2C simulado e verifica propriedades,
texto vazio/curto/de 32 caracteres, terminador CR e alternância A/B.
A validação de RF e áudio continua dependendo de teste de bancada.

# Diagnóstico comparativo dos registradores do RDA5807

Branch do firmware completo: `test/rda5807-dump-registros`.

O firmware completo imprime três amostras:

- `apos_rda`: depois de inicializar e sintonizar o receptor;
- `apos_si4713`: depois de aplicar toda a configuração do transmissor;
- `apos_scan_i2c`: depois do scanner I2C executado no boot.

O projeto mínimo `../teste_rda5807_si4713` imprime:

- `apos_rda`;
- `apos_si4713`;
- `apos_lcd`.

Cada linha contém os valores brutos dos registradores `0x02` a `0x08`, `0x0A`
e `0x0B`. Para a comparação, capturar somente as linhas iniciadas por
`[RDA-DUMP]` nos dois firmwares. Não alterar frequência, alimentação, módulo ou
ligações entre as duas execuções.

O objetivo é responder separadamente:

1. se o RDA recebe uma configuração diferente já na primeira fase;
2. se a configuração completa do Si4713 altera algum registrador do RDA;
3. se o scanner I2C ou a inicialização do LCD altera o receptor.

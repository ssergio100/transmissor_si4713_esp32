# Handoff — interferência de atividade I2C no áudio

Branch: `fix/audio-reduz-atividade-i2c`

## Sintomas observados

- ruído baixo e periódico, descrito como aproximadamente cinco `tap` por
  segundo;
- pulso ou apito perceptível quando o valor de RSSI mostrado no LCD muda.

## Evidência no firmware

O firmware não configura nem habilita I2S. No dump usado na investigação do
RDA5807, o registrador `0x04` permaneceu em `0x0000`. O áudio ouvido é o caminho
analógico do receptor.

O RSSI era atualizado a cada 250 ms. Cada atualização lia o registrador `0x0B`
duas vezes: uma por `RDA5807::getRssi()` e outra pelo acesso direto usado no
diagnóstico. Assim havia oito leituras do mesmo registrador por segundo.

O display já mantinha cache por linha, mas qualquer mudança de um único dígito
do RSSI fazia a linha completa de 20 caracteres ser reenviada ao LCD I2C. Essa
rajada adicional coincide especificamente com a mudança visual do RSSI.

## Alterações deste teste

1. O RDA usa somente uma leitura direta de `0x0B` por ciclo. A comparação com o
   valor da biblioteca foi removida porque a investigação anterior já mostrou
   que os dois resultados eram iguais.
2. O LCD escreve somente os trechos cujos caracteres mudaram. Uma mudança de um
   dígito do RSSI não reenvia mais a linha inteira.
3. A cadência de 250 ms foi preservada neste primeiro teste, permitindo avaliar
   a redução causada apenas pela remoção de tráfego redundante.

## Limite da correção por software

Enquanto o RSSI for acompanhado continuamente, ainda haverá uma transação I2C
periódica. Se o `tap` continuar audível, o teste seguinte deve reduzir ou
desativar temporariamente esse polling. Alterações de clock, filtros, alimentação
ou aterramento dependem do esquema e de observação no hardware.

# Teste de NO AR com anti-aliasing

Este teste mantém Adafruit_GFX/Adafruit_ST7789 e altera somente o texto NO AR.
A antena e os estados TX OFF, TX PAUSADO e FALHA TX continuam com o desenho anterior.

- Fonte: Nunito, peso 800 (ExtraBold), tamanho 36 pixels antes do recorte.
- Origem: https://github.com/google/fonts/tree/main/ofl/nunito
- Fonte e licença SIL OFL: `fontes/Nunito.ttf` e `fontes/OFL-Nunito.txt`.
- Imagem gerada: 120×29 pixels, 16 níveis de cobertura, 1.740 bytes em flash.
- `no_ar_previa.png`: prévia do bloco; a antena é uma aproximação em Pillow.
  A área do texto usa os mesmos níveis de cobertura e mistura RGB565 do firmware.

## Como ajustar

Em `tela_principal_tft.cpp`, `NO_AR_SUAVIZADO = false` volta ao texto original.
`TRANSMISSAO_AJUSTE_VERTICAL` continua controlando a posição vertical.
A fonte/escala da tabela AREAS permanece válida para os estados de texto padrão;
NO AR suavizado usa as dimensões da imagem, sem ampliá-la no ESP32.
As cores do texto e do fundo continuam em `TX_NO_AR`, no `tema_tft.h`.

Para mudar tamanho ou peso, edite `TAMANHO` / `PESO` no gerador e execute:

```sh
python3 scripts/gerar_no_ar.py
./scripts/compilar.sh
```

O gerador requer Pillow. A compilação normal usa o cabeçalho já gerado e não
precisa de Python, arquivos no ESP32, TFT_eSPI ou carregamento de fonte no boot.
O gerador verifica se o texto cabe no espaço atual. O firmware mantém o cache
por bloco e envia a imagem linha a linha, sem alocação dinâmica. Se o bloco for
reduzido e a imagem não couber, usa o texto padrão como alternativa.

## Validação na placa

Conferir inicialização, aparência em tamanho real, alternância NO AR/TX OFF,
retorno dos menus e mudança de cores do tema. Compilação e prévia não comprovam
funcionamento físico. Este teste não adiciona anti-aliasing aos demais textos.

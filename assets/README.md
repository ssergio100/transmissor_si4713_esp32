# Textos suavizados da tela TFT

Os estados `NO AR`, `TX OFF`, `ON`, `OFF`, `STEREO`, `MONO` e `MUTE` usam
máscaras de cobertura geradas previamente. O ESP32 mistura essa cobertura com
as cores definidas em `tema_tft.h` e envia a imagem ao display sem carregar
arquivos durante a execução.

- Fonte: Nunito ExtraBold, distribuída sob a licença SIL OFL.
- Arquivos da fonte: `fontes/Nunito.ttf` e `fontes/OFL-Nunito.txt`.
- `imagens_tft/*_suave_imagem.h`: dados usados pelo firmware.
- `previews/*_previa.png`: imagens auxiliares para conferência visual.

## Gerar ou alterar um texto

Use o gerador genérico informando texto, cor do texto, cor de fundo e nome do
cabeçalho:

```sh
python3 scripts/gerar_texto_suave.py \
    "OFF" \
    "90,190,255" \
    "8,12,8" \
    off_suave_imagem
```

Depois, inclua o cabeçalho e cadastre a máscara na tabela `VALORES_SUAVES`, em
`tela_principal_tft.cpp`. O script `gerar_no_ar.py` permanece disponível para
regenerar especificamente o estado `NO AR` usando as cores do tema.

Os geradores requerem Pillow. A compilação normal utiliza somente os cabeçalhos
já gerados e não depende de Python nem dos arquivos PNG.

## Validação na placa

Confira em tamanho real a posição vertical, as mudanças de estado e as cores do
tema. A compilação e as prévias não comprovam a aparência física do display.

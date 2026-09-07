#!/usr/bin/env python3

"""
Gera um texto suavizado para TFT e salva em .h + imagem de prévia.

Uso:

    python3 scripts/gerar_texto_suave.py \
        "NO AR" \
        "255,40,40" \
        "0,0,0" \
        no_ar

    python3 scripts/gerar_texto_suave.py \
        "99.7" \
        "255,255,255" \
        "8,12,8" \
        frequencia_99_7

Parâmetros:

    1. Texto
    2. Cor do texto em R,G,B
    3. Cor do fundo em R,G,B
    4. Nome base do arquivo

Saídas:

    <raiz>/assets/imagens_tft/<nome>.h
    <raiz>/assets/previews/<nome>_previa.png

Requer:

    pip install pillow
"""

from pathlib import Path
import argparse
import re

from PIL import Image, ImageDraw, ImageFont


# ============================================================
# CONFIGURAÇÃO PADRÃO
# ============================================================

RAIZ = Path(__file__).resolve().parent.parent

FONTE = RAIZ / "assets/fontes/Nunito.ttf"
PASTA_IMAGENS_TFT = RAIZ / "assets/imagens_tft"
PASTA_PREVIAS = RAIZ / "assets/previews"

TAMANHO_PADRAO = 36
PESO_PADRAO = 800
ESCALA_PADRAO = 4

LARGURA_MAXIMA_PADRAO = 144
ALTURA_MAXIMA_PADRAO = 46


# ============================================================
# CORES
# ============================================================

def interpretar_rgb(valor):
    """
    Converte uma string no formato:

        255,40,40

    para:

        (255, 40, 40)
    """

    partes = valor.split(",")

    if len(partes) != 3:
        raise argparse.ArgumentTypeError(
            f'Cor inválida: "{valor}". '
            'Use o formato "R,G,B", por exemplo "255,40,40".'
        )

    try:
        r, g, b = (int(p.strip()) for p in partes)
    except ValueError:
        raise argparse.ArgumentTypeError(
            f'Cor inválida: "{valor}". '
            "Os valores R, G e B precisam ser números."
        )

    if not all(0 <= v <= 255 for v in (r, g, b)):
        raise argparse.ArgumentTypeError(
            f'Cor inválida: "{valor}". '
            "Cada componente deve estar entre 0 e 255."
        )

    return r, g, b


def rgb888_para_rgb565(r, g, b):
    return (
        ((r & 0xF8) << 8)
        | ((g & 0xFC) << 3)
        | (b >> 3)
    )


def canais_rgb565(cor):
    return (
        cor >> 11,
        (cor >> 5) & 0x3F,
        cor & 0x1F,
    )


def rgb565_para_rgb888(cor):
    r, g, b = canais_rgb565(cor)

    return (
        round(r * 255 / 31),
        round(g * 255 / 63),
        round(b * 255 / 31),
    )


# ============================================================
# NOMES
# ============================================================

def slug_arquivo(nome):
    """
    Limpa o nome do arquivo.

    Ex.:
        "Frequencia 99.7" -> "frequencia_99_7"
    """

    nome = nome.strip().lower()
    nome = re.sub(r"[^\w]+", "_", nome, flags=re.UNICODE)
    nome = re.sub(r"_+", "_", nome)
    nome = nome.strip("_")

    return nome or "texto_suave"


def identificador_cpp(nome):
    """
    Gera namespace C++ válido.

    Ex.:
        frequencia_99_7
        ->
        TextoSuave_frequencia_99_7
    """

    nome = re.sub(r"[^\w]", "_", nome, flags=re.UNICODE)

    if not nome:
        nome = "texto_suave"

    if nome[0].isdigit():
        nome = "texto_" + nome

    return "TextoSuave_" + nome


# ============================================================
# FONTE / RENDERIZAÇÃO
# ============================================================

def carregar_fonte(tamanho, peso, escala):
    if not FONTE.exists():
        raise RuntimeError(
            f"Fonte não encontrada:\n{FONTE}"
        )

    fonte = ImageFont.truetype(
        str(FONTE),
        tamanho * escala,
    )

    try:
        fonte.set_variation_by_axes([peso])
    except Exception:
        raise RuntimeError(
            "Não foi possível selecionar o peso da fonte. "
            "Verifique se Nunito.ttf é uma fonte variável."
        )

    return fonte


def renderizar_texto(
    texto,
    tamanho,
    peso,
    escala,
    largura_max,
    altura_max,
):
    fonte = carregar_fonte(
        tamanho,
        peso,
        escala,
    )

    limites = fonte.getbbox(texto)

    largura = (
        (limites[2] - limites[0] + escala - 1)
        // escala
    ) + 2

    altura = (
        (limites[3] - limites[1] + escala - 1)
        // escala
    ) + 2

    if largura > largura_max or altura > altura_max:
        raise RuntimeError(
            f'O texto "{texto}" não cabe no bloco.\n'
            f"Tamanho produzido: {largura}x{altura}\n"
            f"Máximo permitido: {largura_max}x{altura_max}"
        )

    mascara = Image.new(
        "L",
        (
            largura * escala,
            altura * escala,
        ),
        0,
    )

    desenho = ImageDraw.Draw(mascara)

    desenho.text(
        (
            escala - limites[0],
            escala - limites[1],
        ),
        texto,
        font=fonte,
        fill=255,
    )

    # Reduz a imagem 4x usando LANCZOS.
    # Isso cria as bordas suavizadas.
    mascara = mascara.resize(
        (largura, altura),
        Image.Resampling.LANCZOS,
    )

    # Converte 0..255 para 0..15.
    #
    # Cada pixel passa a ter 16 níveis de cobertura:
    #
    # 0  = completamente fundo
    # 15 = completamente texto
    #
    niveis = [
        (valor * 15 + 127) // 255
        for valor in mascara.getdata()
    ]

    return largura, altura, niveis


# ============================================================
# EMPACOTAMENTO 4 BITS POR PIXEL
# ============================================================

def empacotar_niveis_4bpp(niveis):
    pixels = list(niveis)

    # Precisamos de quantidade par para armazenar
    # dois pixels em cada byte.
    if len(pixels) % 2:
        pixels.append(0)

    dados = []

    for i in range(0, len(pixels), 2):
        byte = (
            (pixels[i] << 4)
            | pixels[i + 1]
        )

        dados.append(byte)

    return dados


# ============================================================
# GERAÇÃO DO HEADER
# ============================================================

def escrever_header(
    caminho_saida,
    namespace,
    largura,
    altura,
    dados,
    texto,
    cor_texto,
    cor_fundo,
):
    linhas = []

    for i in range(0, len(dados), 16):
        linha = ", ".join(
            f"0x{valor:02X}"
            for valor in dados[i:i + 16]
        )

        linhas.append("  " + linha)

    r1, g1, b1 = cor_texto
    r2, g2, b2 = cor_fundo

    conteudo = (
        "// Gerado por scripts/gerar_texto_suave.py.\n"
        f'// Texto: "{texto}"\n'
        f"// Previa texto: RGB({r1}, {g1}, {b1})\n"
        f"// Previa fundo: RGB({r2}, {g2}, {b2})\n"
        "//\n"
        "// As cores NAO fazem parte dos dados abaixo.\n"
        "// COBERTURA armazena somente 16 niveis de alpha.\n"
        "// Nao editar os pixels manualmente.\n\n"

        "#pragma once\n"
        "#include <Arduino.h>\n\n"

        f"namespace {namespace} {{\n\n"

        f"constexpr int LARGURA = {largura};\n"
        f"constexpr int ALTURA = {altura};\n\n"

        "const uint8_t COBERTURA[] PROGMEM = {\n"
        + ",\n".join(linhas)
        + "\n};\n\n"

        "}\n"
    )

    caminho_saida.write_text(
        conteudo,
        encoding="utf-8",
    )


# ============================================================
# PRÉVIA RGB565
# ============================================================

def gerar_paleta_16_niveis(
    cor_fundo_565,
    cor_texto_565,
):
    """
    Gera os mesmos 16 níveis intermediários que
    podem ser usados pelo firmware no RGB565.
    """

    fundo = canais_rgb565(
        cor_fundo_565
    )

    texto = canais_rgb565(
        cor_texto_565
    )

    paleta = []

    for alpha in range(16):

        r, g, b = [
            (
                texto[i] * alpha
                + fundo[i] * (15 - alpha)
                + 7
            ) // 15

            for i in range(3)
        ]

        cor565 = (
            (r << 11)
            | (g << 5)
            | b
        )

        paleta.append(
            rgb565_para_rgb888(cor565)
        )

    return paleta


def gerar_previa(
    caminho_saida,
    largura,
    altura,
    niveis,
    cor_fundo,
    cor_texto,
):
    cor_fundo_565 = rgb888_para_rgb565(
        *cor_fundo
    )

    cor_texto_565 = rgb888_para_rgb565(
        *cor_texto
    )

    paleta = gerar_paleta_16_niveis(
        cor_fundo_565,
        cor_texto_565,
    )

    margem = 10

    previa = Image.new(
        "RGB",
        (
            largura + margem * 2,
            altura + margem * 2,
        ),
        rgb565_para_rgb888(
            cor_fundo_565
        ),
    )

    imagem_texto = Image.new(
        "RGB",
        (largura, altura),
    )

    imagem_texto.putdata(
        [
            paleta[alpha]
            for alpha
            in niveis[:largura * altura]
        ]
    )

    previa.paste(
        imagem_texto,
        (margem, margem),
    )

    previa.save(
        caminho_saida
    )


# ============================================================
# MAIN
# ============================================================

def main():
    parser = argparse.ArgumentParser(
        description=(
            "Gera texto suavizado para TFT "
            "em formato 4bpp + prévia RGB565."
        )
    )

    parser.add_argument(
        "texto",
        help='Texto a renderizar. Ex.: "NO AR"',
    )

    parser.add_argument(
        "cor_texto",
        type=interpretar_rgb,
        help='Cor do texto em RGB. Ex.: "255,40,40"',
    )

    parser.add_argument(
        "cor_fundo",
        type=interpretar_rgb,
        help='Cor do fundo em RGB. Ex.: "0,0,0"',
    )

    parser.add_argument(
        "arquivo",
        help="Nome base do arquivo de saída.",
    )

    parser.add_argument(
        "--tamanho",
        type=int,
        default=TAMANHO_PADRAO,
        help=f"Tamanho da fonte. Padrão: {TAMANHO_PADRAO}",
    )

    parser.add_argument(
        "--peso",
        type=int,
        default=PESO_PADRAO,
        help=f"Peso da Nunito. Padrão: {PESO_PADRAO}",
    )

    parser.add_argument(
        "--escala",
        type=int,
        default=ESCALA_PADRAO,
        help=f"Supersampling. Padrão: {ESCALA_PADRAO}",
    )

    parser.add_argument(
        "--largura-max",
        type=int,
        default=LARGURA_MAXIMA_PADRAO,
    )

    parser.add_argument(
        "--altura-max",
        type=int,
        default=ALTURA_MAXIMA_PADRAO,
    )

    args = parser.parse_args()

    texto = args.texto
    cor_texto = args.cor_texto
    cor_fundo = args.cor_fundo

    nome_base = slug_arquivo(
        args.arquivo
    )

    namespace = identificador_cpp(
        nome_base
    )

    saida_h = (
        PASTA_IMAGENS_TFT
        / f"{nome_base}.h"
    )

    saida_previa = (
        PASTA_PREVIAS
        / f"{nome_base}_previa.png"
    )

    # --------------------------------------------------------
    # Renderiza
    # --------------------------------------------------------

    largura, altura, niveis = renderizar_texto(
        texto=texto,
        tamanho=args.tamanho,
        peso=args.peso,
        escala=args.escala,
        largura_max=args.largura_max,
        altura_max=args.altura_max,
    )

    # --------------------------------------------------------
    # Empacota
    # --------------------------------------------------------

    dados = empacotar_niveis_4bpp(
        niveis
    )

    # --------------------------------------------------------
    # Header
    # --------------------------------------------------------

    escrever_header(
        caminho_saida=saida_h,
        namespace=namespace,
        largura=largura,
        altura=altura,
        dados=dados,
        texto=texto,
        cor_texto=cor_texto,
        cor_fundo=cor_fundo,
    )

    # --------------------------------------------------------
    # Prévia
    # --------------------------------------------------------

    gerar_previa(
        caminho_saida=saida_previa,
        largura=largura,
        altura=altura,
        niveis=niveis,
        cor_fundo=cor_fundo,
        cor_texto=cor_texto,
    )

    # --------------------------------------------------------
    # Resultado
    # --------------------------------------------------------

    print()
    print(f'Texto:          "{texto}"')
    print(
        "Cor texto:      "
        f"RGB{cor_texto}"
    )
    print(
        "Cor fundo:      "
        f"RGB{cor_fundo}"
    )
    print(
        f"Tamanho final:  {largura}x{altura}"
    )
    print(
        f"Pixels:         {largura * altura}"
    )
    print(
        f"Bytes:          {len(dados)}"
    )
    print(
        f"Namespace:      {namespace}"
    )
    print(
        f"Header:         {saida_h}"
    )
    print(
        f"Prévia:         {saida_previa}"
    )
    print()


if __name__ == "__main__":
    main()

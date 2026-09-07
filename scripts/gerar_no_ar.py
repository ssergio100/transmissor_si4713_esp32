#!/usr/bin/env python3

"""
Gera o texto "NO AR" suavizado para o TFT.

Requer:
    pip install pillow

O firmware utiliza somente o arquivo .h gerado.

A prévia utiliza as cores definidas em tema_tft.h e aceita:

    constexpr uint16_t VERMELHO = RGB(255, 0, 0);

ou o formato antigo:

    constexpr uint16_t VERMELHO = 0xF800;
"""

from pathlib import Path
import re

from PIL import Image, ImageDraw, ImageFont


# ============================================================
# CONFIGURAÇÃO
# ============================================================

RAIZ = Path(__file__).resolve().parent.parent

TEXTO = "NO AR"

FONTE = RAIZ / "assets/fontes/Nunito.ttf"

TAMANHO = 36
PESO = 800
ESCALA = 4

LARGURA_MAXIMA = 144
ALTURA_MAXIMA = 46

ARQUIVO_H = RAIZ / "no_ar_suave_imagem.h"
ARQUIVO_PREVIA = RAIZ / "assets/no_ar_previa.png"
ARQUIVO_TEMA = RAIZ / "tema_tft.h"


# ============================================================
# RGB
# ============================================================

def rgb888_para_rgb565(r, g, b):
    """Converte RGB 0..255 para RGB565."""

    return (
        ((r & 0xF8) << 8)
        | ((g & 0xFC) << 3)
        | (b >> 3)
    )


def canais_rgb565(cor):
    """Retorna os canais RGB565: R=0..31, G=0..63, B=0..31."""

    return (
        cor >> 11,
        (cor >> 5) & 0x3F,
        cor & 0x1F,
    )


def rgb565_para_rgb888(cor):
    """Converte RGB565 para RGB 0..255, para a prévia."""

    r, g, b = canais_rgb565(cor)

    return (
        round(r * 255 / 31),
        round(g * 255 / 63),
        round(b * 255 / 31),
    )


# ============================================================
# LEITURA DAS CORES DO TEMA
# ============================================================

def carregar_cores_tema():
    tema = ARQUIVO_TEMA.read_text(encoding="utf-8")

    cores = {}

    # --------------------------------------------------------
    # Formato antigo:
    #
    # constexpr uint16_t VERMELHO = 0xF800;
    # --------------------------------------------------------

    padrao_hex = re.compile(
        r"constexpr\s+uint16_t\s+(\w+)\s*=\s*"
        r"(0x[0-9A-Fa-f]+)\s*;"
    )

    for nome, valor in padrao_hex.findall(tema):
        cores[nome] = int(valor, 16)

    # --------------------------------------------------------
    # Formato novo:
    #
    # constexpr uint16_t VERMELHO = RGB(255, 0, 0);
    # --------------------------------------------------------

    padrao_rgb = re.compile(
        r"constexpr\s+uint16_t\s+(\w+)\s*=\s*"
        r"RGB\s*\(\s*"
        r"(\d+)\s*,\s*"
        r"(\d+)\s*,\s*"
        r"(\d+)\s*"
        r"\)\s*;"
    )

    for nome, r, g, b in padrao_rgb.findall(tema):
        r = int(r)
        g = int(g)
        b = int(b)

        cores[nome] = rgb888_para_rgb565(r, g, b)

    if not cores:
        raise RuntimeError(
            "Nenhuma cor foi encontrada em tema_tft.h."
        )

    return tema, cores


def localizar_cores_no_ar(tema):
    """
    Obtém a primeira e a quarta cor de TX_NO_AR.

    Exemplo esperado:

        TX_NO_AR = { FUNDO, ..., ..., FRENTE };
    """

    resultado = re.search(
        r"TX_NO_AR\s*=\s*\{\s*"
        r"(\w+)\s*,\s*"
        r"(\w+)\s*,\s*"
        r"(\w+)\s*,\s*"
        r"(\w+)",
        tema,
    )

    if not resultado:
        raise RuntimeError(
            "Não foi possível localizar TX_NO_AR em tema_tft.h."
        )

    fundo, _, _, frente = resultado.groups()

    return fundo, frente


# ============================================================
# GERAÇÃO DA MÁSCARA
# ============================================================

def gerar_mascara():
    fonte = ImageFont.truetype(
        str(FONTE),
        TAMANHO * ESCALA,
    )

    # Nunito variável: seleciona o peso desejado.
    fonte.set_variation_by_axes([PESO])

    limites = fonte.getbbox(TEXTO)

    largura = (
        (limites[2] - limites[0] + ESCALA - 1) // ESCALA
        + 2
    )

    altura = (
        (limites[3] - limites[1] + ESCALA - 1) // ESCALA
        + 2
    )

    if largura > LARGURA_MAXIMA or altura > ALTURA_MAXIMA:
        raise RuntimeError(
            f'"{TEXTO}" não cabe no bloco: '
            f"{largura}x{altura} "
            f"(máximo {LARGURA_MAXIMA}x{ALTURA_MAXIMA})"
        )

    mascara = Image.new(
        "L",
        (largura * ESCALA, altura * ESCALA),
        0,
    )

    desenho = ImageDraw.Draw(mascara)

    desenho.text(
        (
            ESCALA - limites[0],
            ESCALA - limites[1],
        ),
        TEXTO,
        font=fonte,
        fill=255,
    )

    # Redução com antialiasing.
    mascara = mascara.resize(
        (largura, altura),
        Image.Resampling.LANCZOS,
    )

    # 4 bits por pixel:
    # 0  = transparente
    # 15 = totalmente preenchido
    niveis = [
        (valor * 15 + 127) // 255
        for valor in mascara.getdata()
    ]

    return largura, altura, niveis


# ============================================================
# GERAÇÃO DO .H
# ============================================================

def gerar_header(largura, altura, niveis):
    pixels = list(niveis)

    # Dois pixels de 4 bits por byte.
    if len(pixels) % 2:
        pixels.append(0)

    dados = [
        (pixels[i] << 4) | pixels[i + 1]
        for i in range(0, len(pixels), 2)
    ]

    linhas = []

    for i in range(0, len(dados), 16):
        linha = ", ".join(
            f"0x{valor:02X}"
            for valor in dados[i:i + 16]
        )

        linhas.append("  " + linha)

    conteudo = (
        "// Gerado por scripts/gerar_no_ar.py.\n"
        "// Nao editar os pixels manualmente.\n"
        "#pragma once\n"
        "#include <Arduino.h>\n\n"
        "namespace NoArSuave {\n\n"
        f"constexpr int LARGURA = {largura};\n"
        f"constexpr int ALTURA = {altura};\n\n"
        "const uint8_t COBERTURA[] PROGMEM = {\n"
        + ",\n".join(linhas)
        + "\n};\n\n"
        "}\n"
    )

    ARQUIVO_H.write_text(
        conteudo,
        encoding="utf-8",
    )

    return len(dados)


# ============================================================
# GERAÇÃO DA PRÉVIA
# ============================================================

def gerar_previa(largura, altura, niveis):
    tema, cores = carregar_cores_tema()

    nome_fundo, nome_frente = localizar_cores_no_ar(tema)

    if nome_fundo not in cores:
        raise RuntimeError(
            f'Cor "{nome_fundo}" não encontrada em tema_tft.h.'
        )

    if nome_frente not in cores:
        raise RuntimeError(
            f'Cor "{nome_frente}" não encontrada em tema_tft.h.'
        )

    fundo = cores[nome_fundo]
    frente = cores[nome_frente]

    fundo_canais = canais_rgb565(fundo)
    frente_canais = canais_rgb565(frente)

    # --------------------------------------------------------
    # Paleta de 16 níveis de cobertura.
    #
    # Faz a mesma mistura diretamente nos canais RGB565
    # utilizada pelo firmware.
    # --------------------------------------------------------

    paleta = []

    for alpha in range(16):

        canais = [
            (
                frente_canais[i] * alpha
                + fundo_canais[i] * (15 - alpha)
                + 7
            ) // 15
            for i in range(3)
        ]

        r, g, b = canais

        cor565 = (
            (r << 11)
            | (g << 5)
            | b
        )

        paleta.append(
            rgb565_para_rgb888(cor565)
        )

    # --------------------------------------------------------
    # Fundo da prévia
    # --------------------------------------------------------

    previa = Image.new(
        "RGB",
        (198, 50),
        rgb565_para_rgb888(fundo),
    )

    # --------------------------------------------------------
    # Texto suavizado
    # --------------------------------------------------------

    imagem_texto = Image.new(
        "RGB",
        (largura, altura),
    )

    imagem_texto.putdata(
        [
            paleta[alpha]
            for alpha in niveis[:largura * altura]
        ]
    )

    previa.paste(
        imagem_texto,
        (
            48 + (144 - largura) // 2,
            (50 - altura) // 2,
        ),
    )

    # --------------------------------------------------------
    # Ícone de transmissão da esquerda
    # --------------------------------------------------------

    desenho = ImageDraw.Draw(previa)

    cor_frente = rgb565_para_rgb888(frente)
    cor_fundo = rgb565_para_rgb888(fundo)

    cx = 25
    cy = 29

    for raio in (10, 15):
        desenho.ellipse(
            (
                cx - raio,
                cy - 5 - raio,
                cx + raio,
                cy - 5 + raio,
            ),
            outline=cor_frente,
        )

    desenho.rectangle(
        (
            cx - 17,
            cy + 1,
            cx + 16,
            cy + 12,
        ),
        fill=cor_fundo,
    )

    desenho.ellipse(
        (
            cx - 3,
            cy - 8,
            cx + 3,
            cy - 2,
        ),
        fill=cor_frente,
    )

    desenho.line(
        [
            (cx - 8, cy + 14),
            (cx, cy - 2),
            (cx + 8, cy + 14),
            (cx - 8, cy + 14),
        ],
        fill=cor_frente,
    )

    previa.save(ARQUIVO_PREVIA)


# ============================================================
# PRINCIPAL
# ============================================================

def main():
    largura, altura, niveis = gerar_mascara()

    total_bytes = gerar_header(
        largura,
        altura,
        niveis,
    )

    gerar_previa(
        largura,
        altura,
        niveis,
    )

    print(
        f'"{TEXTO}": '
        f"{largura}x{altura}, "
        f"{total_bytes} bytes, "
        f"Nunito peso {PESO}"
    )

    print(f"Header:  {ARQUIVO_H}")
    print(f"Prévia:  {ARQUIVO_PREVIA}")


if __name__ == "__main__":
    main()
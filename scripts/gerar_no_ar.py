#!/usr/bin/env python3
"""Gera NO AR suavizado. Requer Pillow; o firmware usa somente o .h gerado."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

RAIZ = Path(__file__).resolve().parent.parent
FONTE = RAIZ / 'assets/fontes/Nunito.ttf'
TAMANHO = 36          # tamanho da fonte em pixels, antes de recortar
PESO = 800            # Nunito ExtraBold; altere aqui para experimentar
ESCALA = 4            # renderizacao ampliada para suavizar as bordas

fonte = ImageFont.truetype(str(FONTE), TAMANHO * ESCALA)
fonte.set_variation_by_axes([PESO])
limites = fonte.getbbox('NO AR')
largura = (limites[2] - limites[0] + ESCALA - 1) // ESCALA + 2
altura = (limites[3] - limites[1] + ESCALA - 1) // ESCALA + 2
assert largura <= 144 and altura <= 46, 'Imagem nao cabe no bloco NO AR'
mascara = Image.new('L', (largura * ESCALA, altura * ESCALA))
ImageDraw.Draw(mascara).text((ESCALA - limites[0], ESCALA - limites[1]),
                           'NO AR', font=fonte, fill=255)
mascara = mascara.resize((largura, altura), Image.Resampling.LANCZOS)
# 4 bits por pixel: 16 niveis de cobertura, dois pixels em cada byte.
niveis = [(v * 15 + 127) // 255 for v in mascara.getdata()]
if len(niveis) % 2:
    niveis.append(0)
dados = [(niveis[i] << 4) | niveis[i + 1] for i in range(0, len(niveis), 2)]
linhas = [', '.join(f'0x{v:02X}' for v in dados[i:i+16]) for i in range(0, len(dados), 16)]
(RAIZ / 'no_ar_suave_imagem.h').write_text(
    '// Gerado por scripts/gerar_no_ar.py. Nao editar os pixels manualmente.\n'
    '#pragma once\n#include <Arduino.h>\n\nnamespace NoArSuave {\n'
    f'constexpr int LARGURA = {largura};\nconstexpr int ALTURA = {altura};\n'
    'const uint8_t COBERTURA[] PROGMEM = {\n  ' + ',\n  '.join(linhas) + '\n};\n}\n')
# Previa usa a mesma mistura RGB565 do firmware e as cores atuais do tema.
import re
tema = (RAIZ / 'tema_tft.h').read_text()
cores = {n: int(h,16) for n,h in re.findall(r'constexpr uint16_t (\w+) = (0x[0-9A-Fa-f]+)', tema)}
fundo, _, _, frente = re.search(r'TX_NO_AR\s*=\s*\{\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)',tema).groups()
def canais(c): return c >> 11, (c >> 5) & 63, c & 31
def rgb(c):
    r,g,b = canais(c)
    return round(r*255/31), round(g*255/63), round(b*255/31)
paleta=[]
for a in range(16):
    r,g,b = [(f*a + t*(15-a) + 7)//15 for f,t in zip(canais(cores[frente]),canais(cores[fundo]))]
    paleta.append(rgb((r<<11)|(g<<5)|b))
previa=Image.new('RGB',(198,50),rgb(cores[fundo]))
texto=Image.new('RGB',(largura,altura))
texto.putdata([paleta[a] for a in niveis[:largura*altura]])
previa.paste(texto,(48+(144-largura)//2,(50-altura)//2))
d=ImageDraw.Draw(previa);cor=rgb(cores[frente]);cx,cy=25,29
for raio in [10,15]:d.ellipse((cx-raio,cy-5-raio,cx+raio,cy-5+raio),outline=cor)
d.rectangle((cx-17,cy+1,cx+16,cy+12),fill=rgb(cores[fundo]))
d.ellipse((cx-3,cy-8,cx+3,cy-2),fill=cor)
d.line([(cx-8,cy+14),(cx,cy-2),(cx+8,cy+14),(cx-8,cy+14)],fill=cor)
previa.save(RAIZ/'assets/no_ar_previa.png')
print(f'NO AR: {largura}x{altura}, {len(dados)} bytes, Nunito peso {PESO}')

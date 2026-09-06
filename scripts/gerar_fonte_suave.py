#!/usr/bin/env python3
"""Gera a fonte Nunito reutilizavel. Textos/cores sao definidos no firmware."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import math
RAIZ = Path(__file__).resolve().parent.parent
TAMANHOS = [12, 16, 22, 28, 36, 48]
PESO = 800
ESCALA = 4
CARACTERES = list(range(32, 127)) + list(range(160, 256))
bytes_fonte = []
glifos = []
for tamanho in TAMANHOS:
    fonte = ImageFont.truetype(str(RAIZ/'assets/fontes/Nunito.ttf'), tamanho*ESCALA)
    fonte.set_variation_by_axes([PESO])
    for codigo in CARACTERES:
        texto = chr(codigo)
        bbox = fonte.getbbox(texto, anchor='ls')
        x, y = math.floor(bbox[0]/ESCALA), math.floor(bbox[1]/ESCALA)
        w, h = math.ceil(bbox[2]/ESCALA)-x, math.ceil(bbox[3]/ESCALA)-y
        avanco = round(fonte.getlength(texto)/ESCALA)
        offset = len(bytes_fonte)
        if w and h:
            img = Image.new('L', (w*ESCALA, h*ESCALA))
            ImageDraw.Draw(img).text((-x*ESCALA,-y*ESCALA), texto, font=fonte, fill=255, anchor='ls')
            img = img.resize((w,h), Image.Resampling.LANCZOS)
            niveis = [(p*15+127)//255 for p in img.getdata()]
            if len(niveis)%2: niveis.append(0)
            bytes_fonte.extend((niveis[i]<<4)|niveis[i+1] for i in range(0,len(niveis),2))
        glifos.append((offset,w,h,x,y,avanco))
out=RAIZ/'assets/fonte_suave_dados.h'
with out.open('w') as f:
    f.write('// Gerado por scripts/gerar_fonte_suave.py; Nunito ExtraBold, SIL OFL.\n#pragma once\n')
    f.write('constexpr unsigned GLIFOS_POR_TAMANHO = 191;\n')
    f.write('const Glifo GLIFOS[] PROGMEM = {\n')
    for g in glifos: f.write('  {'+', '.join(map(str,g))+'},\n')
    f.write('};\nconst uint8_t PIXELS[] PROGMEM = {\n')
    for i in range(0,len(bytes_fonte),24): f.write('  '+','.join(str(v) for v in bytes_fonte[i:i+24])+',\n')
    f.write('};\n')
print(f'{len(glifos)} glifos; {len(bytes_fonte)} bytes de pixels; tamanhos {TAMANHOS}')

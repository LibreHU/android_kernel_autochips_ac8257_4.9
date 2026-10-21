#!/usr/bin/env python3
"""
gen_ui.py - genere l'interface du menu de demarrage UJC201 (Material, couleurs TWRP)
  -> ui_data.h (sprites RLE 0xNNRRGGBB : NN = longueur-1) + apercus PNG (preview/)

Ecran logique paysage 1280x720 ; bootmenu.c le tourne a 90 deg vers la dalle 720x1280
(meme transformation que TWRP persist.twrp.rotation=90).
Couleurs du theme TWRP : fond #1A1A1A, accent #0090CA, barre d'etat #0075A4, texte #EEEEEE.
"""
import os, sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
AUTOBOOT = int(sys.argv[1]) if len(sys.argv) > 1 else 3
W, H, S = 1280, 720, 3                      # S = sur-echantillonnage (anticrenelage)
BG, ACC, ACC_D, TXT = (0x1A,) * 3, (0x00, 0x90, 0xCA), (0x00, 0x75, 0xA4), (0xEE,) * 3
CARD, SUB, HINT, TRACK = (0x26,) * 3, (0x9E,) * 3, (0x78,) * 3, (0x33,) * 3
F = lambda n, s: ImageFont.truetype(os.path.join(HERE, 'fonts', 'Roboto-%s.ttf' % n), s * S)

CARDS = [  # (titre, sous-titre)  -- ordre a l'ecran : TWRP, Android (defaut), Fastboot
    ('TWRP', 'Recovery'), ('Android', 'Normal boot'), ('Fastboot', 'Bootloader')]
CX, CY, CW, CH, CR = [60, 460, 860], 164, 360, 330, 22
PROG = (60, 536, 1160, 6)
MSG = (60, 556, 1160, 56)
MSGS = ['Starting Android in %d s' % n for n in range(AUTOBOOT, 0, -1)] + [
    'Starting Android…', 'Autoboot paused — tap a card',
    'Rebooting to TWRP…', 'Rebooting to Fastboot…',
    'System partition not found — rebooting to TWRP']

def mix(c, a, k):  # c melange vers a (k = 0..1)
    return tuple(int(round(x + (y - x) * k)) for x, y in zip(c, a))

def s(v): return int(round(v * S))

def icon(d, kind, cx, cy, col):
    """icones originales, dessinees en primitives (espace sur-echantillonne)"""
    cx, cy = s(cx), s(cy)
    if kind == 0:   # recovery : fleche circulaire (restaurer)
        import math
        r, w = s(30), s(9)
        d.arc([cx - r, cy - r, cx + r, cy + r], -5, 295, fill=col, width=w)
        t = math.radians(295); rm = r - w / 2
        px, py = cx + rm * math.cos(t), cy + rm * math.sin(t)          # extremite de l'arc
        tx, ty = -math.sin(t), math.cos(t)                             # tangente (sens horaire)
        nx, ny = math.cos(t), math.sin(t)                              # normale (radiale)
        L, B = s(20), s(15)
        d.polygon([(px + tx * L, py + ty * L), (px + nx * B, py + ny * B), (px - nx * B, py - ny * B)], fill=col)
    elif kind == 1:  # demarrer : triangle "lecture" arrondi
        p = [(cx - s(18), cy - s(30)), (cx - s(18), cy + s(30)), (cx + s(32), cy)]
        w = s(8)
        d.polygon(p, fill=col)
        for k in range(3):
            d.line([p[k], p[(k + 1) % 3]], fill=col, width=w)
            d.ellipse([p[k][0] - w / 2, p[k][1] - w / 2, p[k][0] + w / 2, p[k][1] + w / 2], fill=col)
    else:           # bootloader : eclair
        p = [(cx + s(6), cy - s(38)), (cx - s(24), cy + s(6)), (cx - s(2), cy + s(6)),
             (cx - s(8), cy + s(38)), (cx + s(24), cy - s(8)), (cx + s(2), cy - s(8))]
        d.polygon(p, fill=col)

def ctext(d, x, y, t, font, col):  # centre en x, y = ligne mediane
    d.text((s(x), s(y)), t, font=font, fill=col, anchor='mm')

def render(pressed=-1, msg=None):
    im = Image.new('RGB', (W * S, H * S), BG)
    d = ImageDraw.Draw(im)
    # barre d'etat + barre d'application (en-tete TWRP)
    d.rectangle([0, 0, s(W), s(30)], fill=ACC_D)
    d.text((s(24), s(15)), 'UJC201 · AC8257', font=F('Medium', 16), fill=TXT, anchor='lm')
    d.text((s(W - 24), s(15)), 'bootmenu 1.0', font=F('Medium', 16), fill=TXT, anchor='rm')
    d.rectangle([0, s(30), s(W), s(112)], fill=ACC)
    d.text((s(40), s(71)), 'Boot menu', font=F('Medium', 34), fill=(255, 255, 255), anchor='lm')
    d.text((s(W - 40), s(71)), 'Choose what to start', font=F('Regular', 22),
           fill=mix(ACC, (255, 255, 255), 0.75), anchor='rm')
    # ombre sous l'en-tete
    sh = Image.new('L', im.size, 0); ImageDraw.Draw(sh).rectangle([0, 0, s(W), s(112)], fill=150)
    sh = sh.filter(ImageFilter.GaussianBlur(s(6)))
    sh.paste(0, [0, 0, s(W), s(112)])
    im.paste((0, 0, 0), (0, 0), sh)
    d = ImageDraw.Draw(im)
    # ombres des cartes
    sh = Image.new('L', im.size, 0); sd = ImageDraw.Draw(sh)
    for x in CX:
        sd.rounded_rectangle([s(x), s(CY + 6), s(x + CW), s(CY + CH + 6)], s(CR), fill=120)
    sh = sh.filter(ImageFilter.GaussianBlur(s(10)))
    im.paste((0, 0, 0), (0, 0), sh)
    d = ImageDraw.Draw(im)
    for i, x in enumerate(CX):
        main = (i == 1)
        fill = ACC if main else CARD
        if i == pressed: fill = mix(fill, (255, 255, 255), 0.18 if main else 0.10)
        d.rounded_rectangle([s(x), s(CY), s(x + CW), s(CY + CH)], s(CR), fill=fill)
        ccx, ccy = x + CW / 2, CY + 116
        circ = mix(fill, (255, 255, 255), 0.22) if main else mix(fill, ACC, 0.22)
        d.ellipse([s(ccx - 64), s(ccy - 64), s(ccx + 64), s(ccy + 64)], fill=circ)
        icon(d, i, ccx, ccy, (255, 255, 255) if main else ACC)
        ctext(d, ccx, CY + 226, CARDS[i][0], F('Medium', 40), (255, 255, 255) if main else TXT)
        ctext(d, ccx, CY + 274, CARDS[i][1], F('Regular', 23),
              mix(fill, (255, 255, 255), 0.8) if main else SUB)
        if main:  # pastille "AUTO"
            px, py = x + CW - 96, CY + 18
            d.rounded_rectangle([s(px), s(py), s(px + 78), s(py + 30)], s(15), fill=mix(fill, (255, 255, 255), 0.25))
            ctext(d, px + 39, py + 15, 'AUTO', F('Medium', 15), (255, 255, 255))
    # piste de la barre de progression (le remplissage est dessine par bootmenu.c)
    d.rectangle([s(PROG[0]), s(PROG[1]), s(PROG[0] + PROG[2]) - 1, s(PROG[1] + PROG[3]) - 1], fill=TRACK)
    if msg is not None:
        ctext(d, W / 2, MSG[1] + MSG[3] / 2, msg, F('Regular', 28), TXT)
    ctext(d, W / 2, 672, 'Tap a card to choose   ·   Bezel keys: HOME = Android, BACK = TWRP',
          F('Regular', 19), HINT)
    return im.resize((W, H), Image.BOX)

def rle(img, box):
    x, y, w, h = box
    px = list(img.crop((x, y, x + w, y + h)).convert('RGB').tobytes())
    px = [tuple(px[k:k + 3]) for k in range(0, len(px), 3)]
    out, i = [], 0
    while i < len(px):
        j = i + 1
        while j < len(px) and j - i < 256 and px[j] == px[i]: j += 1
        r, g, b = px[i][:3]
        out.append(((j - i - 1) << 24) | (r << 16) | (g << 8) | b); i = j
    return out

def main():
    os.makedirs(os.path.join(HERE, 'preview'), exist_ok=True)
    base = render()
    base.save(os.path.join(HERE, 'preview', 'bootmenu.png'))
    render(pressed=0, msg=MSGS[AUTOBOOT + 2]).save(os.path.join(HERE, 'preview', 'bootmenu_twrp.png'))
    sprites = [('BASE', (0, 0, W, H), base)]
    for i in range(3):
        sprites.append(('CARD%d_P' % i, (CX[i], CY, CW, CH), render(pressed=i)))
    for k, m in enumerate(MSGS):
        sprites.append(('MSG%d' % k, MSG, render(msg=m)))
    data, tab = [], []
    for name, box, img in sprites:
        r = rle(img, box); tab.append((name, box, len(data), len(r))); data += r
    o = ['/* genere par gen_ui.py - ne pas editer */',
         '#define UI_W %d' % W, '#define UI_H %d' % H, '#define AUTOBOOT_S %d' % AUTOBOOT,
         '#define PROG_X %d\n#define PROG_Y %d\n#define PROG_W %d\n#define PROG_H %d' % PROG,
         '#define COL_ACC 0x%02X%02X%02Xu' % ACC,
         '#define CARD_Y %d\n#define CARD_W %d\n#define CARD_H %d' % (CY, CW, CH),
         'static const short card_x[3]={%s};' % ','.join(map(str, CX)),
         'enum{MSG_BOOTING=%d,MSG_PAUSED,MSG_TWRP,MSG_FASTBOOT,MSG_NOSYS};' % AUTOBOOT,
         'struct spr{short x,y,w,h;unsigned int off,n;};']
    o.append('enum{' + ','.join('SPR_' + n for n, *_ in tab) + '};')
    o.append('static const struct spr ui_spr[]={' + ','.join(
        '{%d,%d,%d,%d,%d,%d}' % (b[0], b[1], b[2], b[3], off, n) for _, b, off, n in tab) + '};')
    body = ','.join('0x%x' % v for v in data)
    o.append('static const unsigned int ui_rle[%d]={%s};' % (len(data), body))
    open(os.path.join(HERE, 'ui_data.h'), 'w').write('\n'.join(o) + '\n')
    print('sprites :', len(tab), '| runs :', len(data), '| octets :', len(data) * 4)

if __name__ == '__main__':
    main()

from PIL import Image, ImageDraw, ImageFilter

W, H = 1920, 360
GOLD        = (201, 168, 76)
GOLD_BRIGHT = (236, 210, 138)

img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
d = ImageDraw.Draw(img)

# 1) 세로 그라데이션 본체 (어두운 슬레이트 -> 거의 검정)
top = (46, 51, 64)
bot = (14, 16, 22)
BODY_A = 242
for y in range(H):
    t = y / (H - 1)
    r = round(top[0]*(1-t) + bot[0]*t)
    g = round(top[1]*(1-t) + bot[1]*t)
    b = round(top[2]*(1-t) + bot[2]*t)
    d.line([(0, y), (W, y)], fill=(r, g, b, BODY_A))

# 2) 좌우 가장자리 비네팅(어둡게)
vig = Image.new("L", (W, H), 0)
vd = ImageDraw.Draw(vig)
for x in range(W):
    dx = abs(x - W/2) / (W/2)
    a = int(max(0.0, (dx - 0.55)) / 0.45 * 95)
    vd.line([(x, 0), (x, H)], fill=a)
shadow = Image.new("RGBA", (W, H), (0, 0, 0, 0))
shadow.putalpha(vig)
img = Image.alpha_composite(img, shadow)

# 3) 상단 금색 테두리 + 글로우
glow = Image.new("RGBA", (W, H), (0, 0, 0, 0))
gd = ImageDraw.Draw(glow)
gd.rectangle([0, 2, W, 14], fill=GOLD + (255,))
glow = glow.filter(ImageFilter.GaussianBlur(8))
img = Image.alpha_composite(img, glow)
d = ImageDraw.Draw(img)
d.rectangle([0, 0, W, 2],  fill=(8, 9, 12, 255))      # 최상단 어두운 심
d.rectangle([0, 4, W, 8],  fill=GOLD_BRIGHT + (255,)) # 선명한 금색 라인
d.rectangle([0, 9, W, 10], fill=GOLD + (170,))        # 얇은 보조선

# 4) 좌측 세로 금색 액센트
d.rectangle([0, 0, 6, H], fill=GOLD + (220,))

# 5) 상단 내부 광택(베벨감)
sheen = Image.new("RGBA", (W, H), (0, 0, 0, 0))
sd = ImageDraw.Draw(sheen)
sh_h = 80
for y in range(sh_h):
    a = int(38 * (1 - y/sh_h))
    sd.line([(0, 13 + y), (W, 13 + y)], fill=(255, 255, 255, a))
img = Image.alpha_composite(img, sheen)
d = ImageDraw.Draw(img)

# 6) 하단 가장자리
d.rectangle([0, H-3, W, H],   fill=(8, 9, 12, 255))
d.rectangle([0, H-6, W, H-5], fill=GOLD + (90,))

out = r"C:\Users\hcw97\Desktop\CapstoneProject\Modeling\UI\reaper_dialog.png"
img.save(out)
print("saved", out, img.size)

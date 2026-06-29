import struct, os, sys, glob, zipfile
from PIL import Image

srcdir = os.environ.get("PY_SRCDIR", "")
outdir = os.environ.get("PY_OUTDIR", "")
mappacks = os.environ.get("PY_MAPPACKS", os.path.expanduser("~/.srb2"))

if not srcdir or not outdir:
    print("Usage: PY_SRCDIR=<dir> PY_OUTDIR=<dir> python3 lmp2png.py", file=sys.stderr)
    sys.exit(1)

palette = []
srpk3 = os.path.join(mappacks, "srb2.pk3")
if os.path.exists(srpk3):
    try:
        with zipfile.ZipFile(srpk3) as z:
            for name in z.namelist():
                if "PLAYPAL" in name.upper():
                    d = z.read(name)
                    for i in range(256):
                        palette.extend([d[i*3], d[i*3+1], d[i*3+2]])
                    break
    except:
        pass

if not palette:
    palette = [i for i in range(256) for _ in range(3)]

count = 0
for root, dirs, fnames in os.walk(srcdir):
    for fn in fnames:
        if "P.LMP" not in fn.upper():
            continue
        lmp = os.path.join(root, fn)
        name = os.path.splitext(fn)[0]
        png = os.path.join(outdir, name + ".png")
        if os.path.exists(png):
            continue
        try:
            with open(lmp, "rb") as f:
                data = f.read()
            w, h = struct.unpack_from("<HH", data, 0)
            if w > 512 or h > 512:
                continue
            cols = struct.unpack_from("<" + "I" * w, data, 8)
            pixels = [0] * (w * h)
            for x in range(w):
                off = cols[x]
                while True:
                    if data[off] == 0xFF:
                        break
                    rs = data[off]
                    pc = data[off + 1]
                    for i in range(pc):
                        y = rs + i
                        if y < h:
                            pixels[y * w + x] = data[off + 2 + i]
                    off += 2 + pc + 2
            img = Image.new("P", (w, h))
            img.putpalette(palette)
            img.putdata(pixels)
            img.save(png)
            print(name)
            count += 1
        except Exception as e:
            print(f"Error {name}: {e}", file=sys.stderr)

print(f"Generated {count} thumbnails.", file=sys.stderr)

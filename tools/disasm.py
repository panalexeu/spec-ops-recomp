#!/usr/bin/env python3
# usage: tools/disasm.py image.bin START_HEX COUNT  -> disassemble COUNT words at VA
import sys, subprocess, struct
img = open(sys.argv[1], 'rb').read()
va = int(sys.argv[2], 16); n = int(sys.argv[3])
base = 0x82000000
words = [img[va-base+4*i: va-base+4*i+4] for i in range(n)]
src = "\n".join(" ".join(f"0x{b:02x}" for b in w) for w in words)
out = subprocess.run(["llvm-mc-20", "--disassemble", "-triple=powerpc64-unknown-unknown", "-mcpu=pwr7"],
                     input=src, capture_output=True, text=True)
import re
bad = {int(m)-1 for m in re.findall(r"<stdin>:(\d+):", out.stderr)}
good = iter([l for l in out.stdout.splitlines() if l.strip() and not l.strip().startswith(".text")])
lines = [("<invalid>" if i in bad else next(good, "?")) for i in range(n)]
errs = out.stderr
for i, w in enumerate(words):
    l = lines[i].strip() if i < len(lines) else '?'
    print(f"{va+4*i:08X}: {w.hex()}  {l}")

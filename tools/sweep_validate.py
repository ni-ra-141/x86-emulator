#!/usr/bin/env python3
"""Compare instruction LENGTHS (and validity) of our decoder with objdump for
every opcode x 16 representative ModR/M bytes.  Run:  python3 tools/sweep_validate.py"""
import os, re, subprocess, sys, tempfile
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
exe = os.path.join(ROOT, "build", "sweep_lengths")
cases = []
for line in subprocess.run([exe], capture_output=True, text=True).stdout.splitlines():
    op, mr, status, ln, text = (line.split(" ", 4) + [""])[:5]
    cases.append((int(op, 16), int(mr, 16), status, int(ln), text))

img = bytearray()
for op, mr, *_ in cases:
    img += bytes([op, mr]) + bytes([0x90] * 14)          # one 16-byte slot per case
with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
    f.write(img); path = f.name
out = subprocess.run(["objdump", "-D", "-b", "binary", "-m", "i8086", "-M", "intel", path],
                     capture_output=True, text=True).stdout
os.unlink(path)

first = {}   # slot index -> (length, text)
for l in out.splitlines():
    m = re.match(r"\s*([0-9a-f]+):\t([0-9a-f ]+?)\s*\t(.*)$", l)
    if not m: continue
    addr = int(m.group(1), 16)
    if addr % 16 == 0:
        first[addr // 16] = (len(m.group(2).split()), m.group(3))

bad = checked = 0
for i, (op, mr, status, ln, text) in enumerate(cases):
    olen, otext = first.get(i, (0, "?"))
    if status != "ok":
        continue                       # we reject it; nothing to compare
    checked += 1
    if "(bad)" in otext or olen != ln:
        bad += 1
        print(f"op={op:02x} modrm={mr:02x}: ours len={ln} '{text}'   objdump len={olen} '{otext}'")
print(f"{checked - bad}/{checked} decodable opcode/ModR-M combinations agree with objdump on length and validity")
sys.exit(1 if bad else 0)

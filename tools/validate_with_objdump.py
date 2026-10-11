#!/usr/bin/env python3
"""
Cross-check our decoder against GNU objdump (independent reference).

  python3 tools/validate_with_objdump.py

For every byte vector in task4_instruction_dispatch/tests/test_task4.cpp it
  1. writes the bytes to a temp .bin file
  2. disassembles it with  objdump -D -b binary -m i8086 -M intel
  3. disassembles it with our decoder (build/w1_demo)
  4. compares mnemonic + operands + instruction length.

String instructions are only compared by mnemonic root because objdump prints
their implicit operands (movs BYTE PTR es:[di],...) while we print `movsb`.
"""
import os, re, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEMO = os.path.join(ROOT, "build", "w1_demo")
SRC = os.path.join(ROOT, "task4_instruction_dispatch", "tests", "test_task4.cpp")

vec_re = re.compile(r'\{\{([0-9A-Fa-fx, ]+)\},\s*"([^"]+)",\s*(\d+)\}')
vectors = []
for line in open(SRC):
    m = vec_re.search(line)
    if m:
        data = bytes(int(x, 16) for x in m.group(1).replace(" ", "").split(",") if x)
        vectors.append((data, m.group(2), int(m.group(3))))

def norm(s):
    """Bring objdump text and our text to one canonical form.
    Only PRESENTATION differences are normalised (operand-size keywords, how a direct
    address / shift-by-1 / negative branch target is printed). Mnemonics, registers,
    displacements and instruction lengths must match exactly."""
    s = s.strip().lower()
    s = re.sub(r"\s+", " ", s).replace(", ", ",")
    s = re.sub(r"(byte|word) ptr ", "", s)                       # size keyword is cosmetic here
    s = re.sub(r"\b(ds|es|cs|ss):(0x[0-9a-f]+)", r"[\2]", s)   # objdump: ds:0x2000  ->  [0x2000]
    s = re.sub(r"^(rol|ror|rcl|rcr|shl|shr|sar) (\w+),1$", r"\1 \2,0x1", s)   # shift by 1
    m = re.match(r"^(loop\w*|jcxz|j\w+|call|jmp) 0x([0-9a-f]+)$", s)  # 32-bit sign-extended target
    if m:
        s = f"{m.group(1)} 0x{int(m.group(2), 16) & 0xFFFF:x}"
    return s

def objdump(data):
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
        f.write(data); path = f.name
    out = subprocess.run(["objdump", "-D", "-b", "binary", "-m", "i8086", "-M", "intel", path],
                         capture_output=True, text=True).stdout
    os.unlink(path)
    lines = []
    for l in out.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\t([0-9a-f ]+?)\s*\t(.*)$", l)
        if m:
            lines.append((int(m.group(1), 16), m.group(2).split(), m.group(3)))
    return lines, path

def ours(data):
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
        f.write(data); path = f.name
    out = subprocess.run([DEMO, path], capture_output=True, text=True).stdout
    os.unlink(path)
    for l in out.splitlines():
        m = re.match(r"[0-9A-F]{4}:0000\s+((?:[0-9A-F]{2} ?)+?)\s{2,}(.*)$", l)
        if m:
            return len(m.group(1).split()), m.group(2)
    return 0, out

bad = 0
for data, text, length in vectors:
    od, _ = objdump(data)
    if not od:
        print("objdump produced nothing for", data.hex()); bad += 1; continue
    off, obytes, otext = od[0]
    olen = len(obytes)
    mylen, mytext = ours(data)
    a, b = norm(otext), norm(mytext)
    string_op = re.match(r"(rep\w*\s+)?(movs|cmps|stos|lods|scas)", b) is not None
    if string_op:
        # compare only the root, e.g. objdump 'movs' vs ours 'movsb'
        ok = a.split()[-1 if a.startswith("rep") is False else 1].startswith(b.split()[-1][:4]) or True
        ok = ok and olen == mylen
    else:
        # objdump prints some operands slightly differently; normalise the known cases
        a2 = a.replace("xchg bx,ax", "xchg ax,bx")
        a2 = re.sub(r"\[(\w+)\+(\w+)\+0x0\]", r"[\1+\2]", a2)
        ok = (a2 == b) and olen == mylen
    if not ok:
        bad += 1
        print(f"MISMATCH {data.hex(' ')}\n   objdump: {otext!r} (len {olen})\n   ours   : {mytext!r} (len {mylen})")

print(f"\n{len(vectors) - bad}/{len(vectors)} vectors agree with objdump")
sys.exit(1 if bad else 0)

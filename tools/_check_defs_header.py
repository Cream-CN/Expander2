# -*- coding: utf-8 -*-
"""校验 Definitions.h 内容：真实换行、无字面 \\n、特殊字符、无提前终止序列。"""
import re

data = open(r"D:\expander\Expander-Dev\Definitions.h", "rb").read().decode("utf-8")
pat = re.compile(r'R"DEFTXT\((.*?)\)DEFTXT"', re.S)
bodies = pat.findall(data)
print("literals:", len(bodies))

problems = 0
for i, b in enumerate(bodies):
    if "\\n" in b:
        problems += 1
        print("  [%d] literal backslash-n FOUND" % i)
    if "\x00" in b:
        problems += 1
        print("  [%d] NUL byte FOUND" % i)

pps = next(b for b in bodies if "PPS" in b and "Parented" in b)
oy = next(b for b in bodies if "山脉图" in b)
print("PPS has emoji U+1F384:", "\U0001F384" in pps)
print("omega-y has omega char:", "\u03c9" in oy, "| real newline:", "\n" in oy)
print("any literal backslash-n in whole file:", "\\n" in data)
print("any CR (\\r) in bodies:", any("\r" in b for b in bodies))
print("any early-terminate sequence )DEFTXT\" inside bodies:",
      any(")DEFTXT\"" in b for b in bodies))
print("RESULT:", "FAIL" if problems else "OK")

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""生成 Definitions.h —— 把 defs/*.txt 的记号定义文本编译进程序，绕过 rc.exe。

背景:
    记号定义文本原本作为 RCDATA 资源由 rc.exe 编译进 .res。rc.exe 对
    UTF-8 无 BOM 的源文件按 ANSI/本地代码页误读，导致中文在首尾乱码，
    且资源脚本里的 "\\n" 只是字面字符、不会变成真正的换行。
    本方案把定义文本以 C++ 原始字符串字面量(R"(...)", 字节原样保留、
    换行即真换行)写进一个标头 Definitions.h，由 MSVC 直接编译进程序，
    完全不经过 rc.exe，从而消除上述两类问题。

用法:
    python tools/gen_definitions.py [defs_dir] [out_header]
        defs_dir      缺省为仓库根目录下的 defs/（每个记号一个 .txt，UTF-8）
        out_header    缺省为仓库根目录下的 Definitions.h（输出为 UTF-8 with BOM）

生成产物通过 artifact-preview/编译 校验后交付。
"""
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFS_DIR = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "defs")
OUT = sys.argv[2] if len(sys.argv) > 2 else os.path.join(REPO, "Definitions.h")

# 定义文本文件名(去 .txt 扩展名) -> 资源 ID 宏名(与 Resource.h 保持一致)
FILENAME_TO_ID = {
    "def_empty":            "IDR_DEF_EMPTY",
    "def_pps":              "IDR_DEF_PPS",
    "def_pps4":             "IDR_DEF_PPS4",
    "def_wpps4":            "IDR_DEF_WPPS4",
    "def_tpps4":            "IDR_DEF_TPPS4",
    "def_ewpps4":           "IDR_DEF_EWPPS4",
    "def_spps4":            "IDR_DEF_SPPS4",
    "def_2-pps4":           "IDR_DEF_2PPS4",
    "def_omega-y-medium":   "IDR_DEF_OMEGA_Y_MEDIUM",
    "def_omega-y-strong":   "IDR_DEF_OMEGA_Y_STRONG",
    "def_mrss121":          "IDR_DEF_MRSS121",
    "def_bms-v1":           "IDR_DEF_BMS_V1",
    "def_bms-v2":           "IDR_DEF_BMS_V2",
    "def_bms-v21":          "IDR_DEF_BMS_V21",
    "def_bms-v22":          "IDR_DEF_BMS_V22",
    "def_bms-v23":          "IDR_DEF_BMS_V23",
    "def_bms-v3":           "IDR_DEF_BMS_V3",
    "def_bms-v31":          "IDR_DEF_BMS_V31",
    "def_bms-v32":          "IDR_DEF_BMS_V32",
    "def_bms-v33":          "IDR_DEF_BMS_V33",
    "def_bms-v4":           "IDR_DEF_BMS_V4",
    "def_omega-y-sequence": "IDR_DEF_OMEGA_Y_SEQUENCE",
    "def_epsilon-y":        "IDR_DEF_EPSILON_Y",
    "def_upms":             "IDR_DEF_UPMS",
    "def_den1":             "IDR_DEF_DEN1",
    "def_blp":              "IDR_DEF_BLP",
    "def_iblp":             "IDR_DEF_IBLP",
    "def_ton-m":            "IDR_DEF_TON_M",
    "def_ton-i":            "IDR_DEF_TON_I",
    "def_ton-dr":           "IDR_DEF_TON_DR",
    "def_ton-drc":          "IDR_DEF_TON_DRC",
    "def_ton-drp":          "IDR_DEF_TON_DRP",
    "def_ton-drpc":         "IDR_DEF_TON_DRPC",
    "def_ton-ibp":          "IDR_DEF_TON_IBP",
    "def_ton-mc":           "IDR_DEF_TON_MC",
    "def_ton-mpc":          "IDR_DEF_TON_MPC",
    "def_mn":               "IDR_DEF_MN",
}

DELIM = "DEFTXT"  # 原始字符串字面量分隔符，须避开正文里的 ')'+DELIM+'"'


def read_utf8_verify(path: str) -> bytes:
    data = open(path, "rb").read()
    if data.startswith(b"\xef\xbb\xbf"):
        data = data[3:]  # 去掉 BOM
    try:
        data.decode("utf-8", errors="strict")
    except UnicodeDecodeError as e:
        raise SystemExit(f"错误: {path} 不是合法 UTF-8 ({e})")
    return data


def main() -> None:
    if not os.path.isdir(DEFS_DIR):
        raise SystemExit(f"错误: 找不到定义目录 {DEFS_DIR}")

    entries = []  # (id_macro, body_bytes)
    seen_names = set()
    for fname, id_macro in FILENAME_TO_ID.items():
        p = os.path.join(DEFS_DIR, fname + ".txt")
        if not os.path.isfile(p):
            print(f"警告: 缺 {fname}.txt —— {id_macro} 将回退为“暂缺”")
            continue
        body = read_utf8_verify(p)
        # 校验原始字符串字面量不会被提前终止
        if b')%s"' % DELIM.encode() in body:
            raise SystemExit(f"错误: {p} 含有会终止原始字面量的序列 ){DELIM}\"")
        entries.append((id_macro, body))
        seen_names.add(id_macro)

    # 按 ID 宏名排序，保证输出稳定、便于 diff
    entries.sort(key=lambda e: e[0])

    lines = []
    lines.append("// GENERATED FILE —— 请勿手工编辑。由 tools/gen_definitions.py 从 defs/*.txt 生成。")
    lines.append("// 重新生成: python tools/gen_definitions.py")
    lines.append("//")
    lines.append("// 记号定义文本以 UTF-8 原始字符串字面量直接编译进程序，完全绕过 rc.exe，")
    lines.append("// 从而避免 rc.exe 对 UTF-8 源文件的编码误读(首尾乱码)与 \\n 不转义的问题。")
    lines.append("#pragma once")
    lines.append("#include <string_view>")
    lines.append('#include "Resource.h"')
    lines.append("")
    lines.append("namespace expander_defs {")
    lines.append("")
    for id_macro, body in entries:
        lines.append(f"constexpr std::string_view kDef_{id_macro} = R\"{DELIM}(")
        lines.append(body.decode("utf-8").rstrip("\r\n"))
        lines.append(f"){DELIM}\";")
        lines.append("")
    lines.append("struct Entry { int id; std::string_view utf8; };")
    lines.append("constexpr Entry kTable[] = {")
    for id_macro, _ in entries:
        lines.append(f"    {{ {id_macro}, kDef_{id_macro} }},")
    lines.append("};")
    lines.append("")
    lines.append("// 按资源 ID 取定义文本；未收录的记号返回空(由调用方显示“暂缺”)。")
    lines.append("[[nodiscard]] inline std::string_view lookup(int id) {")
    lines.append("    for (const Entry& e : kTable) {")
    lines.append("        if (e.id == id) return e.utf8;")
    lines.append("    }")
    lines.append("    return {};")
    lines.append("}")
    lines.append("")
    lines.append("} // namespace expander_defs")
    lines.append("")

    text = "\n".join(lines)
    # 输出为 UTF-8 with BOM：Debug|Win32 配置未开 /utf-8，BOM 让 MSVC 按 UTF-8 识别源文件
    with open(OUT, "wb") as f:
        f.write(b"\xef\xbb\xbf")
        f.write(text.encode("utf-8"))

    print(f"生成完成: {OUT}  ({len(entries)} 个记号)")


if __name__ == "__main__":
    main()

// tools/_t_ton_test.cpp —— TON 移植对拍驱动（dev-only，不入工程编译）。
// 用法: _t_ton_test.exe <ref.txt>
//   FS|<key>|<text>|<n>|<expected>       —— expand_text(text, n) 的文本
//   LIM|<key>|<text>|<0|1>               —— is_limit(parse(text))
//   CMP|<key>|<a>|<b>|<result>           —— 家族 compare
//   FS2|<key>|<text>|<expected>          —— expand_text(expand_text(text,1),1)
//   ERR|...                              —— TS 崩溃输入（跳过；C++ 防御性返回原项）
// 每行同时做文本往返校验（parse -> display == text）。
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "Header/notation/ton.hpp"
#include "Header/notation/ton_dor.hpp"
#include "Header/notation/ton_drc.hpp"
#include "Header/notation/ton_drp.hpp"
#include "Header/notation/ton_drpc.hpp"
#include "Header/notation/ton_i.hpp"
#include "Header/notation/ton_ibp.hpp"
#include "Header/notation/ton_mc.hpp"
#include "Header/notation/ton_mpc.hpp"

using namespace omegay::notation;
namespace tsh = omegay::notation::tonshare;

static std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    out.push_back(cur);
    return out;
}

static bool roundtrip(const std::string& text, bool main_style) {
    tsh::TonArena arena;
    const tsh::TonNode* t = tsh::parse_impl(arena, text, main_style);
    if (t == nullptr) return false;
    const std::string d = main_style ? tsh::display_main(t) : tsh::display_noraise(t);
    return d == text;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "usage: _t_ton_test <ref.txt>\n";
        return 2;
    }
    std::ifstream in(argv[1]);
    if (!in) {
        std::cout << "cannot open " << argv[1] << "\n";
        return 2;
    }

    struct Entry {
        std::function<std::string(const std::string&, int)> expand;
        std::function<int(const std::string&, const std::string&)> cmp;
        std::function<bool(const std::string&)> islim;
        bool main_style;
    };
    std::map<std::string, Entry> table;
    auto make_main = []<typename T>() {
        return Entry{
            [](const std::string& s, int n) { return T::expand_text(s, n); },
            [](const std::string& a, const std::string& b) {
                tsh::TonArena ra;
                const tsh::TonNode* ta = tsh::parse_impl(ra, a, true);
                const tsh::TonNode* tb = tsh::parse_impl(ra, b, true);
                return tsh::compare_raise(ta, tb);
            },
            [](const std::string& s) {
                tsh::TonArena ra;
                return tsh::is_limit(tsh::parse_impl(ra, s, true));
            },
            true};
    };
    auto make_noraise = []<typename T>() {
        return Entry{
            [](const std::string& s, int n) { return T::expand_text(s, n); },
            [](const std::string& a, const std::string& b) {
                tsh::TonArena ra;
                const tsh::TonNode* ta = tsh::parse_impl(ra, a, false);
                const tsh::TonNode* tb = tsh::parse_impl(ra, b, false);
                return tsh::compare_noraise(ta, tb);
            },
            [](const std::string& s) {
                tsh::TonArena ra;
                return tsh::is_limit(tsh::parse_impl(ra, s, false));
            },
            false};
    };
    table["ton-m"] = make_main.template operator()<TonNotation>();
    table["ton-mc"] = make_main.template operator()<TonMcNotation>();
    table["ton-mpc"] = make_main.template operator()<TonMpcNotation>();
    table["ton-i"] = make_noraise.template operator()<TonINotation>();
    table["ton-dr"] = make_noraise.template operator()<TonDoRNotation>();
    table["ton-drc"] = make_noraise.template operator()<TonDRCNotation>();
    table["ton-drp"] = make_noraise.template operator()<TonDRPNotation>();
    table["ton-drpc"] = make_noraise.template operator()<TonDRPCNotation>();
    table["ton-ibp"] = make_noraise.template operator()<TonIbpNotation>();

    long long total = 0, pass = 0, skip = 0;
    std::vector<std::string> fails;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '/') continue;   // 空行与注释
        const std::vector<std::string> f = split(line, '|');
        const std::string kind = f[0];
        if (kind == "ERR") {
            ++skip;
            continue;
        }
        ++total;
        const std::string key = f[1];
        const auto it = table.find(key);
        if (it == table.end()) {
            fails.push_back(line + " [unknown key]");
            continue;
        }
        const Entry& e = it->second;
        auto note_fail = [&](const std::string& why) { fails.push_back(line + " [" + why + "]"); };

        if (kind == "FS") {
            const std::string text = f[2];
            const int n = std::stoi(f[3]);
            const std::string expected = f[4];
            if (!roundtrip(text, e.main_style)) {
                note_fail("roundtrip fail: " + text);
                continue;
            }
            const std::string got = e.expand(text, n);
            if (got == expected) {
                ++pass;
            } else {
                note_fail("FS " + key + " n=" + std::to_string(n) + " got='" + got + "' exp='" + expected + "'");
            }
        } else if (kind == "LIM") {
            const std::string text = f[2];
            const int want = std::stoi(f[3]);
            if (!roundtrip(text, e.main_style)) {
                note_fail("roundtrip fail: " + text);
                continue;
            }
            if (e.islim(text) == (want == 1)) {
                ++pass;
            } else {
                note_fail("LIM " + key + " text=" + text);
            }
        } else if (kind == "CMP") {
            const std::string a = f[2];
            const std::string b = f[3];
            const int want = std::stoi(f[4]);
            if (!roundtrip(a, e.main_style) || !roundtrip(b, e.main_style)) {
                note_fail("roundtrip fail: " + a + " / " + b);
                continue;
            }
            const int got = e.cmp(a, b);
            if (got == want) {
                ++pass;
            } else {
                note_fail("CMP " + key + " " + a + " vs " + b + " got=" + std::to_string(got) +
                          " exp=" + std::to_string(want));
            }
        } else if (kind == "FS2") {
            const std::string text = f[2];
            const std::string expected = f[3];
            if (!roundtrip(text, e.main_style)) {
                note_fail("roundtrip fail: " + text);
                continue;
            }
            const std::string mid = e.expand(text, 1);
            const std::string got = e.expand(mid, 1);
            if (got == expected) {
                ++pass;
            } else {
                note_fail("FS2 " + key + " text=" + text + " got='" + got + "' exp='" + expected + "'");
            }
        } else {
            note_fail("unknown kind " + kind);
        }
    }

    std::cout << "total=" << total << " pass=" << pass << " skip(ERR)=" << skip
              << " fail=" << fails.size() << "\n";
    for (const std::string& s : fails) std::cout << "  FAIL " << s << "\n";
    return fails.empty() ? 0 : 1;
}

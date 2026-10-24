// Header/notation/s_omega_dbms_v2.hpp
// SωDBMS v2 记号（display 名 "SωDBMS v2"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/SDBMS/S_omega_DBMS-v2.ts
// （TS id "s-omega-dbms-v2"，家族/类别：category-sdbms）。
// 表达式结构（TS）：HeightEntry=[p, v]（p=位置、v=值，均可为 undefined=ω）；
// Height=HeightEntry[]；Entry=[value, Height]；Column/Expr 常规。
// 显示 v+1^((…)@p 或 ω)；"(0)" 表示空列。∞ 用 bool 标志表示。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

struct SOmegaDbmsV2HEntry {
    bool pDef = false;
    int p = 0;
    bool vDef = false;
    int v = 0;
};
using SOmegaDbmsV2Height = std::vector<SOmegaDbmsV2HEntry>;

struct SOmegaDbmsV2Entry {
    int value;
    SOmegaDbmsV2Height height;
};
using SOmegaDbmsV2Column = std::vector<SOmegaDbmsV2Entry>;
using SOmegaDbmsV2Expr = std::vector<SOmegaDbmsV2Column>;

struct SOmegaDbmsV2Parsed {
    bool infinity = false;
    SOmegaDbmsV2Expr expr;
};

struct SOmegaDBMSV2Notation {
    static constexpr const char* kName = "SωDBMS v2";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/false);
    }
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::optional<SOmegaDbmsV2Parsed> parse(std::string_view s) {
        if (s == "Limit" || s == "limit" || s == "LIMIT") {
            return SOmegaDbmsV2Parsed{ true, {} };
        }
        std::size_t i = 0;
        auto error = [&]() { return std::optional<SOmegaDbmsV2Parsed>{}; };
        auto skip_spaces = [&]() {
            while (i < s.size() && s[i] == ' ') ++i;
        };
        auto skip_index = [&]() {
            if (i < s.size() && s[i] == ':') {
                ++i;
                skip_spaces();
                while (i < s.size() && s[i] >= '0' && s[i] <= '9') ++i;
            }
        };
        auto parse_number = [&]() -> std::optional<int> {
            skip_spaces();
            const std::size_t start = i;
            while (i < s.size() && s[i] >= '0' && s[i] <= '9') ++i;
            if (start == i) return std::nullopt;
            int v = 0;
            for (std::size_t k = start; k < i; ++k) v = v * 10 + (s[k] - '0');
            return v;
        };
        // 行高的值：'ω'/'w' → undefined；数字 n → n-1
        auto parse_height_value = [&]() -> std::optional<std::pair<bool, int>> {
            skip_spaces();
            // 注意：MSVC 的 char 有符号，须按 unsigned char 比较 UTF-8 首字节 0xCF
            if (i < s.size() && (static_cast<unsigned char>(s[i]) == 0xCF || s[i] == 'w')) {
                if (static_cast<unsigned char>(s[i]) == 0xCF) {
                    // 检查 utf-8 的 ω（0xCF 0x89）
                    if (i + 1 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0x89) {
                        i += 2;
                        return std::pair<bool, int>{ true, 0 };
                    }
                    return std::nullopt;
                }
                ++i;
                return std::pair<bool, int>{ true, 0 };
            }
            auto n = parse_number();
            if (!n) return std::nullopt;
            return std::pair<bool, int>{ false, *n - 1 };
        };
        // 行高单项 'v@p'（v 见上；省略 '@p' 只允许首位，即 p=无穷）。
        // 注意：parse_height_value 的 first 标志是“是否为 ω/未定义”，vDef 须取反。
        auto parse_height_item = [&](bool first) -> std::optional<SOmegaDbmsV2HEntry> {
            auto vv = parse_height_value();
            if (!vv) return std::nullopt;
            skip_spaces();
            if (i < s.size() && s[i] == '@') {
                ++i;
                auto p = parse_number();
                if (!p) return std::nullopt;
                return SOmegaDbmsV2HEntry{ true, *p, !vv->first, vv->second };
            }
            if (!first) return std::nullopt;
            return SOmegaDbmsV2HEntry{ false, 0, !vv->first, vv->second };
        };
        // 行高 '(项,项,…)'，容忍一层多余外层括号
        auto parse_height = [&]() -> std::optional<SOmegaDbmsV2Height> {
            skip_spaces();
            if (i >= s.size() || s[i] != '(') return std::nullopt;
            ++i;
            skip_spaces();
            bool extra_paren = false;
            if (i < s.size() && s[i] == '(') {
                extra_paren = true;
                ++i;
                skip_spaces();
            }
            SOmegaDbmsV2Height result;
            if (i < s.size() && s[i] != ')') {
                auto e = parse_height_item(true);
                if (!e) return std::nullopt;
                result.push_back(*e);
                skip_spaces();
                while (i < s.size() && s[i] == ',') {
                    ++i;
                    skip_spaces();
                    auto e2 = parse_height_item(false);
                    if (!e2) return std::nullopt;
                    result.push_back(*e2);
                    skip_spaces();
                }
            }
            if (i >= s.size() || s[i] != ')') return std::nullopt;
            ++i;
            if (extra_paren) {
                skip_spaces();
                if (i >= s.size() || s[i] != ')') return std::nullopt;
                ++i;
            }
            return result;
        };
        auto parse_entry = [&]() -> std::optional<SOmegaDbmsV2Entry> {
            auto v = parse_number();
            if (!v) return std::nullopt;
            --*v;
            skip_spaces();
            if (i < s.size() && s[i] == '^') {
                ++i;
                auto h = parse_height();
                if (!h) return std::nullopt;
                return SOmegaDbmsV2Entry{ *v, *h };
            }
            return SOmegaDbmsV2Entry{ *v, {} };  // '(0)' 等省略上标
        };
        auto parse_column = [&]() -> std::optional<SOmegaDbmsV2Column> {
            skip_spaces();
            if (i >= s.size() || s[i] != '(') return std::nullopt;
            ++i;
            SOmegaDbmsV2Column entries;
            skip_spaces();
            if (i < s.size() && s[i] != ')' && s[i] != ':') {
                auto e = parse_entry();
                if (!e) return std::nullopt;
                entries.push_back(*e);
                skip_spaces();
                while (i < s.size() && s[i] == ',') {
                    ++i;
                    skip_spaces();
                    if (i < s.size() && s[i] == ')') break;
                    auto e2 = parse_entry();
                    if (!e2) return std::nullopt;
                    entries.push_back(*e2);
                    skip_spaces();
                }
            }
            skip_spaces();
            skip_index();
            skip_spaces();
            if (i >= s.size() || s[i] != ')') return std::nullopt;
            ++i;
            while (!entries.empty() && entries.back().value == -1) entries.pop_back();
            return entries;
        };

        SOmegaDbmsV2Expr result;
        skip_spaces();
        while (i < s.size() && s[i] == '(') {
            auto col = parse_column();
            if (!col) return std::nullopt;
            result.push_back(std::move(*col));
            skip_spaces();
        }
        skip_spaces();
        if (i != s.size()) return std::nullopt;
        return SOmegaDbmsV2Parsed{ false, std::move(result) };
    }

    [[nodiscard]] static std::string to_string(const SOmegaDbmsV2Parsed& p) {
        if (p.infinity) return "Limit";
        return display(p.expr);
    }

private:
    using HEntry = SOmegaDbmsV2HEntry;
    using Height = SOmegaDbmsV2Height;
    using Entry = SOmegaDbmsV2Entry;
    using Column = SOmegaDbmsV2Column;
    using Expr = SOmegaDbmsV2Expr;

    [[nodiscard]] static std::string display(const Expr& expr) {
        std::string out;
        for (const Column& col : expr) {
            if (col.empty()) {
                out += "(0)";
                continue;
            }
            out += '(';
            for (std::size_t k = 0; k < col.size(); ++k) {
                if (k) out += ',';
                out += std::to_string(col[k].value + 1);
                out += '^';
                out += '(';
                for (std::size_t m = 0; m < col[k].height.size(); ++m) {
                    if (m) out += ',';
                    const HEntry& he = col[k].height[m];
                    if (!he.vDef) {
                        out += "\xCF\x89";  // ω
                    } else {
                        out += std::to_string(he.v + 1);
                    }
                    if (he.pDef) {
                        out += '@';
                        out += std::to_string(he.p);
                    }
                }
                out += ')';
            }
            out += ')';
        }
        return out;
    }

    [[nodiscard]] static int compare_int(int a, int b) { return a < b ? -1 : a > b ? 1 : 0; }

    // compare_undefined_last：undefined 最大
    [[nodiscard]] static int compare_undef(int aDef, int a, int bDef, int b) {
        if (aDef && bDef) return compare_int(a, b);
        if (aDef) return -1;
        if (bDef) return 1;
        return 0;
    }

    // HeightEntry 比较：[p, v] 字典序，分量 undefined-last
    [[nodiscard]] static int compare_hentry(const HEntry& a, const HEntry& b) {
        int c = compare_undef(a.pDef, a.p, b.pDef, b.p);
        if (c != 0) return c;
        return compare_undef(a.vDef, a.v, b.vDef, b.v);
    }

    // compare_height = lex_compare(h1, h2, pair_cmp)（较长者大）
    [[nodiscard]] static int compare_height(const Height& a, const Height& b) {
        const std::size_t len = a.size() < b.size() ? a.size() : b.size();
        for (std::size_t k = 0; k < len; ++k) {
            int c = compare_hentry(a[k], b[k]);
            if (c != 0) return c;
        }
        return compare_int(static_cast<int>(a.size()), static_cast<int>(b.size()));
    }

    [[nodiscard]] static bool column_height(const Column& col, Height& out) {
        if (col.empty()) return false;
        out = col.back().height;
        return true;
    }

    [[nodiscard]] static Column filter_height_greater(const Column& col, const Height& h0) {
        Column out;
        for (const Entry& e : col) {
            if (compare_height(e.height, h0) > 0) out.push_back(e);
        }
        return out;
    }

    // TS merge_column(col1, col2)：v2 的 height(col1) 对空列返回 []（非 undefined），
    // 故始终执行 filter_height_greater(col2, height(col1))（空高项被过滤掉）。
    [[nodiscard]] static Column merge_column(const Column& col1, const Column& col2) {
        Column out = col1;
        Height h;
        if (!column_height(col1, h)) h = {};   // TS height([]) = []
        const Column filtered = filter_height_greater(col2, h);
        out.insert(out.end(), filtered.begin(), filtered.end());
        return out;
    }

    [[nodiscard]] static int copy_value(int value, int r, int offset) {
        return value >= r ? value + offset : value;
    }

    [[nodiscard]] static HEntry copy_hentry(const HEntry& he, int r, int offset) {
        HEntry out;
        out.pDef = he.pDef;
        out.p = he.p;
        out.vDef = he.vDef;
        out.v = he.vDef ? copy_value(he.v, r, offset) : 0;
        return out;
    }

    [[nodiscard]] static Height copy_height(const Height& h, int r, int offset) {
        Height out;
        out.reserve(h.size());
        for (const HEntry& he : h) out.push_back(copy_hentry(he, r, offset));
        return out;
    }

    [[nodiscard]] static Entry copy_entry(const Entry& e, int r, int offset) {
        return Entry{ copy_value(e.value, r, offset), copy_height(e.height, r, offset) };
    }

    [[nodiscard]] static Column copy_column(const Column& col, int r, int offset) {
        Column out;
        out.reserve(col.size());
        for (const Entry& e : col) out.push_back(copy_entry(e, r, offset));
        return out;
    }

    // top_separator：末项 v 为 undefined 时返回 p+1（p 须有定义），否则 0
    [[nodiscard]] static int top_separator(const Height& h) {
        const HEntry& last = h.back();
        if (!last.vDef) return last.pDef ? last.p + 1 : 0;
        return 0;
    }

    // height_fill(h, p, v)：保留长度>1 且末项 p<=p 时 pop；末项 v 不同则 push [p, v]
    [[nodiscard]] static Height height_fill(const Height& h, int p, bool vDef, int v) {
        Height new_h = h;
        while (new_h.size() > 1 && new_h.back().pDef && new_h.back().p <= p) new_h.pop_back();
        const HEntry& last = new_h.back();
        if (!(last.vDef == vDef && (!vDef || last.v == v))) {
            new_h.push_back(HEntry{ true, p, vDef, v });
        }
        return new_h;
    }

    [[nodiscard]] static Height compute_new_height(const Height& h, const Expr& expr, int r) {
        const HEntry& last = h.back();
        Height new_h = h;
        if (!last.vDef) {
            // p 无定义 → TS 抛 'Illegal state'
            new_h = height_fill(new_h, last.p, true, r);
            if (last.p > 0) new_h = height_fill(new_h, last.p - 1, false, 0);
        } else if (!last.pDef) {
            Height out;
            if (column_height(expr[static_cast<std::size_t>(last.v)], out)) new_h = out;
            else new_h.clear();
        } else {
            const Column& col_rh = expr[static_cast<std::size_t>(last.v)];
            // hj = 第一个 compare_height(h, hv) <= 0
            std::size_t hj = col_rh.size();
            for (std::size_t k = 0; k < col_rh.size(); ++k) {
                if (compare_height(h, col_rh[k].height) <= 0) {
                    hj = k;
                    break;
                }
            }
            if (hj == col_rh.size()) {
                Height out;
                if (column_height(col_rh, out)) new_h = out;
                else new_h.clear();
            } else {
                const int new_value = col_rh[hj].value;
                new_h = height_fill(new_h, last.p, true, new_value);
                if (last.p > 0) new_h = height_fill(new_h, last.p - 1, false, 0);
                if (hj > 0) {
                    const Height& lower = col_rh[hj - 1].height;
                    if (compare_height(lower, new_h) > 0) new_h = lower;
                }
            }
        }
        return new_h;
    }

    [[nodiscard]] static Expr infinity_fs(int index) {
        Expr result;
        result.emplace_back();
        Height h;
        h.push_back(HEntry{ false, 0, true, 0 });
        h.push_back(HEntry{ true, index, false, 0 });
        result.push_back(Column{ Entry{ 0, std::move(h) } });
        return result;
    }

    [[nodiscard]] static Expr expand(const Expr& expr, int index, bool shorter) {
        if (expr.empty()) return expr;
        const int right = static_cast<int>(expr.size()) - 1;
        if (expr[static_cast<std::size_t>(right)].empty()) {
            return Expr(expr.begin(), expr.end() - 1);
        }
        const int top = static_cast<int>(expr[static_cast<std::size_t>(right)].size()) - 1;
        const Entry& tr = expr[static_cast<std::size_t>(right)][static_cast<std::size_t>(top)];
        const int r = tr.value;

        const Height new_h = compute_new_height(tr.height, expr, r);

        const Column& rc = expr[static_cast<std::size_t>(right)];
        const Column& rc2 = expr[static_cast<std::size_t>(r)];
        Column new_col(rc.begin(), rc.end() - 1);
        Column mid{ Entry{ r, new_h } };
        new_col = merge_column(new_col, mid);
        new_col = merge_column(new_col, rc2);

        Expr result(expr.begin(), expr.end() - 1);
        result.push_back(std::move(new_col));

        for (int w = 1; w <= index; ++w) {
            for (int i = r + 1; i <= right; ++i) {
                result.push_back(copy_column(result[static_cast<std::size_t>(i)], r,
                                             (right - r) * w));
            }
        }
        if (shorter) result.pop_back();
        return result;
    }

    static std::string expand_text_impl(std::string_view text, int term, bool longer) {
        if (term < 1) term = 1;
        const auto parsed = parse(text);
        if (!parsed) {
            return "解析失败，请确认你没有使用全角括号，逗号，分号";
        }
        if (parsed->infinity) return display(infinity_fs(term));
        const Expr& e = parsed->expr;
        if (e.empty()) return {};
        // 值域校验：value 与行高 v 分量作为列索引，须在 [0, size)；行高不允许为空
        // （空行高会让 compute_new_height 的 h.back() 越界；TS 对空行高的退化输入
        // 语义不稳，故按本家族统一策略拒绝）。
        for (const Column& col : e) {
            for (const Entry& en : col) {
                if (en.value < 0 || en.value >= static_cast<int>(e.size())) {
                    return "解析失败，请确认你没有使用全角括号，逗号，分号";
                }
                if (en.height.empty()) {
                    return "解析失败，请确认你没有使用全角括号，逗号，分号";
                }
                for (const HEntry& he : en.height) {
                    if (he.vDef && (he.v < 0 || he.v >= static_cast<int>(e.size()))) {
                        return "解析失败，请确认你没有使用全角括号，逗号，分号";
                    }
                    if (!he.vDef && !he.pDef) {
                        // 对应 TS compute_new_height 的 'Illegal state'
                        return "解析失败，请确认你没有使用全角括号，逗号，分号";
                    }
                }
            }
        }
        if (e.back().empty()) return display(Expr(e.begin(), e.end() - 1));
        return display(expand(e, term, !longer));
    }
};

} // namespace omegay::notation

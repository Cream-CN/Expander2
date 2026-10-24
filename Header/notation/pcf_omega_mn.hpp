// Header/notation/pcf_omega_mn.hpp
// PCF's ωMN 记号（display 名 "PCF's ωMN"，TS id "pcf-omega-mn"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/pcf-omega_MN.ts
// （家族/类别：category-mn，credit.pcf-omega-mn）。
// 表达式结构（TS）：Vertical=number[]（Cantor 系数数组，小端：v[j]=ω^j 系数，
// 反字典序比较）；Entry=[int,Vertical]（p 为 0 起列标）；Column=Entry[]；
// Expr=Column[]。∞ 用顶层标志表示（display 为 "Limit"）。
// 展开是"坏根复制"的变体：stretch/normalize_col/magma_verticals/base_vertical。
// "MN" 显示等价（convert_to_mn）与 FS_short 为 UI 特性，不移植。
// TS 的 memo 缓存为纯函数优化，本移植省略。
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

namespace pcf_omega_mn_detail {

using Vertical = std::vector<int>;
struct Entry {
    int p;         // 0 起列标（TS 元组第 0 项，显示时 +1）
    Vertical v;    // TS 元组第 1 项
};
using Column = std::vector<Entry>;
using Expr = std::vector<Column>;

inline int num_compare(int a, int b) {
    return a == b ? 0 : (a < b ? -1 : 1);
}

// TS anti_lex_compare：长度优先，再从末元素向前逐项比较。
template <typename T, typename Cmp>
inline int anti_lex_compare(const std::vector<T>& a, const std::vector<T>& b, Cmp cmp) {
    if (a.size() != b.size()) return a.size() > b.size() ? 1 : -1;
    for (std::size_t i = a.size(); i > 0; --i) {
        const int c = cmp(a[i - 1], b[i - 1]);
        if (c != 0) return c;
    }
    return 0;
}

// TS lex_compare。
template <typename T, typename Cmp>
inline int lex_compare(const std::vector<T>& a, const std::vector<T>& b, Cmp cmp) {
    const std::size_t n = a.size() < b.size() ? a.size() : b.size();
    for (std::size_t i = 0; i < n; ++i) {
        const int c = cmp(a[i], b[i]);
        if (c != 0) return c;
    }
    return a.size() == b.size() ? 0 : (a.size() > b.size() ? 1 : -1);
}

inline int vertical_compare(const Vertical& v1, const Vertical& v2) {
    return anti_lex_compare(v1, v2, [](int a, int b) { return num_compare(a, b); });
}

inline int entry_compare(const Entry& e1, const Entry& e2) {
    const int c = num_compare(e1.p, e2.p);
    if (c != 0) return c;
    return vertical_compare(e1.v, e2.v);
}

inline int column_compare(const Column& c1, const Column& c2) {
    return lex_compare(c1, c2, [](const Entry& a, const Entry& b) { return entry_compare(a, b); });
}

inline int compare(const Expr& e1, const Expr& e2) {
    return lex_compare(e1, e2, [](const Column& a, const Column& b) { return column_compare(a, b); });
}

// ---------------------------------------------------------------------
// 竖向运算（TS vertical_* / top_sep）
// ---------------------------------------------------------------------

inline Vertical vertical_increase(const Vertical& v, int s) {
    if (static_cast<int>(v.size()) <= s) {
        Vertical r(static_cast<std::size_t>(s), 0);
        r.push_back(1);
        return r;
    }
    Vertical r = v;
    ++r[static_cast<std::size_t>(s)];
    for (int k = 0; k < s; ++k) r[static_cast<std::size_t>(k)] = 0;
    return r;
}

inline int top_sep(const Vertical& v) {
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (v[i] > 0) return static_cast<int>(i);
    }
    return -1;
}

inline Vertical vertical_decrease(const Vertical& v) {
    const int s = top_sep(v);
    if (s < 0) return {};
    if (v[static_cast<std::size_t>(s)] == 1 && s == static_cast<int>(v.size()) - 1) return {};
    Vertical r = v;
    --r[static_cast<std::size_t>(s)];
    return r;
}

inline Vertical vertical_add(const Vertical& v, const Vertical& a) {
    if (v.size() < a.size()) return a;
    if (a.empty()) return v;
    Vertical r(a.begin(), a.end() - 1);
    r.push_back(v[a.size() - 1] + a[a.size() - 1]);
    r.insert(r.end(), v.begin() + static_cast<std::ptrdiff_t>(a.size()), v.end());
    return r;
}

inline Vertical vertical_sub(const Vertical& v, const Vertical& base) {
    if (v.size() > base.size()) return v;
    if (v.size() < base.size()) return {};
    int j = static_cast<int>(v.size()) - 1;
    while (j >= 0) {
        if (v[static_cast<std::size_t>(j)] < base[static_cast<std::size_t>(j)]) return {};
        if (v[static_cast<std::size_t>(j)] > base[static_cast<std::size_t>(j)]) {
            Vertical r(v.begin(), v.begin() + j);
            r.push_back(v[static_cast<std::size_t>(j)] - base[static_cast<std::size_t>(j)]);
            return r;
        }
        --j;
    }
    return {};
}

inline Vertical max_vertical(const Vertical& a, const Vertical& b) {
    return vertical_compare(a, b) < 0 ? b : a;
}

// TS find_index_below_row：col 中最大的 m 使 col[m-1][1] < v。
inline int find_index_below_row(const Column& col, const Vertical& v) {
    int l = 0, r = static_cast<int>(col.size());
    while (l < r) {
        const int m = (l + r + 1) >> 1;
        if (vertical_compare(v, col[static_cast<std::size_t>(m - 1)].v) > 0) l = m;
        else r = m - 1;
    }
    return l;
}

inline Vertical base_vertical(const Expr& expr, const Vertical& top, int r) {
    Vertical bottom = vertical_decrease(top);
    int i = r;
    while (true) {
        const Column& col = expr[static_cast<std::size_t>(i)];
        const int j = find_index_below_row(col, top);
        bottom = max_vertical(bottom, j == 0 ? Vertical{} : col[static_cast<std::size_t>(j - 1)].v);
        if (j == static_cast<int>(col.size())) return bottom;
        i = col[static_cast<std::size_t>(j)].p;
    }
}

inline Vertical stretch(const Vertical& v, const Vertical& threshold,
                        const Vertical& target, int w) {
    if (vertical_compare(v, threshold) <= 0) return v;
    Vertical result = v;
    for (int j = 0; j < w; ++j) {
        result = vertical_add(target, vertical_sub(result, threshold));
    }
    return result;
}

inline Column normalize_col(const Column& col) {
    Column result;
    Vertical current;
    for (const Entry& e : col) {
        if (vertical_compare(e.v, current) > 0) {
            current = e.v;
            result.push_back(e);
        }
    }
    return result;
}

inline Column copy_column(const Column& col, int r, const Vertical& threshold,
                          const Vertical& target, const Vertical& magma,
                          int offset, int w) {
    Column result;
    const Vertical stretched_magma = stretch(magma, threshold, target, w);
    for (const Entry& e : col) {
        const int p1 = e.p >= r ? e.p + offset * w : e.p;
        if (vertical_compare(e.v, magma) <= 0) {
            result.push_back(Entry{ p1, stretch(e.v, threshold, target, w) });
        } else if (e.p < r) {
            result.push_back(Entry{ p1, e.v });
        } else {
            result.push_back(Entry{ p1, max_vertical(e.v, stretched_magma) });
        }
    }
    return normalize_col(result);
}

inline std::vector<Vertical> magma_verticals(const Expr& expr, int r) {
    std::vector<Vertical> result(expr.size());
    for (std::size_t i = static_cast<std::size_t>(r) + 1; i < expr.size(); ++i) {
        const Column& col = expr[i];
        for (std::size_t j = 0; j < col.size(); ++j) {
            const Entry& e = col[j];
            if (e.p < r) break;
            if (e.p == r) {
                result[i] = e.v;
                break;
            }
            const Vertical& mv_p = result[static_cast<std::size_t>(e.p)];
            if (vertical_compare(mv_p, e.v) <= 0) {
                if (vertical_compare(result[i], mv_p) < 0) result[i] = mv_p;
                break;
            }
            result[i] = e.v;
        }
    }
    return result;
}

// TS expand(expr, index, shorter)。前置条件：expr 为极限（调用方保证）。
inline Expr expand(const Expr& expr0, int index, bool shorter) {
    Expr expr = expr0;
    if (expr.empty()) return expr;
    const int right = static_cast<int>(expr.size()) - 1;
    if (expr[static_cast<std::size_t>(right)].empty()) {
        expr.pop_back();
        return expr;
    }
    const int up = static_cast<int>(expr[static_cast<std::size_t>(right)].size()) - 1;
    const Entry& top_entry = expr[static_cast<std::size_t>(right)][static_cast<std::size_t>(up)];
    const int r = top_entry.p;
    const Vertical top = top_entry.v;
    const int s = top_sep(top);
    const Vertical threshold = base_vertical(expr, top, r);
    const Vertical target_candidate =
        s == 0 ? threshold : vertical_increase(threshold, s - 1);

    Expr result(expr.begin(), expr.end() - 1);
    // JS 语义：result[right] = X 会扩展数组到 right+1；C++ 用 push_back。
    {
        Column new_right;
        new_right.reserve(expr[static_cast<std::size_t>(right)].size() + expr[static_cast<std::size_t>(r)].size());
        new_right.insert(new_right.end(), expr[static_cast<std::size_t>(right)].begin(),
                         expr[static_cast<std::size_t>(right)].end() - 1);
        new_right.push_back(Entry{ r, target_candidate });
        new_right.insert(new_right.end(), expr[static_cast<std::size_t>(r)].begin(),
                         expr[static_cast<std::size_t>(r)].end());
        result.push_back(normalize_col(std::move(new_right)));
    }

    const Vertical& prev_vert =
        up == 0 ? Vertical{} : expr[static_cast<std::size_t>(right)][static_cast<std::size_t>(up - 1)].v;
    const Vertical target = max_vertical(target_candidate, prev_vert);

    const std::vector<Vertical> magma = magma_verticals(expr, r);
    const int offset = right - r;
    for (int w = 1; w <= index; ++w) {
        for (int j = r + 1; j <= right; ++j) {
            result.push_back(copy_column(result[static_cast<std::size_t>(j)], r,
                                         threshold, target, magma[static_cast<std::size_t>(j)],
                                         offset, w));
        }
    }
    if (shorter) result.pop_back();
    return result;
}

// ---------------------------------------------------------------------
// 显示（TS vertical_cantor_display / entry_display / col_display / display）
// ---------------------------------------------------------------------

inline std::string vertical_cantor_display(const Vertical& v, bool html) {
    std::string result;
    for (int j = static_cast<int>(v.size()) - 1; j >= 0; --j) {
        const int c = v[static_cast<std::size_t>(j)];
        if (c > 0) {
            if (!result.empty()) result += '+';
            if (j == 0) {
                result += std::to_string(c);
            } else {
                const std::string prim =
                    j > 1 ? (html ? "ω<sup>" + std::to_string(j) + "</sup>"
                                  : "ω^" + std::to_string(j))
                          : "ω";
                result += prim;
                if (c > 1) result += html ? std::to_string(c) : "*" + std::to_string(c);
            }
        }
    }
    if (result.empty()) return "0";
    return result;
}

inline std::string entry_display(const Entry& e, bool html) {
    return std::to_string(e.p + 1) + ":" + vertical_cantor_display(e.v, html);
}

inline std::string col_display(const Column& col, bool html) {
    std::string result = "(";
    for (std::size_t i = 0; i < col.size(); ++i) {
        if (i) result += ',';
        result += entry_display(col[i], html);
    }
    result += ')';
    return result;
}

inline std::string display(const Expr& expr, bool html = false) {
    std::string result;
    for (const Column& c : expr) result += col_display(c, html);
    return result;
}

// ---------------------------------------------------------------------
// 解析（TS from_display；非法输入返回 nullopt）
// ---------------------------------------------------------------------

inline constexpr char kW = '\x7F';

inline std::string normalize_w(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size();) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (c == 0xCF && i + 1 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0x89) {
            out.push_back(kW);
            i += 2;
        } else if (c == 'w' || c == 'W') {
            out.push_back(kW);
            ++i;
        } else {
            out.push_back(s[i]);
            ++i;
        }
    }
    return out;
}

struct Parser {
    const std::string& s;
    std::size_t i = 0;
    bool failed = false;

    void skip_spaces() {
        while (i < s.size() && s[i] == ' ') ++i;
    }
    std::optional<int> parse_number() {
        const std::size_t start = i;
        while (i < s.size() && s[i] >= '0' && s[i] <= '9') ++i;
        if (start == i) return std::nullopt;
        int v = 0;
        for (std::size_t k = start; k < i; ++k) {
            v = v * 10 + (s[k] - '0');
            if (v > 1000000) return std::nullopt;
        }
        return v;
    }
    // TS parse_omega_term：返回 [m, n]（ω^m 系数 n）。
    std::optional<std::pair<int, int>> parse_omega_term() {
        ++i;  // 'ω'/'w'
        skip_spaces();
        int m = 1;
        if (i < s.size() && s[i] == '^') {
            ++i;
            skip_spaces();
            if (i < s.size() && s[i] == '(') {
                ++i;
                skip_spaces();
                auto v = parse_number();
                if (!v) return std::nullopt;
                m = *v;
                skip_spaces();
                if (i >= s.size() || s[i] != ')') return std::nullopt;
                ++i;
            } else {
                auto v = parse_number();
                if (!v) return std::nullopt;
                m = *v;
            }
            skip_spaces();
        }
        int n = 1;
        if (i < s.size() && s[i] == '*') {
            ++i;
            skip_spaces();
            auto v = parse_number();
            if (!v) return std::nullopt;
            n = *v;
        }
        return std::make_pair(m, n);
    }
    std::optional<std::pair<int, int>> parse_vertical_term() {
        if (i < s.size() && (s[i] == kW)) return parse_omega_term();
        auto n = parse_number();
        if (!n) return std::nullopt;
        return std::make_pair(0, *n);
    }
    std::optional<Vertical> parse_vertical() {
        Vertical result;
        while (true) {
            skip_spaces();
            auto term = parse_vertical_term();
            if (!term) return std::nullopt;
            if (term->second > 0) {
                Vertical a(static_cast<std::size_t>(term->first), 0);
                a.push_back(term->second);
                result = vertical_add(result, a);
            }
            skip_spaces();
            if (i >= s.size() || s[i] != '+') break;
            ++i;
        }
        return result;
    }
    std::optional<Entry> parse_entry() {
        skip_spaces();
        auto p = parse_number();
        if (!p || *p < 1) return std::nullopt;
        const int p0 = *p - 1;
        skip_spaces();
        if (i >= s.size() || s[i] != ':') return std::nullopt;
        ++i;
        auto v = parse_vertical();
        if (!v) return std::nullopt;
        return Entry{ p0, std::move(*v) };
    }
    std::optional<Column> parse_column() {
        ++i;  // '('
        skip_spaces();
        Column col;
        if (i < s.size() && s[i] == ')') {
            ++i;
            return col;
        }
        while (true) {
            auto e = parse_entry();
            if (!e) return std::nullopt;
            col.push_back(std::move(*e));
            skip_spaces();
            if (i >= s.size()) return std::nullopt;
            if (s[i] == ',') {
                ++i;
                continue;
            }
            if (s[i] != ')') return std::nullopt;
            ++i;
            return col;
        }
    }
};

inline std::optional<Expr> from_display(std::string_view str) {
    const std::string s = normalize_w(str);
    Parser p{ s, 0, false };
    p.skip_spaces();
    Expr result;
    while (true) {
        p.skip_spaces();
        if (p.i >= s.size()) break;
        if (s[p.i] != '(') return std::nullopt;
        auto col = p.parse_column();
        if (!col) return std::nullopt;
        result.push_back(std::move(*col));
    }
    return result;
}

inline Expr infinity_fs(int index) {
    Vertical v(static_cast<std::size_t>(index), 0);
    v.push_back(1);
    Expr e;
    e.push_back(Column{});
    e.push_back(Column{ Entry{ 0, std::move(v) } });
    return e;
}

} // namespace pcf_omega_mn_detail

// Expr：非 ∞ 时持有 Expr；∞（显示 "Limit"）用标志表示。
struct PcfOmegaMnExpr {
    bool infinity = false;
    pcf_omega_mn_detail::Expr expr;
};

struct PcfOmegaMNNotation {
    static constexpr const char* kName = "PCF's ωMN";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/false);
    }
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<PcfOmegaMnExpr> parse(std::string_view text) {
        std::string s0(text);
        std::size_t b = 0, e = s0.size();
        while (b < e && (s0[b] == ' ' || s0[b] == '\t')) ++b;
        while (e > b && (s0[e - 1] == ' ' || s0[e - 1] == '\t')) --e;
        const std::string_view raw = std::string_view(s0).substr(b, e - b);
        if (raw == "Limit" || raw == "limit" || raw == "LIMIT") {
            return PcfOmegaMnExpr{ true, {} };
        }
        if (raw.empty() || raw == "0" || raw == "∅") {
            return PcfOmegaMnExpr{ false, {} };
        }
        auto m = pcf_omega_mn_detail::from_display(raw);
        if (!m) return std::nullopt;
        return PcfOmegaMnExpr{ false, std::move(*m) };
    }

    [[nodiscard]] static std::string to_string(const PcfOmegaMnExpr& e) {
        if (e.infinity) return "Limit";
        return pcf_omega_mn_detail::display(e.expr, false);
    }

    // 非 ∞ 表达式的比较（供测试用）。
    [[nodiscard]] static int compare(const PcfOmegaMnExpr& a, const PcfOmegaMnExpr& b) {
        if (a.infinity || b.infinity) {
            return a.infinity == b.infinity ? 0 : (a.infinity ? 1 : -1);
        }
        return pcf_omega_mn_detail::compare(a.expr, b.expr);
    }

private:
    static std::string expand_text_impl(std::string_view text, int term, bool longer) {
        if (term < 1) term = 1;
        const auto parsed = parse(text);
        if (!parsed) {
            return "解析失败，请确认你没有使用全角括号，逗号，分号";
        }
        if (parsed->infinity) {
            return pcf_omega_mn_detail::display(pcf_omega_mn_detail::infinity_fs(term), false);
        }
        const pcf_omega_mn_detail::Expr& e = parsed->expr;
        if (e.empty()) return {};
        if (!(e.size() > 0 && !e.back().empty())) {
            return pcf_omega_mn_detail::display(
                pcf_omega_mn_detail::Expr(e.begin(), e.end() - 1), false);
        }
        return pcf_omega_mn_detail::display(
            pcf_omega_mn_detail::expand(e, term, !longer), false);
    }
};

} // namespace omegay::notation

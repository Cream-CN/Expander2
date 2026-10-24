// Header/notation/t_omega_mn.hpp
// TωMN 记号（display 名 "TωMN"，TS id "t-omega-mn"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/T_omega_MN.ts
// （家族/类别：category-mn，credit.hypcos_mn）。
// 表达式结构（TS）：Sep=Expr（递归！）；Entry=[int,Sep]；Column=Entry[]；
// Expr=Column[]。∞ 用顶层标志表示（display 为 "Limit"）。
// C++ 递归类型：struct TEntry { int value; TExpr sep; } 通过
// TColumn=vector<TEntry>、TExpr=vector<TColumn> 形成合法递归（vector 支持不完整元素）。
// 与 ωMN 相比的关键差异：分隔符本身是 Expr，且其"极限分隔符"要递归展开
// （threshold 取最小 n 使 low/high 两个竖向在该基本列下汇合）。
// TS 的 memo 缓存为纯函数优化，本移植省略；FS_short 为 UI 等价表示，不移植。
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

namespace t_omega_mn_detail {

struct TEntry;
using TColumn = std::vector<TEntry>;
using TExpr = std::vector<TColumn>;
struct TEntry {
    int value;
    TExpr sep;
};

inline int mountain_compare(const TExpr& a, const TExpr& b);

inline int entry_compare(const TEntry& a, const TEntry& b) {
    if (a.value != b.value) return a.value < b.value ? -1 : 1;
    return mountain_compare(a.sep, b.sep);
}

inline int column_compare(const TColumn& a, const TColumn& b) {
    const std::size_t n = a.size() < b.size() ? a.size() : b.size();
    for (std::size_t i = 0; i < n; ++i) {
        const int c = entry_compare(a[i], b[i]);
        if (c != 0) return c;
    }
    return a.size() == b.size() ? 0 : (a.size() > b.size() ? 1 : -1);
}

inline int mountain_compare(const TExpr& a, const TExpr& b) {
    const std::size_t n = a.size() < b.size() ? a.size() : b.size();
    for (std::size_t i = 0; i < n; ++i) {
        const int c = column_compare(a[i], b[i]);
        if (c != 0) return c;
    }
    return a.size() == b.size() ? 0 : (a.size() > b.size() ? 1 : -1);
}

inline bool is_limit(const TExpr& m) {
    return !m.empty() && !m.back().empty();
}

inline bool is_one(const TExpr& m) {
    return m.size() == 1 && m[0].empty();
}

// 分隔符是"纯逗号"表示：所有列为空列，数量即逗号个数。
inline bool sep_all_empty(const TExpr& sep) {
    for (const TColumn& c : sep) {
        if (!c.empty()) return false;
    }
    return true;
}

// ---------------------------------------------------------------------
// 显示（TS sep_display / entry_display / column_display / mountain_display）
// ---------------------------------------------------------------------

inline std::string sep_display(const TExpr& sep, bool simple);
inline std::string mountain_display(const TExpr& m, bool simple);

inline std::string sep_display(const TExpr& sep, bool simple) {
    if (sep_all_empty(sep)) {
        const std::size_t sep_len = sep.size();
        if (sep_len == 1 && simple) return "";
        return std::string(sep_len, ',');
    }
    std::string d = mountain_display(sep, simple);
    return simple ? "[" + d + "]" : d;
}

inline std::string entry_display(const TEntry& e, bool simple) {
    std::string d = sep_display(e.sep, simple);
    std::string dv = std::to_string(e.value);
    if (simple && dv.size() >= 2) dv = "(" + dv + ")";
    return d + dv;
}

inline std::string column_display(const TColumn& c, bool simple) {
    if (simple && c.empty()) return "0";
    std::string result;
    for (const TEntry& e : c) result += entry_display(e, simple);
    return simple ? result : "(" + result + ")";
}

inline std::string mountain_display(const TExpr& m, bool simple) {
    std::string out;
    for (std::size_t i = 0; i < m.size(); ++i) {
        if (i > 0 && simple) out += ' ';
        out += column_display(m[i], simple);
    }
    return out;
}

// ---------------------------------------------------------------------
// 解析（TS mountain_from_display；非法输入返回 nullopt）
// ---------------------------------------------------------------------

struct TParser {
    const std::string& s;
    std::size_t i = 0;
    bool failed = false;

    void skip_spaces() {
        while (i < s.size() && s[i] == ' ') ++i;
    }
    std::optional<int> parse_number() {
        skip_spaces();
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
    std::optional<TExpr> parse_sep() {
        skip_spaces();
        if (i < s.size() && s[i] == ',') {
            std::size_t count = 0;
            while (i < s.size() && s[i] == ',') {
                ++count;
                ++i;
            }
            return TExpr(count);
        }
        if (i < s.size() && s[i] == '(') {
            return parse_expr();
        }
        return std::nullopt;
    }
    std::optional<TEntry> parse_entry() {
        auto sep = parse_sep();
        if (!sep) return std::nullopt;
        auto v = parse_number();
        if (!v) return std::nullopt;
        return TEntry{ *v, std::move(*sep) };
    }
    void skip_index() {
        if (i < s.size() && s[i] == ':') {
            ++i;
            skip_spaces();
            while (i < s.size() && s[i] >= '0' && s[i] <= '9') ++i;
        }
    }
    std::optional<TColumn> parse_column() {
        skip_spaces();
        if (i >= s.size() || s[i] != '(') return std::nullopt;
        ++i;
        TColumn col;
        skip_spaces();
        while (i < s.size() && s[i] != ')' && s[i] != ':') {
            auto e = parse_entry();
            if (!e) return std::nullopt;
            col.push_back(std::move(*e));
            skip_spaces();
        }
        skip_index();
        skip_spaces();
        if (i >= s.size()) return std::nullopt;
        ++i;  // ')'
        return col;
    }
    std::optional<TExpr> parse_expr() {
        TExpr result;
        skip_spaces();
        while (i < s.size() && s[i] == '(') {
            auto col = parse_column();
            if (!col) return std::nullopt;
            result.push_back(std::move(*col));
            skip_spaces();
        }
        return result;
    }
};

inline std::optional<TExpr> from_display(std::string_view s0) {
    std::string s(s0);
    TParser p{ s, 0, false };
    p.skip_spaces();
    auto result = p.parse_expr();
    if (!result) return std::nullopt;
    p.skip_spaces();
    if (p.i != s.size()) return std::nullopt;
    return result;
}

// ---------------------------------------------------------------------
// 竖向机制（TS vertical_* / parent / column_verticals / get_references）
// ---------------------------------------------------------------------

using TVertical = std::vector<TExpr>;

inline int vertical_compare(const TVertical& a, const TVertical& b) {
    std::size_t i = 0;
    while (true) {
        if (i >= a.size()) return i >= b.size() ? 0 : -1;
        if (i >= b.size()) return 1;
        const int c = mountain_compare(a[i], b[i]);
        if (c != 0) return c;
        ++i;
    }
}

inline TVertical vertical_increase(const TVertical& v, const TExpr& m) {
    std::size_t i = v.size();
    while (i >= 1 && mountain_compare(v[i - 1], m) < 0) --i;
    TVertical out(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(i));
    out.push_back(m);
    return out;
}

inline int find_index_below_row(const std::vector<TVertical>& verticals,
                                const TVertical& y) {
    std::vector<TVertical> working;
    working.reserve(verticals.size() + 1);
    working.push_back(TVertical{});
    working.insert(working.end(), verticals.begin(), verticals.end());
    int l = 0, r = static_cast<int>(working.size()) - 1;
    while (l < r) {
        const int m = (l + r + 1) / 2;
        if (vertical_compare(working[static_cast<std::size_t>(m)], y) < 0) l = m;
        else r = m - 1;
    }
    return l;
}

inline std::vector<TVertical> column_verticals(const TColumn& column) {
    std::vector<TVertical> result;
    result.reserve(column.size());
    TVertical v;
    for (const TEntry& e : column) {
        v = vertical_increase(v, e.sep);
        result.push_back(v);
    }
    return result;
}

struct TPosition {
    int i;
    int j;
};

inline TPosition parent(const TExpr& A, const std::vector<std::vector<TVertical>>& V,
                        TPosition pos) {
    const int value = A[static_cast<std::size_t>(pos.i)][static_cast<std::size_t>(pos.j)].value;
    const int target_i = value - 1;
    if (target_i < 0) return TPosition{ -1, 0 };
    return TPosition{ target_i,
                      find_index_below_row(V[static_cast<std::size_t>(target_i)],
                                           V[static_cast<std::size_t>(pos.i)][static_cast<std::size_t>(pos.j)]) };
}

inline std::vector<int> get_references(const TExpr& A,
                                       const std::vector<TVertical>& r_tops) {
    std::vector<TVertical> verticals = column_verticals(A.back());
    verticals.insert(verticals.begin(), TVertical{});
    std::vector<int> ref(r_tops.size(), -1);
    std::size_t i = 0, j = 0;
    while (i < verticals.size() && j < r_tops.size()) {
        if (vertical_compare(verticals[i], r_tops[j]) < 0) {
            ref[j] = static_cast<int>(i);
            ++i;
        } else {
            ++j;
        }
    }
    return ref;
}

// ---------------------------------------------------------------------
// 展开（TS threshold / expand）
// ---------------------------------------------------------------------

inline int threshold(const TExpr& A, bool shorter, const TVertical& low,
                     const TVertical& high);

inline TExpr expand(const TExpr& A0, int index, bool shorter);

inline int threshold(const TExpr& A, bool shorter, const TVertical& low,
                     const TVertical& high) {
    int n = 0;
    while (true) {
        const TExpr res = expand(A, n, shorter);
        if (vertical_compare(vertical_increase(low, res), vertical_increase(high, res)) >= 0)
            return n;
        ++n;
    }
}

inline TExpr expand(const TExpr& A0, int index, bool shorter) {
    const int rightmost = static_cast<int>(A0.size()) - 1;
    const int topmost = static_cast<int>(A0[static_cast<std::size_t>(rightmost)].size()) - 1;
    TExpr A = A0;

    if (topmost == -1) {
        A.pop_back();
        return A;
    }

    TEntry& top_right_entry = A[static_cast<std::size_t>(rightmost)][static_cast<std::size_t>(topmost)];
    const TExpr top_right_separator = top_right_entry.sep;
    std::vector<std::vector<TVertical>> V0;
    V0.reserve(A.size());
    for (const TColumn& c : A) V0.push_back(column_verticals(c));
    const TPosition BRij = parent(A, V0, TPosition{ rightmost, topmost });
    const int width = rightmost - BRij.i;

    if (is_limit(top_right_separator)) {
        const TVertical& v_low =
            BRij.j - 1 < 0 ? TVertical{}
                           : V0[static_cast<std::size_t>(BRij.i)][static_cast<std::size_t>(BRij.j - 1)];
        const TVertical& v_high =
            topmost - 1 < 0 ? TVertical{}
                            : V0[static_cast<std::size_t>(rightmost)][static_cast<std::size_t>(topmost - 1)];
        A[static_cast<std::size_t>(rightmost)][static_cast<std::size_t>(topmost)].sep =
            expand(top_right_separator,
                   threshold(top_right_separator, shorter, v_low, v_high) + index,
                   shorter);
        return A;
    }

    const std::size_t top_len =
        std::min<std::size_t>(BRij.j < 0 ? 0 : static_cast<std::size_t>(BRij.j),
                              V0[static_cast<std::size_t>(BRij.i)].size());
    std::vector<TVertical> top_verticals(
        V0[static_cast<std::size_t>(BRij.i)].begin(),
        V0[static_cast<std::size_t>(BRij.i)].begin() + static_cast<std::ptrdiff_t>(top_len));
    top_verticals.push_back(V0[static_cast<std::size_t>(rightmost)][static_cast<std::size_t>(topmost)]);

    if (is_one(top_right_separator)) {
        A[static_cast<std::size_t>(rightmost)].pop_back();
    } else {
        TExpr new_sep(top_right_separator.begin(), top_right_separator.end() - 1);
        const TVertical& v_parent =
            BRij.j - 1 < 0 ? TVertical{}
                           : V0[static_cast<std::size_t>(BRij.i)][static_cast<std::size_t>(BRij.j - 1)];
        const TVertical& v_bottom =
            topmost - 1 < 0 ? TVertical{}
                            : V0[static_cast<std::size_t>(rightmost)][static_cast<std::size_t>(topmost - 1)];
        if (vertical_compare(vertical_increase(v_parent, new_sep), v_bottom) <= 0) {
            A[static_cast<std::size_t>(rightmost)].pop_back();
        } else {
            A[static_cast<std::size_t>(rightmost)][static_cast<std::size_t>(topmost)].sep = std::move(new_sep);
        }
    }
    {
        TColumn tail = A[static_cast<std::size_t>(rightmost)];
        const TColumn& br_col = A[static_cast<std::size_t>(BRij.i)];
        if (BRij.j >= 0 && static_cast<std::size_t>(BRij.j) < br_col.size()) {
            tail.insert(tail.end(),
                        br_col.begin() + static_cast<std::ptrdiff_t>(BRij.j),
                        br_col.end());
        }
        A[static_cast<std::size_t>(rightmost)] = std::move(tail);
    }

    std::vector<std::vector<TVertical>> V;
    V.reserve(A.size());
    for (const TColumn& c : A) V.push_back(column_verticals(c));

    const int br_i = BRij.i;
    std::vector<std::vector<int>> magma_checks_list(static_cast<std::size_t>(rightmost + 1));
    for (int i = br_i + 1; i <= rightmost; ++i) {
        std::vector<int>& row = magma_checks_list[static_cast<std::size_t>(i)];
        row.resize(A[static_cast<std::size_t>(i)].size());
        for (std::size_t j = 0; j < A[static_cast<std::size_t>(i)].size(); ++j) {
            int wi = i;
            int wj = static_cast<int>(j);
            while (wi > br_i) {
                if (static_cast<int>(A[static_cast<std::size_t>(wi)].size()) <= wj) --wj;
                if (wj < 0) {
                    wi = -1;
                    wj = 0;
                    break;
                }
                const TPosition p = parent(A, V, TPosition{ wi, wj });
                wi = p.i;
                wj = p.j;
            }
            const TVertical& v_work =
                wi == -1 || wj - 1 < 0 ? TVertical{}
                                       : V[static_cast<std::size_t>(wi)][static_cast<std::size_t>(wj - 1)];
            const TVertical& v_src =
                j == 0 ? TVertical{} : V[static_cast<std::size_t>(i)][j - 1];
            row[j] = (wi == br_i && wj <= BRij.j &&
                      vertical_compare(v_work, v_src) == 0)
                         ? wj
                         : -1;
        }
    }

    for (int n = 1; n <= index; ++n) {
        std::vector<int> refs = get_references(A, top_verticals);
        auto ref_at = [&](int k) { return k == -1 ? -1 : refs[static_cast<std::size_t>(k)]; };
        const int target_end = br_i + width * n + width + 1;
        if (static_cast<int>(A.size()) < target_end) A.resize(static_cast<std::size_t>(target_end));
        for (int dx = 1; dx <= width; ++dx) {
            const int x = br_i + dx;
            const std::vector<int>& source_magmas = magma_checks_list[static_cast<std::size_t>(x)];
            TColumn target_column;
            target_column.reserve(A[static_cast<std::size_t>(x)].size());
            for (std::size_t y = 0; y < A[static_cast<std::size_t>(x)].size(); ++y) {
                const TEntry& entry = A[static_cast<std::size_t>(x)][y];
                const int value = entry.value;
                const int sm = static_cast<int>(y) < static_cast<int>(source_magmas.size())
                                   ? source_magmas[y]
                                   : -1;
                if (sm >= 0) {
                    const int br_index = sm;
                    const int lo = ref_at(br_index - 1) + 1;
                    const int hi = ref_at(br_index);
                    for (int j = lo; j <= hi; ++j) {
                        if (j == hi) {
                            target_column.push_back(TEntry{ value + width * n, entry.sep });
                        } else {
                            target_column.push_back(TEntry{
                                value + width * n,
                                A[static_cast<std::size_t>(br_i + width * n)][static_cast<std::size_t>(j)].sep });
                        }
                    }
                } else {
                    target_column.push_back(TEntry{
                        value + (value > br_i ? width * n : 0), entry.sep });
                }
            }
            A[static_cast<std::size_t>(x + width * n)] = std::move(target_column);
        }
    }
    if (shorter) A.pop_back();
    return A;
}

inline TExpr infinity_fs(int n) {
    if (n <= 0) {
        TExpr e;
        e.push_back(TColumn{});
        return e;
    }
    TExpr e;
    e.push_back(TColumn{});
    TColumn col;
    col.push_back(TEntry{ 1, infinity_fs(n - 1) });
    e.push_back(std::move(col));
    return e;
}

} // namespace t_omega_mn_detail

// Expr：非 ∞ 时持有 TExpr；∞（显示 "Limit"）用标志表示。
struct TOmegaMnExpr {
    bool infinity = false;
    t_omega_mn_detail::TExpr expr;
};

struct TOmegaMNNotation {
    static constexpr const char* kName = "TωMN";

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
    [[nodiscard]] static std::optional<TOmegaMnExpr> parse(std::string_view text) {
        std::string s0(text);
        std::size_t b = 0, e = s0.size();
        while (b < e && (s0[b] == ' ' || s0[b] == '\t')) ++b;
        while (e > b && (s0[e - 1] == ' ' || s0[e - 1] == '\t')) --e;
        const std::string_view raw = std::string_view(s0).substr(b, e - b);
        if (raw == "Limit" || raw == "limit" || raw == "LIMIT") {
            return TOmegaMnExpr{ true, {} };
        }
        if (raw.empty() || raw == "0" || raw == "∅") {
            return TOmegaMnExpr{ false, {} };
        }
        auto m = t_omega_mn_detail::from_display(raw);
        if (!m) return std::nullopt;
        return TOmegaMnExpr{ false, std::move(*m) };
    }

    [[nodiscard]] static std::string to_string(const TOmegaMnExpr& e) {
        if (e.infinity) return "Limit";
        return t_omega_mn_detail::mountain_display(e.expr, false);
    }

    // 非 ∞ 表达式的比较（供测试用）。
    [[nodiscard]] static int compare(const TOmegaMnExpr& a, const TOmegaMnExpr& b) {
        if (a.infinity || b.infinity) {
            return a.infinity == b.infinity ? 0 : (a.infinity ? 1 : -1);
        }
        return t_omega_mn_detail::mountain_compare(a.expr, b.expr);
    }

private:
    static std::string expand_text_impl(std::string_view text, int term, bool longer) {
        if (term < 1) term = 1;
        const auto parsed = parse(text);
        if (!parsed) {
            return "解析失败，请确认你没有使用全角括号，逗号，分号";
        }
        if (parsed->infinity) {
            return t_omega_mn_detail::mountain_display(
                t_omega_mn_detail::infinity_fs(term), false);
        }
        const t_omega_mn_detail::TExpr& m = parsed->expr;
        if (m.empty()) return {};
        if (!t_omega_mn_detail::is_limit(m)) {
            return t_omega_mn_detail::mountain_display(
                t_omega_mn_detail::TExpr(m.begin(), m.end() - 1), false);
        }
        return t_omega_mn_detail::mountain_display(
            t_omega_mn_detail::expand(m, term, !longer), false);
    }
};

} // namespace omegay::notation

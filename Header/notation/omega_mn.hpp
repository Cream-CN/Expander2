// Header/notation/omega_mn.hpp
// ωMN 记号（display 名 "ωMN"，TS id "omega-mn"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/Omega_MN.ts
// （家族/类别：category-mn，credit.hypcos_mn）。
// 表达式结构（TS）：Sep=int；Vertical=Sep[]；Entry=[int,Sep]；
// Column=Entry[]；Expr=Column[]。竖向机制（vertical_* / parent /
// column_verticals / find_index_below）复用 smnshare.hpp。
// 与 n-MN 的关键区别在 expand：ωMN 用 get_references + magma_checks_list
// 的"坏根复制"算法（而非 fill_ghost/extend），且 ∞ 的基本列为 [[],[[1,n]]]。
// 层显示等价（convert_to_layer 等）为 UI 特性，C++ 侧不移植。
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "smnshare.hpp"

namespace omegay::notation {

// Expr：非 ∞ 时持有 Mountain；∞（显示 "Limit"）用标志表示。
struct OmegaMnExpr {
    bool infinity = false;
    smnshare::Mountain m;
};

namespace omega_mn_detail {

using smnshare::Column;
using smnshare::Entry;
using smnshare::Mountain;
using smnshare::Sep;
using smnshare::Vertical;

// ---------------------------------------------------------------------
// 解析（TS mountain_from_display；非 simple 语法，含 '0' 后紧跟 ',' 的分支）
// ---------------------------------------------------------------------

inline std::optional<Mountain> from_display(std::string_view s0) {
    std::string s(s0);
    std::size_t i = 0;

    auto fail = []() { return std::optional<Mountain>{}; };
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
    auto parse_sep = [&]() -> Sep {
        int count = 0;
        while (i < s.size() && s[i] == ',') {
            ++count;
            ++i;
        }
        return count == 0 ? 0 : count - 1;
    };
    auto parse_number = [&]() -> std::optional<int> {
        const std::size_t start = i;
        while (i < s.size() && s[i] >= '0' && s[i] <= '9') ++i;
        if (start == i) return std::nullopt;
        int v = 0;
        for (std::size_t k = start; k < i; ++k) {
            v = v * 10 + (s[k] - '0');
            if (v > 1000000) return std::nullopt;
        }
        return v;
    };
    auto parse_parenthesized_column = [&]() -> std::optional<Column> {
        ++i;  // '('
        Column col;
        skip_spaces();
        while (i < s.size() && s[i] != ')' && s[i] != ':') {
            skip_spaces();
            const Sep sep = parse_sep();
            skip_spaces();
            auto v = parse_number();
            if (!v) return std::nullopt;
            col.push_back(Entry{ *v, sep });
            skip_spaces();
        }
        skip_index();
        skip_spaces();
        if (i >= s.size() || s[i] != ')') return std::nullopt;
        ++i;
        return col;
    };
    auto parse_unparenthesized_column = [&]() -> std::optional<Column> {
        skip_spaces();
        if (i >= s.size()) return std::nullopt;
        if (s[i] == '0' &&
            (i + 1 >= s.size() || s[i + 1] == ':' || s[i + 1] == ' ' ||
             s[i + 1] == '(' || s[i + 1] == ',')) {
            ++i;
            skip_index();
            return Column{};
        }
        Column col;
        while (i < s.size() && s[i] != ' ' && s[i] != '(' && s[i] != ':') {
            if (s[i] != ',') return std::nullopt;
            const Sep sep = parse_sep();
            skip_spaces();
            auto v = parse_number();
            if (!v) return std::nullopt;
            col.push_back(Entry{ *v, sep });
        }
        skip_index();
        return col;
    };

    Mountain result;
    skip_spaces();
    while (i < s.size()) {
        std::optional<Column> col =
            s[i] == '(' ? parse_parenthesized_column()
                        : parse_unparenthesized_column();
        if (!col) return fail();
        result.push_back(std::move(*col));
        skip_spaces();
    }
    return result;
}

// ---------------------------------------------------------------------
// 比较 / 极限判定（TS mountain_compare / mountain_is_limit）
// ---------------------------------------------------------------------

inline int mountain_compare(const Mountain& a, const Mountain& b) {
    return smnshare::mountain_compare(a, b);
}

inline bool is_limit(const Mountain& m) {
    return !m.empty() && !m.back().empty();
}

inline Mountain infinity_fs(int n) {
    Mountain m;
    m.push_back(Column{});
    m.push_back(Column{ Entry{ 1, n } });
    return m;
}

// ---------------------------------------------------------------------
// 展开（TS get_references + expand）
// ---------------------------------------------------------------------

inline std::vector<int> get_references(const Mountain& m,
                                       const std::vector<Vertical>& r_tops) {
    std::vector<Vertical> verticals = smnshare::column_verticals(m.back());
    verticals.insert(verticals.begin(), Vertical{});
    std::vector<int> ref(r_tops.size(), -1);
    std::size_t i = 0, j = 0;
    while (i < verticals.size() && j < r_tops.size()) {
        if (smnshare::vertical_compare(verticals[i], r_tops[j]) < 0) {
            ref[j] = static_cast<int>(i);
            ++i;
        } else {
            ++j;
        }
    }
    return ref;
}

// TS expand(m0, index, shorter)。
inline Mountain expand(const Mountain& m0, int index, bool shorter) {
    const int rightmost = static_cast<int>(m0.size()) - 1;
    const int topmost = static_cast<int>(m0[static_cast<std::size_t>(rightmost)].size()) - 1;
    Mountain m = m0;

    if (topmost == -1) {
        m.pop_back();
        return m;
    }

    const Sep tr_separator = m[static_cast<std::size_t>(rightmost)][static_cast<std::size_t>(topmost)].sep;
    std::vector<std::vector<Vertical>> V0;
    V0.reserve(m.size());
    for (const Column& c : m) V0.push_back(smnshare::column_verticals(c));
    const smnshare::Position BRij =
        smnshare::parent(m, V0, smnshare::Position{ rightmost, topmost });
    const int width = rightmost - BRij.i;
    const std::size_t top_len =
        std::min<std::size_t>(BRij.j < 0 ? 0 : static_cast<std::size_t>(BRij.j),
                              V0[static_cast<std::size_t>(BRij.i)].size());
    std::vector<Vertical> top_verticals(
        V0[static_cast<std::size_t>(BRij.i)].begin(),
        V0[static_cast<std::size_t>(BRij.i)].begin() + static_cast<std::ptrdiff_t>(top_len));
    top_verticals.push_back(V0[static_cast<std::size_t>(rightmost)][static_cast<std::size_t>(topmost)]);

    if (tr_separator == 0) {
        m[static_cast<std::size_t>(rightmost)].pop_back();
    } else {
        const Sep new_tr_separator = tr_separator - 1;
        const Vertical& v_parent =
            BRij.j - 1 < 0 || BRij.j - 1 >= static_cast<int>(V0[static_cast<std::size_t>(BRij.i)].size())
                ? Vertical{}
                : V0[static_cast<std::size_t>(BRij.i)][static_cast<std::size_t>(BRij.j - 1)];
        const Vertical& v_bottom =
            topmost - 1 < 0 ? Vertical{}
                            : V0[static_cast<std::size_t>(rightmost)][static_cast<std::size_t>(topmost - 1)];
        if (smnshare::vertical_compare(smnshare::vertical_increase(v_parent, new_tr_separator),
                                       v_bottom) <= 0) {
            m[static_cast<std::size_t>(rightmost)].pop_back();
        } else {
            m[static_cast<std::size_t>(rightmost)][static_cast<std::size_t>(topmost)].sep = new_tr_separator;
        }
    }
    // TS: m[rightmost] = m[rightmost].concat(m[BRij[0]].slice(BRij[1]))
    {
        Column tail = m[static_cast<std::size_t>(rightmost)];
        const Column& br_col = m[static_cast<std::size_t>(BRij.i)];
        if (BRij.j >= 0 && static_cast<std::size_t>(BRij.j) < br_col.size()) {
            tail.insert(tail.end(),
                        br_col.begin() + static_cast<std::ptrdiff_t>(BRij.j),
                        br_col.end());
        }
        m[static_cast<std::size_t>(rightmost)] = std::move(tail);
    }

    std::vector<std::vector<Vertical>> V;
    V.reserve(m.size());
    for (const Column& c : m) V.push_back(smnshare::column_verticals(c));

    const int br_i = BRij.i;
    std::vector<std::vector<int>> magma_checks_list(static_cast<std::size_t>(rightmost + 1));
    for (int i = br_i + 1; i <= rightmost; ++i) {
        std::vector<int>& row = magma_checks_list[static_cast<std::size_t>(i)];
        row.resize(m[static_cast<std::size_t>(i)].size());
        for (std::size_t j = 0; j < m[static_cast<std::size_t>(i)].size(); ++j) {
            int wi = i;
            int wj = static_cast<int>(j);
            while (wi > br_i) {
                if (static_cast<int>(m[static_cast<std::size_t>(wi)].size()) <= wj) --wj;
                if (wj < 0) {
                    wi = -1;
                    wj = 0;
                    break;
                }
                const smnshare::Position p =
                    smnshare::parent(m, V, smnshare::Position{ wi, wj });
                wi = p.i;
                wj = p.j;
            }
            const Vertical& v_work =
                wi == -1 || wj - 1 < 0
                    ? Vertical{}
                    : V[static_cast<std::size_t>(wi)][static_cast<std::size_t>(wj - 1)];
            const Vertical& v_src =
                j == 0 ? Vertical{} : V[static_cast<std::size_t>(i)][j - 1];
            row[j] = (wi == br_i && wj <= BRij.j &&
                      smnshare::vertical_compare(v_work, v_src) == 0)
                         ? wj
                         : -1;
        }
    }

    for (int n = 1; n <= index; ++n) {
        std::vector<int> refs = get_references(m, top_verticals);
        auto ref_at = [&](int k) { return k == -1 ? -1 : refs[static_cast<std::size_t>(k)]; };
        const int target_end = br_i + width * n + width + 1;
        if (static_cast<int>(m.size()) < target_end) m.resize(static_cast<std::size_t>(target_end));
        for (int dx = 1; dx <= width; ++dx) {
            const int x = br_i + dx;
            const std::vector<int>& source_magmas = magma_checks_list[static_cast<std::size_t>(x)];
            Column target_column;
            target_column.reserve(m[static_cast<std::size_t>(x)].size());
            for (std::size_t y = 0; y < m[static_cast<std::size_t>(x)].size(); ++y) {
                const Entry& entry = m[static_cast<std::size_t>(x)][y];
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
                            target_column.push_back(Entry{ value + width * n, entry.sep });
                        } else {
                            target_column.push_back(Entry{
                                value + width * n,
                                m[static_cast<std::size_t>(br_i + width * n)][static_cast<std::size_t>(j)].sep });
                        }
                    }
                } else {
                    target_column.push_back(Entry{
                        value + (value > br_i ? width * n : 0), entry.sep });
                }
            }
            m[static_cast<std::size_t>(x + width * n)] = std::move(target_column);
        }
    }
    if (shorter) m.pop_back();
    return m;
}

} // namespace omega_mn_detail

struct OmegaMNNotation {
    static constexpr const char* kName = "ωMN";

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
    [[nodiscard]] static std::optional<OmegaMnExpr> parse(std::string_view text) {
        std::string s0(text);
        std::size_t b = 0, e = s0.size();
        while (b < e && (s0[b] == ' ' || s0[b] == '\t')) ++b;
        while (e > b && (s0[e - 1] == ' ' || s0[e - 1] == '\t')) --e;
        const std::string_view raw = std::string_view(s0).substr(b, e - b);
        if (raw == "Limit" || raw == "limit" || raw == "LIMIT") {
            return OmegaMnExpr{ true, {} };
        }
        if (raw.empty() || raw == "0" || raw == "∅") {
            return OmegaMnExpr{ false, {} };
        }
        auto m = omega_mn_detail::from_display(raw);
        if (!m) return std::nullopt;
        return OmegaMnExpr{ false, std::move(*m) };
    }

    [[nodiscard]] static std::string to_string(const OmegaMnExpr& e) {
        if (e.infinity) return "Limit";
        return smnshare::mountain_display(e.m, false);
    }

    // 非 ∞ 表达式的比较（供测试用）。
    [[nodiscard]] static int compare(const OmegaMnExpr& a, const OmegaMnExpr& b) {
        if (a.infinity || b.infinity) {
            return a.infinity == b.infinity ? 0 : (a.infinity ? 1 : -1);
        }
        return omega_mn_detail::mountain_compare(a.m, b.m);
    }

private:
    static std::string expand_text_impl(std::string_view text, int term, bool longer) {
        if (term < 1) term = 1;
        const auto parsed = parse(text);
        if (!parsed) {
            return "解析失败，请确认你没有使用全角括号，逗号，分号";
        }
        if (parsed->infinity) {
            return smnshare::mountain_display(omega_mn_detail::infinity_fs(term), false);
        }
        const smnshare::Mountain& m = parsed->m;
        if (m.empty()) return {};
        if (!omega_mn_detail::is_limit(m)) {
            return smnshare::mountain_display(smnshare::Mountain(m.begin(), m.end() - 1), false);
        }
        return smnshare::mountain_display(
            omega_mn_detail::expand(m, term, !longer), false);
    }
};

} // namespace omegay::notation

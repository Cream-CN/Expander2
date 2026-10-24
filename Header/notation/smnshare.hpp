// Header/notation/smnshare.hpp
// SMN 家族共享核心：n-MN（n_MN.ts）的表达式类型、显示/解析、比较与展开机制。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/SMN/n_MN.ts。
// 表达式结构（TS）：Sep=int；Vertical=Sep[]；Entry=[number,Sep]；
// Column=Entry[]；Mountain=Column[]；∞ 用顶层标志表示（display 为 "Limit"）。
// 说明：TS 的 memoization 缓存（按 display 键）为纯函数优化，本移植省略；
// fill_ghost 中 "undefined" Sep 用 kUndefSep 哨兵表达（undefined>任意值、==自身）。
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation::smnshare {

using Sep = int;
using Vertical = std::vector<Sep>;
struct Entry {
    int value;   // TS 元组第 0 项
    Sep sep;     // TS 元组第 1 项
};
using Column = std::vector<Entry>;
using Mountain = std::vector<Column>;

// TS "undefined" Sep 的哨兵（仅 fill_ghost 的 v[v.length-2] 取不到时产生）。
inline constexpr Sep kUndefSep = 2147483647;

// ---------------------------------------------------------------------
// 基础比较
// ---------------------------------------------------------------------

inline int num_compare(int a, int b) {
    return a == b ? 0 : (a < b ? -1 : 1);
}
inline int bool_compare(bool a, bool b) {
    return (a ? 1 : 0) - (b ? 1 : 0);
}

// TS lex_compare：元素逐个 cmp；长度按 longer_last 决定谁更大。
template <typename T, typename Cmp>
inline int lex_compare(const std::vector<T>& a, const std::vector<T>& b,
                       Cmp cmp, bool longer_last = true) {
    const std::size_t n = a.size() < b.size() ? a.size() : b.size();
    for (std::size_t i = 0; i < n; ++i) {
        const int r = cmp(a[i], b[i]);
        if (r != 0) return r;
    }
    return (longer_last ? 1 : -1) * num_compare(static_cast<int>(a.size()),
                                                static_cast<int>(b.size()));
}

inline int sep_compare(Sep a, Sep b) {
    if (a == kUndefSep || b == kUndefSep) {
        if (a == b) return 0;
        return a == kUndefSep ? 1 : -1;
    }
    return num_compare(a, b);
}

inline int vertical_compare(const Vertical& a, const Vertical& b) {
    return lex_compare(a, b, [](Sep x, Sep y) { return sep_compare(x, y); });
}

inline int entry_compare(const Entry& a, const Entry& b) {
    const int r = num_compare(a.value, b.value);
    if (r != 0) return r;
    return sep_compare(a.sep, b.sep);
}

inline int column_compare(const Column& a, const Column& b) {
    return lex_compare(a, b, [](const Entry& x, const Entry& y) {
        return entry_compare(x, y);
    });
}

inline int mountain_compare(const Mountain& a, const Mountain& b) {
    return lex_compare(a, b, [](const Column& x, const Column& y) {
        return column_compare(x, y);
    });
}

// ---------------------------------------------------------------------
// 显示（TS mountain_display / entry_display / column_display / sep_display）
// ---------------------------------------------------------------------

inline std::string sep_display(Sep sep, bool simple) {
    if (simple && sep == 0) return "";
    return std::string(static_cast<std::size_t>(sep + 1), ',');
}

inline std::string entry_display(const Entry& e, bool simple) {
    std::string d = sep_display(e.sep, simple);
    std::string dv = std::to_string(e.value);
    if (simple && dv.size() >= 2) dv = "(" + dv + ")";
    return d + dv;
}

inline std::string column_display(const Column& c, bool simple) {
    if (simple && c.empty()) return "0";
    std::string result;
    for (const Entry& e : c) result += entry_display(e, simple);
    return simple ? result : "(" + result + ")";
}

inline std::string mountain_display(const Mountain& m, bool simple) {
    std::string out;
    for (std::size_t i = 0; i < m.size(); ++i) {
        if (i > 0 && simple) out += ' ';
        out += column_display(m[i], simple);
    }
    return out;
}

inline std::string to_data_key(const Mountain& m) {
    return mountain_display(m, true);
}

// ---------------------------------------------------------------------
// 解析（TS from_display / from_display_simple；非法输入返回 nullopt）
// ---------------------------------------------------------------------

inline std::optional<Mountain> from_display(std::string_view s) {
    std::size_t i = 0;
    auto error = [&]() { return std::optional<Mountain>{}; };
    if (s == "Limit") {
        // 由调用方（FS 包装器）识别 ∞；此处不返回哨兵值，直接视为非法。
        return error();
    }

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
        for (std::size_t k = start; k < i; ++k) v = v * 10 + (s[k] - '0');
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
             s[i + 1] == '(')) {
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
        if (!col) return std::nullopt;
        result.push_back(std::move(*col));
        skip_spaces();
    }
    return result;
}

inline std::optional<Mountain> from_display_simple(std::string_view s) {
    std::size_t i = 0;
    auto error = [&]() { return std::optional<Mountain>{}; };
    auto skip_spaces = [&]() {
        while (i < s.size() && s[i] == ' ') ++i;
    };
    auto parse_sep = [&]() -> Sep {
        int count = 0;
        while (i < s.size() && s[i] == ',') {
            ++count;
            ++i;
        }
        return count == 0 ? 0 : count - 1;
    };
    auto parse_entry = [&]() -> std::optional<Entry> {
        const Sep sep = parse_sep();
        int v;
        if (i < s.size() && s[i] == '(') {
            ++i;
            const std::size_t start = i;
            while (i < s.size() && s[i] >= '0' && s[i] <= '9') ++i;
            if (start == i) return std::nullopt;
            if (i >= s.size() || s[i] != ')') return std::nullopt;
            v = 0;
            for (std::size_t k = start; k < i; ++k) v = v * 10 + (s[k] - '0');
            ++i;
        } else if (i < s.size() && s[i] >= '0' && s[i] <= '9') {
            v = s[i] - '0';
            ++i;
        } else {
            return std::nullopt;
        }
        return Entry{ v, sep };
    };
    auto parse_column = [&]() -> std::optional<Column> {
        Column col;
        while (i < s.size() && s[i] != ' ') {
            auto e = parse_entry();
            if (!e) return std::nullopt;
            col.push_back(std::move(*e));
        }
        return col;
    };
    auto parse_expr = [&]() -> std::optional<Mountain> {
        Mountain result;
        while (true) {
            skip_spaces();
            if (i >= s.size()) break;
            if (s[i] == '0' && (i + 1 >= s.size() || s[i + 1] == ' ')) {
                result.push_back(Column{});
                ++i;
                continue;
            }
            auto col = parse_column();
            if (!col) return std::nullopt;
            result.push_back(std::move(*col));
        }
        return result;
    };

    skip_spaces();
    if (i + 5 <= s.size() && s.substr(i, 5) == "Limit") {
        i += 5;
        skip_spaces();
        if (i != s.size()) return error();
        // ∞ 由调用方识别。
        return error();
    }
    auto result = parse_expr();
    if (!result) return std::nullopt;
    skip_spaces();
    if (i != s.size()) return error();
    return result;
}

// ---------------------------------------------------------------------
// 山结构辅助（TS vertical_* / column_verticals / find_index_* / parent）
// ---------------------------------------------------------------------

// TS vertical_diff：返回第一个不相等的元素（越界按 undefined → 哨兵）。
inline Sep vertical_diff(const Vertical& v1, const Vertical& v2) {
    std::size_t i = 0;
    while (i < v2.size() && i < v1.size() && v1[i] == v2[i]) ++i;
    return i < v1.size() ? v1[i] : kUndefSep;
}

inline Vertical vertical_increase(const Vertical& v, Sep s) {
    std::size_t i = v.size();
    while (i >= 1 && sep_compare(v[i - 1], s) < 0) --i;
    Vertical result(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(i));
    result.push_back(s);
    return result;
}

inline std::vector<Vertical> column_verticals(const Column& c) {
    std::vector<Vertical> result;
    result.reserve(c.size());
    Vertical current;
    for (const Entry& e : c) {
        current = vertical_increase(current, e.sep);
        result.push_back(current);
    }
    return result;
}

// TS find_index_below：二分查找第一个 Vi[j-1] < v 的位置（升序表）。
inline int find_index_below(const std::vector<Vertical>& Vi, const Vertical& v) {
    int l = 0, r = static_cast<int>(Vi.size());
    while (l < r) {
        const int j = (l + r + 1) / 2;   // Math.ceil((l+r)/2)
        const Vertical& Vij = j == 0 ? Vertical{} : Vi[j - 1];
        if (vertical_compare(Vij, v) < 0) l = j;
        else r = j - 1;
    }
    return l;
}

inline int find_index_below_equal(const std::vector<Vertical>& Vi, const Vertical& v) {
    int l = 0, r = static_cast<int>(Vi.size());
    while (l < r) {
        const int j = (l + r + 1) / 2;
        const Vertical& Vij = j == 0 ? Vertical{} : Vi[j - 1];
        if (vertical_compare(Vij, v) <= 0) l = j;
        else r = j - 1;
    }
    return l;
}

struct Position {
    int i;
    int j;
};

// TS parent：父位置；pi==-1 时 pj=0。
inline Position parent(const Mountain& m, const std::vector<std::vector<Vertical>>& V,
                       Position pos) {
    const int value = m[static_cast<std::size_t>(pos.i)][static_cast<std::size_t>(pos.j)].value;
    const int pi = value - 1;
    const int pj = pi == -1 ? 0 : find_index_below(V[static_cast<std::size_t>(pi)],
                                                   V[static_cast<std::size_t>(pos.i)][static_cast<std::size_t>(pos.j)]);
    return Position{ pi, pj };
}

// TS magma_indices。
inline std::vector<std::vector<int>> magma_indices(
    const Mountain& m, const std::vector<std::vector<Vertical>>& V,
    Position RiRj, std::vector<std::vector<int>> result = {}) {
    const int Ri = RiRj.i, Rj = RiRj.j;
    for (std::size_t i = result.size(); i < m.size(); ++i) {
        result.push_back({});
        if (static_cast<int>(i) <= Ri) continue;
        for (std::size_t j = 0; j < m[i].size(); ++j) {
            const Position p = parent(m, V, Position{ static_cast<int>(i), static_cast<int>(j) });
            int pi = p.i, pj = p.j;
            if (pi < Ri) {
                break;
            } else if (pi == Ri) {
                result[i].push_back(pj < Rj ? pj : Rj);
            } else {
                if (pj == static_cast<int>(m[static_cast<std::size_t>(pi)].size())) --pj;
                if (pj >= static_cast<int>(result[static_cast<std::size_t>(pi)].size())) break;
                result[i].push_back(result[static_cast<std::size_t>(pi)][static_cast<std::size_t>(pj)]);
            }
        }
    }
    return result;
}

// TS fill_ghost：给父列补幽灵行。
inline Mountain fill_ghost(const Mountain& m0) {
    Mountain m = m0;
    std::vector<std::vector<Vertical>> V;
    V.reserve(m.size());
    for (const Column& c : m) V.push_back(column_verticals(c));

    for (std::size_t i = 0; i < m.size(); ++i) {
        for (std::size_t j = 0; j < m[i].size(); ++j) {
            const Position p = parent(m, V, Position{ static_cast<int>(i), static_cast<int>(j) });
            if (p.j != static_cast<int>(m[static_cast<std::size_t>(p.i)].size())) continue;
            const Vertical& v_parent = p.j == 0 ? Vertical{} : V[static_cast<std::size_t>(p.i)][static_cast<std::size_t>(p.j) - 1];
            const Vertical& v = V[i][j];
            const Sep sep = m[i][j].sep;
            if (vertical_compare(vertical_increase(v_parent, sep), v) < 0) {
                const Sep undef_sep = v.size() >= 2 ? v[v.size() - 2] : kUndefSep;
                m[static_cast<std::size_t>(p.i)].push_back(Entry{ 0, undef_sep });
                Vertical vv(v.begin(), v.end() - 1);
                V[static_cast<std::size_t>(p.i)].push_back(std::move(vv));
            }
        }
    }
    return m;
}

// TS clear_ghost。
inline Mountain clear_ghost(const Mountain& m) {
    Mountain out;
    out.reserve(m.size());
    for (const Column& c : m) {
        Column nc;
        nc.reserve(c.size());
        for (const Entry& e : c) {
            if (e.value != 0) nc.push_back(e);
        }
        out.push_back(std::move(nc));
    }
    return out;
}

// TS subtract_1。
inline Mountain subtract_1(const Mountain& m0, std::vector<std::vector<Vertical>>* V = nullptr) {
    std::vector<std::vector<Vertical>> V_own;
    if (!V) {
        V_own.reserve(m0.size());
        for (const Column& c : m0) V_own.push_back(column_verticals(c));
        V = &V_own;
    }
    const int right = static_cast<int>(m0.size()) - 1;
    const int top = static_cast<int>(m0[static_cast<std::size_t>(right)].size()) - 1;
    const Sep top_right_sep = m0[static_cast<std::size_t>(right)][static_cast<std::size_t>(top)].sep;
    const Position p = parent(m0, *V, Position{ right, top });
    const int Ri = p.i, Rj = p.j;

    Mountain result = m0;
    result[static_cast<std::size_t>(right)].pop_back();

    if (top_right_sep > 0) {
        const Sep new_sep = top_right_sep - 1;
        const Vertical& v_parent = Rj == 0 ? Vertical{} : (*V)[static_cast<std::size_t>(Ri)][static_cast<std::size_t>(Rj) - 1];
        const Vertical& v_bottom = top == 0 ? Vertical{} : (*V)[static_cast<std::size_t>(right)][static_cast<std::size_t>(top) - 1];
        if (vertical_compare(vertical_increase(v_parent, new_sep), v_bottom) > 0) {
            result[static_cast<std::size_t>(right)].push_back(Entry{ Ri + 1, new_sep });
        }
    }

    for (std::size_t j = static_cast<std::size_t>(Rj); j < m0[static_cast<std::size_t>(Ri)].size(); ++j) {
        result[static_cast<std::size_t>(right)].push_back(m0[static_cast<std::size_t>(Ri)][j]);
    }
    return result;
}

// TS copy_column。
inline Column copy_column(const Column& m0i, const std::vector<int>& MI0i,
                          const Column& mr, const std::vector<int>& MIr,
                          Position RiRj, int offset) {
    const int Ri = RiRj.i, Rj = RiRj.j;
    Column result;
    result.reserve(m0i.size());
    int last_mi = -1;
    std::size_t ref_j = 0;
    for (std::size_t j = 0; j < m0i.size(); ++j) {
        if (j >= MI0i.size()) {
            Entry entry = m0i[j];
            if (entry.value >= Ri + 1) entry.value += offset;
            result.push_back(entry);
        } else {
            const Entry& e = m0i[j];
            const int new_value = e.value + offset;
            const int current_mi = MI0i[j];
            if (current_mi != last_mi) {
                last_mi = current_mi;
                while (ref_j < MIr.size() && MIr[ref_j] == current_mi) {
                    const bool is_row_lifting =
                        current_mi == Rj ||
                        (ref_j + 1 < MIr.size() && MIr[ref_j + 1] == current_mi);
                    if (is_row_lifting) {
                        result.push_back(Entry{ new_value, mr[ref_j].sep });
                    }
                    ++ref_j;
                }
            }
            result.push_back(Entry{ new_value, e.sep });
        }
    }
    return result;
}

// TS extend。
inline Mountain extend(const Mountain& m0) {
    const int right = static_cast<int>(m0.size()) - 1;
    const int top = static_cast<int>(m0[static_cast<std::size_t>(right)].size()) - 1;

    std::vector<std::vector<Vertical>> V0;
    V0.reserve(m0.size());
    for (const Column& c : m0) V0.push_back(column_verticals(c));
    const Position p = parent(m0, V0, Position{ right, top });
    const std::vector<std::vector<int>> MI0 = magma_indices(m0, V0, p);

    Mountain m = subtract_1(m0, &V0);
    std::vector<std::vector<Vertical>> V = V0;
    V.resize(static_cast<std::size_t>(right));
    V.push_back(column_verticals(m[static_cast<std::size_t>(right)]));
    std::vector<std::vector<int>> MI_partial(MI0.begin(), MI0.begin() + right);
    const std::vector<std::vector<int>> MI = magma_indices(m, V, p, MI_partial);

    const int offset = right - p.i;
    for (int i = p.i + 1; i < static_cast<int>(m0.size()); ++i) {
        m.push_back(copy_column(m0[static_cast<std::size_t>(i)], MI0[static_cast<std::size_t>(i)],
                                m[static_cast<std::size_t>(right)], MI[static_cast<std::size_t>(right)],
                                p, offset));
    }
    return m;
}

// TS NT_infinity_FS(n)(index)：∞ 的第 index 个基本列。
inline Mountain nt_infinity_fs(int n, int index) {
    Mountain m;
    m.push_back(Column{});
    Column col;
    col.reserve(static_cast<std::size_t>(index < 0 ? 0 : index));
    for (int k = 0; k < index; ++k) col.push_back(Entry{ 1, n - 1 });
    m.push_back(std::move(col));
    return m;
}

// TS expand(m, index, shorter)。前置条件：m 非 ∞（FS 包装器已处理）。
inline Mountain expand(const Mountain& m, int index, bool shorter) {
    if (m.empty()) return m;
    if (m.back().empty()) return Mountain(m.begin(), m.end() - 1);
    Mountain current = fill_ghost(m);
    for (int i = 0; i < index; ++i) current = extend(current);
    current = shorter ? Mountain(current.begin(), current.end() - 1)
                      : subtract_1(current);
    current = clear_ghost(current);
    return current;
}

// ---------------------------------------------------------------------
// 层变换（TS convert_to_layer / convert_from_layer）
// ---------------------------------------------------------------------

inline std::vector<std::vector<int>> calc_ancestor_depths(const Mountain& m) {
    std::vector<std::vector<Vertical>> V;
    V.reserve(m.size());
    for (const Column& c : m) V.push_back(column_verticals(c));
    std::vector<std::vector<int>> depth_map(m.size());
    for (std::size_t i = 0; i < m.size(); ++i) {
        depth_map[i].resize(m[i].size());
        for (std::size_t j = 0; j < m[i].size(); ++j) {
            const Position p = parent(m, V, Position{ static_cast<int>(i), static_cast<int>(j) });
            depth_map[i][j] = p.j == static_cast<int>(m[static_cast<std::size_t>(p.i)].size())
                                  ? 1
                                  : 1 + depth_map[static_cast<std::size_t>(p.i)][static_cast<std::size_t>(p.j)];
        }
    }
    return depth_map;
}

inline Mountain convert_to_layer(const Mountain& om) {
    const auto depth_map = calc_ancestor_depths(om);
    Mountain dm = om;
    for (std::size_t i = 0; i < dm.size(); ++i) {
        for (std::size_t j = 0; j < dm[i].size(); ++j) {
            dm[i][j].value = depth_map[i][j];
        }
    }
    return dm;
}

inline Mountain convert_from_layer(const Mountain& dm) {
    Mountain om = dm;
    std::vector<std::vector<Vertical>> V;
    V.reserve(om.size());
    for (const Column& c : om) V.push_back(column_verticals(c));
    for (std::size_t i = 0; i < om.size(); ++i) {
        for (std::size_t j = 0; j < om[i].size(); ++j) {
            Entry& entry = om[i][j];
            int i1 = static_cast<int>(i);
            int j1 = static_cast<int>(j) - 1;
            while (true) {
                if (i1 == 0) {
                    entry.value = 1;
                    break;
                }
                if (j1 >= 0) {
                    const Position p = parent(om, V, Position{ i1, j1 });
                    i1 = p.i;
                    j1 = p.j;
                } else {
                    i1 = i1 - 1;
                }
                const Vertical& v_below = j == 0 ? Vertical{} : V[i][j - 1];
                const int j0 = find_index_below_equal(V[static_cast<std::size_t>(i1)], v_below);
                if (j0 == static_cast<int>(dm[static_cast<std::size_t>(i1)].size()) ||
                    dm[static_cast<std::size_t>(i1)][static_cast<std::size_t>(j0)].value < entry.value) {
                    entry.value = i1 + 1;
                    break;
                }
            }
        }
    }
    return om;
}

} // namespace omegay::notation::smnshare

// Header/notation/aw2mn_share.hpp
// Aω2MN2 / Aω2MN3（及弱版 wAω2MN2 / wAω2MN3）的共享实现核心。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/Aw2MN2.ts / Aw2MN3.ts
// （家族/类别：category-hypcos-w2mn）。
// 结构（TS）：Sep=Expr（递归！）；Entry=[value(1-based), Sep, mark?]；Column=Entry[]；
// Expr=Column[]；Vertical=Sep[]。∞ 用 bool 标志表示。
// 差异：Aω2MN2 用 subtract1 版 2（limit 分隔符分支：J 由 BR_separator 构造并仅在
// vertical_increase(alpha,J) 高于 working_vertical 时入列）；Aω2MN3 用 subtract1 版 3
// （is_one 提前返回；limit 分支用 while 循环加长 J 至两侧 vertical 相等）。
// 弱版与普通版仅 weak=true（stretch 时 V0[x][y] 不低于 top_vertical 的条目保持原分隔符）。
// UI 能力（display_equiv layer/marked、draw_diagram 等）不在文本记号契约内，未移植。
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {
namespace aw2mn_share {

struct Entry;
using Column = std::vector<Entry>;
using Expr = std::vector<Column>;
using Sep = Expr;
using Vertical = std::vector<Sep>;
using VV = std::vector<std::vector<Vertical>>;

struct Entry {
    int value;    // 1-based（TS 元组第 0 项）
    Sep sep;      // 第 1 项（递归结构）
    bool mark;    // 第 2 项
};

inline int num_compare(int a, int b) { return a == b ? 0 : (a < b ? -1 : 1); }

template <typename T, typename Cmp>
inline int lex_compare(const std::vector<T>& a, const std::vector<T>& b, Cmp cmp,
                       bool longer_last = true) {
    const std::size_t n = a.size() < b.size() ? a.size() : b.size();
    for (std::size_t i = 0; i < n; ++i) {
        const int r = cmp(a[i], b[i]);
        if (r != 0) return r;
    }
    return (longer_last ? 1 : -1) * num_compare(static_cast<int>(a.size()), static_cast<int>(b.size()));
}

inline int entry_compare(const Entry& a, const Entry& b);
inline int column_compare(const Column& a, const Column& b) {
    return lex_compare(a, b, [](const Entry& x, const Entry& y) { return entry_compare(x, y); });
}
inline int mountain_compare(const Expr& a, const Expr& b) {
    return lex_compare(a, b, [](const Column& x, const Column& y) { return column_compare(x, y); });
}
inline int entry_compare(const Entry& a, const Entry& b) {
    const int r = num_compare(a.value, b.value);
    if (r != 0) return r;
    return mountain_compare(a.sep, b.sep);
}
inline int vertical_compare(const Vertical& a, const Vertical& b) {
    return lex_compare(a, b, [](const Sep& x, const Sep& y) { return mountain_compare(x, y); });
}

// ---- 谓词（对任意 Expr 生效，分隔符亦为 Expr）----

inline bool mountain_is_limit(const Expr& m) {
    return !m.empty() && !m.back().empty();
}
inline bool mountain_is_one(const Expr& m) {
    return m.size() == 1 && m[0].empty();
}

// ---- 显示（TS sep_display / entry_display / column_display / mountain_display）----

inline std::string mountain_display(const Expr& m);

inline bool is_omega_sep(const Sep& sep) {
    if (sep.size() < 2 || !sep[0].empty()) return false;
    const Column& c1 = sep[1];
    if (c1.size() != 1) return false;
    const Entry& e = c1[0];
    if (e.value != 1 || e.mark) return false;
    if (e.sep.size() != 1 || !e.sep[0].empty()) return false;
    for (std::size_t k = 2; k < sep.size(); ++k) {
        if (!sep[k].empty()) return false;
    }
    return true;
}

inline std::string sep_display(const Sep& sep) {
    bool all_empty = true;
    for (const Column& c : sep) {
        if (!c.empty()) { all_empty = false; break; }
    }
    if (all_empty) return std::string(sep.size(), ',');
    if (is_omega_sep(sep)) {
        std::string out = ";";
        out.append(sep.size() > 2 ? sep.size() - 2 : 0, ',');
        return out;
    }
    return mountain_display(sep);
}

inline std::string entry_display(const Entry& e) {
    return sep_display(e.sep) + (e.mark ? "*" : "") + std::to_string(e.value);
}
inline std::string column_display(const Column& c) {
    std::string out;
    for (const Entry& e : c) out += entry_display(e);
    return "(" + out + ")";
}
inline std::string mountain_display(const Expr& m) {
    std::string out;
    for (const Column& c : m) out += column_display(c);
    return out;
}

// ---- 解析（TS from_display；非法输入返回 nullopt）----

struct Parsed {
    bool infinity = false;
    Expr expr;
};

inline std::optional<Parsed> parse(std::string_view s) {
    if (s == "Limit" || s == "limit" || s == "LIMIT") return Parsed{ true, {} };
    std::size_t i = 0;
    auto error = [&]() { return std::optional<Parsed>{}; };

    // Omega = [[], [[1, [[]]]]]
    auto omega_sep = []() {
        Sep sep;
        sep.emplace_back();
        Column c1;
        Sep inner;
        inner.emplace_back();
        c1.push_back(Entry{ 1, std::move(inner), false });
        sep.push_back(std::move(c1));
        return sep;
    };
    auto parse_sep = [&]() -> std::optional<Sep> {
        if (i < s.size() && s[i] == ';') {
            ++i;
            int c0 = 0;
            while (i < s.size() && s[i] == ',') { ++c0; ++i; }
            Sep sep = omega_sep();
            for (int k = 0; k < c0; ++k) sep.emplace_back();
            return sep;
        }
        int c0 = 0;
        while (i < s.size() && s[i] == ',') { ++c0; ++i; }
        if (c0 == 0) return std::nullopt;
        Sep sep;
        for (int k = 0; k < c0; ++k) sep.emplace_back();
        return sep;
    };
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
        const std::size_t start = i;
        while (i < s.size() && s[i] >= '0' && s[i] <= '9') ++i;
        if (start == i) return std::nullopt;
        int v = 0;
        for (std::size_t k = start; k < i; ++k) v = v * 10 + (s[k] - '0');
        return v;
    };
    auto parse_entry = [&]() -> std::optional<Entry> {
        skip_spaces();
        auto sep = parse_sep();
        if (!sep) return std::nullopt;
        skip_spaces();
        bool mark = false;
        if (i < s.size() && s[i] == '*') { mark = true; ++i; }
        auto v = parse_number();
        if (!v) return std::nullopt;
        return Entry{ *v, std::move(*sep), mark };
    };
    auto parse_parenthesized_column = [&]() -> std::optional<Column> {
        ++i;  // '('
        Column col;
        skip_spaces();
        while (i < s.size() && s[i] != ')' && s[i] != ':') {
            auto e = parse_entry();
            if (!e) return std::nullopt;
            col.push_back(std::move(*e));
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
        if (s[i] == '0' && (i + 1 >= s.size() || s[i + 1] == ':' || s[i + 1] == ' ' ||
                            s[i + 1] == '(' || s[i + 1] == ',' || s[i + 1] == ';')) {
            ++i;
            skip_index();
            return Column{};
        }
        if (s[i] == ':') {
            skip_index();
            return Column{};
        }
        Column col;
        while (i < s.size() && s[i] != ' ' && s[i] != '(' && s[i] != ':') {
            auto e = parse_entry();
            if (!e) return std::nullopt;
            col.push_back(std::move(*e));
            skip_spaces();
        }
        skip_index();
        return col;
    };

    Expr result;
    skip_spaces();
    while (i < s.size()) {
        auto col = s[i] == '(' ? parse_parenthesized_column() : parse_unparenthesized_column();
        if (!col) return error();
        result.push_back(std::move(*col));
        skip_spaces();
    }
    return Parsed{ false, std::move(result) };
}

// ---- 山结构辅助 ----

inline Vertical vertical_increase(const Vertical& v, const Sep& m) {
    int i = static_cast<int>(v.size()) - 1;
    while (i >= 0 && mountain_compare(v[static_cast<std::size_t>(i)], m) < 0) --i;
    Vertical result(v.begin(), v.begin() + i + 1);
    result.push_back(m);
    return result;
}

inline std::vector<Vertical> column_verticals(const Column& c) {
    std::vector<Vertical> v;
    v.push_back(Vertical{});
    for (const Entry& e : c) v.push_back(vertical_increase(v.back(), e.sep));
    v.erase(v.begin());
    return v;
}

// TS find_index_below_row：working = [[], ...verticals]
inline int find_index_below_row(const std::vector<Vertical>& verticals, const Vertical& y) {
    int l = 0, r = static_cast<int>(verticals.size());
    while (l < r) {
        const int m = (l + r + 1) / 2;
        const Vertical& wm = m == 0 ? Vertical{} : verticals[static_cast<std::size_t>(m - 1)];
        if (vertical_compare(wm, y) < 0) l = m;
        else r = m - 1;
    }
    return l;
}

inline std::pair<int, int> Parent(const Expr& A, const VV& V, std::pair<int, int> ij) {
    const int target_column = A[static_cast<std::size_t>(ij.first)][static_cast<std::size_t>(ij.second)].value - 1;
    const int target_j = find_index_below_row(V[static_cast<std::size_t>(target_column)],
                                              V[static_cast<std::size_t>(ij.first)][static_cast<std::size_t>(ij.second)]);
    return { target_column, target_j };
}

// TS get_references：返回按 rtop 下标索引的 optional 引用；refs[-1]=-1 由调用方约定。
inline std::vector<std::optional<int>> get_references(const Expr& A, const std::vector<Vertical>& rtops,
                                                      int& refs_length) {
    std::vector<Vertical> verticals = column_verticals(A.back());
    verticals.insert(verticals.begin(), Vertical{});
    std::vector<std::optional<int>> ref(rtops.size());
    refs_length = 0;
    std::size_t i = 0, j = 0;
    while (i < verticals.size() && j < rtops.size()) {
        if (vertical_compare(verticals[i], rtops[j]) < 0) {
            ref[j] = static_cast<int>(i);
            refs_length = static_cast<int>(j) + 1;
            ++i;
        } else {
            ++j;
        }
    }
    return ref;
}

inline Sep S(const Expr& A, int i, int j) {
    if (i < 0 || i >= static_cast<int>(A.size()) || j < 0 ||
        j >= static_cast<int>(A[static_cast<std::size_t>(i)].size())) {
        return {};
    }
    const Entry& e = A[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
    return e.mark ? e.sep : S(A, i, j - 1);
}

enum class SubtractKind { V2, V3 };

inline Expr subtract1(const Expr& A0, const VV& V0, SubtractKind kind) {
    const int rightmost = static_cast<int>(A0.size()) - 1;
    const int topmost = static_cast<int>(A0[static_cast<std::size_t>(rightmost)].size()) - 1;
    Expr A = A0;
    const int top_right_value = A[static_cast<std::size_t>(rightmost)][static_cast<std::size_t>(topmost)].value;
    const Sep top_right_separator = A[static_cast<std::size_t>(rightmost)][static_cast<std::size_t>(topmost)].sep;
    const std::pair<int, int> BRij = Parent(A, V0, { rightmost, topmost });

    A[static_cast<std::size_t>(rightmost)].pop_back();

    const auto alpha = BRij.second - 1 >= 0
                           ? V0[static_cast<std::size_t>(BRij.first)][static_cast<std::size_t>(BRij.second - 1)]
                           : Vertical{};
    const auto working_vertical_0 = topmost - 1 >= 0
                                        ? V0[static_cast<std::size_t>(rightmost)][static_cast<std::size_t>(topmost - 1)]
                                        : Vertical{};

    auto copy_entries_loop = [&](Vertical& working_vertical) {
        if (vertical_compare(alpha, working_vertical) <= 0) return;
        int i = working_vertical.empty() ? -1
                                         : find_index_below_row(V0[static_cast<std::size_t>(BRij.first)], working_vertical);
        while (++i < BRij.second) {
            const Entry& src = A[static_cast<std::size_t>(BRij.first)][static_cast<std::size_t>(i)];
            A[static_cast<std::size_t>(rightmost)].push_back(Entry{ top_right_value, src.sep, src.mark });
            working_vertical = vertical_increase(working_vertical, src.sep);
        }
    };

    if (kind == SubtractKind::V2) {
        if (mountain_is_limit(top_right_separator)) {
            const Sep BR_separator = S(A, BRij.first, BRij.second - 1);
            Sep J = mountain_compare(BR_separator, top_right_separator) >= 0
                        ? Sep{ Column{} }
                        : [&]() {
                              Sep s = BR_separator;
                              s.emplace_back();
                              return s;
                          }();
            Vertical working_vertical = working_vertical_0;
            if (vertical_compare(vertical_increase(alpha, J), working_vertical) > 0) {
                if (vertical_compare(alpha, working_vertical) > 0) {
                    copy_entries_loop(working_vertical);
                }
                A[static_cast<std::size_t>(rightmost)].push_back(Entry{ top_right_value, std::move(J), true });
            }
        } else if (!mountain_is_one(top_right_separator)) {
            Sep truncated = top_right_separator;
            if (!truncated.empty()) truncated.pop_back();
            Vertical working_vertical = working_vertical_0;
            copy_entries_loop(working_vertical);
            if (vertical_compare(alpha, working_vertical) == 0) {
                A[static_cast<std::size_t>(rightmost)].push_back(Entry{ top_right_value, std::move(truncated), false });
            }
        }
    } else {  // V3
        if (mountain_is_one(top_right_separator)) return A;
        Vertical working_vertical = working_vertical_0;
        copy_entries_loop(working_vertical);
        if (mountain_is_limit(top_right_separator)) {
            const Sep BR_separator = S(A, BRij.first, BRij.second - 1);
            Sep J = mountain_compare(BR_separator, top_right_separator) >= 0
                        ? Sep{ Column{} }
                        : [&]() {
                              Sep s = BR_separator;
                              s.emplace_back();
                              return s;
                          }();
            while (vertical_compare(vertical_increase(alpha, J), vertical_increase(working_vertical, J)) != 0) {
                J.emplace_back();
            }
            A[static_cast<std::size_t>(rightmost)].push_back(Entry{ top_right_value, std::move(J), true });
        } else {
            Sep truncated = top_right_separator;
            if (!truncated.empty()) truncated.pop_back();
            if (vertical_compare(vertical_increase(alpha, truncated), working_vertical) > 0) {
                A[static_cast<std::size_t>(rightmost)].push_back(Entry{ top_right_value, std::move(truncated), false });
            }
        }
    }
    return A;
}

inline Expr extend(const Expr& A0, bool is_small, bool weak, SubtractKind kind) {
    const int rightmost = static_cast<int>(A0.size()) - 1;
    const int topmost = static_cast<int>(A0[static_cast<std::size_t>(rightmost)].size()) - 1;
    VV V0;
    V0.reserve(A0.size());
    for (const Column& c : A0) V0.push_back(column_verticals(c));
    const std::pair<int, int> BRij = Parent(A0, V0, { rightmost, topmost });

    std::vector<Sep> top_separators;
    for (std::size_t k = 0; k < static_cast<std::size_t>(BRij.second); ++k) {
        top_separators.push_back(A0[static_cast<std::size_t>(BRij.first)][k].sep);
    }
    top_separators.push_back(A0[static_cast<std::size_t>(rightmost)][static_cast<std::size_t>(topmost)].sep);
    std::vector<Vertical> top_verticals(V0[static_cast<std::size_t>(BRij.first)].begin(),
                                        V0[static_cast<std::size_t>(BRij.first)].begin() + BRij.second);
    top_verticals.push_back(V0[static_cast<std::size_t>(rightmost)][static_cast<std::size_t>(topmost)]);

    const int width = rightmost - BRij.first;

    // magma_checks_list：下标 i 从 BRij.first 起；BRij.first 行全 -1。
    std::vector<std::vector<int>> magma_checks_list(static_cast<std::size_t>(rightmost) + 1);
    for (int i = BRij.first + 1; i <= rightmost; ++i) {
        std::vector<int>& magmas = magma_checks_list[static_cast<std::size_t>(i)];
        magmas.assign(A0[static_cast<std::size_t>(i)].size(), -1);
        for (std::size_t j = 0; j < A0[static_cast<std::size_t>(i)].size(); ++j) {
            std::pair<int, int> working{ i, static_cast<int>(j) };
            while (working.first > BRij.first) {
                if (static_cast<int>(A0[static_cast<std::size_t>(working.first)].size()) <= working.second) {
                    --working.second;
                }
                if (working.second < 0) break;
                working = Parent(A0, V0, working);
            }
            if (working.first != BRij.first) {
                magmas[j] = -1;
                continue;
            }
            const Vertical& left = working.second - 1 >= 0
                                       ? V0[static_cast<std::size_t>(working.first)][static_cast<std::size_t>(working.second - 1)]
                                       : Vertical{};
            const Vertical& right = j == 0 ? Vertical{}
                                           : V0[static_cast<std::size_t>(i)][static_cast<std::size_t>(j - 1)];
            magmas[j] = (working.second <= BRij.second && vertical_compare(left, right) == 0)
                            ? working.second
                            : -1;
        }
    }
    magma_checks_list[static_cast<std::size_t>(BRij.first)].assign(
        A0[static_cast<std::size_t>(BRij.first)].size(), -1);

    Expr A = subtract1(A0, V0, kind);

    int refs_length = 0;
    std::vector<std::optional<int>> refs = get_references(A, top_verticals, refs_length);
    auto ref_value = [&](int idx) -> std::optional<int> {
        if (idx < 0) return -1;
        if (idx >= static_cast<int>(refs.size())) return std::nullopt;
        return refs[static_cast<std::size_t>(idx)];
    };

    std::vector<Sep> stretch_threshold(top_separators.size());
    std::vector<int> stretch_value(top_separators.size(), 0);
    for (std::size_t i = 0; i < top_separators.size(); ++i) {
        if (!mountain_is_limit(top_separators[i])) {
            stretch_value[i] = 0;
            continue;
        }
        const Sep S_i = S(A0, BRij.first, static_cast<int>(i) - 1);
        if (mountain_compare(S_i, top_separators[i]) >= 0) {
            stretch_threshold[i] = Sep{ Column{} };
        } else {
            stretch_threshold[i] = S_i;
            stretch_threshold[i].emplace_back();
        }
        stretch_value[i] = 0;
        if (i == 0 ? (ref_value(0).has_value())
                   : (ref_value(static_cast<int>(i)).has_value() &&
                      ref_value(static_cast<int>(i) - 1).has_value())) {
            const int j_start = *ref_value(static_cast<int>(i));
            const int cmp_ref = i == 0 ? -1 : *ref_value(static_cast<int>(i) - 1);
            for (int j = j_start; j - 1 > cmp_ref; --j) {
                Sep M;
                if (j - 1 >= 0 && j - 1 < static_cast<int>(A[static_cast<std::size_t>(rightmost)].size())) {
                    M = A[static_cast<std::size_t>(rightmost)][static_cast<std::size_t>(j - 1)].sep;
                }
                if (mountain_compare(M, top_separators[i]) < 0 &&
                    mountain_compare(M, stretch_threshold[i]) >= 0) {
                    stretch_value[i] = static_cast<int>(M.size()) -
                                       static_cast<int>(stretch_threshold[i].size()) + 1;
                    break;
                }
            }
        }
    }

    auto stretched = [&](int br, const Sep& entry_sep, int x, int y) -> Sep {
        if (br < 0) return entry_sep;
        const int sv = stretch_value[static_cast<std::size_t>(br)];
        if (sv == 0) return entry_sep;
        if (weak &&
            vertical_compare(V0[static_cast<std::size_t>(x)][static_cast<std::size_t>(y)],
                             top_verticals[static_cast<std::size_t>(br)]) >= 0) {
            return entry_sep;
        }
        if (mountain_compare(entry_sep, top_separators[static_cast<std::size_t>(br)]) >= 0) {
            return entry_sep;
        }
        if (mountain_compare(entry_sep, stretch_threshold[static_cast<std::size_t>(br)]) < 0) {
            return entry_sep;
        }
        Sep result = entry_sep;
        for (int k = 0; k < sv; ++k) result.emplace_back();
        return result;
    };

    for (int dx = 0; dx <= (is_small ? 0 : width); ++dx) {
        const int x = BRij.first + dx;
        const std::vector<int>& source_magmas = magma_checks_list[static_cast<std::size_t>(x)];
        if (dx) A.emplace_back();
        const int target_idx = x + width;
        int BR_index = dx ? -1 : refs_length - 1;
        const std::vector<Entry>& src_col = A0[static_cast<std::size_t>(x)];
        for (std::size_t y = 0; y < src_col.size(); ++y) {
            if (!dx && static_cast<int>(y) < BRij.second) continue;
            const Entry& entry = src_col[y];
            const int value = entry.value;
            if (source_magmas[y] != -1) {
                BR_index = source_magmas[y];
                if (BR_index >= 0 && ref_value(BR_index).has_value() &&
                    (BR_index == 0 || ref_value(BR_index - 1).has_value())) {
                    const int j_start = (BR_index == 0 ? -1 : *ref_value(BR_index - 1)) + 1;
                    const int j_end = *ref_value(BR_index);
                    for (int j = j_start; j <= j_end; ++j) {
                        if (j == j_end) {
                            A[static_cast<std::size_t>(target_idx)].push_back(
                                Entry{ value + width, stretched(BR_index, entry.sep, x, static_cast<int>(y)),
                                       entry.mark });
                        } else {
                            // 先拷贝再 push（dx=0 时目标列即 rightmost 列，push 可能触发重分配）
                            const Sep src_sep =
                                A[static_cast<std::size_t>(BRij.first + width)][static_cast<std::size_t>(j)].sep;
                            const bool src_mark =
                                A[static_cast<std::size_t>(BRij.first + width)][static_cast<std::size_t>(j)].mark;
                            A[static_cast<std::size_t>(target_idx)].push_back(
                                Entry{ value + width, src_sep, src_mark });
                        }
                    }
                }
            } else {
                A[static_cast<std::size_t>(target_idx)].push_back(
                    Entry{ value + (value > BRij.first ? width : 0),
                           stretched(BR_index, entry.sep, x, static_cast<int>(y)), entry.mark });
            }
        }
    }
    return A;
}

inline Expr expand(const Expr& A0, int index, bool shorter, bool weak, SubtractKind kind) {
    Expr A = A0;
    for (int n = 1; n <= index; ++n) A = extend(A, /*small=*/false, weak, kind);
    if (shorter) return Expr(A.begin(), A.end() - 1);
    return extend(A, /*small=*/true, weak, kind);
}

// TS infinity_FS(n) = [[], [[1, [Omega, ...(n 个空列)]]]]
inline Expr infinity_fs(int n) {
    Sep sep;
    sep.emplace_back();
    {
        Column c1;
        Sep inner;
        inner.emplace_back();
        c1.push_back(Entry{ 1, std::move(inner), false });
        sep.push_back(std::move(c1));
    }
    for (int k = 0; k < n; ++k) sep.emplace_back();
    Expr result;
    result.emplace_back();
    Column col;
    col.push_back(Entry{ 1, std::move(sep), false });
    result.push_back(std::move(col));
    return result;
}

inline constexpr const char* kParseFail = "解析失败，请确认你没有使用全角括号，逗号，分号";

} // namespace aw2mn_share
} // namespace omegay::notation

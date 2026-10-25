
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

namespace s_omega_pow_omega_mn_detail {

using Sep = std::vector<int>;
using Vertical = std::vector<Sep>;
using Entry = std::pair<int, Sep>;
using Column = std::vector<Entry>;
using Mountain = std::vector<Column>;

inline int num_compare(int a, int b) { return a == b ? 0 : (a < b ? -1 : 1); }
inline int bool_compare(bool a, bool b) { return (a ? 1 : 0) - (b ? 1 : 0); }

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

template <typename T, typename Cmp>
inline int anti_lex_compare(const std::vector<T>& a, const std::vector<T>& b, Cmp cmp) {
    const int len_cmp = num_compare(static_cast<int>(a.size()), static_cast<int>(b.size()));
    if (len_cmp != 0) return len_cmp;
    for (std::size_t i = a.size(); i-- > 0;) {
        const int r = cmp(a[i], b[i]);
        if (r != 0) return r;
    }
    return 0;
}

inline int sep_compare(const Sep& a, const Sep& b) {
    return anti_lex_compare(a, b, [](int x, int y) { return num_compare(x, y); });
}
inline int vertical_compare(const Vertical& a, const Vertical& b) {
    return lex_compare(a, b, [](const Sep& x, const Sep& y) { return sep_compare(x, y); });
}
inline int entry_compare(const Entry& a, const Entry& b) {
    const int r = num_compare(a.first, b.first);
    if (r != 0) return r;
    return sep_compare(a.second, b.second);
}
inline int column_compare(const Column& a, const Column& b) {
    return lex_compare(a, b, [](const Entry& x, const Entry& y) { return entry_compare(x, y); });
}
inline int mountain_compare(const Mountain& a, const Mountain& b) {
    return lex_compare(a, b, [](const Column& x, const Column& y) { return column_compare(x, y); });
}

// ---- 显示（TS sep_display：长度<=2 用 ;/,，否则 [倒序]）----

inline std::string sep_display(const Sep& s) {
    if (s.size() <= 2) {
        std::string out;
        out.append(s.size() > 1 ? static_cast<std::size_t>(s[1]) : 0, ';');
        out.append(s.empty() ? 0 : static_cast<std::size_t>(s[0]), ',');
        return out;
    }
    std::string out = "[";
    for (std::size_t k = s.size(); k-- > 0;) {
        if (k + 1 < s.size()) out += ",";
        out += std::to_string(s[k]);
    }
    out += "]";
    return out;
}
inline std::string entry_display(const Entry& e) {
    return sep_display(e.second) + std::to_string(e.first);
}
inline std::string column_display(const Column& c) {
    std::string out;
    for (const Entry& e : c) out += entry_display(e);
    return "(" + out + ")";
}
inline std::string mountain_display(const Mountain& m) {
    std::string out;
    for (const Column& c : m) out += column_display(c);
    return out;
}

// ---- 解析（TS from_display；支持 [..] 分隔符；非法输入返回 nullopt）----

inline std::optional<Mountain> from_display(std::string_view s) {
    if (s == "Limit") return std::nullopt;
    std::size_t i = 0;

    auto normalize_sep = [](Sep& sep) {
        while (!sep.empty() && sep.back() == 0) sep.pop_back();
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
    auto is_sep_start = [&](char ch) { return ch == ',' || ch == ';' || ch == '['; };
    auto parse_sep = [&]() -> std::optional<Sep> {
        if (i < s.size() && s[i] == '[') {
            ++i;
            Sep parts;
            skip_spaces();
            while (i < s.size() && s[i] != ']') {
                const std::size_t start = i;
                while (i < s.size() && s[i] >= '0' && s[i] <= '9') ++i;
                if (start == i) return std::nullopt;
                int v = 0;
                for (std::size_t k = start; k < i; ++k) v = v * 10 + (s[k] - '0');
                parts.push_back(v);
                skip_spaces();
                if (i < s.size() && s[i] == ',') {
                    ++i;
                    skip_spaces();
                }
            }
            if (i >= s.size() || s[i] != ']') return std::nullopt;
            ++i;
            // 显示顺序与内部顺序相反。
            Sep reversed(parts.rbegin(), parts.rend());
            return normalize_sep(reversed);
        }
        int c0 = 0, c1 = 0;
        while (i < s.size() && s[i] == ';') { ++c1; ++i; }
        while (i < s.size() && s[i] == ',') { ++c0; ++i; }
        if (c0 == 0 && c1 == 0) return std::nullopt;
        Sep sep{ c0, c1 };
        return normalize_sep(sep);
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
        ++i;
        Column col;
        skip_spaces();
        while (i < s.size() && s[i] != ')' && s[i] != ':') {
            skip_spaces();
            auto sep = parse_sep();
            if (!sep) return std::nullopt;
            skip_spaces();
            auto v = parse_number();
            if (!v) return std::nullopt;
            col.push_back(Entry{ *v, std::move(*sep) });
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
                            s[i + 1] == '(' || is_sep_start(s[i + 1]))) {
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
            auto sep = parse_sep();
            if (!sep) return std::nullopt;
            skip_spaces();
            auto v = parse_number();
            if (!v) return std::nullopt;
            col.push_back(Entry{ *v, std::move(*sep) });
        }
        skip_index();
        return col;
    };

    Mountain result;
    skip_spaces();
    while (i < s.size()) {
        auto col = s[i] == '(' ? parse_parenthesized_column() : parse_unparenthesized_column();
        if (!col) return std::nullopt;
        result.push_back(std::move(*col));
        skip_spaces();
    }
    return result;
}

// ---- 山结构辅助 ----

inline bool sep_is_one(const Sep& s) { return s.size() == 1 && s[0] == 1; }

inline int sep_dimension(const Sep& s) {
    std::size_t d = 0;
    while (d < s.size() && s[d] == 0) ++d;
    return static_cast<int>(d);
}

inline Sep sep_add(const Sep& a, const Sep& b) {
    if (b.empty()) return a;
    Sep result = a;
    while (result.size() < b.size()) result.push_back(0);
    result[b.size() - 1] += b.back();
    for (std::size_t d = 0; d + 1 < b.size(); ++d) result[d] = b[d];
    return result;
}

inline Sep sep_sub(const Sep& a, const Sep& b) {
    if (a.size() > b.size()) return a;
    if (a.size() < b.size()) return {};
    std::size_t d = a.size();
    while (d > 0 && a[d - 1] == b[d - 1]) --d;
    if (d == 0 || a[d - 1] < b[d - 1]) return {};
    Sep result(a.begin(), a.begin() + static_cast<std::ptrdiff_t>(d));
    result[d - 1] -= b[d - 1];
    return result;
}

inline Sep sep_increase(const Sep& a, int d) {
    Sep result = a;
    while (static_cast<int>(result.size()) <= d) result.push_back(0);
    ++result[static_cast<std::size_t>(d)];
    for (int k = 0; k < d; ++k) result[static_cast<std::size_t>(k)] = 0;
    return result;
}

inline Vertical vertical_increase(const Vertical& v, const Sep& s) {
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
        current = vertical_increase(current, e.second);
        result.push_back(current);
    }
    return result;
}

inline int find_index_below(const std::vector<Vertical>& Vi, const Vertical& v) {
    int l = 0, r = static_cast<int>(Vi.size());
    while (l < r) {
        const int j = (l + r + 1) / 2;
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

struct Position { int i; int j; };

inline Position parent(const Mountain& m, const std::vector<std::vector<Vertical>>& V, Position pos) {
    const int value = m[static_cast<std::size_t>(pos.i)][static_cast<std::size_t>(pos.j)].first;
    const int pi = value - 1;
    const int pj = pi == -1 ? 0 : find_index_below(V[static_cast<std::size_t>(pi)],
                                                   V[static_cast<std::size_t>(pos.i)][static_cast<std::size_t>(pos.j)]);
    return Position{ pi, pj };
}

inline std::vector<std::vector<int>> magma_indices(
    const Mountain& m, const std::vector<std::vector<Vertical>>& V, Position RiRj,
    std::vector<std::vector<int>> result = {}) {
    const int Ri = RiRj.i, Rj = RiRj.j;
    for (std::size_t i = result.size(); i < m.size(); ++i) {
        result.push_back({});
        if (static_cast<int>(i) <= Ri) continue;
        for (std::size_t j = 0; j < m[i].size(); ++j) {
            const Position p = parent(m, V, Position{ static_cast<int>(i), static_cast<int>(j) });
            int pi = p.i, pj = p.j;
            if (pi < Ri) break;
            if (pi == Ri) {
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

// ---- 展开核心（S_default / S / stretch / subtract_1 / copy_column / extend / expand）----

inline Sep S_default(const Sep& bound) {
    const int d = sep_dimension(bound);
    if (d == static_cast<int>(bound.size()) - 1 && bound[static_cast<std::size_t>(d)] == 1) return {};
    Sep result = bound;
    result[static_cast<std::size_t>(d)] -= 1;
    return result;
}

inline Sep S(const Column& c, int j, const Sep& bound) {
    if (j > static_cast<int>(c.size())) return S(c, j - 1, bound);
    if (j < 0) return S_default(bound);
    if (sep_compare(c[static_cast<std::size_t>(j)].second, bound) >= 0) return S_default(bound);
    const Sep& current = c[static_cast<std::size_t>(j)].second;
    const Sep previous = S(c, j - 1, bound);
    return sep_compare(current, previous) < 0 ? previous : current;
}

struct StretchData {
    Sep threshold;
    Sep stretch_to;
    bool force;
};

inline StretchData stretch_data_top(const Mountain& m, const std::vector<std::vector<Vertical>>& V) {
    const int right = static_cast<int>(m.size()) - 1;
    const int top = static_cast<int>(m[static_cast<std::size_t>(right)].size()) - 1;
    const Position p = parent(m, V, Position{ right, top });
    const int Ri = p.i, Rj = p.j;

    const Sep& top_right_sep = m[static_cast<std::size_t>(right)][static_cast<std::size_t>(top)].second;

    const Sep threshold = S(m[static_cast<std::size_t>(Ri)], Rj - 1, top_right_sep);
    Sep stretch_to = S(m[static_cast<std::size_t>(right)], top - 1, top_right_sep);
    bool force = false;
    if (sep_is_one(top_right_sep)) {
        // do nothing
    } else if (sep_dimension(top_right_sep) > 0) {
        const int dim = sep_dimension(top_right_sep);
        if (sep_compare(stretch_to, sep_increase(threshold, dim - 1)) < 0) {
            stretch_to = sep_increase(stretch_to, dim - 1);
            force = true;
        }
    } else {
        const Vertical& v_parent = Rj == 0 ? Vertical{} : V[static_cast<std::size_t>(Ri)][static_cast<std::size_t>(Rj) - 1];
        const Vertical& v_bottom = top == 0 ? Vertical{} : V[static_cast<std::size_t>(right)][static_cast<std::size_t>(top) - 1];
        if (vertical_compare(vertical_increase(v_parent, stretch_to), v_bottom) > 0) force = true;
    }
    return StretchData{ std::move(threshold), std::move(stretch_to), force };
}

inline std::vector<StretchData> stretch_data_list(const Mountain& m,
                                                  const std::vector<std::vector<Vertical>>& V,
                                                  const std::vector<std::vector<int>>& MI) {
    const int right = static_cast<int>(m.size()) - 1;
    const int top = static_cast<int>(m[static_cast<std::size_t>(right)].size()) - 1;
    const Position p = parent(m, V, Position{ right, top });
    const int Ri = p.i, Rj = p.j;

    std::vector<StretchData> result(static_cast<std::size_t>(Rj) + 1);
    int ref_j = -1;
    for (int j = 0; j < Rj; ++j) {
        while (ref_j + 1 <= top && MI[static_cast<std::size_t>(right)][static_cast<std::size_t>(ref_j) + 1] <= j) ++ref_j;
        const Sep& current_top_sep = m[static_cast<std::size_t>(Ri)][static_cast<std::size_t>(j)].second;
        const Sep threshold = S(m[static_cast<std::size_t>(Ri)], j - 1, current_top_sep);
        const Sep stretch_to = S(m[static_cast<std::size_t>(right)], ref_j - 1, current_top_sep);
        result[static_cast<std::size_t>(j)] = StretchData{ threshold, stretch_to, false };
    }
    result[static_cast<std::size_t>(Rj)] = stretch_data_top(m, V);
    return result;
}

inline Mountain subtract_1(const Mountain& m0, const std::vector<std::vector<Vertical>>* V = nullptr,
                           const StretchData* SD_top = nullptr) {
    std::vector<std::vector<Vertical>> V_own;
    if (!V) {
        V_own.reserve(m0.size());
        for (const Column& c : m0) V_own.push_back(column_verticals(c));
        V = &V_own;
    }
    std::optional<StretchData> SD_own;
    if (!SD_top) {
        SD_own = stretch_data_top(m0, *V);
        SD_top = &*SD_own;
    }

    const int right = static_cast<int>(m0.size()) - 1;
    const int top = static_cast<int>(m0[static_cast<std::size_t>(right)].size()) - 1;
    const Position p = parent(m0, *V, Position{ right, top });
    const int Ri = p.i, Rj = p.j;

    Mountain result = m0;
    result[static_cast<std::size_t>(right)].pop_back();

    if (SD_top->force) {
        result[static_cast<std::size_t>(right)].push_back(Entry{ Ri + 1, SD_top->stretch_to });
    }
    for (std::size_t j = static_cast<std::size_t>(Rj); j < m0[static_cast<std::size_t>(Ri)].size(); ++j) {
        result[static_cast<std::size_t>(right)].push_back(m0[static_cast<std::size_t>(Ri)][j]);
    }
    return result;
}

inline Sep compute_stretch(const Sep& sep, const StretchData* data) {
    if (!data) return sep;
    if (sep_compare(sep, data->threshold) <= 0) return sep;
    return sep_add(data->stretch_to, sep_sub(sep, data->threshold));
}

inline Column copy_column(const Column& m0i, const std::vector<int>& MI0i,
                          const std::vector<Vertical>& V0i, const Column& mr,
                          const std::vector<int>& MIr, Position RiRj,
                          const std::vector<StretchData>& SD, int offset,
                          const Vertical& stretch_v_max) {
    const int Ri = RiRj.i, Rj = RiRj.j;
    Column result;
    result.reserve(m0i.size());
    int last_mi = -1;
    std::size_t ref_j = 0;
    for (std::size_t j = 0; j < m0i.size(); ++j) {
        if (j >= MI0i.size()) {
            Entry entry = m0i[j];
            if (entry.first >= Ri + 1) entry.first += offset;
            result.push_back(entry);
        } else {
            const Entry& e = m0i[j];
            const int new_value = e.first + offset;
            const int current_mi = MI0i[j];
            if (current_mi != last_mi) {
                last_mi = current_mi;
                while (ref_j < MIr.size() && MIr[ref_j] == current_mi) {
                    const bool is_row_lifting =
                        current_mi == Rj ||
                        (ref_j + 1 < MIr.size() && MIr[ref_j + 1] == current_mi);
                    if (is_row_lifting) {
                        result.push_back(Entry{ new_value, mr[ref_j].second });
                    }
                    ++ref_j;
                }
            }
            const Sep& new_sep = vertical_compare(V0i[j], stretch_v_max) > 0
                                     ? e.second
                                     : compute_stretch(e.second, &SD[static_cast<std::size_t>(current_mi)]);
            result.push_back(Entry{ new_value, new_sep });
        }
    }
    return result;
}

inline Mountain extend(const Mountain& m0) {
    const int right = static_cast<int>(m0.size()) - 1;
    const int top = static_cast<int>(m0[static_cast<std::size_t>(right)].size()) - 1;

    std::vector<std::vector<Vertical>> V0;
    V0.reserve(m0.size());
    for (const Column& c : m0) V0.push_back(column_verticals(c));
    const Position p = parent(m0, V0, Position{ right, top });
    const std::vector<std::vector<int>> MI0 = magma_indices(m0, V0, p);
    const std::vector<StretchData> SD0 = stretch_data_list(m0, V0, MI0);

    Mountain m = subtract_1(m0, &V0, &SD0[static_cast<std::size_t>(p.j)]);
    std::vector<std::vector<Vertical>> V(V0.begin(), V0.begin() + right);
    V.push_back(column_verticals(m[static_cast<std::size_t>(right)]));
    std::vector<std::vector<int>> MI_partial(MI0.begin(), MI0.begin() + right);
    const std::vector<std::vector<int>> MI = magma_indices(m, V, p, MI_partial);

    const int offset = right - p.i;
    for (int i = p.i + 1; i < static_cast<int>(m0.size()); ++i) {
        m.push_back(copy_column(m0[static_cast<std::size_t>(i)], MI0[static_cast<std::size_t>(i)],
                                V0[static_cast<std::size_t>(i)], m[static_cast<std::size_t>(right)],
                                MI[static_cast<std::size_t>(right)], p, SD0, offset,
                                V0[static_cast<std::size_t>(right)][static_cast<std::size_t>(top)]));
    }
    return m;
}

// TS infinity_FS(index) = [[], [[1, [...(index 个 0), 1]]]]。
inline Mountain infinity_fs(int index) {
    Mountain m;
    m.push_back(Column{});
    Sep sep(static_cast<std::size_t>(index < 0 ? 0 : index), 0);
    sep.push_back(1);
    m.push_back(Column{ Entry{ 1, std::move(sep) } });
    return m;
}

inline Mountain expand(const Mountain& m, int index, bool shorter) {
    if (m.empty()) return m;
    if (m.back().empty()) return Mountain(m.begin(), m.end() - 1);
    Mountain current = m;
    for (int i = 0; i < index; ++i) current = extend(current);
    if (shorter) current = Mountain(current.begin(), current.end() - 1);
    else current = subtract_1(current);
    return current;
}

} // namespace s_omega_pow_omega_mn_detail

// Expr：非 ∞ 时持有 Mountain；∞（显示 "Limit"）用标志表示。
struct SOmegaPowOmegaMnExpr {
    bool infinity = false;
    s_omega_pow_omega_mn_detail::Mountain mountain;
};

struct SOmegaPowOmegaMNNotation {
    static constexpr const char* kName = "Sω^ωMN";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/false);
    }
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::optional<SOmegaPowOmegaMnExpr> parse(std::string_view text) {
        if (text == "Limit" || text == "limit" || text == "LIMIT") return SOmegaPowOmegaMnExpr{ true, {} };
        auto m = s_omega_pow_omega_mn_detail::from_display(text);
        if (!m) return std::nullopt;
        return SOmegaPowOmegaMnExpr{ false, std::move(*m) };
    }

    [[nodiscard]] static std::string to_string(const SOmegaPowOmegaMnExpr& e) {
        if (e.infinity) return "Limit";
        return s_omega_pow_omega_mn_detail::mountain_display(e.mountain);
    }

private:
    static std::string expand_text_impl(std::string_view text, int term, bool longer) {
        if (term < 1) term = 1;
        const auto expr = parse(text);
        if (!expr) {
            return "解析失败，请确认你没有使用全角括号，逗号，分号";
        }
        if (expr->infinity) {
            const auto m = s_omega_pow_omega_mn_detail::infinity_fs(term);
            return s_omega_pow_omega_mn_detail::mountain_display(m);
        }
        using namespace s_omega_pow_omega_mn_detail;
        const Mountain& m = expr->mountain;
        if (m.empty()) return {};
        // 值域校验：value(1-based) 作为列索引 pi=value-1 须在 [-1, size)
        for (const Column& col : m) {
            for (const Entry& e : col) {
                if (e.first < 0 || e.first > static_cast<int>(m.size())) {
                    return "解析失败，请确认你没有使用全角括号，逗号，分号";
                }
            }
        }
        if (m.back().empty()) {
            return mountain_display(Mountain(m.begin(), m.end() - 1));
        }
        return mountain_display(expand(m, term, !longer));
    }
};

} // namespace omegay::notation

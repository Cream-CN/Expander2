// Header/notation/mn_smn_share.hpp
// SMN 家族（Smile's MN 系列：SAω2MN / Sω2MN / Sω^ωMN）共享核心。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/SMN/ 下三个 TS 文件
// （S_omega2_MN.ts / SA_omega2_MN.ts / S_omega_pow_omega_MN.ts）。
// 结构：Sep=number[]（'[',';' 语法，二维 [逗号,分号] 或任意维）；Vertical=Sep[]；
// Entry=[value(1-based), Sep, mark?]；Column/Expr。值的语义 1-based，value 0 表示底部哨兵
// （parent pi=value-1，pi==-1 → pj=0）。
// 各记号仅在 S / stretch_data_top / stretch_data_list / subtract_1 / parse / display /
// infinity_FS 上不同；本文件提供其余全部共享机制，派生类经 CRTP 提供差异函数。
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace omegay::notation::mnsmn {

using Sep = std::vector<int>;
using Vertical = std::vector<Sep>;
struct Entry {
    int value;
    Sep sep;
    bool mark = false;
};
using Column = std::vector<Entry>;
using Expr = std::vector<Column>;
using VV = std::vector<Vertical>;
using MI = std::vector<std::vector<int>>;

struct StretchData {
    Sep threshold;
    Sep stretch_to;
    bool force = false;
};

inline int compare_int(int a, int b) { return a < b ? -1 : a > b ? 1 : 0; }

// anti_lex_compare：长度不等先比长度（短者小）；等长从尾比较
inline int sep_compare(const Sep& a, const Sep& b) {
    if (a.size() != b.size()) {
        return compare_int(static_cast<int>(a.size()), static_cast<int>(b.size()));
    }
    for (std::size_t k = a.size(); k-- > 0;) {
        const int c = compare_int(a[k], b[k]);
        if (c != 0) return c;
    }
    return 0;
}

// lex_compare：较长者大
inline int vertical_compare(const Vertical& a, const Vertical& b) {
    const std::size_t len = a.size() < b.size() ? a.size() : b.size();
    for (std::size_t k = 0; k < len; ++k) {
        const int c = sep_compare(a[k], b[k]);
        if (c != 0) return c;
    }
    return compare_int(static_cast<int>(a.size()), static_cast<int>(b.size()));
}

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
    result[static_cast<std::size_t>(d)]++;
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

inline Vertical column_verticals(const Column& c) {
    Vertical result;
    Vertical current;
    result.reserve(c.size());
    for (const Entry& e : c) {
        current = vertical_increase(current, e.sep);
        result.push_back(current);
    }
    return result;
}

inline VV verticals(const Expr& m) {
    VV out;
    out.reserve(m.size());
    for (const Column& c : m) out.push_back(column_verticals(c));
    return out;
}

inline int find_index_below(const Vertical& Vi, const Vertical& v) {
    std::size_t l = 0, r = Vi.size();
    while (l < r) {
        const std::size_t j = (l + r + 1) / 2;
        const Vertical Vij = j == 0 ? Vertical{} : Vi[j - 1];
        if (vertical_compare(Vij, v) < 0) l = j;
        else r = j - 1;
    }
    return static_cast<int>(l);
}

inline int find_index_below_equal(const Vertical& Vi, const Vertical& v) {
    std::size_t l = 0, r = Vi.size();
    while (l < r) {
        const std::size_t j = (l + r + 1) / 2;
        const Vertical Vij = j == 0 ? Vertical{} : Vi[j - 1];
        if (vertical_compare(Vij, v) <= 0) l = j;
        else r = j - 1;
    }
    return static_cast<int>(l);
}

// parent：pi = value-1；value 0 时 pi=-1 → pj=0
inline std::pair<int, int> parent(const Expr& m, const VV& V, int i, int j) {
    const int value =
        m[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)].value;
    const int pi = value - 1;
    const int pj =
        pi == -1
            ? 0
            : find_index_below(V[static_cast<std::size_t>(pi)],
                                V[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)]);
    return { pi, pj };
}

inline MI magma_indices(const Expr& m, const VV& V, int Ri, int Rj, const MI& MI_partial) {
    MI result = MI_partial;
    for (std::size_t ii = result.size(); ii < m.size(); ++ii) {
        result.push_back({});
        const int i = static_cast<int>(ii);
        if (i > Ri) {
            for (int j = 0; j < static_cast<int>(m[static_cast<std::size_t>(i)].size()); ++j) {
                const auto [pi, pj] = parent(m, V, i, j);
                if (pi < Ri) {
                    break;
                } else if (pi == Ri) {
                    result[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] =
                        pj < Rj ? pj : Rj;
                } else {
                    int pj2 = pj;
                    if (pj2 == static_cast<int>(m[static_cast<std::size_t>(pi)].size())) --pj2;
                    if (pj2 >= static_cast<int>(result[static_cast<std::size_t>(pi)].size())) break;
                    result[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] =
                        result[static_cast<std::size_t>(pi)][static_cast<std::size_t>(pj2)];
                }
            }
        }
    }
    return result;
}

inline Sep compute_stretch(const Sep& sep, const std::optional<StretchData>& data) {
    if (!data) return sep;
    if (sep_compare(sep, data->threshold) <= 0) return sep;
    return sep_add(data->stretch_to, sep_sub(sep, data->threshold));
}

inline Column copy_column(const Column& m0i, const std::vector<int>& MI0i,
                          const Vertical& V0i, const Column& mr,
                          const std::vector<int>& MIr, int Ri, int Rj,
                          const std::vector<std::optional<StretchData>>& SD, int offset,
                          const Vertical& stretch_v_max) {
    Column result;
    int last_mi = -1;
    std::size_t ref_j = 0;
    for (int j = 0; j < static_cast<int>(m0i.size()); ++j) {
        if (j >= static_cast<int>(MI0i.size())) {
            Entry entry = m0i[static_cast<std::size_t>(j)];
            if (entry.value >= Ri + 1) entry.value += offset;
            result.push_back(entry);
        } else {
            const Entry& src = m0i[static_cast<std::size_t>(j)];
            const int new_value = src.value + offset;
            const int current_mi = MI0i[static_cast<std::size_t>(j)];
            if (current_mi != last_mi) {
                last_mi = current_mi;
                while (ref_j < MIr.size() && MIr[ref_j] == current_mi) {
                    const bool is_row_lifting =
                        current_mi == Rj ||
                        (ref_j + 1 < MIr.size() && MIr[ref_j + 1] == current_mi);
                    if (is_row_lifting) {
                        const Entry& ref = mr[ref_j];
                        result.push_back(Entry{ new_value, ref.sep, ref.mark });
                    }
                    ++ref_j;
                }
            }
            const Sep new_sep =
                vertical_compare(V0i[static_cast<std::size_t>(j)], stretch_v_max) > 0
                    ? src.sep
                    : compute_stretch(src.sep, SD[static_cast<std::size_t>(current_mi)]);
            result.push_back(Entry{ new_value, new_sep, src.mark });
        }
    }
    return result;
}

// CRTP 基座：extend / expand 的公共流程，调用派生类的差异函数。
template <typename Derived>
struct SmnMountainCore {
    using Sep = mnsmn::Sep;
    using Vertical = mnsmn::Vertical;
    using Entry = mnsmn::Entry;
    using Column = mnsmn::Column;
    using Expr = mnsmn::Expr;
    using VV = mnsmn::VV;
    using MI = mnsmn::MI;
    using StretchData = mnsmn::StretchData;

    static Expr extend(const Expr& m0) {
        const int right = static_cast<int>(m0.size()) - 1;
        const int top = static_cast<int>(m0[static_cast<std::size_t>(right)].size()) - 1;

        const VV V0 = mnsmn::verticals(m0);
        const auto [Ri, Rj] = mnsmn::parent(m0, V0, right, top);
        const MI MI0 = mnsmn::magma_indices(m0, V0, Ri, Rj, MI{});
        const std::vector<std::optional<StretchData>> SD0 =
            Derived::stretch_data_list(m0, V0, MI0);

        Expr m = Derived::subtract_1(m0, V0, SD0[static_cast<std::size_t>(Rj)]);
        VV V = V0;
        V.resize(static_cast<std::size_t>(right));
        V.push_back(mnsmn::column_verticals(m[static_cast<std::size_t>(right)]));
        MI MI_partial(MI0.begin(), MI0.begin() + static_cast<std::ptrdiff_t>(right));
        const MI MI_full = mnsmn::magma_indices(m, V, Ri, Rj, MI_partial);

        const int offset = right - Ri;
        for (int i = Ri + 1; i <= right; ++i) {
            m.push_back(mnsmn::copy_column(
                m0[static_cast<std::size_t>(i)], MI0[static_cast<std::size_t>(i)],
                V0[static_cast<std::size_t>(i)], m[static_cast<std::size_t>(right)],
                MI_full[static_cast<std::size_t>(right)], Ri, Rj, SD0, offset,
                V0[static_cast<std::size_t>(right)][static_cast<std::size_t>(top)]));
        }
        return m;
    }

    static Expr expand_impl(const Expr& m, int index, bool shorter) {
        if (m.empty()) return m;
        if (m.back().empty()) return Expr(m.begin(), m.end() - 1);
        Expr current = m;
        for (int i = 0; i < index; ++i) current = extend(current);
        if (shorter) {
            current.pop_back();
            return current;
        }
        const VV V = mnsmn::verticals(current);
        const std::optional<StretchData> sd_top = Derived::stretch_data_top(current, V);
        return Derived::subtract_1(current, V, sd_top);
    }
};

} // namespace omegay::notation::mnsmn

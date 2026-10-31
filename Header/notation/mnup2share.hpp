// Header/notation/mnup2share.hpp
// UP2 家族（UP2MN-v1b / v1b-plus / UP2DBMS-v1 / v1b-plus / v1c）共享核心。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/UPMN/ 下各 TS 文件。
// 表达式结构（TS）：Entry=[value(0-based), sep(int)]；Column=Entry[]；
// Expr=Column[]；Vertical=number[]（sep 为层号）。
// 显示/解析复用 smnshare（n-MN）：value+1 转换后交给 mountain_display /
// from_display。∞ 用 bool 标志表示（display 为 "Limit"）。
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "smnshare.hpp"

namespace omegay::notation::mnup2 {

using Sep = int;
using Vertical = std::vector<Sep>;
struct Entry {
    int value;  // 0-based
    int sep;
};
using Column = std::vector<Entry>;
using Expr = std::vector<Column>;

inline int entry_compare(const Entry& a, const Entry& b) {
    if (a.value != b.value) return a.value < b.value ? -1 : 1;
    if (a.sep != b.sep) return a.sep < b.sep ? -1 : 1;
    return 0;
}

inline int column_compare(const Column& a, const Column& b) {
    const std::size_t n = a.size() < b.size() ? a.size() : b.size();
    for (std::size_t i = 0; i < n; ++i) {
        const int c = entry_compare(a[i], b[i]);
        if (c != 0) return c;
    }
    return a.size() < b.size() ? -1 : a.size() > b.size() ? 1 : 0;
}

inline int expr_compare(const Expr& a, const Expr& b) {
    const std::size_t n = a.size() < b.size() ? a.size() : b.size();
    for (std::size_t i = 0; i < n; ++i) {
        const int c = column_compare(a[i], b[i]);
        if (c != 0) return c;
    }
    return a.size() < b.size() ? -1 : a.size() > b.size() ? 1 : 0;
}

// TS vertical_increase：v.length <= sep 时返回 [0]*sep + [1]，否则 v[sep]++ 且 v[0..sep)=0。
inline Vertical vertical_increase(const Vertical& v, int sep) {
    if (static_cast<int>(v.size()) <= sep) {
        Vertical r(static_cast<std::size_t>(sep), 0);
        r.push_back(1);
        return r;
    }
    Vertical r = v;
    r[static_cast<std::size_t>(sep)]++;
    for (int k = 0; k < sep; ++k) r[static_cast<std::size_t>(k)] = 0;
    return r;
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

inline std::vector<std::vector<Vertical>> expr_verticals(const Expr& e) {
    std::vector<std::vector<Vertical>> result;
    result.reserve(e.size());
    for (const Column& c : e) result.push_back(column_verticals(c));
    return result;
}

// TS anti_lex_compare：从尾部开始比，前缀（从尾）相同则短的更小。
inline int vertical_compare(const Vertical& a, const Vertical& b) {
    std::size_t i = a.size(), j = b.size();
    while (i > 0 && j > 0) {
        --i;
        --j;
        if (a[i] != b[j]) return a[i] < b[j] ? -1 : 1;
    }
    return a.size() < b.size() ? -1 : a.size() > b.size() ? 1 : 0;
}

// TS find_index_below_row：working = [[], ...V]；v 大于末位时返回 V.length。
inline int find_index_below_row(const std::vector<Vertical>& V, const Vertical& v) {
    const int n = static_cast<int>(V.size());
    if (n == 0) return 0;
    if (vertical_compare(v, V[static_cast<std::size_t>(n - 1)]) > 0) return n;
    int l = 0, r = n;
    while (l < r) {
        const int mid = (l + r + 1) >> 1;
        const Vertical& w = mid == 0 ? Vertical{} : V[static_cast<std::size_t>(mid - 1)];
        if (vertical_compare(v, w) > 0) l = mid;
        else r = mid - 1;
    }
    return l;
}

struct Position {
    int i;
    int j;
};

// TS compute_parent：pi = entry[0]（0-based），pj = find_index_below_row(V[pi], v)。
inline Position compute_parent(const Expr& expr, const std::vector<std::vector<Vertical>>& V,
                               Position pos) {
    const int pi = expr[static_cast<std::size_t>(pos.i)][static_cast<std::size_t>(pos.j)].value;
    const int pj = find_index_below_row(V[static_cast<std::size_t>(pi)],
                                        V[static_cast<std::size_t>(pos.i)][static_cast<std::size_t>(pos.j)]);
    return Position{ pi, pj };
}

inline std::vector<std::vector<Position>> parents(const Expr& expr,
                                                  const std::vector<std::vector<Vertical>>& V) {
    std::vector<std::vector<Position>> result(expr.size());
    for (std::size_t i = 0; i < expr.size(); ++i) {
        result[i].reserve(expr[i].size());
        for (std::size_t j = 0; j < expr[i].size(); ++j) {
            result[i].push_back(compute_parent(expr, V, Position{ static_cast<int>(i), static_cast<int>(j) }));
        }
    }
    return result;
}

// 相对列比较（TS RelColumn / compare_rel_column）。
struct RelEntry {
    bool flag;  // v >= r
    int value;  // v - r（flag）或 v
    int sep;
};

inline int rel_entry_compare(const RelEntry& a, const RelEntry& b) {
    if (a.flag != b.flag) return a.flag ? 1 : -1;  // false < true
    if (a.value != b.value) return a.value < b.value ? -1 : 1;
    if (a.sep != b.sep) return a.sep < b.sep ? -1 : 1;
    return 0;
}

inline std::vector<RelEntry> to_rel_column(const Column& c, int r) {
    std::vector<RelEntry> result;
    result.reserve(c.size());
    for (const Entry& e : c) {
        result.push_back(e.value >= r ? RelEntry{ true, e.value - r, e.sep }
                                      : RelEntry{ false, e.value, e.sep });
    }
    return result;
}

inline int rel_column_compare(const std::vector<RelEntry>& a, const std::vector<RelEntry>& b) {
    const std::size_t n = a.size() < b.size() ? a.size() : b.size();
    for (std::size_t i = 0; i < n; ++i) {
        const int c = rel_entry_compare(a[i], b[i]);
        if (c != 0) return c;
    }
    return a.size() < b.size() ? -1 : a.size() > b.size() ? 1 : 0;
}

// 显示/解析：经 n-MN（smnshare）转换，value +1。
inline smnshare::Mountain to_nMN(const Expr& e) {
    smnshare::Mountain m;
    m.reserve(e.size());
    for (const Column& c : e) {
        smnshare::Column mc;
        mc.reserve(c.size());
        for (const Entry& en : c) mc.push_back(smnshare::Entry{ en.value + 1, en.sep });
        m.push_back(std::move(mc));
    }
    return m;
}

inline std::string display(const Expr& e) {
    return smnshare::mountain_display(to_nMN(e), false);
}

inline std::optional<Expr> from_display(std::string_view s) {
    if (s == "Limit" || s == "limit" || s == "LIMIT") return std::nullopt;  // ∞ 由包装器处理
    auto m = smnshare::from_display(s);
    if (!m) return std::nullopt;
    Expr e;
    e.reserve(m->size());
    for (const smnshare::Column& c : *m) {
        Column nc;
        nc.reserve(c.size());
        for (const smnshare::Entry& en : c) nc.push_back(Entry{ en.value - 1, en.sep });
        e.push_back(std::move(nc));
    }
    return e;
}

// 值域校验：所有 value 必须落在 [0, expr.size())（作为列索引使用）。
// TS 对越界值在 expand 时抛错（undefined.slice），这里等价地按解析失败处理。
inline bool expr_valid(const Expr& e) {
    const int n = static_cast<int>(e.size());
    for (const Column& c : e) {
        for (const Entry& en : c) {
            if (en.value < 0 || en.value >= n) return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------
// UP2MN-v1b / v1b-plus 共享实现（template Plus）。
// ---------------------------------------------------------------------

// compute_up_1mn（两版本相同）。
inline std::vector<int> up2mn_compute_up_1mn(
    const Expr& expr, const std::vector<std::vector<Position>>& P, Position rp) {
    const int Ri = rp.i, Rj = rp.j;
    const int right = static_cast<int>(expr.size()) - 1;
    std::vector<int> result(expr.size(), -1);
    for (int k = 0; k < Ri; ++k) result[static_cast<std::size_t>(k)] = 0;
    result[static_cast<std::size_t>(Ri)] = 1;
    if (Rj == 0) {
        for (int k = Ri; k < static_cast<int>(expr.size()); ++k) result[static_cast<std::size_t>(k)] = 1;
        return result;
    }
    for (int i = Ri + 1; i < static_cast<int>(expr.size()); ++i) {
        const Column& col = expr[static_cast<std::size_t>(i)];
        if (static_cast<int>(col.size()) <= Rj) {
            result[static_cast<std::size_t>(i)] = 0;
            continue;
        }
        if (P[static_cast<std::size_t>(i)][static_cast<std::size_t>(Rj)].i != Ri) {
            result[static_cast<std::size_t>(i)] =
                result[static_cast<std::size_t>(P[static_cast<std::size_t>(i)][static_cast<std::size_t>(Rj)].i)];
            continue;
        }
        int j = -1;
        for (std::size_t k = 0; k < col.size(); ++k) {
            if (col[k].value == Ri) {
                j = static_cast<int>(k);
                break;
            }
        }
        if (j < 0) {  // TS 会在 findIndex 返回 -1 后死循环；此处安全兜底。
            result[static_cast<std::size_t>(i)] = 0;
            continue;
        }
        if (j > 0) {
            int p = i;
            while (P[static_cast<std::size_t>(p)][static_cast<std::size_t>(j - 1)].i != Ri) {
                p = P[static_cast<std::size_t>(p)][static_cast<std::size_t>(j - 1)].i;
            }
            if (!result[static_cast<std::size_t>(p)]) {
                result[static_cast<std::size_t>(i)] = 0;
                continue;
            }
        }
        // UP check
        const int X_start = i;
        int Y_start = right;
        while (P[static_cast<std::size_t>(Y_start)][static_cast<std::size_t>(j)].i != Ri) {
            Y_start = P[static_cast<std::size_t>(Y_start)][static_cast<std::size_t>(j)].i;
        }
        if (Y_start <= X_start) {
            result[static_cast<std::size_t>(i)] = X_start == Y_start ? 1 : 0;
            continue;
        }
        const Column X0(expr[static_cast<std::size_t>(X_start)].begin() + j,
                        expr[static_cast<std::size_t>(X_start)].end());
        const Column Y0(expr[static_cast<std::size_t>(Y_start)].begin() + j,
                        expr[static_cast<std::size_t>(Y_start)].end());
        const int cmp_0 = column_compare(X0, Y0);
        if (cmp_0 != 0) {
            result[static_cast<std::size_t>(i)] = cmp_0 > 0 ? 1 : 0;
            continue;
        }
        bool decided = false;
        for (int k = 1; Y_start + k < static_cast<int>(expr.size()); ++k) {
            const auto Xk = to_rel_column(expr[static_cast<std::size_t>(X_start + k)], X_start);
            const auto Yk = to_rel_column(expr[static_cast<std::size_t>(Y_start + k)], Y_start);
            const int cmp = rel_column_compare(Xk, Yk);
            if (cmp != 0) {
                result[static_cast<std::size_t>(i)] = cmp > 0 ? 1 : 0;
                decided = true;
                break;
            }
        }
        if (!decided) result[static_cast<std::size_t>(i)] = 1;
    }
    return result;
}

// compute_up_2mn：Plus=false（v1b）时 p!=Ri 分支为 false，Plus=true（v1b-plus）时沿用 result[p]。
inline std::vector<int> up2mn_compute_up_2mn(
    const Expr& expr, const std::vector<std::vector<Position>>& P, Position rp, bool plus) {
    const int Ri = rp.i, Rj = rp.j;
    const int right = static_cast<int>(expr.size()) - 1;
    std::vector<int> result(expr.size(), -1);
    for (int k = 0; k < Ri; ++k) result[static_cast<std::size_t>(k)] = 0;
    result[static_cast<std::size_t>(Ri)] = 1;
    for (int i = Ri + 1; i < static_cast<int>(expr.size()); ++i) {
        const Column& col = expr[static_cast<std::size_t>(i)];
        if (static_cast<int>(col.size()) <= Rj) {
            result[static_cast<std::size_t>(i)] = 0;
            continue;
        }
        if (static_cast<int>(col.size()) >= Rj + 2) {
            result[static_cast<std::size_t>(i)] =
                result[static_cast<std::size_t>(P[static_cast<std::size_t>(i)][static_cast<std::size_t>(Rj)].i)];
            continue;
        }
        const bool is_finite = col.back().sep == 0;
        if (is_finite) {
            result[static_cast<std::size_t>(i)] = 0;
            continue;
        }
        const int p = P[static_cast<std::size_t>(i)][static_cast<std::size_t>(Rj)].i;
        if (p != Ri) {
            result[static_cast<std::size_t>(i)] = plus ? result[static_cast<std::size_t>(p)] : 0;
            continue;
        }
        // UP check
        const int X_start = i;
        int Y_start = right;
        while (static_cast<int>(expr[static_cast<std::size_t>(Y_start)].size()) != Rj + 1) {
            Y_start = P[static_cast<std::size_t>(Y_start)][static_cast<std::size_t>(Rj)].i;
        }
        if (Y_start <= X_start) {
            result[static_cast<std::size_t>(i)] = X_start == Y_start ? 1 : 0;
            continue;
        }
        bool decided = false;
        for (int k = 1; Y_start + k < static_cast<int>(expr.size()); ++k) {
            const auto Xk = to_rel_column(expr[static_cast<std::size_t>(X_start + k)], X_start);
            const auto Yk = to_rel_column(expr[static_cast<std::size_t>(Y_start + k)], Y_start);
            const int cmp = rel_column_compare(Xk, Yk);
            if (cmp != 0) {
                result[static_cast<std::size_t>(i)] = cmp > 0 ? 1 : 0;
                decided = true;
                break;
            }
        }
        if (!decided) result[static_cast<std::size_t>(i)] = 1;
    }
    return result;
}

inline Expr up2mn_infinity_fs(int index, bool plus) {
    Expr result;
    result.emplace_back();
    for (int i = 1; i <= index; ++i) {
        Column col;
        if (plus) {
            col.push_back(Entry{ i - 1, 1 });
        } else {
            for (int j = 0; j < i - 1; ++j) col.push_back(Entry{ i - 1, 0 });
            col.push_back(Entry{ 0, 1 });
        }
        result.push_back(std::move(col));
    }
    return result;
}

inline Column up2mn_copy_column(const Column& col, Position rp, int offset, int y_offset, bool up) {
    const int Ri = rp.i, Rj = rp.j;
    Column result;
    result.reserve(col.size());
    for (const Entry& e : col) {
        const int nv = e.value > Ri ? e.value + offset : e.value < Ri ? e.value : up ? e.value + offset : e.value;
        result.push_back(Entry{ nv, e.sep });
    }
    if (up && y_offset > 0 && static_cast<std::size_t>(Rj) < result.size()) {
        const int fill_value = result[static_cast<std::size_t>(Rj)].value;
        Column head(result.begin(), result.begin() + Rj);
        Column tail(result.begin() + Rj, result.end());
        for (int k = 0; k < y_offset; ++k) head.push_back(Entry{ fill_value, 0 });
        head.insert(head.end(), tail.begin(), tail.end());
        result = std::move(head);
    }
    return result;
}

inline Expr up2mn_expand(const Expr& expr, int index, bool shorter, bool plus) {
    if (expr.empty()) return expr;
    const int right = static_cast<int>(expr.size()) - 1;
    if (expr[static_cast<std::size_t>(right)].empty()) {
        return Expr(expr.begin(), expr.end() - 1);
    }
    const int top = static_cast<int>(expr[static_cast<std::size_t>(right)].size()) - 1;

    const auto V = expr_verticals(expr);
    const auto P = parents(expr, V);

    const Position rp = P[static_cast<std::size_t>(right)][static_cast<std::size_t>(top)];
    const int Ri = rp.i, Rj = rp.j;

    const bool is_finite = expr[static_cast<std::size_t>(right)][static_cast<std::size_t>(top)].sep == 0;

    const std::vector<int> up_list =
        is_finite ? up2mn_compute_up_1mn(expr, P, rp) : up2mn_compute_up_2mn(expr, P, rp, plus);

    Expr result(expr.begin(), expr.end() - 1);
    {
        const Column& rc = expr[static_cast<std::size_t>(right)];
        Column new_col(rc.begin(), rc.end() - 1);
        if (!is_finite && top == Rj) new_col.push_back(Entry{ Ri, 0 });
        const Column& ri_col = expr[static_cast<std::size_t>(Ri)];
        new_col.insert(new_col.end(), ri_col.begin() + Rj, ri_col.end());
        result.push_back(std::move(new_col));
    }

    const int y_offset = is_finite ? 0 : (top - Rj > 1 ? top - Rj : 1);
    for (int w = 1; w <= index; ++w) {
        for (int i = Ri + 1; i <= right; ++i) {
            result.push_back(up2mn_copy_column(result[static_cast<std::size_t>(i)], rp,
                                               (right - Ri) * w, y_offset * w,
                                               up_list[static_cast<std::size_t>(i)] != 0));
        }
    }
    if (shorter) result.pop_back();
    return result;
}

// ---------------------------------------------------------------------
// UP2DBMS-v1 / v1b-plus / v1c 共享实现。
// v1 与 v1b-plus/v1c 的差异：
//   - compute_up_1mn：v1 恒 true（从 Ri 起）；v1b-plus/v1c 用真实算法。
//   - compute_up_2mn：v1b-plus 与 v1 的 while 循环用 Rj+1 索引；
//     v1c 的 while 循环用 Rj 索引且追加 P[Y][Rj][0] !== Ri 条件。
//   - copy_column：v1 的 value==Ri 分支含 j<=Rj 判据；v1b-plus/v1c 无。
// ---------------------------------------------------------------------

inline Expr up2dbms_infinity_fs(int index) {
    Expr result;
    result.emplace_back();
    for (int i = 1; i <= index; ++i) {
        Column col;
        for (int j = 0; j < i; ++j) col.push_back(Entry{ i - 1, 0 });
        col.push_back(Entry{ i - 1, 1 });
        result.push_back(std::move(col));
    }
    return result;
}

// v1 的 compute_up_1mn：直接 fill(true, Ri)。
inline std::vector<int> up2dbms_v1_compute_up_1mn(const Expr& expr, Position rp) {
    const int Ri = rp.i;
    std::vector<int> result(expr.size());
    for (int k = 0; k < Ri; ++k) result[static_cast<std::size_t>(k)] = 0;
    for (int k = Ri; k < static_cast<int>(expr.size()); ++k) result[static_cast<std::size_t>(k)] = 1;
    return result;
}

// v1b-plus / v1c 的 compute_up_2mn：与 v1 同界（Rj+1/Rj+3），
// while 循环按 v1c_flag 区分：false（v1/v1b-plus）用 Rj+1 索引，
// true（v1c）用 Rj 索引且追加 P[Y][Rj][0] !== Ri 条件。
inline std::vector<int> up2dbms_compute_up_2mn(
    const Expr& expr, const std::vector<std::vector<Position>>& P, Position rp, bool v1c) {
    const int Ri = rp.i, Rj = rp.j;
    const int right = static_cast<int>(expr.size()) - 1;
    std::vector<int> result(expr.size(), -1);
    for (int k = 0; k < Ri; ++k) result[static_cast<std::size_t>(k)] = 0;
    result[static_cast<std::size_t>(Ri)] = 1;
    for (int i = Ri + 1; i < static_cast<int>(expr.size()); ++i) {
        const Column& col = expr[static_cast<std::size_t>(i)];
        if (static_cast<int>(col.size()) <= Rj + 1) {
            result[static_cast<std::size_t>(i)] = 0;
            continue;
        }
        if (static_cast<int>(col.size()) >= Rj + 3) {
            result[static_cast<std::size_t>(i)] =
                result[static_cast<std::size_t>(P[static_cast<std::size_t>(i)][static_cast<std::size_t>(Rj + 1)].i)];
            continue;
        }
        const bool is_finite = col.back().sep == 0;
        if (is_finite) {
            result[static_cast<std::size_t>(i)] =
                result[static_cast<std::size_t>(P[static_cast<std::size_t>(i)][static_cast<std::size_t>(Rj + 1)].i)];
            continue;
        }
        const int p = P[static_cast<std::size_t>(i)][static_cast<std::size_t>(Rj + 1)].i;
        if (p != Ri) {
            result[static_cast<std::size_t>(i)] = result[static_cast<std::size_t>(p)];
            continue;
        }
        // UP check
        const int X_start = i;
        int Y_start = right;
        if (v1c) {
            while (static_cast<int>(expr[static_cast<std::size_t>(Y_start)].size()) != Rj + 2 ||
                   P[static_cast<std::size_t>(Y_start)][static_cast<std::size_t>(Rj)].i != Ri) {
                Y_start = P[static_cast<std::size_t>(Y_start)][static_cast<std::size_t>(Rj)].i;
            }
        } else {
            while (static_cast<int>(expr[static_cast<std::size_t>(Y_start)].size()) != Rj + 2) {
                Y_start = P[static_cast<std::size_t>(Y_start)][static_cast<std::size_t>(Rj + 1)].i;
            }
        }
        if (Y_start <= X_start) {
            result[static_cast<std::size_t>(i)] = X_start == Y_start ? 1 : 0;
            continue;
        }
        bool decided = false;
        for (int k = 1; Y_start + k < static_cast<int>(expr.size()); ++k) {
            const auto Xk = to_rel_column(expr[static_cast<std::size_t>(X_start + k)], X_start);
            const auto Yk = to_rel_column(expr[static_cast<std::size_t>(Y_start + k)], Y_start);
            const int cmp = rel_column_compare(Xk, Yk);
            if (cmp != 0) {
                result[static_cast<std::size_t>(i)] = cmp > 0 ? 1 : 0;
                decided = true;
                break;
            }
        }
        if (!decided) result[static_cast<std::size_t>(i)] = 1;
    }
    return result;
}

// v1 的 copy_column：value==Ri 时 up || j<=Rj 才 +offset（v1_flag=true）。
// v1b-plus/v1c：value==Ri 时仅 up 才 +offset。
inline Column up2dbms_copy_column(const Column& col, Position rp, int offset, int y_offset, bool up, bool v1_flag) {
    const int Ri = rp.i, Rj = rp.j;
    Column result;
    result.reserve(col.size());
    for (std::size_t j = 0; j < col.size(); ++j) {
        const Entry& e = col[j];
        int nv;
        if (e.value > Ri) {
            nv = e.value + offset;
        } else if (e.value < Ri) {
            nv = e.value;
        } else {
            nv = (up || (v1_flag && static_cast<int>(j) <= Rj)) ? e.value + offset : e.value;
        }
        result.push_back(Entry{ nv, e.sep });
    }
    if (up && y_offset > 0 && static_cast<std::size_t>(Rj) < result.size()) {
        const int fill_value = result[static_cast<std::size_t>(Rj)].value;
        Column head(result.begin(), result.begin() + Rj);
        Column tail(result.begin() + Rj, result.end());
        for (int k = 0; k < y_offset; ++k) head.push_back(Entry{ fill_value, 0 });
        head.insert(head.end(), tail.begin(), tail.end());
        result = std::move(head);
    }
    return result;
}

inline Expr up2dbms_expand(const Expr& expr, int index, bool shorter, bool v1_flag, bool v1c) {
    if (expr.empty()) return expr;
    const int right = static_cast<int>(expr.size()) - 1;
    if (expr[static_cast<std::size_t>(right)].empty()) {
        return Expr(expr.begin(), expr.end() - 1);
    }
    const int top = static_cast<int>(expr[static_cast<std::size_t>(right)].size()) - 1;

    const auto V = expr_verticals(expr);
    const auto P = parents(expr, V);

    const Position rp = P[static_cast<std::size_t>(right)][static_cast<std::size_t>(top)];
    const int Ri = rp.i, Rj = rp.j;

    const bool is_finite = expr[static_cast<std::size_t>(right)][static_cast<std::size_t>(top)].sep == 0;

    std::vector<int> up_list;
    if (is_finite) {
        up_list = v1_flag ? up2dbms_v1_compute_up_1mn(expr, rp) : up2mn_compute_up_1mn(expr, P, rp);
    } else {
        up_list = up2dbms_compute_up_2mn(expr, P, rp, v1c);
    }

    Expr result(expr.begin(), expr.end() - 1);
    {
        const Column& rc = expr[static_cast<std::size_t>(right)];
        Column new_col(rc.begin(), rc.end() - 1);
        const Column& ri_col = expr[static_cast<std::size_t>(Ri)];
        new_col.insert(new_col.end(), ri_col.begin() + Rj, ri_col.end());
        result.push_back(std::move(new_col));
    }

    const int y_offset = top - Rj;
    for (int w = 1; w <= index; ++w) {
        for (int i = Ri + 1; i <= right; ++i) {
            result.push_back(up2dbms_copy_column(result[static_cast<std::size_t>(i)], rp,
                                                 (right - Ri) * w, y_offset * w,
                                                 up_list[static_cast<std::size_t>(i)] != 0, v1_flag));
        }
    }
    if (shorter) result.pop_back();
    return result;
}

}  // namespace omegay::notation::mnup2

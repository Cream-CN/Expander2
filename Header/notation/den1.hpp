// Header/notation/den1.hpp
// DEN1 记号（display 名为 "DEN1"）。
//
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/DEN/DEN.ts。
// DEN1 的表达式是“扁平行”序列：
//   - rows[0] 是父标号列表（display 中位于 ';' 之后）；
//   - 其余每行 row[0] 是 step，row[1..] 是条目值（无标记）。
// 记号极限以 expr.infinity 表示，display 为 "Limit"。
//
// 对外提供文本接口（expand_text / expand_text_alter，分别对应 FS / FS_alter），
// 与 UI 的 NotationEntry::expand_text 槽位对接；纯函数、不抛异常。
#pragma once

#include "denshare.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

struct Den1Notation {
    static constexpr const char* kName = "DEN1";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view text, int term);
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term);
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<denshare::Den1Expr> parse(std::string_view text);
    [[nodiscard]] static std::string to_string(const denshare::Den1Expr& expr);
};

namespace den1_detail {

using denshare::Den1Expr;
using denshare::Den1Row;
using denshare::TextParser;

// ---------------------------------------------------------------------
// 显示与解析
// ---------------------------------------------------------------------

// TS: toShort —— 主行 slice(1, -step) 再补最后一个值。
[[nodiscard]] inline std::vector<int> to_short_row(const Den1Row& row) {
    std::vector<int> out;
    denshare::append_slice_except_tail(out, row, 1, row.empty() ? 0 : row[0]);
    if (!row.empty()) out.push_back(row.back());
    return out;
}

[[nodiscard]] inline std::string to_string(const Den1Expr& expr) {
    if (expr.infinity) return "Limit";
    std::string out;
    for (std::size_t i = 1; i < expr.rows.size(); ++i) {
        const Den1Row& row = expr.rows[i];
        out += '(';
        for (std::size_t k = 1; k < row.size(); ++k) {
            if (k > 1) out += ',';
            out += std::to_string(row[k]);
        }
        out += ')';
        out += std::to_string(row[0]);
    }
    out += ';';
    if (!expr.rows.empty()) {
        for (std::size_t k = 0; k < expr.rows[0].size(); ++k) {
            if (k > 0) out += ',';
            out += std::to_string(expr.rows[0][k]);
        }
    }
    return out;
}

// 解析一行：'(' 值列表 ')' step。
[[nodiscard]] inline std::optional<Den1Row> parse_row(TextParser& p) {
    p.advance(1);   // '('（调用方已确认）
    p.skip_spaces();
    Den1Row row;
    if (!p.eof() && p.peek() != ')') {
        while (true) {
            auto v = p.parse_digits();
            if (!v) return std::nullopt;
            row.push_back(*v);
            p.skip_spaces();
            if (!p.eof() && p.peek() == ',') {
                p.advance(1);
                p.skip_spaces();
                continue;
            }
            break;
        }
    }
    if (p.eof() || p.peek() != ')') return std::nullopt;
    p.advance(1);
    p.skip_spaces();
    auto step = p.parse_digits();
    if (!step) return std::nullopt;
    row.insert(row.begin(), *step);
    return row;
}

[[nodiscard]] inline std::optional<Den1Expr> parse(std::string_view s) {
    TextParser p(s);
    p.skip_spaces();
    if (p.match("Limit")) {
        p.skip_spaces();
        if (!p.eof()) return std::nullopt;
        Den1Expr e;
        e.infinity = true;
        return e;
    }

    Den1Expr expr;
    expr.rows.emplace_back();   // 父标号列表，初始为空

    // ';' 之前是主体行
    while (!p.eof()) {
        p.skip_spaces();
        if (p.eof() || p.peek() == ';') break;
        if (p.peek() != '(') return std::nullopt;
        auto row = parse_row(p);
        if (!row) return std::nullopt;
        expr.rows.push_back(std::move(*row));
    }

    p.skip_spaces();
    if (p.eof() || p.peek() != ';') return std::nullopt;
    p.advance(1);
    p.skip_spaces();

    // ';' 之后是父标号列表（可为空；不允许尾逗号）
    if (!p.eof()) {
        while (true) {
            auto v = p.parse_digits();
            if (!v) return std::nullopt;
            expr.rows[0].push_back(*v);
            p.skip_spaces();
            if (!p.eof() && p.peek() == ',') {
                p.advance(1);
                p.skip_spaces();
                continue;
            }
            break;
        }
    }
    if (!p.eof()) return std::nullopt;
    return expr;
}

// ---------------------------------------------------------------------
// 比较
// ---------------------------------------------------------------------

[[nodiscard]] inline int compare(const Den1Expr& a, const Den1Expr& b) {
    if (a.infinity || b.infinity) return denshare::bool_compare(a.infinity, b.infinity);
    std::vector<std::vector<int>> sa, sb;
    for (std::size_t i = 1; i < a.rows.size(); ++i) sa.push_back(to_short_row(a.rows[i]));
    for (std::size_t i = 1; i < b.rows.size(); ++i) sb.push_back(to_short_row(b.rows[i]));
    return denshare::matrix_compare(sa, sb);
}

// ---------------------------------------------------------------------
// 极限判定
// ---------------------------------------------------------------------

[[nodiscard]] inline bool is_limit(const Den1Expr& expr) {
    if (expr.infinity) return true;
    if (expr.rows.empty()) return false;
    const Den1Row& active = expr.rows.back();
    const std::size_t idx = 1 + static_cast<std::size_t>(active[0]);
    if (idx >= active.size() || active[idx] == 0) return false;

    // pleasantUntil(expr.slice(active[1+active[0]], -1), active) === -1
    std::vector<std::vector<int>> rows;
    for (std::size_t i = static_cast<std::size_t>(active[idx]);
         i + 1 < expr.rows.size(); ++i) {
        rows.emplace_back(expr.rows[i].begin() + 1, expr.rows[i].end());
    }
    std::vector<int> tcheck(active.begin() + static_cast<std::ptrdiff_t>(idx), active.end());
    return denshare::pleasant_until(rows, tcheck) == -1;
}

// ---------------------------------------------------------------------
// 展开核心
// ---------------------------------------------------------------------

// 删除最后一行，并弹出父标号列表的最后一个元素。
[[nodiscard]] inline Den1Expr cut(const Den1Expr& raw) {
    if (raw.rows.empty()) return Den1Expr{};
    Den1Expr expr;
    expr.rows.assign(raw.rows.begin(), raw.rows.end() - 1);
    if (!expr.rows.empty() && !expr.rows[0].empty()) expr.rows[0].pop_back();
    return expr;
}

// isAncestor(R, i, j)：R 是父标号列表，j 是 1 起的主行下标。
[[nodiscard]] inline bool is_ancestor(const std::vector<int>& R, int i, int j) {
    while (true) {
        if (i == j) return true;
        if (!(i < j) || j <= 0 || static_cast<std::size_t>(j) > R.size()) return false;
        j = R[j - 1];
    }
}

// compute_parent_for_mapped_row：为新复制行计算父标号。
[[nodiscard]] inline int compute_parent_for_mapped_row(
    const std::vector<int>& r_old,
    int row_idx, int start, int end, int old_height, int tmin) {
    int parent = 0;
    if (row_idx >= 1 && row_idx <= static_cast<int>(r_old.size())) {
        parent = r_old[row_idx - 1];
    }
    if (parent && start <= parent && parent <= end) return parent - start + old_height;
    int ancestor = parent;
    while (ancestor) {
        if (ancestor < tmin) return ancestor;
        if (ancestor >= 1 && ancestor <= static_cast<int>(r_old.size())) {
            ancestor = r_old[ancestor - 1];
        } else {
            return 0;
        }
    }
    return 0;
}

// ap：把一行按 active 行的映射规则复制。
[[nodiscard]] inline Den1Row apply_row(const Den1Row& s, const Den1Row& t) {
    std::vector<int> tvals(t.begin() + 1, t.end());
    Den1Row out;
    out.reserve(s.size());
    out.push_back(s.empty() ? 0 : s[0]);   // step 不变
    for (std::size_t k = 1; k < s.size(); ++k) {
        out.push_back(denshare::ap_value(s[k], tvals, t[0]));
    }
    return out;
}

// copy：复制 [begin, end) 区间内的行（flag >= 0 时 end = begin+flag，
// 否则 end = raw.length+1），并补算父标号。
// begin 是 active 行第 1+step 个值，在 DEN1 中直接用作 0 起的行下标。
[[nodiscard]] inline Den1Expr copy(const Den1Expr& raw, int flag) {
    const Den1Row& active = raw.rows.back();
    Den1Expr expr = cut(raw);
    const int begin = active[1 + active[0]];
    const int end = flag >= 0 ? begin + flag : static_cast<int>(raw.rows.size()) + 1;
    const int old_height = static_cast<int>(raw.rows.size()) - 1;

    for (int i = begin; i < end; ++i) {
        expr.rows.push_back(apply_row(raw.rows[i], active));
    }
    for (int row_idx = begin; row_idx < end; ++row_idx) {
        expr.rows[0].push_back(compute_parent_for_mapped_row(
            raw.rows[0], row_idx, begin, end - 1, old_height, active.back()));
    }
    return expr;
}

// extend：复制 [begin, raw.length) 区间内的行（含 active 行自身）。
[[nodiscard]] inline Den1Expr extend(const Den1Expr& raw) {
    const Den1Row& active = raw.rows.back();
    Den1Expr expr = cut(raw);
    const int begin = active[1 + active[0]];
    const int old_height = static_cast<int>(raw.rows.size()) - 1;

    for (int i = begin; i < static_cast<int>(raw.rows.size()); ++i) {
        expr.rows.push_back(apply_row(raw.rows[i], active));
    }
    for (int row_idx = begin; row_idx < static_cast<int>(raw.rows.size()); ++row_idx) {
        expr.rows[0].push_back(compute_parent_for_mapped_row(
            raw.rows[0], row_idx, begin, old_height, old_height, active.back()));
    }
    return expr;
}

// comp：对第 i 行做“压缩”（TS 的 comp）。
[[nodiscard]] inline Den1Expr comp(const Den1Expr& raw, int i, const std::vector<int>& T) {
    Den1Expr expr;
    expr.rows.assign(raw.rows.begin(), raw.rows.begin() + i);   // 前 i 行深拷贝
    const int u = static_cast<int>(T.size());
    const Den1Row& ri = raw.rows[i];
    const std::size_t ri_len = ri.size();
    const bool short_row = ri_len < static_cast<std::size_t>(ri[0]) * 2 + 1;
    const int li = short_row ? ri[0] : ri[0] + 1;

    // ci：短行 slice(1, -step) concat slice(1+step)；长行 slice(1)
    std::vector<int> ci;
    if (short_row) {
        denshare::append_slice_except_tail(ci, ri, 1, ri.empty() ? 0 : ri[0]);
        denshare::append_slice_from(ci, ri, 1 + (ri.empty() ? 0 : ri[0]));
    } else {
        denshare::append_slice_from(ci, ri, 1);
    }

    for (int r = 0; r < u; ++r) {
        std::vector<int> values = ci;
        values.insert(values.end(), T.begin(), T.begin() + (1 + r));
        for (int rr = 0; rr < r; ++rr) values.push_back(ri[1] + 1 + rr);
        std::stable_sort(values.begin(), values.end(), std::greater<int>());
        Den1Row nrow;
        nrow.reserve(values.size() + 1);
        nrow.push_back(li + r);
        nrow.insert(nrow.end(), values.begin(), values.end());
        if (i + r < expr.rows.size()) expr.rows[i + r] = std::move(nrow);
        else expr.rows.push_back(std::move(nrow));
    }

    for (int ii = i; ii < static_cast<int>(raw.rows.size()); ++ii) {
        std::vector<int> values;
        for (std::size_t k = 1; k < raw.rows[ii].size(); ++k) {
            const int x = raw.rows[ii][k];
            values.push_back(x <= i ? x : x + u);
        }
        // TS: values.findIndex(x => x <= i) <= raw[ii][0]；
        // 找不到时 findIndex 返回 -1，-1 <= step 恒真。
        std::ptrdiff_t find_idx = -1;
        for (std::size_t k = 0; k < values.size(); ++k) {
            if (values[k] <= i) { find_idx = static_cast<std::ptrdiff_t>(k); break; }
        }
        const bool flag = is_ancestor(raw.rows[0], i, ii) &&
            (find_idx == -1 || find_idx <= raw.rows[ii][0]);
        if (flag) {
            values.insert(values.end(), T.begin(), T.end());
            for (int uu = 0; uu < u; ++uu) values.push_back(i + 1 + uu);
            std::stable_sort(values.begin(), values.end(), std::greater<int>());
        }
        Den1Row nrow;
        nrow.reserve(values.size() + 1);
        nrow.push_back(raw.rows[ii][0] + (flag ? u : 0));
        nrow.insert(nrow.end(), values.begin(), values.end());
        if (ii + u < expr.rows.size()) expr.rows[ii + u] = std::move(nrow);
        else expr.rows.push_back(std::move(nrow));
    }

    // 父标号列表
    auto m = [i, u](int x) { return x < i ? x : x + u; };
    Den1Row parents;
    parents.reserve(static_cast<std::size_t>(i + u + (raw.rows.size() - 1 - i)));
    parents.assign(raw.rows[0].begin(),
        raw.rows[0].begin() + std::min<std::size_t>(i, raw.rows[0].size()));
    for (int r = 0; r < u; ++r) parents.push_back(i + r);
    for (int ii = i + 1; ii < static_cast<int>(raw.rows.size()); ++ii) {
        if (ii - 1 < static_cast<int>(raw.rows[0].size())) {
            const int idx = m(ii) - 1;
            if (idx >= static_cast<int>(parents.size())) parents.push_back(m(raw.rows[0][ii - 1]));
            else parents[idx] = m(raw.rows[0][ii - 1]);
        }
    }
    if (expr.rows.empty()) expr.rows.push_back(std::move(parents));
    else expr.rows[0] = std::move(parents);
    return expr;
}

// fullcomp：对第 i 行构造 T 后调用 comp；T 为空则保持原样。
[[nodiscard]] inline Den1Expr fullcomp(const Den1Expr& expr, int i) {
    const Den1Row& row = expr.rows[i];
    std::vector<int> T{ row[row[0]] };
    while (true) {
        const int row_idx = T[0];
        int v = 0;
        if (row_idx >= 0 && row_idx < static_cast<int>(expr.rows.size())) {
            v = denshare::at_or(expr.rows[row_idx], 2);
        }
        T.insert(T.begin(), v);
        if (!(T[0] > denshare::at_or(row, static_cast<std::size_t>(row[0]) + 1))) break;
    }
    if (!T.empty()) T.erase(T.begin());    // slice(1, -1)
    if (!T.empty()) T.pop_back();
    return T.empty() ? expr : comp(expr, i, T);
}

// expand：核心展开。longer=false 对应 FS（较短），true 对应 FS_alter（较长）。
[[nodiscard]] inline Den1Expr expand(const Den1Expr& raw, int fs_term, bool longer) {
    if (raw.rows.empty()) return raw;
    const Den1Row& active = raw.rows.back();
    const std::size_t idx = active.empty() ? 1 : 1 + static_cast<std::size_t>(active[0]);
    if (active.empty() || idx >= active.size() || active[idx] == 0) return cut(raw);

    std::vector<std::vector<int>> rows;
    for (std::size_t i = static_cast<std::size_t>(active[idx]);
         i + 1 < raw.rows.size(); ++i) {
        rows.emplace_back(raw.rows[i].begin() + 1, raw.rows[i].end());
    }
    std::vector<int> tcheck(active.begin() + static_cast<std::ptrdiff_t>(idx), active.end());
    const int flag = denshare::pleasant_until(rows, tcheck);

    Den1Expr expr = raw;
    if (flag >= 0) {
        expr = copy(expr, flag);
    } else {
        for (int n = 1; n <= fs_term; ++n) expr = extend(expr);
        expr = longer ? copy(expr, 1) : cut(expr);
    }

    // 对复制区逐行做 fullcomp
    for (int i = static_cast<int>(raw.rows.size()) - 1;
         i < static_cast<int>(expr.rows.size()); ++i) {
        const Den1Row& row = expr.rows[i];
        if (row.size() <= static_cast<std::size_t>(row[0]) * 2 + 1) {
            expr = fullcomp(expr, i);
        }
    }
    return expr;
}

// ---------------------------------------------------------------------
// 极限的 FS（TS 的 Limit）
// ---------------------------------------------------------------------

// LimitR(n)：极限的父标号行。
[[nodiscard]] inline Den1Row limit_r(int n) {
    Den1Row out;
    if (n == 0) {
        out = { 0, 0 };
    } else {
        out = { 0, 0, 0 };
        for (int nn = 0; nn < n - 1; ++nn) out.push_back(3 + nn);
    }
    return out;
}

// Limit_row(n)：极限主体行。
[[nodiscard]] inline Den1Row limit_row(int n) {
    Den1Row raw;
    for (int nn = 0; nn < 3 + n; ++nn) raw.push_back(nn);
    raw.push_back(2);
    std::reverse(raw.begin(), raw.end());
    return raw;
}

// Limit(n)：记号极限的第 n 个基本列。
[[nodiscard]] inline Den1Expr limit(int n) {
    Den1Expr expr;
    expr.rows.push_back(limit_r(n));
    expr.rows.push_back({ 1, 1, 0 });
    expr.rows.push_back({ 1, 2, 1, 0 });
    for (int nn = 0; nn < n; ++nn) expr.rows.push_back(limit_row(1 + nn));
    return expr;
}

// FS / FS_alter 的公共骨架。
[[nodiscard]] inline std::string expand_text_impl(std::string_view text, int term, bool longer) {
    if (term < 1) term = 1;
    auto parsed = parse(text);
    if (!parsed) {
        return "解析失败，请确认你没有使用全角括号，逗号，分号";
    }
    if (parsed->infinity) {
        return to_string(limit(term));
    }
    if (parsed->rows.size() <= 1) {
        Den1Expr e;
        e.rows.emplace_back();
        return to_string(e);
    }
    return to_string(expand(*parsed, term, longer));
}

} // namespace den1_detail

// ---------------------------------------------------------------------
// 公共接口
// ---------------------------------------------------------------------

[[nodiscard]] inline std::string Den1Notation::expand_text(std::string_view text, int term) {
    return den1_detail::expand_text_impl(text, term, /*longer=*/false);
}

[[nodiscard]] inline std::string Den1Notation::expand_text_alter(std::string_view text, int term) {
    return den1_detail::expand_text_impl(text, term, /*longer=*/true);
}

[[nodiscard]] inline std::optional<denshare::Den1Expr> Den1Notation::parse(std::string_view text) {
    return den1_detail::parse(text);
}

[[nodiscard]] inline std::string Den1Notation::to_string(const denshare::Den1Expr& expr) {
    return den1_detail::to_string(expr);
}

} // namespace omegay::notation

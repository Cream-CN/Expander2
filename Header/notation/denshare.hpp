// Header/notation/denshare.hpp
// DEN 系列记号（DEN1 / blp / iblp）的公共部分。
//
// 仅存放三种记号共用的类型、文本解析器与比较/映射工具；
// 各记号自身的展开算法位于 den1.hpp / blp.hpp / iblp.hpp。
// 逻辑移植自 notation-explorer（hypcos）的 DEN / DEN2 / DEN3，
// 对应源文件：src/notations/DEN/DEN.ts、DEN2.ts、DEN3.ts。
#pragma once

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation::denshare {

// =====================================================================
// 基本类型
// =====================================================================

// DEN2 / DEN3 的条目：一个整数值 + 是否带标记（display 中以 '*' 表示）。
struct DenEntry {
    int value = 0;
    bool marked = false;
};

// DEN2 的一行：(step, 条目列表)。条目按下标 0 起访问。
struct DenRow {
    int step = 0;
    std::vector<DenEntry> entries;
};

// DEN1 的一行（“扁平行”）：row[0] 是 step，row[1..] 是条目值（无标记）。
using Den1Row = std::vector<int>;

// DEN3 的一行（“扁平行”）：rows[0] 是 step（视为占位条目），
// rows[1..] 是条目。位置 p 的元素就是 rows[p]，与 TS 源码一一对应。
using Den3Row = std::vector<DenEntry>;

// DEN1 表达式。约定 rows[0] 是“父标号列表”，显示在 ';' 之后。
struct Den1Expr {
    bool infinity = false;      // 记号极限（display 为 "Limit"）
    std::vector<Den1Row> rows;
};

// DEN2 表达式。
struct DenExpr {
    bool infinity = false;
    std::vector<DenRow> rows;
};

// DEN3 表达式。
struct Den3Expr {
    bool infinity = false;
    std::vector<Den3Row> rows;
};

// =====================================================================
// 比较工具（对应 utils.ts 的 boolean_compare / number_compare /
// lex_compare，以及各记号内的 seqseq_compare / seq_seq_compare）
// =====================================================================

// 布尔比较：false < true。
[[nodiscard]] inline int bool_compare(bool a, bool b) noexcept {
    return (a ? 1 : 0) - (b ? 1 : 0);
}

[[nodiscard]] inline int number_compare(int a, int b) noexcept {
    return a == b ? 0 : (a < b ? -1 : 1);
}

// 字典序比较两个整数行：先比元素，再比长度（更长者更大）。
[[nodiscard]] inline int lex_compare(
    const std::vector<int>& a, const std::vector<int>& b) noexcept {
    const std::size_t n = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < n; ++i) {
        if (const int c = number_compare(a[i], b[i]); c != 0) return c;
    }
    return number_compare(static_cast<int>(a.size()), static_cast<int>(b.size()));
}

// 行序列的字典序比较（先比第一行，再依次向后）。
[[nodiscard]] inline int matrix_compare(
    const std::vector<std::vector<int>>& m1,
    const std::vector<std::vector<int>>& m2) noexcept {
    const std::size_t n = std::min(m1.size(), m2.size());
    for (std::size_t i = 0; i < n; ++i) {
        if (const int c = lex_compare(m1[i], m2[i]); c != 0) return c;
    }
    return number_compare(static_cast<int>(m1.size()), static_cast<int>(m2.size()));
}

// =====================================================================
// 文本解析器（对应 TS from_display 中的 skip_spaces / parse_digits）。
// TS 在非法输入时直接 throw；这里改为返回 nullopt（不抛异常，符合项目约定）。
// =====================================================================

class TextParser {
public:
    explicit TextParser(std::string_view s) noexcept : s_(s) {}

    [[nodiscard]] bool eof() const noexcept { return pos_ >= s_.size(); }
    [[nodiscard]] char peek() const noexcept { return eof() ? '\0' : s_[pos_]; }

    // TS 只跳过空格 ' '（不跳过制表符、换行）。
    void skip_spaces() noexcept {
        while (!eof() && s_[pos_] == ' ') ++pos_;
    }

    void advance(std::size_t n) noexcept { pos_ = std::min(pos_ + n, s_.size()); }

    // 从当前位置尝试匹配字面量（调用前先 skip_spaces），成功则消费。
    [[nodiscard]] bool match(std::string_view lit) noexcept {
        if (s_.substr(pos_, lit.size()) != lit) return false;
        pos_ += lit.size();
        return true;
    }

    // 解析一串十进制数字；当前位置没有数字时返回 nullopt。
    // 用 long long 累积并截断到 int，避免溢出 UB。
    [[nodiscard]] std::optional<int> parse_digits() {
        skip_spaces();
        const std::size_t start = pos_;
        long long value = 0;
        while (!eof() && s_[pos_] >= '0' && s_[pos_] <= '9') {
            value = value * 10 + (s_[pos_] - '0');
            if (value > std::numeric_limits<int>::max()) {
                value = std::numeric_limits<int>::max();
            }
            ++pos_;
        }
        if (pos_ == start) return std::nullopt;
        return static_cast<int>(value);
    }

private:
    std::string_view s_;
    std::size_t pos_ = 0;
};

// =====================================================================
// 行值提取与安全下标
// =====================================================================

// DEN2 行的条目值列表。
[[nodiscard]] inline std::vector<int> row_values(const DenRow& row) noexcept {
    std::vector<int> out;
    out.reserve(row.entries.size());
    for (const DenEntry& e : row.entries) out.push_back(e.value);
    return out;
}

// 数组越界时返回 0，对齐 TS 中 undefined ?? 0 / 假值判断的语义。
[[nodiscard]] inline int at_or(const std::vector<int>& v, std::size_t i) noexcept {
    return i < v.size() ? v[i] : 0;
}

// DEN2：按下标取条目值；越界返回 0。
[[nodiscard]] inline int entry_at(const DenRow& row, std::size_t i) noexcept {
    return i < row.entries.size() ? row.entries[i].value : 0;
}

// DEN3：按扁平位置取条目值；位置 0（step）或越界均返回 0。
[[nodiscard]] inline int den3_entry_at(const Den3Row& row, std::size_t pos) noexcept {
    return (pos != 0 && pos < row.size()) ? row[pos].value : 0;
}

// 取 DEN3 扁平行的全部条目值（位置 1..）。
[[nodiscard]] inline std::vector<int> den3_row_values(const Den3Row& row) noexcept {
    std::vector<int> out;
    if (row.size() > 1) {
        out.reserve(row.size() - 1);
        for (std::size_t i = 1; i < row.size(); ++i) out.push_back(row[i].value);
    }
    return out;
}

// =====================================================================
// pleasant_until 与 ap 映射（三种记号共用的判据与公式）
// =====================================================================

// 第一个 < limit 的元素下标；没有则返回 -1。
[[nodiscard]] inline std::ptrdiff_t index_of_first_less(
    const std::vector<int>& v, int limit) noexcept {
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (v[i] < limit) return static_cast<std::ptrdiff_t>(i);
    }
    return -1;
}

// 最后一个 > limit 的元素下标；没有则返回 -1。
[[nodiscard]] inline std::ptrdiff_t index_of_last_greater(
    const std::vector<int>& v, int limit) noexcept {
    for (std::size_t i = v.size(); i-- > 0;) {
        if (v[i] > limit) return static_cast<std::ptrdiff_t>(i);
    }
    return -1;
}

// v[i1..i2] 中是否存在不属于 tcheck 的值。
[[nodiscard]] inline bool contains_any_not_in(
    const std::vector<int>& v,
    std::size_t i1, std::size_t i2,
    const std::vector<int>& tcheck) noexcept {
    for (std::size_t i = i1; i <= i2 && i < v.size(); ++i) {
        if (std::find(tcheck.begin(), tcheck.end(), v[i]) == tcheck.end()) {
            return true;
        }
    }
    return false;
}

// pleasant_until：在若干行的条目值中找第一个“不愉快”行。
// rows 的第 n 个元素是第 n 行（在考察区间内）的条目值列表；
// tcheck 是 active 行的检查值列表。找不到返回 -1。
[[nodiscard]] inline int pleasant_until(
    const std::vector<std::vector<int>>& rows,
    const std::vector<int>& tcheck) noexcept {
    if (tcheck.empty()) return -1;
    const int tmax = tcheck.front();
    const int tmin = tcheck.back();
    for (std::size_t n = 0; n < rows.size(); ++n) {
        const std::vector<int>& scheck = rows[n];
        const std::ptrdiff_t i1 = index_of_first_less(scheck, tmax);
        const std::ptrdiff_t i2 = index_of_last_greater(scheck, tmin);
        if (i1 >= 0 && i2 >= 0 && i1 <= i2 &&
            contains_any_not_in(scheck,
                static_cast<std::size_t>(i1), static_cast<std::size_t>(i2), tcheck)) {
            return static_cast<int>(n);
        }
    }
    return -1;
}

// 最后一个等于 x 的下标；找不到返回 npos。
[[nodiscard]] inline std::size_t last_index_of(
    const std::vector<int>& v, int x) noexcept {
    for (std::size_t i = v.size(); i-- > 0;) {
        if (v[i] == x) return i;
    }
    return std::string_view::npos;
}

// ap 的单值映射（TS 的 apv / ap 中的同一公式）。
// tvals 是 active 行的条目值列表（不含 step），step 是 active 的 step。
[[nodiscard]] inline int ap_value(int x, const std::vector<int>& tvals, int step) noexcept {
    if (x < tvals.back()) return x;
    if (static_cast<std::size_t>(step) < tvals.size() && x >= tvals[step]) {
        return x - tvals[step] + tvals.front();
    }
    // 第三分支：t[t.lastIndexOf(x) - step]
    const std::size_t j = last_index_of(tvals, x);
    if (j == std::string_view::npos || j < static_cast<std::size_t>(step)) {
        return x;   // 仅在非法输入下可达；保守保持原值
    }
    return tvals[j - step];
}

// =====================================================================
// 切片辅助：复刻 TS 的 slice(1, -tail) / slice(from) 语义。
// 注意 TS 中 -0 会被视为 0，导致 slice(1, 0) 返回空段。
// =====================================================================

// 把 row[from..] 追加入 out（from 可能为负，按 0 处理）。
template <typename T>
inline void append_slice_from(std::vector<T>& out, const std::vector<T>& row, int from) {
    std::size_t i = from <= 0 ? 0 : static_cast<std::size_t>(from);
    for (; i < row.size(); ++i) out.push_back(row[i]);
}

// 把 row[from .. size-tail) 追加入 out（对齐 slice(from, -tail)）。
template <typename T>
inline void append_slice_except_tail(
    std::vector<T>& out, const std::vector<T>& row, int from, int tail) {
    if (tail <= 0) return;   // -0 语义：空段
    const std::size_t n = row.size();
    if (static_cast<std::size_t>(tail) >= n) return;
    const std::size_t end = n - static_cast<std::size_t>(tail);
    std::size_t i = from <= 0 ? 0 : static_cast<std::size_t>(from);
    for (; i < end; ++i) out.push_back(row[i]);
}

// =====================================================================
// 极限行的公共构造（DEN2 / DEN3 共用）
// =====================================================================

// Limit_row(n) 的条目列表（含标记规则 3 <= nn < 2+n，末尾追加 2 后反转）。
[[nodiscard]] inline std::vector<DenEntry> limit_row_entries(int n) {
    std::vector<DenEntry> out;
    out.reserve(static_cast<std::size_t>(3 + n) + 1);
    for (int nn = 0; nn < 3 + n; ++nn) {
        out.push_back({ nn, 3 <= nn && nn < 2 + n });
    }
    out.push_back({ 2, false });
    std::reverse(out.begin(), out.end());
    return out;
}

} // namespace omegay::notation::denshare


#pragma once

#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace omegay::notation::sdbmscore {

// ---------------------------------------------------------------------
// 通用比较器（TS utils.ts）
// ---------------------------------------------------------------------

inline int number_compare(int a, int b) { return a < b ? -1 : a > b ? 1 : 0; }

inline int boolean_compare(bool a, bool b) { return a == b ? 0 : a ? 1 : -1; }

// lex_compare：逐元素比较；全部相等时 longer_last 决定谁大（默认长更大）。
template <typename T, typename Cmp>
inline int lex_compare(const std::vector<T>& a, const std::vector<T>& b, Cmp cmp,
                       bool longer_last = true) {
    const std::size_t n = a.size() < b.size() ? a.size() : b.size();
    for (std::size_t i = 0; i < n; ++i) {
        const int c = cmp(a[i], b[i]);
        if (c != 0) return c;
    }
    if (a.size() == b.size()) return 0;
    if (longer_last) return a.size() > b.size() ? 1 : -1;
    return a.size() > b.size() ? -1 : 1;
}

// anti_lex_compare：先长度（长更大），再自后向前逐元素比较。
template <typename T, typename Cmp>
inline int anti_lex_compare(const std::vector<T>& a, const std::vector<T>& b, Cmp cmp) {
    if (a.size() != b.size()) return a.size() > b.size() ? 1 : -1;
    for (std::size_t i = a.size(); i-- > 0;) {
        const int c = cmp(a[i], b[i]);
        if (c != 0) return c;
    }
    return 0;
}

// compare_undefined_last_by(number_compare)：undefined 视为最大（TS 语义）。
// 用 kUndef 哨兵表示 undefined；哨兵 > 任何真实数、哨兵 == 哨兵。
constexpr int kUndef = std::numeric_limits<int>::max();

inline int undef_last_compare(int a, int b) {
    if (a == kUndef) return b == kUndef ? 0 : 1;
    if (b == kUndef) return -1;
    return number_compare(a, b);
}

// ---------------------------------------------------------------------
// 文本解析器（TS from_display 的递归下降骨架）
// ---------------------------------------------------------------------

struct Parser {
    const char* s = nullptr;
    std::size_t i = 0;
    std::size_t len = 0;

    explicit Parser(std::string_view sv) : s(sv.data()), len(sv.size()) {}

    void skip_spaces() {
        while (i < len && s[i] == ' ') ++i;
    }
    bool at_end() const { return i >= len; }
    char peek() const { return i < len ? s[i] : '\0'; }

    // 跳过 display_marked 写入的列标（如 ':2'），其值被舍弃。
    void skip_index() {
        if (i < len && s[i] == ':') {
            ++i;
            skip_spaces();
            while (i < len && s[i] >= '0' && s[i] <= '9') ++i;
        }
    }

    // 解析无符号整数；失败返回 false。
    bool parse_number(int& out) {
        skip_spaces();
        if (i >= len || s[i] < '0' || s[i] > '9') return false;
        int v = 0;
        while (i < len && s[i] >= '0' && s[i] <= '9') {
            v = v * 10 + (s[i] - '0');
            ++i;
        }
        out = v;
        return true;
    }

    std::size_t remaining() const { return len - i; }
};

// ---------------------------------------------------------------------
// 通用矩阵机
// ---------------------------------------------------------------------

template <typename Ops>
class Machine {
public:
    using Height = typename Ops::Height;
    using Entry = std::pair<int, Height>;   // [v, h]：v 为行值（0 起），h 为行高
    using Column = std::vector<Entry>;
    using Expr = std::vector<Column>;

    // TS copy_value：>= r 才加 offset。
    static int copy_value(int v, int r, int offset) { return v >= r ? v + offset : v; }

    static std::optional<Height> height_of(const Column& col) { return Ops::height_of(col); }

    static Column filter_height_greater(const Column& col, const Height& h0) {
        Column out;
        out.reserve(col.size());
        for (const Entry& e : col) {
            if (Ops::compare_height(e.second, h0) > 0) out.push_back(e);
        }
        return out;
    }

    // TS merge_column(col1, col2)：col1 + 高于 height(col1) 的 col2 尾部。
    static Column merge_column(const Column& c1, const Column& c2) {
        const auto h1 = Ops::height_of(c1);
        Column out = c1;
        if (!h1) {
            out.insert(out.end(), c2.begin(), c2.end());
        } else {
            Column f = filter_height_greater(c2, *h1);
            out.insert(out.end(), f.begin(), f.end());
        }
        return out;
    }

    static Entry copy_entry(const Entry& e, int r, int offset) {
        return { copy_value(e.first, r, offset), Ops::copy_height(e.second, r, offset) };
    }

    static Column copy_column(const Column& col, int r, int offset) {
        Column out;
        out.reserve(col.size());
        for (const Entry& e : col) out.push_back(copy_entry(e, r, offset));
        return out;
    }

    static int compare_entry(const Entry& a, const Entry& b) {
        const int c = number_compare(a.first, b.first);
        if (c != 0) return c;
        return Ops::compare_height(a.second, b.second);
    }

    static int compare_column(const Column& a, const Column& b) {
        return lex_compare(a, b,
                           [](const Entry& x, const Entry& y) { return compare_entry(x, y); });
    }

    static int compare(const Expr& a, const Expr& b) {
        return lex_compare(a, b,
                           [](const Column& x, const Column& y) { return compare_column(x, y); });
    }

    static bool is_limit(const Expr& e) { return !e.empty() && !e.back().empty(); }

    static Expr infinity_fs(int index) { return Ops::infinity_fs(index); }

    // TS expand：标准 DBMS 基本列展开（短/长由 shorter 区分）。
    static Expr expand(const Expr& expr, int index, bool shorter) {
        if (expr.empty()) return expr;
        const std::size_t right = expr.size() - 1;
        if (expr[right].empty()) {
            return Expr(expr.begin(), expr.begin() + static_cast<std::ptrdiff_t>(right));
        }
        const std::size_t top = expr[right].size() - 1;
        const Entry& tr = expr[right][top];
        const int r = tr.first;
        const Height& h = tr.second;

        const auto new_h = Ops::compute_new_height(h, expr, r);

        Column new_col(expr[right].begin(), expr[right].begin() + static_cast<std::ptrdiff_t>(top));
        if (new_h) new_col = merge_column(new_col, Column{ Entry{ r, *new_h } });
        new_col = merge_column(new_col, expr[r]);

        Expr result(expr.begin(), expr.begin() + static_cast<std::ptrdiff_t>(right));
        result.push_back(std::move(new_col));

        const int r_right = static_cast<int>(right);
        for (int w = 1; w <= index; ++w) {
            const int offset = (r_right - r) * w;
            for (int i = r + 1; i <= r_right; ++i) {
                result.push_back(copy_column(result[static_cast<std::size_t>(i)], r, offset));
            }
        }
        if (shorter && !result.empty()) result.pop_back();
        return result;
    }
};

// ---------------------------------------------------------------------
// 通用解析与显示骨架（行高语法经 Ops::parse_entry / entry_to_string 差异）
// ---------------------------------------------------------------------

// TS from_display：解析表达式；返回（是否 ∞, 矩阵）；失败返回 nullopt。
template <typename Ops>
inline std::optional<std::pair<bool, typename Machine<Ops>::Expr>> parse_expr(
    std::string_view text) {
    using M = Machine<Ops>;
    Parser p(text);
    p.skip_spaces();
    if (p.remaining() >= 5 && std::string_view(p.s + p.i, 5) == "Limit") {
        p.i += 5;
        p.skip_spaces();
        if (!p.at_end()) return std::nullopt;
        return std::pair{ true, typename M::Expr{} };
    }

    typename M::Expr result;
    p.skip_spaces();
    while (!p.at_end() && p.peek() == '(') {
        // parse_column
        p.skip_spaces();
        if (p.peek() != '(') return std::nullopt;
        ++p.i;
        typename M::Column col;
        p.skip_spaces();
        if (!p.at_end() && p.peek() != ')' && p.peek() != ':') {
            auto e = Ops::parse_entry(p);
            if (!e) return std::nullopt;
            col.push_back(*e);
            p.skip_spaces();
            while (!p.at_end() && p.peek() == ',') {
                ++p.i;
                p.skip_spaces();
                if (!p.at_end() && p.peek() == ')') break;
                auto e2 = Ops::parse_entry(p);
                if (!e2) return std::nullopt;
                col.push_back(*e2);
                p.skip_spaces();
            }
        }
        p.skip_spaces();
        p.skip_index();
        p.skip_spaces();
        if (p.at_end() || p.peek() != ')') return std::nullopt;
        ++p.i;
        // 删去列尾表示 0 的项（显示 0 ↔ 内部 v = -1）：空列的显示形式为 (0)。
        while (!col.empty() && col.back().first == -1) col.pop_back();
        result.push_back(std::move(col));
        p.skip_spaces();
    }
    p.skip_spaces();
    if (!p.at_end()) return std::nullopt;
    return std::pair{ false, std::move(result) };
}

// TS display（plain）：∞ 不经过本函数（外层处理）；空列 "(0)"。
template <typename Ops>
inline std::string display(const typename Machine<Ops>::Expr& e) {
    std::string out;
    for (const auto& col : e) {
        if (col.empty()) {
            out += "(0)";
            continue;
        }
        out += '(';
        for (std::size_t k = 0; k < col.size(); ++k) {
            if (k) out += ',';
            out += Ops::entry_to_string(col[k]);
        }
        out += ')';
    }
    return out;
}

} // namespace omegay::notation::sdbmscore

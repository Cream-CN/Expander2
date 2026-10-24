// Header/notation/up2dbms_v2.hpp
// UP2DBMS v2 记号（display 名 "UP2DBMS v2"，TS id "up2dbms-v2"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/UPMN/UP2DBMS-v2.ts
// （家族/类别：category-upmn，credit.up2dbms-v2）。
// 表达式结构（TS）：Height=[h0,h1]（内部 0-based；显示 '(h1,h0+1)'）；
// Entry=[value(0-based), Height]；Column=Entry[]；Expr=Column[]；∞ 用 bool 标志表示。
// 与 UP2DBMS-v1 不同：显示/解析为高度对形式（非 n-MN 分隔符形式）；
// compute_new_height/merge_column 同 SDBMS 家族；compute_up 为 v2 专属版本；
// copy_column 带 BMS 升级（entry[0]==r 且 !up 时分两支）。UI 视图（dbms/RMN/Y/layer、
// draw_diagram、from_y_seq）未移植。
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

namespace up2dbms_v2_detail {

struct Height {
    int h0;  // 父列列标（0-based）
    int h1;
};
struct Entry {
    int value;  // 0-based
    Height height;
};
using Column = std::vector<Entry>;
using Expr = std::vector<Column>;

inline int num_compare(int a, int b) { return a == b ? 0 : (a < b ? -1 : 1); }

// TS compare_height = anti_lex_compare（先比长度，再从尾部比 → 先 h1 后 h0）。
inline int compare_height(const Height& a, const Height& b) {
    const int c1 = num_compare(a.h1, b.h1);
    if (c1 != 0) return c1;
    return num_compare(a.h0, b.h0);
}

inline int entry_compare(const Entry& a, const Entry& b) {
    const int c = num_compare(a.value, b.value);
    if (c != 0) return c;
    return compare_height(a.height, b.height);
}

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

inline int column_compare(const Column& a, const Column& b) {
    return lex_compare(a, b, [](const Entry& x, const Entry& y) { return entry_compare(x, y); });
}
inline int expr_compare(const Expr& a, const Expr& b) {
    return lex_compare(a, b, [](const Column& x, const Column& y) { return column_compare(x, y); });
}

// ---- 显示 / 解析（TS display / from_display）----

inline std::string height_display(const Height& h) {
    return "(" + std::to_string(h.h1) + "," + std::to_string(h.h0 + 1) + ")";
}
inline std::string entry_display(const Entry& e) {
    return std::to_string(e.value + 1) + "^" + height_display(e.height);
}
inline std::string column_display(const Column& c) {
    if (c.empty()) return "(0)";
    std::string out = "(";
    for (std::size_t j = 0; j < c.size(); ++j) {
        if (j) out += ",";
        out += entry_display(c[j]);
    }
    out += ")";
    return out;
}
inline std::string display(const Expr& e) {
    std::string out;
    for (const Column& c : e) out += column_display(c);
    return out;
}

struct Parsed {
    bool infinity = false;
    Expr expr;
};

inline std::optional<Parsed> parse(std::string_view s) {
    std::size_t i = 0;
    auto error = [&]() { return std::optional<Parsed>{}; };
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
        skip_spaces();
        const std::size_t start = i;
        while (i < s.size() && s[i] >= '0' && s[i] <= '9') ++i;
        if (start == i) return std::nullopt;
        int v = 0;
        for (std::size_t k = start; k < i; ++k) v = v * 10 + (s[k] - '0');
        return v;
    };
    auto parse_height = [&]() -> std::optional<Height> {
        skip_spaces();
        if (i >= s.size() || s[i] != '(') return std::nullopt;
        ++i;
        const auto h1 = parse_number();
        if (!h1) return std::nullopt;
        skip_spaces();
        if (i >= s.size() || s[i] != ',') return std::nullopt;
        ++i;
        const auto h0p = parse_number();
        if (!h0p) return std::nullopt;
        const int h0 = *h0p - 1;
        skip_spaces();
        if (i >= s.size() || s[i] != ')') return std::nullopt;
        ++i;
        return Height{ h0, *h1 };
    };
    auto parse_entry = [&]() -> std::optional<Entry> {
        const auto v = parse_number();
        if (!v) return std::nullopt;
        const int value = *v - 1;
        skip_spaces();
        if (i < s.size() && s[i] == '^') {
            ++i;
            const auto h = parse_height();
            if (!h) return std::nullopt;
            return Entry{ value, *h };
        }
        if (i < s.size() && s[i] == '(') {
            const auto h = parse_height();
            if (!h) return std::nullopt;
            return Entry{ value, *h };
        }
        // 无上标写法：哨兵行高 [0, -1] 占位
        return Entry{ value, Height{ 0, -1 } };
    };
    auto parse_column = [&]() -> std::optional<Column> {
        skip_spaces();
        if (i >= s.size() || s[i] != '(') return std::nullopt;
        ++i;
        Column entries;
        skip_spaces();
        if (i < s.size() && s[i] != ')' && s[i] != ':') {
            auto e = parse_entry();
            if (!e) return std::nullopt;
            entries.push_back(*e);
            skip_spaces();
            while (i < s.size() && s[i] == ',') {
                ++i;
                skip_spaces();
                if (i < s.size() && s[i] == ')') break;
                auto e2 = parse_entry();
                if (!e2) return std::nullopt;
                entries.push_back(*e2);
                skip_spaces();
            }
        }
        skip_spaces();
        skip_index();
        skip_spaces();
        if (i >= s.size() || s[i] != ')') return std::nullopt;
        ++i;
        // 删去列尾表示 0 的项（显示 0 ↔ 内部 value=-1）
        while (!entries.empty() && entries.back().value == -1) entries.pop_back();
        return entries;
    };
    auto parse_expr = [&]() -> std::optional<Expr> {
        Expr result;
        skip_spaces();
        while (i < s.size() && s[i] == '(') {
            auto c = parse_column();
            if (!c) return std::nullopt;
            result.push_back(std::move(*c));
            skip_spaces();
        }
        return result;
    };

    skip_spaces();
    if (i + 5 <= s.size() && s.substr(i, 5) == "Limit") {
        i += 5;
        skip_spaces();
        if (i != s.size()) return error();
        return Parsed{ true, {} };
    }
    if (s.substr(i) == "limit" || s.substr(i) == "LIMIT") {
        if (s.size() - i != 5) return error();
        return Parsed{ true, {} };
    }
    auto result = parse_expr();
    if (!result) return error();
    skip_spaces();
    if (i != s.size()) return error();
    return Parsed{ false, std::move(*result) };
}

// ---- 核心辅助 ----

inline std::optional<Height> height(const Column& c) {
    if (c.empty()) return std::nullopt;
    return c.back().height;
}
inline int height1(const Column& c) {
    const auto h = height(c);
    return h ? h->h1 : 0;
}

inline Column filter_height_greater(const Column& c, const Height& h0) {
    Column r;
    for (const Entry& e : c) {
        if (compare_height(e.height, h0) > 0) r.push_back(e);
    }
    return r;
}

inline Column merge_column(const Column& col1, const Column& col2) {
    if (col1.empty()) return col2;
    const Height& h = col1.back().height;
    Column r = col1;
    for (const Entry& e : col2) {
        if (compare_height(e.height, h) > 0) r.push_back(e);
    }
    return r;
}

inline int copy_value(int value, int r, int offset, bool up) {
    return value > r || (value == r && up) ? value + offset : value;
}
inline Height copy_height(const Height& h, int r, int offset, bool up) {
    return Height{ copy_value(h.h0, r, offset, up), h.h1 };
}
inline Entry copy_entry(const Entry& e, int r, int offset, bool up) {
    return Entry{ copy_value(e.value, r, offset, up), copy_height(e.height, r, offset, up) };
}

// TS copy_column：entry[0]==r 且 !up 时带 BMS 升级分支。
inline Column copy_column(const Column& col, int r, int offset, bool up) {
    Column result;
    result.reserve(col.size());
    for (std::size_t j = 0; j < col.size(); ++j) {
        const Entry& entry = col[j];
        const std::optional<Height> prev_height =
            j == 0 ? std::nullopt : std::optional<Height>(col[j - 1].height);
        if (entry.value == r && !up) {
            if (entry.height.h1 == 0 || (prev_height && entry.height.h1 == prev_height->h1)) {
                result.push_back(Entry{ r + offset, copy_height(entry.height, r, offset, false) });
            } else {
                const Height finite{ r, prev_height ? prev_height->h1 : 0 };
                result.push_back(Entry{ r + offset, finite });
                result.push_back(copy_entry(entry, r, offset, false));
            }
        } else {
            result.push_back(copy_entry(entry, r, offset, up));
        }
    }
    return result;
}

// TS compute_new_height(h=[v,h1], expr, r)。
inline std::optional<Height> compute_new_height(const Height& h, const Expr& expr, int r) {
    const int v = h.h0;
    const Column& col_rh = expr[static_cast<std::size_t>(v)];
    int hj = -1;
    for (std::size_t j = 0; j < col_rh.size(); ++j) {
        if (compare_height(h, col_rh[j].height) <= 0) {
            hj = static_cast<int>(j);
            break;
        }
    }
    if (hj == -1) {
        if (height1(col_rh) < h.h1) return Height{ r, h.h1 - 1 };
        return height(col_rh);
    }
    Height new_h{ col_rh[static_cast<std::size_t>(hj)].value, h.h1 };
    if (hj > 0) {
        const Height& lower = col_rh[static_cast<std::size_t>(hj - 1)].height;
        if (compare_height(lower, new_h) > 0) new_h = lower;
    }
    return new_h;
}

// ---- compute_up（v2 专属；TS compute_up / parent_at / height_after / discard_after / 相对列）----

inline std::optional<Height> height_after(const Column& col, int r) {
    for (std::size_t j = col.size(); j-- > 0;) {
        if (col[j].value >= r) return col[j].height;
    }
    return std::nullopt;
}

inline int parent_at(const Column& col, const std::optional<Height>& h) {
    if (!h) return col.empty() ? -1 : col[0].value;
    for (std::size_t j = 0; j < col.size(); ++j) {
        if (compare_height(col[j].height, *h) >= 0) return col[j].value;
    }
    return -1;
}

inline Column discard_after(const Column& col, int r) {
    Column result;
    for (const Entry& e : col) {
        if (e.value <= r) result.push_back(e);
    }
    return result;
}

struct RelEntry {
    bool f0;
    int v0;
    bool f1;
    int v1;
    int h1;
};

inline int rel_value_compare(bool fa, int va, bool fb, int vb) {
    if (fa != fb) return fa ? 1 : -1;
    return num_compare(va, vb);
}
inline int rel_entry_compare(const RelEntry& a, const RelEntry& b) {
    int c = rel_value_compare(a.f0, a.v0, b.f0, b.v0);
    if (c != 0) return c;
    c = rel_value_compare(a.f1, a.v1, b.f1, b.v1);
    if (c != 0) return c;
    return num_compare(a.h1, b.h1);
}

inline std::vector<RelEntry> to_rel_column(const Column& c, int r) {
    std::vector<RelEntry> result;
    result.reserve(c.size());
    for (const Entry& e : c) {
        const bool f0 = e.value >= r;
        const bool f1 = e.height.h0 >= r;
        result.push_back(RelEntry{ f0, f0 ? e.value - r : e.value,
                                   f1, f1 ? e.height.h0 - r : e.height.h0,
                                   e.height.h1 });
    }
    return result;
}

inline int rel_column_compare(const std::vector<RelEntry>& a, const std::vector<RelEntry>& b) {
    return lex_compare(a, b, [](const RelEntry& x, const RelEntry& y) { return rel_entry_compare(x, y); });
}

inline std::vector<bool> compute_up(const Expr& expr, int r, const Height& h) {
    const int right = static_cast<int>(expr.size()) - 1;
    std::vector<bool> result(expr.size(), false);
    for (int k = 0; k < r; ++k) result[static_cast<std::size_t>(k)] = false;
    result[static_cast<std::size_t>(r)] = true;
    if (h.h1 == 0) {
        for (int k = r + 1; k < static_cast<int>(expr.size()); ++k) result[static_cast<std::size_t>(k)] = true;
        return result;
    }
    for (int i = r + 1; i <= right; ++i) {
        const Column& col = expr[static_cast<std::size_t>(i)];
        const std::optional<Height> h_root = height_after(col, r);
        if (!h_root || h_root->h1 < h.h1) {
            result[static_cast<std::size_t>(i)] = false;
            continue;
        }
        const std::optional<Height> h_proj = height_after(col, r + 1);
        const Height threshold{ r + 1, h_proj ? std::min(h_proj->h1, h.h1 - 1) : 0 };
        if (h_proj && compare_height(*h_proj, threshold) >= 0) {
            const int pa = parent_at(col, threshold);
            if (pa >= 0) {
                result[static_cast<std::size_t>(i)] = result[static_cast<std::size_t>(pa)];
            } else {
                result[static_cast<std::size_t>(i)] = false;
            }
            continue;
        }
        const int p_proj = parent_at(col, h_proj);
        if (p_proj < 0 || !result[static_cast<std::size_t>(p_proj)]) {
            result[static_cast<std::size_t>(i)] = false;
            continue;
        }
        const int X_start = i;
        int Y_start = right;
        {
            bool walk_ok = true;
            while (true) {
                const int next = parent_at(expr[static_cast<std::size_t>(Y_start)], threshold);
                if (next == r) break;
                if (next < 0) {  // 防御：TS 此处会死循环/越界；合法输入不触发
                    walk_ok = false;
                    break;
                }
                Y_start = next;
            }
            if (!walk_ok) {
                result[static_cast<std::size_t>(i)] = false;
                continue;
            }
        }
        if (Y_start <= X_start) {
            result[static_cast<std::size_t>(i)] = X_start == Y_start;
            continue;
        }
        const Column X0 = discard_after(expr[static_cast<std::size_t>(X_start)], r);
        const Column Y0 = discard_after(expr[static_cast<std::size_t>(Y_start)], r);
        const int cmp_0 = column_compare(X0, Y0);
        if (cmp_0 != 0) {
            result[static_cast<std::size_t>(i)] = cmp_0 > 0;
            continue;
        }
        bool decided = false;
        for (int k = 1; Y_start + k < static_cast<int>(expr.size()); ++k) {
            const auto Xk = to_rel_column(expr[static_cast<std::size_t>(X_start + k)], X_start);
            const auto Yk = to_rel_column(expr[static_cast<std::size_t>(Y_start + k)], Y_start);
            const int cmp = rel_column_compare(Xk, Yk);
            if (cmp != 0) {
                result[static_cast<std::size_t>(i)] = cmp > 0;
                decided = true;
                break;
            }
        }
        if (!decided) result[static_cast<std::size_t>(i)] = true;
    }
    return result;
}

// ---- 展开 ----

inline Expr expand(const Expr& expr, int index, bool shorter) {
    if (expr.empty()) return expr;
    const int right = static_cast<int>(expr.size()) - 1;
    if (expr[static_cast<std::size_t>(right)].empty()) {
        return Expr(expr.begin(), expr.end() - 1);
    }
    const int top = static_cast<int>(expr[static_cast<std::size_t>(right)].size()) - 1;
    const Entry& tr = expr[static_cast<std::size_t>(right)][static_cast<std::size_t>(top)];
    const int r = tr.value;
    const Height h = tr.height;

    const std::optional<Height> new_h = compute_new_height(h, expr, r);

    Column new_col(expr[static_cast<std::size_t>(right)].begin(),
                   expr[static_cast<std::size_t>(right)].end() - 1);
    if (new_h) {
        Column merged = merge_column(new_col, Column{ Entry{ r, *new_h } });
        new_col = std::move(merged);
    }
    new_col = merge_column(new_col, expr[static_cast<std::size_t>(r)]);

    Expr result(expr.begin(), expr.end() - 1);
    result.push_back(std::move(new_col));

    const std::vector<bool> up = compute_up(expr, r, h);

    for (int w = 1; w <= index; ++w) {
        for (int i = r + 1; i <= right; ++i) {
            result.push_back(copy_column(result[static_cast<std::size_t>(i)], r,
                                         (right - r) * w, up[static_cast<std::size_t>(i)]));
        }
    }
    if (shorter) result.pop_back();
    return result;
}

// TS infinity_FS(index) = [[], [ [i, [i, i+1]] for i in 0..index-1 ]]
inline Expr infinity_fs(int index) {
    Expr result;
    result.emplace_back();
    for (int i = 0; i < index; ++i) {
        Column col;
        col.push_back(Entry{ i, Height{ i, i + 1 } });
        result.push_back(std::move(col));
    }
    return result;
}

} // namespace up2dbms_v2_detail

struct Up2DbmsV2Notation {
    static constexpr const char* kName = "UP2DBMS v2";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/false);
    }
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::optional<up2dbms_v2_detail::Parsed> parse(std::string_view text) {
        return up2dbms_v2_detail::parse(text);
    }

    [[nodiscard]] static std::string to_string(const up2dbms_v2_detail::Parsed& p) {
        if (p.infinity) return "Limit";
        return up2dbms_v2_detail::display(p.expr);
    }

private:
    static std::string expand_text_impl(std::string_view text, int term, bool longer) {
        if (term < 1) term = 1;
        const auto parsed = up2dbms_v2_detail::parse(text);
        if (!parsed) {
            return "解析失败，请确认你没有使用全角括号，逗号，分号";
        }
        if (parsed->infinity) {
            return up2dbms_v2_detail::display(up2dbms_v2_detail::infinity_fs(term));
        }
        const up2dbms_v2_detail::Expr& e = parsed->expr;
        if (e.empty()) return {};
        // 值域校验：value / 行高 h0 均作为列索引使用，须在 [0, size)
        for (const auto& col : e) {
            for (const auto& en : col) {
                if (en.value < 0 || en.value >= static_cast<int>(e.size()) ||
                    en.height.h0 < 0 || en.height.h0 >= static_cast<int>(e.size())) {
                    return "解析失败，请确认你没有使用全角括号，逗号，分号";
                }
            }
        }
        if (e.back().empty()) {
            return up2dbms_v2_detail::display(
                up2dbms_v2_detail::Expr(e.begin(), e.end() - 1));
        }
        return up2dbms_v2_detail::display(
            up2dbms_v2_detail::expand(e, term, /*shorter=*/!longer));
    }
};

} // namespace omegay::notation

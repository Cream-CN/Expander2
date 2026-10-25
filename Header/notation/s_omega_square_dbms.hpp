
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

struct SOmegaSquareHeightPos {
    int a = 0;          // p1
    int b = 0;          // p0
    bool bDef = false;  // p0 是否定义（undefined = ω）
};

struct SOmegaSquareHEntry {
    int v;
    SOmegaSquareHeightPos p;
};
using SOmegaSquareHeight = std::vector<SOmegaSquareHEntry>;

struct SOmegaSquareEntry {
    int value;
    SOmegaSquareHeight height;
};
using SOmegaSquareColumn = std::vector<SOmegaSquareEntry>;
using SOmegaSquareExpr = std::vector<SOmegaSquareColumn>;

struct SOmegaSquareParsed {
    bool infinity = false;
    SOmegaSquareExpr expr;
};

struct SOmegaSquareDBMSNotation {
    static constexpr const char* kName = "S ω^2 DBMS";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/false);
    }
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::optional<SOmegaSquareParsed> parse(std::string_view s) {
        if (s == "Limit" || s == "limit" || s == "LIMIT") {
            return SOmegaSquareParsed{ true, {} };
        }
        std::size_t i = 0;
        auto error = [&]() { return std::optional<SOmegaSquareParsed>{}; };
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
        auto parse_height_value = [&]() -> std::optional<int> {
            auto n = parse_number();
            if (!n) return std::nullopt;
            return *n - 1;
        };
        // 位置：'ω'/'w' → [0,undefined]；'ω'+n → [n-1,undefined]；'ω'+n+'+'m → [n,m-1]；数字 n → [0,n-1]。
        // 注意：MSVC 的 char 有符号，须按 unsigned char 比较 UTF-8 首字节 0xCF
        auto parse_height_pos = [&]() -> std::optional<SOmegaSquareHeightPos> {
            skip_spaces();
            if (i < s.size() && (static_cast<unsigned char>(s[i]) == 0xCF || s[i] == 'w')) {
                if (static_cast<unsigned char>(s[i]) == 0xCF) {
                    if (i + 1 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0x89) {
                        i += 2;
                    } else {
                        return std::nullopt;
                    }
                } else {
                    ++i;
                }
                skip_spaces();
                int n = 1;
                if (i < s.size() && s[i] >= '0' && s[i] <= '9') {
                    auto nn = parse_number();
                    if (!nn) return std::nullopt;
                    n = *nn;
                }
                skip_spaces();
                if (i < s.size() && s[i] == '+') {
                    ++i;
                    auto m = parse_number();
                    if (!m) return std::nullopt;
                    return SOmegaSquareHeightPos{ n, *m - 1, true };
                }
                if (n == 1) return SOmegaSquareHeightPos{ 0, 0, false };
                return SOmegaSquareHeightPos{ n - 1, 0, false };
            }
            auto m = parse_height_value();
            if (!m) return std::nullopt;
            return SOmegaSquareHeightPos{ 0, *m, true };
        };
        auto parse_height_item = [&]() -> std::optional<SOmegaSquareHEntry> {
            auto v = parse_height_value();
            if (!v) return std::nullopt;
            skip_spaces();
            if (i >= s.size() || s[i] != '@') return std::nullopt;
            ++i;
            auto p = parse_height_pos();
            if (!p) return std::nullopt;
            return SOmegaSquareHEntry{ *v, *p };
        };
        auto parse_height = [&]() -> std::optional<SOmegaSquareHeight> {
            skip_spaces();
            if (i >= s.size() || s[i] != '(') return std::nullopt;
            ++i;
            skip_spaces();
            bool extra_paren = false;
            if (i < s.size() && s[i] == '(') {
                extra_paren = true;
                ++i;
                skip_spaces();
            }
            SOmegaSquareHeight result;
            if (i < s.size() && s[i] != ')') {
                auto e = parse_height_item();
                if (!e) return std::nullopt;
                result.push_back(*e);
                skip_spaces();
                while (i < s.size() && s[i] == ',') {
                    ++i;
                    skip_spaces();
                    auto e2 = parse_height_item();
                    if (!e2) return std::nullopt;
                    result.push_back(*e2);
                    skip_spaces();
                }
            }
            if (i >= s.size() || s[i] != ')') return std::nullopt;
            ++i;
            if (extra_paren) {
                skip_spaces();
                if (i >= s.size() || s[i] != ')') return std::nullopt;
                ++i;
            }
            return result;
        };
        auto parse_entry = [&]() -> std::optional<SOmegaSquareEntry> {
            auto v = parse_number();
            if (!v) return std::nullopt;
            --*v;
            skip_spaces();
            if (i < s.size() && s[i] == '^') {
                ++i;
                auto h = parse_height();
                if (!h) return std::nullopt;
                return SOmegaSquareEntry{ *v, *h };
            }
            return SOmegaSquareEntry{ *v, {} };  // '(0)' 等省略上标
        };
        auto parse_column = [&]() -> std::optional<SOmegaSquareColumn> {
            skip_spaces();
            if (i >= s.size() || s[i] != '(') return std::nullopt;
            ++i;
            SOmegaSquareColumn entries;
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
            while (!entries.empty() && entries.back().value == -1) entries.pop_back();
            return entries;
        };

        SOmegaSquareExpr result;
        skip_spaces();
        while (i < s.size() && s[i] == '(') {
            auto col = parse_column();
            if (!col) return std::nullopt;
            result.push_back(std::move(*col));
            skip_spaces();
        }
        skip_spaces();
        if (i != s.size()) return std::nullopt;
        return SOmegaSquareParsed{ false, std::move(result) };
    }

    [[nodiscard]] static std::string to_string(const SOmegaSquareParsed& p) {
        if (p.infinity) return "Limit";
        return display(p.expr);
    }

private:
    using HeightPos = SOmegaSquareHeightPos;
    using HEntry = SOmegaSquareHEntry;
    using Height = SOmegaSquareHeight;
    using Entry = SOmegaSquareEntry;
    using Column = SOmegaSquareColumn;
    using Expr = SOmegaSquareExpr;

    [[nodiscard]] static std::string display(const Expr& expr) {
        std::string out;
        for (const Column& col : expr) {
            if (col.empty()) {
                out += "(0)";
                continue;
            }
            out += '(';
            for (std::size_t k = 0; k < col.size(); ++k) {
                if (k) out += ',';
                out += std::to_string(col[k].value + 1);
                out += '^';
                out += '(';
                for (std::size_t m = 0; m < col[k].height.size(); ++m) {
                    if (m) out += ',';
                    out += std::to_string(col[k].height[m].v + 1);
                    out += '@';
                    const HeightPos& p = col[k].height[m].p;
                    if (!p.bDef) {
                        out += "\xCF\x89";  // ω
                        if (p.a != 0) out += std::to_string(p.a + 1);
                    } else if (p.a == 0) {
                        out += std::to_string(p.b + 1);
                    } else {
                        out += "\xCF\x89";  // ω
                        if (p.a != 1) out += std::to_string(p.a);
                        out += '+';
                        out += std::to_string(p.b + 1);
                    }
                }
                out += ')';
            }
            out += ')';
        }
        return out;
    }

    [[nodiscard]] static int compare_int(int a, int b) { return a < b ? -1 : a > b ? 1 : 0; }

    // compare_undefined_last：undefined 最大
    [[nodiscard]] static int compare_undef(int aDef, int a, int bDef, int b) {
        if (aDef && bDef) return compare_int(a, b);
        if (aDef) return -1;
        if (bDef) return 1;
        return 0;
    }

    // compare_height_pos = lex_compare([p1,p0], [q1,q0], undefined_last)
    [[nodiscard]] static int compare_hpos(const HeightPos& a, const HeightPos& b) {
        int c = compare_int(a.a, b.a);
        if (c != 0) return c;
        return compare_undef(a.bDef, a.b, b.bDef, b.b);
    }

    [[nodiscard]] static int compare_hentry(const HEntry& a, const HEntry& b) {
        int c = compare_int(a.v, b.v);
        if (c != 0) return c;
        return compare_hpos(a.p, b.p);
    }

    // compare_height = lex(h, pair_cmp, longer_last=false)：较短者大
    [[nodiscard]] static int compare_height(const Height& a, const Height& b) {
        const std::size_t len = a.size() < b.size() ? a.size() : b.size();
        for (std::size_t k = 0; k < len; ++k) {
            int c = compare_hentry(a[k], b[k]);
            if (c != 0) return c;
        }
        return -compare_int(static_cast<int>(a.size()), static_cast<int>(b.size()));
    }

    [[nodiscard]] static bool column_height(const Column& col, Height& out) {
        if (col.empty()) return false;
        out = col.back().height;
        return true;
    }

    [[nodiscard]] static Column filter_height_greater(const Column& col, const Height& h0) {
        Column out;
        for (const Entry& e : col) {
            if (compare_height(e.height, h0) > 0) out.push_back(e);
        }
        return out;
    }

    [[nodiscard]] static Column merge_column(const Column& col1, const Column& col2) {
        Column out = col1;
        Height h;
        if (column_height(col1, h)) {
            const Column filtered = filter_height_greater(col2, h);
            out.insert(out.end(), filtered.begin(), filtered.end());
        } else {
            out.insert(out.end(), col2.begin(), col2.end());
        }
        return out;
    }

    [[nodiscard]] static int copy_value(int value, int r, int offset) {
        return value >= r ? value + offset : value;
    }

    [[nodiscard]] static HeightPos copy_hpos(const HeightPos& p, int r, int offset) {
        return HeightPos{ p.a, p.bDef ? copy_value(p.b, r, offset) : 0, p.bDef };
    }

    [[nodiscard]] static HEntry copy_hentry(const HEntry& he, int r, int offset) {
        return HEntry{ copy_value(he.v, r, offset), copy_hpos(he.p, r, offset) };
    }

    [[nodiscard]] static Height copy_height(const Height& h, int r, int offset) {
        Height out;
        out.reserve(h.size());
        for (const HEntry& he : h) out.push_back(copy_hentry(he, r, offset));
        return out;
    }

    [[nodiscard]] static Entry copy_entry(const Entry& e, int r, int offset) {
        return Entry{ copy_value(e.value, r, offset), copy_height(e.height, r, offset) };
    }

    [[nodiscard]] static Column copy_column(const Column& col, int r, int offset) {
        Column out;
        out.reserve(col.size());
        for (const Entry& e : col) out.push_back(copy_entry(e, r, offset));
        return out;
    }

    // height_fill_dec(h, p, v)：末项 v>=v 时 pop；push [v, p]
    [[nodiscard]] static Height height_fill_dec(const Height& h, const HeightPos& p, int v) {
        Height new_h = h;
        while (!new_h.empty() && new_h.back().v >= v) new_h.pop_back();
        new_h.push_back(HEntry{ v, p });
        return new_h;
    }

    [[nodiscard]] static std::optional<Height> descend_height_to_value(const Expr& expr,
                                                                       const Height& h, int v);
    [[nodiscard]] static std::optional<HeightPos> find_lower_height_pos(const Expr& expr,
                                                                        const Height& h,
                                                                        const HeightPos& p, int r);
    [[nodiscard]] static std::optional<Entry> find_maximal_entry_below(const Expr& expr, int i,
                                                                       const Height& h, int r);

    [[nodiscard]] static std::optional<Height> compute_new_height(const Height& h,
                                                                  const Expr& expr, int r) {
        const HEntry& last = h.back();
        const int v = last.v;
        const HeightPos& p = last.p;
        const std::optional<HeightPos> new_p = find_lower_height_pos(expr, h, p, r);
        if (new_p) return height_fill_dec(h, *new_p, r);
        const std::optional<Entry> e = find_maximal_entry_below(expr, v, h, r);
        if (!e) return std::nullopt;
        return e->height;
    }

    [[nodiscard]] static Expr infinity_fs(int index) {
        Expr result;
        result.emplace_back();
        for (int i = 0; i < index; ++i) {
            Height h;
            h.push_back(HEntry{ i, HeightPos{ i, 0, false } });
            result.push_back(Column{ Entry{ i, std::move(h) } });
        }
        return result;
    }

    [[nodiscard]] static Expr expand(const Expr& expr, int index, bool shorter) {
        if (expr.empty()) return expr;
        const int right = static_cast<int>(expr.size()) - 1;
        if (expr[static_cast<std::size_t>(right)].empty()) {
            return Expr(expr.begin(), expr.end() - 1);
        }
        const int top = static_cast<int>(expr[static_cast<std::size_t>(right)].size()) - 1;
        const Entry& tr = expr[static_cast<std::size_t>(right)][static_cast<std::size_t>(top)];
        const int r = tr.value;

        const std::optional<Height> new_h = compute_new_height(tr.height, expr, r);

        const Column& rc = expr[static_cast<std::size_t>(right)];
        const Column& rc2 = expr[static_cast<std::size_t>(r)];
        Column new_col(rc.begin(), rc.end() - 1);
        if (new_h) {
            Column mid{ Entry{ r, *new_h } };
            new_col = merge_column(new_col, mid);
        }
        new_col = merge_column(new_col, rc2);

        Expr result(expr.begin(), expr.end() - 1);
        result.push_back(std::move(new_col));

        for (int w = 1; w <= index; ++w) {
            for (int i = r + 1; i <= right; ++i) {
                result.push_back(copy_column(result[static_cast<std::size_t>(i)], r,
                                             (right - r) * w));
            }
        }
        if (shorter) result.pop_back();
        return result;
    }

    static std::string expand_text_impl(std::string_view text, int term, bool longer) {
        if (term < 1) term = 1;
        const auto parsed = parse(text);
        if (!parsed) {
            return "解析失败，请确认你没有使用全角括号，逗号，分号";
        }
        if (parsed->infinity) return display(infinity_fs(term));
        const Expr& e = parsed->expr;
        if (e.empty()) return {};
        // 值域校验：value 与行高 v/数字 p0 分量作为列索引，须在 [0, size)
        for (const Column& col : e) {
            for (const Entry& en : col) {
                if (en.value < 0 || en.value >= static_cast<int>(e.size())) {
                    return "解析失败，请确认你没有使用全角括号，逗号，分号";
                }
                if (en.height.empty()) {
                    // 顶层空行高在 TS 中崩溃（h.back() undefined）
                    return "解析失败，请确认你没有使用全角括号，逗号，分号";
                }
                for (const HEntry& he : en.height) {
                    if (he.v < 0 || he.v >= static_cast<int>(e.size()) ||
                        (he.p.bDef && (he.p.b < 0 || he.p.b >= static_cast<int>(e.size())))) {
                        return "解析失败，请确认你没有使用全角括号，逗号，分号";
                    }
                }
            }
        }
        if (e.back().empty()) return display(Expr(e.begin(), e.end() - 1));
        return display(expand(e, term, !longer));
    }
};

// 递归三函数（互相调用，类外定义；成员函数体处于成员作用域，可直接使用私有别名与成员）
inline std::optional<SOmegaSquareDBMSNotation::Height> SOmegaSquareDBMSNotation::descend_height_to_value(
    const SOmegaSquareDBMSNotation::Expr& expr, const SOmegaSquareDBMSNotation::Height& h, int v) {
    std::size_t j = h.size();
    for (std::size_t k = 0; k < h.size(); ++k) {
        if (h[k].v > v) {
            j = k;
            break;
        }
    }
    if (j == h.size()) return h;
    if (j == 0) {
        const int rh = h[0].v;
        Height new_h;
        if (!column_height(expr[static_cast<std::size_t>(rh)], new_h)) return std::nullopt;
        return descend_height_to_value(expr, new_h, v);
    }
    const HeightPos& p_bound = h[j - 1].p;
    const std::optional<HeightPos> new_p = find_lower_height_pos(expr, h, p_bound, v);
    if (!new_p) return std::nullopt;
    return height_fill_dec(h, *new_p, v);
}

inline std::optional<SOmegaSquareDBMSNotation::Entry> SOmegaSquareDBMSNotation::find_maximal_entry_below(
    const SOmegaSquareDBMSNotation::Expr& expr, int i, const SOmegaSquareDBMSNotation::Height& h,
    int r) {
    if (r != i) {
        const std::optional<Entry> e = find_maximal_entry_below(expr, r, h, r);
        if (!e) return std::nullopt;
        return find_maximal_entry_below(expr, i, e->height, e->value);
    }
    const Column& col = expr[static_cast<std::size_t>(i)];
    std::size_t j = col.size();
    for (std::size_t k = 0; k < col.size(); ++k) {
        if (compare_height(col[k].height, h) > 0) {
            j = k;
            break;
        }
    }
    if (j == col.size()) {
        if (col.empty()) return std::nullopt;
        return col.back();
    }
    const Height lower_empty;
    const Height& lower = j == 0 ? lower_empty : col[j - 1].height;
    const bool have_lower = j != 0;
    const int new_value = col[j].value;
    const std::optional<Height> upper = descend_height_to_value(expr, h, new_value);
    if (upper && (!have_lower || compare_height(*upper, lower) > 0)) {
        return Entry{ col[j].value, *upper };
    }
    if (j == 0) return std::nullopt;  // 对应 TS expr[i][-1] = undefined
    return col[j - 1];
}

inline std::optional<SOmegaSquareDBMSNotation::HeightPos>
SOmegaSquareDBMSNotation::find_lower_height_pos(const SOmegaSquareDBMSNotation::Expr& expr,
                                                const SOmegaSquareDBMSNotation::Height& h,
                                                const SOmegaSquareDBMSNotation::HeightPos& p,
                                                int r) {
    if (!p.bDef) return HeightPos{ p.a, r, true };
    const std::optional<HeightPos> default_value =
        p.a == 0 ? std::nullopt : std::optional<HeightPos>(HeightPos{ p.a - 1, 0, false });
    const std::optional<Entry> bound = find_maximal_entry_below(expr, p.b, h, r);
    if (!bound) return default_value;
    const int r1 = bound->value;
    const Height& h1 = bound->height;
    const HEntry& last = h1.back();
    const int v1 = last.v;
    const HeightPos& p1 = last.p;
    if (compare_hpos(p1, p) > 0) return default_value;
    if (p1.a == p.a && p1.bDef && p1.b == v1) return p1;
    return find_lower_height_pos(expr, h1, HeightPos{ p.a, v1, true }, r1);
}

} // namespace omegay::notation

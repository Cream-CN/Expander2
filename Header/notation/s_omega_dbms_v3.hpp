
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

struct SOmegaDbmsV3HEntry {
    int v;
    int p;
};
using SOmegaDbmsV3Height = std::vector<SOmegaDbmsV3HEntry>;

struct SOmegaDbmsV3Entry {
    int value;
    SOmegaDbmsV3Height height;
};
using SOmegaDbmsV3Column = std::vector<SOmegaDbmsV3Entry>;
using SOmegaDbmsV3Expr = std::vector<SOmegaDbmsV3Column>;

struct SOmegaDbmsV3Parsed {
    bool infinity = false;
    SOmegaDbmsV3Expr expr;
};

struct SOmegaDBMSV3Notation {
    static constexpr const char* kName = "SωDBMS v3";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/false);
    }
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::optional<SOmegaDbmsV3Parsed> parse(std::string_view s) {
        if (s == "Limit" || s == "limit" || s == "LIMIT") {
            return SOmegaDbmsV3Parsed{ true, {} };
        }
        std::size_t i = 0;
        auto error = [&]() { return std::optional<SOmegaDbmsV3Parsed>{}; };
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
        // 行高单项 'v@p'：v 减一，p 原样
        auto parse_height_item = [&]() -> std::optional<SOmegaDbmsV3HEntry> {
            auto v = parse_height_value();
            if (!v) return std::nullopt;
            skip_spaces();
            if (i >= s.size() || s[i] != '@') return std::nullopt;
            ++i;
            auto p = parse_number();
            if (!p) return std::nullopt;
            return SOmegaDbmsV3HEntry{ *v, *p };
        };
        auto parse_height = [&]() -> std::optional<SOmegaDbmsV3Height> {
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
            SOmegaDbmsV3Height result;
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
        auto parse_entry = [&]() -> std::optional<SOmegaDbmsV3Entry> {
            auto v = parse_number();
            if (!v) return std::nullopt;
            --*v;
            skip_spaces();
            if (i < s.size() && s[i] == '^') {
                ++i;
                auto h = parse_height();
                if (!h) return std::nullopt;
                return SOmegaDbmsV3Entry{ *v, *h };
            }
            if (i < s.size() && s[i] == '(') {
                auto h = parse_height();
                if (!h) return std::nullopt;
                return SOmegaDbmsV3Entry{ *v, *h };
            }
            return SOmegaDbmsV3Entry{ *v, {} };  // '(0)' 等省略上标
        };
        auto parse_column = [&]() -> std::optional<SOmegaDbmsV3Column> {
            skip_spaces();
            if (i >= s.size() || s[i] != '(') return std::nullopt;
            ++i;
            SOmegaDbmsV3Column entries;
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

        SOmegaDbmsV3Expr result;
        skip_spaces();
        while (i < s.size() && s[i] == '(') {
            auto col = parse_column();
            if (!col) return std::nullopt;
            result.push_back(std::move(*col));
            skip_spaces();
        }
        skip_spaces();
        if (i != s.size()) return std::nullopt;
        return SOmegaDbmsV3Parsed{ false, std::move(result) };
    }

    [[nodiscard]] static std::string to_string(const SOmegaDbmsV3Parsed& p) {
        if (p.infinity) return "Limit";
        return display(p.expr);
    }

private:
    using HEntry = SOmegaDbmsV3HEntry;
    using Height = SOmegaDbmsV3Height;
    using Entry = SOmegaDbmsV3Entry;
    using Column = SOmegaDbmsV3Column;
    using Expr = SOmegaDbmsV3Expr;

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
                    out += std::to_string(col[k].height[m].p);
                }
                out += ')';
            }
            out += ')';
        }
        return out;
    }

    [[nodiscard]] static int compare_int(int a, int b) { return a < b ? -1 : a > b ? 1 : 0; }

    // HeightEntry 比较：[v, p] 字典序
    [[nodiscard]] static int compare_hentry(const HEntry& a, const HEntry& b) {
        int c = compare_int(a.v, b.v);
        if (c != 0) return c;
        return compare_int(a.p, b.p);
    }

    // compare_height = lex_compare(h1, h2, pair_cmp, longer_last=false)：较短者大
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

    [[nodiscard]] static HEntry copy_hentry(const HEntry& he, int r, int offset) {
        return HEntry{ copy_value(he.v, r, offset), he.p };
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

    // height_fill_dec(h, p, v)：末项 p<=p 时 pop；末项 v 相同再 pop；push [v, p]
    [[nodiscard]] static Height height_fill_dec(const Height& h, int p, int v) {
        Height new_h = h;
        while (!new_h.empty() && new_h.back().p <= p) new_h.pop_back();
        if (!new_h.empty() && new_h.back().v == v) new_h.pop_back();
        new_h.push_back(HEntry{ v, p });
        return new_h;
    }

    [[nodiscard]] static std::optional<Height> compute_new_height(const Height& h,
                                                                  const Expr& expr, int r) {
        const HEntry& last = h.back();
        const int v = last.v;
        const int p = last.p;
        if (p > 0) return height_fill_dec(h, p - 1, r);
        if (h.size() == 1) {
            Height out;
            if (!column_height(expr[static_cast<std::size_t>(v)], out)) return std::nullopt;
            return out;
        }
        const int p_bound = h[h.size() - 2].p;
        const Column& col_rh = expr[static_cast<std::size_t>(v)];
        std::size_t hj = col_rh.size();
        for (std::size_t k = 0; k < col_rh.size(); ++k) {
            if (compare_height(h, col_rh[k].height) <= 0) {
                hj = k;
                break;
            }
        }
        if (hj == col_rh.size()) {
            Height out;
            if (!column_height(col_rh, out)) return std::nullopt;
            return out;
        }
        const int new_value = col_rh[hj].value;
        Height new_h = height_fill_dec(h, p_bound - 1, new_value);
        if (hj > 0) {
            const Height& lower = col_rh[hj - 1].height;
            if (compare_height(lower, new_h) > 0) new_h = lower;
        }
        return new_h;
    }

    [[nodiscard]] static Expr infinity_fs(int index) {
        Expr result;
        result.emplace_back();
        Height h;
        h.push_back(HEntry{ 0, index });
        result.push_back(Column{ Entry{ 0, std::move(h) } });
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
        // 值域校验：value 与行高 v 分量作为列索引，须在 [0, size)
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
                    if (he.v < 0 || he.v >= static_cast<int>(e.size())) {
                        return "解析失败，请确认你没有使用全角括号，逗号，分号";
                    }
                }
            }
        }
        if (e.back().empty()) return display(Expr(e.begin(), e.end() - 1));
        return display(expand(e, term, !longer));
    }
};

} // namespace omegay::notation

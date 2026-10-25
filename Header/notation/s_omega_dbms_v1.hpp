
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

using SOmegaDbmsV1Height = std::vector<int>;

struct SOmegaDbmsV1Entry {
    int value;
    SOmegaDbmsV1Height height;
};
using SOmegaDbmsV1Column = std::vector<SOmegaDbmsV1Entry>;
using SOmegaDbmsV1Expr = std::vector<SOmegaDbmsV1Column>;

struct SOmegaDbmsV1Parsed {
    bool infinity = false;
    SOmegaDbmsV1Expr expr;
};

struct SOmegaDBMSV1Notation {
    static constexpr const char* kName = "SωDBMS v1";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/false);
    }
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::optional<SOmegaDbmsV1Parsed> parse(std::string_view s) {
        if (s == "Limit" || s == "limit" || s == "LIMIT") {
            return SOmegaDbmsV1Parsed{ true, {} };
        }
        std::size_t i = 0;
        auto error = [&]() { return std::optional<SOmegaDbmsV1Parsed>{}; };
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
        // 行高 "(s1,s2,…)"：显示时反序 1-based，解析时减一再反序
        auto parse_height = [&]() -> std::optional<SOmegaDbmsV1Height> {
            skip_spaces();
            if (i >= s.size() || s[i] != '(') return std::nullopt;
            ++i;
            SOmegaDbmsV1Height displayed;
            skip_spaces();
            if (i < s.size() && s[i] != ')') {
                auto n = parse_number();
                if (!n) return std::nullopt;
                displayed.push_back(*n);
                skip_spaces();
                while (i < s.size() && s[i] == ',') {
                    ++i;
                    auto n2 = parse_number();
                    if (!n2) return std::nullopt;
                    displayed.push_back(*n2);
                    skip_spaces();
                }
            }
            if (i >= s.size() || s[i] != ')') return std::nullopt;
            ++i;
            SOmegaDbmsV1Height h;
            h.reserve(displayed.size());
            for (auto it = displayed.rbegin(); it != displayed.rend(); ++it) h.push_back(*it - 1);
            return h;
        };
        auto parse_entry = [&]() -> std::optional<SOmegaDbmsV1Entry> {
            auto v = parse_number();
            if (!v) return std::nullopt;
            --*v;
            skip_spaces();
            if (i < s.size() && s[i] == '^') {
                ++i;
                auto h = parse_height();
                if (!h) return std::nullopt;
                return SOmegaDbmsV1Entry{ *v, *h };
            }
            return SOmegaDbmsV1Entry{ *v, {} };  // '(0)' 等省略上标
        };
        auto parse_column = [&]() -> std::optional<SOmegaDbmsV1Column> {
            skip_spaces();
            if (i >= s.size() || s[i] != '(') return std::nullopt;
            ++i;
            SOmegaDbmsV1Column entries;
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

        SOmegaDbmsV1Expr result;
        skip_spaces();
        while (i < s.size() && s[i] == '(') {
            auto col = parse_column();
            if (!col) return std::nullopt;
            result.push_back(std::move(*col));
            skip_spaces();
        }
        skip_spaces();
        if (i != s.size()) return std::nullopt;
        return SOmegaDbmsV1Parsed{ false, std::move(result) };
    }

    [[nodiscard]] static std::string to_string(const SOmegaDbmsV1Parsed& p) {
        if (p.infinity) return "Limit";
        return display(p.expr);
    }

private:
    using Height = SOmegaDbmsV1Height;
    using Entry = SOmegaDbmsV1Entry;
    using Column = SOmegaDbmsV1Column;
    using Expr = SOmegaDbmsV1Expr;

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
                    out += std::to_string(col[k].height[col[k].height.size() - 1 - m] + 1);
                }
                out += ')';
            }
            out += ')';
        }
        return out;
    }

    [[nodiscard]] static int compare_int(int a, int b) { return a < b ? -1 : a > b ? 1 : 0; }

    // anti_lex_compare：长度不等先比长度（短者小）；等长从尾比较
    [[nodiscard]] static int compare_height(const Height& a, const Height& b) {
        if (a.size() != b.size()) return compare_int(static_cast<int>(a.size()),
                                                     static_cast<int>(b.size()));
        for (std::size_t k = a.size(); k-- > 0;) {
            int c = compare_int(a[k], b[k]);
            if (c != 0) return c;
        }
        return 0;
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

    // TS merge_column(col1, col2)：v1 的 height(col1) 对空列返回 []（非 undefined），
    // 故始终执行 filter_height_greater(col2, height(col1))（空高项被过滤掉）。
    [[nodiscard]] static Column merge_column(const Column& col1, const Column& col2) {
        Column out = col1;
        Height h;
        if (!column_height(col1, h)) h = {};   // TS height([]) = []
        const Column filtered = filter_height_greater(col2, h);
        out.insert(out.end(), filtered.begin(), filtered.end());
        return out;
    }

    [[nodiscard]] static int copy_value(int value, int r, int offset) {
        return value >= r ? value + offset : value;
    }

    [[nodiscard]] static Height copy_height(const Height& h, int r, int offset) {
        Height out;
        out.reserve(h.size());
        for (int s : h) out.push_back(copy_value(s, r, offset));
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

    // top_separator(h)：h[0] 的连续前缀长度
    [[nodiscard]] static int top_separator(const Height& h) {
        if (h.empty()) return 0;
        const int rh = h[0];
        int s = 0;
        while (s < static_cast<int>(h.size()) && h[static_cast<std::size_t>(s)] == rh) ++s;
        return s;
    }

    // find_index_below_height：col 中行高 < h 的条目数（二分）
    [[nodiscard]] static std::size_t find_index_below_height(const Column& col, const Height& h) {
        std::size_t lo = 0, hi = col.size();
        while (lo < hi) {
            const std::size_t m = (lo + hi + 1) >> 1;
            if (compare_height(h, col[m - 1].height) > 0) lo = m;
            else hi = m - 1;
        }
        return lo;
    }

    [[nodiscard]] static Height compute_new_height(const Height& h, const Column& col_rh, int r) {
        const int s = top_separator(h);
        Height new_h = h;
        const std::size_t hj = find_index_below_height(col_rh, h);
        if (hj == col_rh.size()) {
            Height h_candidate;
            if (column_height(col_rh, h_candidate)) {
                if (h_candidate.size() < static_cast<std::size_t>(s)) {
                    new_h.pop_back();
                } else {
                    new_h[static_cast<std::size_t>(s - 1)] = h_candidate[static_cast<std::size_t>(s - 1)];
                }
            } else {
                // 空列：h_candidate = []
                if (s > 0) new_h.pop_back();
            }
        } else {
            Height base_ref;
            const bool have_base = hj != 0;
            if (have_base) base_ref = col_rh[hj - 1].height;
            const bool eq =
                have_base && base_ref.size() == h.size() &&
                lex_equal(base_ref, h, static_cast<std::size_t>(s));
            new_h[static_cast<std::size_t>(s - 1)] =
                eq ? base_ref[static_cast<std::size_t>(s - 1)]
                   : col_rh[hj].value;
        }
        for (int k = 0; k < s - 1; ++k) new_h[static_cast<std::size_t>(k)] = r;
        return new_h;
    }

    [[nodiscard]] static bool lex_equal(const Height& a, const Height& b, std::size_t from) {
        for (std::size_t k = from; k < a.size(); ++k) {
            if (a[k] != b[k]) return false;
        }
        return true;
    }

    [[nodiscard]] static Expr infinity_fs(int index) {
        Expr result;
        result.emplace_back();
        Height h(static_cast<std::size_t>(index + 1), 0);
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
        const Height& h = tr.height;
        const int rh = h[0];

        const Height new_h = compute_new_height(h, expr[static_cast<std::size_t>(rh)], r);

        const Column& rc = expr[static_cast<std::size_t>(right)];
        const Column& rc2 = expr[static_cast<std::size_t>(r)];
        Column new_col(rc.begin(), rc.end() - 1);
        Column mid{ Entry{ r, new_h } };
        new_col = merge_column(new_col, mid);
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
        // 值域校验：value 与行高首分量（rh 用作列索引）须在 [0, size)
        for (const Column& col : e) {
            for (const Entry& en : col) {
                if (en.value < 0 || en.value >= static_cast<int>(e.size()) ||
                    en.height.empty() || en.height[0] < 0 ||
                    en.height[0] >= static_cast<int>(e.size())) {
                    return "解析失败，请确认你没有使用全角括号，逗号，分号";
                }
            }
        }
        if (e.back().empty()) return display(Expr(e.begin(), e.end() - 1));
        return display(expand(e, term, !longer));
    }
};

} // namespace omegay::notation

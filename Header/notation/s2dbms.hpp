
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

struct S2DbmsHeight {
    int h0;
    int h1;
};

struct S2DbmsEntry {
    int value;
    S2DbmsHeight height;
};
using S2DbmsColumn = std::vector<S2DbmsEntry>;
using S2DbmsExpr = std::vector<S2DbmsColumn>;

struct S2DbmsParsed {
    bool infinity = false;
    S2DbmsExpr expr;
};

struct S2DBMSNotation {
    static constexpr const char* kName = "S2DBMS";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/false);
    }
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::optional<S2DbmsParsed> parse(std::string_view s) {
        if (s == "Limit" || s == "limit" || s == "LIMIT") {
            return S2DbmsParsed{ true, {} };
        }
        std::size_t i = 0;
        auto error = [&]() { return std::optional<S2DbmsParsed>{}; };
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
        auto parse_height = [&]() -> std::optional<S2DbmsHeight> {
            skip_spaces();
            if (i >= s.size() || s[i] != '(') return std::nullopt;
            ++i;
            auto h1 = parse_number();
            if (!h1) return std::nullopt;
            skip_spaces();
            if (i >= s.size() || s[i] != ',') return std::nullopt;
            ++i;
            auto h0 = parse_number();
            if (!h0) return std::nullopt;
            --*h0;
            skip_spaces();
            if (i >= s.size() || s[i] != ')') return std::nullopt;
            ++i;
            return S2DbmsHeight{ *h0, *h1 };
        };
        auto parse_entry = [&]() -> std::optional<S2DbmsEntry> {
            auto v = parse_number();
            if (!v) return std::nullopt;
            --*v;
            skip_spaces();
            if (i < s.size() && s[i] == '^') {
                ++i;
                auto h = parse_height();
                if (!h) return std::nullopt;
                return S2DbmsEntry{ *v, *h };
            }
            if (i < s.size() && s[i] == '(') {
                auto h = parse_height();
                if (!h) return std::nullopt;
                return S2DbmsEntry{ *v, *h };
            }
            // '(0)' 等省略上标：哨兵行高 [0,-1]，v==-1 时列尾删去
            return S2DbmsEntry{ *v, S2DbmsHeight{ 0, -1 } };
        };
        auto parse_column = [&]() -> std::optional<S2DbmsColumn> {
            skip_spaces();
            if (i >= s.size() || s[i] != '(') return std::nullopt;
            ++i;
            S2DbmsColumn entries;
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

        S2DbmsExpr result;
        skip_spaces();
        while (i < s.size() && s[i] == '(') {
            auto col = parse_column();
            if (!col) return std::nullopt;
            result.push_back(std::move(*col));
            skip_spaces();
        }
        skip_spaces();
        if (i != s.size()) return std::nullopt;
        return S2DbmsParsed{ false, std::move(result) };
    }

    [[nodiscard]] static std::string to_string(const S2DbmsParsed& p) {
        if (p.infinity) return "Limit";
        return display(p.expr);
    }

private:
    using Height = S2DbmsHeight;
    using Entry = S2DbmsEntry;
    using Column = S2DbmsColumn;
    using Expr = S2DbmsExpr;

    [[nodiscard]] static int compare_int(int a, int b) { return a < b ? -1 : a > b ? 1 : 0; }

    // anti_lex_compare：从尾部比较（先 h1 后 h0）
    [[nodiscard]] static int compare_height(const Height& a, const Height& b) {
        int c = compare_int(a.h1, b.h1);
        if (c != 0) return c;
        return compare_int(a.h0, b.h0);
    }

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
                out += std::to_string(col[k].height.h1);
                out += ',';
                out += std::to_string(col[k].height.h0 + 1);
                out += ')';
            }
            out += ')';
        }
        return out;
    }

    // height(col)：末项行高；空列返回 false
    [[nodiscard]] static bool column_height(const Column& col, Height& out) {
        if (col.empty()) return false;
        out = col.back().height;
        return true;
    }

    [[nodiscard]] static int height1(const Column& col) {
        Height h;
        return column_height(col, h) ? h.h1 : 0;
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

    [[nodiscard]] static Height copy_height(const Height& h, int r, int offset) {
        return Height{ copy_value(h.h0, r, offset), h.h1 };
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

    [[nodiscard]] static Expr infinity_fs(int index) {
        Expr result;
        result.emplace_back();
        result.push_back(Column{ Entry{ 0, Height{ 0, index } } });
        return result;
    }

    [[nodiscard]] static std::optional<Height> compute_new_height(const Height& h,
                                                                  const Expr& expr, int r) {
        const int v = h.h0;
        const int h1 = h.h1;
        const Column& col_rh = expr[static_cast<std::size_t>(v)];
        // hj = 第一个 compare_height(h, hv) <= 0 的下标
        int hj = -1;
        for (std::size_t k = 0; k < col_rh.size(); ++k) {
            if (compare_height(h, col_rh[k].height) <= 0) {
                hj = static_cast<int>(k);
                break;
            }
        }
        if (hj == -1) {
            if (height1(col_rh) < h1) return Height{ r, h1 - 1 };
            Height out;
            if (!column_height(col_rh, out)) return std::nullopt;
            return out;
        }
        const int new_value = col_rh[static_cast<std::size_t>(hj)].value;
        Height new_h{ new_value, h1 };
        if (hj > 0) {
            const Height& lower = col_rh[static_cast<std::size_t>(hj - 1)].height;
            if (compare_height(lower, new_h) > 0) new_h = lower;
        }
        return new_h;
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

        const std::optional<Height> new_h = compute_new_height(h, expr, r);

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
        // 值域校验：value 与行高 h0 作为列索引使用，越界时 TS 在 expand 中抛错
        for (const Column& col : e) {
            for (const Entry& en : col) {
                if (en.value < 0 || en.value >= static_cast<int>(e.size()) ||
                    en.height.h0 < 0 || en.height.h0 >= static_cast<int>(e.size())) {
                    return "解析失败，请确认你没有使用全角括号，逗号，分号";
                }
            }
        }
        if (e.back().empty()) return display(Expr(e.begin(), e.end() - 1));
        return display(expand(e, term, !longer));
    }
};

} // namespace omegay::notation

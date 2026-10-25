
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

struct S1DbmsEntry {
    int value;
    int height;
};
using S1DbmsColumn = std::vector<S1DbmsEntry>;
using S1DbmsExpr = std::vector<S1DbmsColumn>;

struct S1DbmsParsed {
    bool infinity = false;
    S1DbmsExpr expr;
};

struct S1DBMSNotation {
    static constexpr const char* kName = "S1DBMS";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/false);
    }
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::optional<S1DbmsParsed> parse(std::string_view s) {
        if (s == "Limit" || s == "limit" || s == "LIMIT") {
            return S1DbmsParsed{ true, {} };
        }
        std::size_t i = 0;
        auto error = [&]() { return std::optional<S1DbmsParsed>{}; };
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
        auto parse_entry = [&]() -> std::optional<S1DbmsEntry> {
            auto x = parse_number();
            if (!x) return std::nullopt;
            --*x;
            skip_spaces();
            if (i < s.size() && s[i] == '^') {
                ++i;
                auto y = parse_number();
                if (!y) return std::nullopt;
                return S1DbmsEntry{ *x, *y - 1 };
            }
            return S1DbmsEntry{ *x, -1 };  // '(0)' 等省略上标，稍后作为空列处理
        };
        auto parse_column = [&]() -> std::optional<S1DbmsColumn> {
            skip_spaces();
            if (i >= s.size() || s[i] != '(') return std::nullopt;
            ++i;
            S1DbmsColumn entries;
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

        S1DbmsExpr result;
        skip_spaces();
        while (i < s.size() && s[i] == '(') {
            auto col = parse_column();
            if (!col) return std::nullopt;
            result.push_back(std::move(*col));
            skip_spaces();
        }
        skip_spaces();
        if (i != s.size()) return std::nullopt;
        return S1DbmsParsed{ false, std::move(result) };
    }

    [[nodiscard]] static std::string to_string(const S1DbmsParsed& p) {
        if (p.infinity) return "Limit";
        return display(p.expr);
    }

private:
    using Entry = S1DbmsEntry;
    using Column = S1DbmsColumn;
    using Expr = S1DbmsExpr;

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
                out += std::to_string(col[k].height + 1);
            }
            out += ')';
        }
        return out;
    }

    [[nodiscard]] static int height(const Column& col) {
        return col.empty() ? -1 : col.back().height;
    }

    [[nodiscard]] static Column filter_height_greater(const Column& col, int h0) {
        Column out;
        for (const Entry& e : col) {
            if (e.height > h0) out.push_back(e);
        }
        return out;
    }

    // TS merge_column(col1, col2)：S1 的 height(col1) 对空列取 -1（非 undefined），
    // 故始终执行 filter_height_greater(col2, height(col1))（h > -1 过滤）。
    [[nodiscard]] static Column merge_column(const Column& col1, const Column& col2) {
        Column out = col1;
        const Column filtered = filter_height_greater(col2, height(col1));
        out.insert(out.end(), filtered.begin(), filtered.end());
        return out;
    }

    [[nodiscard]] static Entry copy_entry(const Entry& e, int r, int offset) {
        return Entry{ e.value >= r ? e.value + offset : e.value,
                      e.height >= r ? e.height + offset : e.height };
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
        for (int i = 0; i < index; ++i) result.push_back(Column{ Entry{ i, i } });
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
        const int h = tr.height;

        const int new_h = height(expr[static_cast<std::size_t>(h)]);

        Expr result(expr.begin(), expr.end() - 1);
        {
            const Column& rc = expr[static_cast<std::size_t>(right)];
            const Column& rc2 = expr[static_cast<std::size_t>(r)];
            Column new_col(rc.begin(), rc.end() - 1);
            Column mid{ Entry{ r, new_h } };
            new_col = merge_column(new_col, mid);
            new_col = merge_column(new_col, rc2);
            result.push_back(std::move(new_col));
        }

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
        for (const Column& col : e) {
            for (const Entry& en : col) {
                if (en.value < 0 || en.value >= static_cast<int>(e.size())) {
                    return "解析失败，请确认你没有使用全角括号，逗号，分号";
                }
            }
        }
        if (!e.back().empty()) {
            const int th = e.back().back().height;
            if (th < 0 || th >= static_cast<int>(e.size())) {
                return "解析失败，请确认你没有使用全角括号，逗号，分号";
            }
        }
        if (e.back().empty()) return display(Expr(e.begin(), e.end() - 1));
        return display(expand(e, term, !longer));
    }
};

}  // namespace omegay::notation

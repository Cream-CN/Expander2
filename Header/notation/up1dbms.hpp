// Header/notation/up1dbms.hpp
// UP1DBMS 记号（display 名 "UP1DBMS"，TS id "up1dbms"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/UPMN/UP1DBMS.ts
// （家族/类别：category-upmn-test）。
// 表达式结构（TS）：Entry=[value(0-based), height(0-based)]；Column=Entry[]；
// Expr=Column[]。显示 v+1 ^ h+1；"(0)" 表示空列。∞ 用 bool 标志表示。
#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

struct Up1DbmsEntry {
    int value;  // 0-based
    int height; // 0-based
};
using Up1DbmsColumn = std::vector<Up1DbmsEntry>;
using Up1DbmsExpr = std::vector<Up1DbmsColumn>;

struct Up1DbmsParsed {
    bool infinity = false;
    Up1DbmsExpr expr;
};

struct UP1DBMSNotation {
    static constexpr const char* kName = "UP1DBMS";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/false);
    }
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::optional<Up1DbmsParsed> parse(std::string_view s) {
        if (s == "Limit" || s == "limit" || s == "LIMIT") {
            return Up1DbmsParsed{ true, {} };
        }
        std::size_t i = 0;
        auto error = [&]() { return std::optional<Up1DbmsParsed>{}; };
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
        auto parse_entry = [&]() -> std::optional<Up1DbmsEntry> {
            auto x = parse_number();
            if (!x) return std::nullopt;
            --*x;
            skip_spaces();
            if (i < s.size() && s[i] == '^') {
                ++i;
                auto y = parse_number();
                if (!y) return std::nullopt;
                return Up1DbmsEntry{ *x, *y - 1 };
            }
            return Up1DbmsEntry{ *x, -1 };  // '(0)' 等省略上标，稍后作为空列处理
        };
        auto parse_column = [&]() -> std::optional<Up1DbmsColumn> {
            skip_spaces();
            if (i >= s.size() || s[i] != '(') return std::nullopt;
            ++i;
            Up1DbmsColumn entries;
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

        Up1DbmsExpr result;
        skip_spaces();
        while (i < s.size() && s[i] == '(') {
            auto col = parse_column();
            if (!col) return std::nullopt;
            result.push_back(std::move(*col));
            skip_spaces();
        }
        skip_spaces();
        if (i != s.size()) return std::nullopt;
        return Up1DbmsParsed{ false, std::move(result) };
    }

    [[nodiscard]] static std::string to_string(const Up1DbmsParsed& p) {
        if (p.infinity) return "Limit";
        return display(p.expr);
    }

private:
    using Entry = Up1DbmsEntry;
    using Column = Up1DbmsColumn;
    using Expr = Up1DbmsExpr;

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

    [[nodiscard]] static Column merge_column(const Column& col1, const Column& col2) {
        Column out = col1;
        const Column filtered = filter_height_greater(col2, height(col1));
        out.insert(out.end(), filtered.begin(), filtered.end());
        return out;
    }

    [[nodiscard]] static int copy_value(int value, int r, int offset, bool up) {
        return value > r || (value == r && up) ? value + offset : value;
    }

    [[nodiscard]] static Entry copy_entry(const Entry& e, int r, int offset, bool up) {
        return Entry{ copy_value(e.value, r, offset, up), copy_value(e.height, r, offset, up) };
    }

    [[nodiscard]] static Column copy_column(const Column& col, int r, int offset, bool up) {
        Column out;
        out.reserve(col.size());
        for (const Entry& e : col) out.push_back(copy_entry(e, r, offset, up));
        return out;
    }

    [[nodiscard]] static int height_after(const Column& col, int r) {
        for (std::size_t j = col.size(); j-- > 0;) {
            if (col[j].value >= r) return col[j].height;
        }
        return -1;
    }

    [[nodiscard]] static int parent_at(const Column& col, int h) {
        for (std::size_t j = 0; j < col.size(); ++j) {
            if (col[j].height >= h) return col[j].value;
        }
        return -1;
    }

    [[nodiscard]] static Column discard_after(const Column& col, int r) {
        Column out;
        for (const Entry& e : col) {
            if (e.value <= r) out.push_back(e);
        }
        return out;
    }

    [[nodiscard]] static std::vector<int> compute_up(const Expr& expr, int r, int h) {
        const int right = static_cast<int>(expr.size()) - 1;
        std::vector<int> result(expr.size(), -1);
        for (int k = 0; k < r; ++k) result[static_cast<std::size_t>(k)] = 0;
        result[static_cast<std::size_t>(r)] = 1;
        for (int i = r + 1; i < static_cast<int>(expr.size()); ++i) {
            const Column& col = expr[static_cast<std::size_t>(i)];

            const int h_root = height_after(col, r);
            if (h_root < h) {
                result[static_cast<std::size_t>(i)] = 0;
                continue;
            }
            const int h_proj = height_after(col, r + 1);
            if (h_proj >= h) {
                const int pa = parent_at(col, h);
                result[static_cast<std::size_t>(i)] = pa < 0 ? 0 : result[static_cast<std::size_t>(pa)];
                continue;
            }
            if (h_proj >= 0) {
                int p_proj = i;
                for (int guard = 0; guard < static_cast<int>(expr.size()) + 1; ++guard) {
                    const int next = parent_at(expr[static_cast<std::size_t>(p_proj)], h_proj);
                    if (next == r) break;
                    p_proj = next;
                    if (p_proj < 0) break;
                }
                if (p_proj < 0) {
                    result[static_cast<std::size_t>(i)] = 0;
                    continue;
                }
                if (!result[static_cast<std::size_t>(p_proj)]) {
                    result[static_cast<std::size_t>(i)] = 0;
                    continue;
                }
            }
            const int X_start = i;
            int Y_start = right;
            {
                for (int guard = 0; guard < static_cast<int>(expr.size()) + 1; ++guard) {
                    const int next = parent_at(expr[static_cast<std::size_t>(Y_start)], h_proj + 1);
                    if (next == r) break;
                    Y_start = next;
                    if (Y_start < 0) break;
                }
            }
            if (Y_start < 0) {
                result[static_cast<std::size_t>(i)] = 0;
                continue;
            }
            if (Y_start <= X_start) {
                result[static_cast<std::size_t>(i)] = X_start == Y_start ? 1 : 0;
                continue;
            }
            const Column X0 = discard_after(expr[static_cast<std::size_t>(X_start)], r);
            const Column Y0 = discard_after(expr[static_cast<std::size_t>(Y_start)], r);
            const std::size_t n0 = X0.size() < Y0.size() ? X0.size() : Y0.size();
            int cmp_0 = 0;
            for (std::size_t k = 0; k < n0; ++k) {
                if (X0[k].value != Y0[k].value) {
                    cmp_0 = X0[k].value < Y0[k].value ? -1 : 1;
                    break;
                }
                if (X0[k].height != Y0[k].height) {
                    cmp_0 = X0[k].height < Y0[k].height ? -1 : 1;
                    break;
                }
            }
            if (cmp_0 == 0) cmp_0 = X0.size() < Y0.size() ? -1 : X0.size() > Y0.size() ? 1 : 0;
            if (cmp_0 != 0) {
                result[static_cast<std::size_t>(i)] = cmp_0 > 0 ? 1 : 0;
                continue;
            }
            bool decided = false;
            for (int k = 1; Y_start + k < static_cast<int>(expr.size()); ++k) {
                std::vector<std::array<int, 4>> Xk, Yk;
                for (const Entry& e : expr[static_cast<std::size_t>(X_start + k)]) {
                    Xk.push_back({ e.value >= X_start, e.value >= X_start ? e.value - X_start : e.value,
                                   e.height >= X_start, e.height >= X_start ? e.height - X_start : e.height });
                }
                for (const Entry& e : expr[static_cast<std::size_t>(Y_start + k)]) {
                    Yk.push_back({ e.value >= Y_start, e.value >= Y_start ? e.value - Y_start : e.value,
                                   e.height >= Y_start, e.height >= Y_start ? e.height - Y_start : e.height });
                }
                const std::size_t nk = Xk.size() < Yk.size() ? Xk.size() : Yk.size();
                int cmp = 0;
                for (std::size_t t = 0; t < nk; ++t) {
                    const auto& a = Xk[t];
                    const auto& b = Yk[t];
                    for (int f = 0; f < 4; ++f) {
                        if (a[static_cast<std::size_t>(f)] != b[static_cast<std::size_t>(f)]) {
                            cmp = f % 2 == 0
                                      ? (a[static_cast<std::size_t>(f)] ? 1 : -1)
                                      : (a[static_cast<std::size_t>(f)] < b[static_cast<std::size_t>(f)] ? -1 : 1);
                            break;
                        }
                    }
                    if (cmp != 0) break;
                }
                if (cmp == 0) cmp = Xk.size() < Yk.size() ? -1 : Xk.size() > Yk.size() ? 1 : 0;
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

        const std::vector<int> up = compute_up(expr, r, h);
        for (int w = 1; w <= index; ++w) {
            for (int i = r + 1; i <= right; ++i) {
                result.push_back(copy_column(result[static_cast<std::size_t>(i)], r,
                                             (right - r) * w, up[static_cast<std::size_t>(i)] != 0));
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
        // 值域校验：value 与顶层 height 作为列索引使用，越界时 TS 在 expand 中抛错。
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

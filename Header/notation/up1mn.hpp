// Header/notation/up1mn.hpp
// UP1MN 记号（display 名 "UP1MN"，TS id "up1mn"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/UPMN/UP1MN.ts
// （家族/类别：category-upmn，credit.up1mn）。
// 表达式结构（TS）：Entry=number（0-based）；Column=Entry[]；Expr=Column[]。
// 显示/解析经 n-MN（smnshare）转换（value+1, sep=0；解析要求 sep 全为 0）。
// ∞ 用 bool 标志表示（display "Limit"）。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "smnshare.hpp"

namespace omegay::notation {

using Up1MnExpr = std::vector<std::vector<int>>;

struct UP1MNNotation {
    static constexpr const char* kName = "UP1MN";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/false);
    }
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    // parse：∞ 不在此处理（"Limit" 走 expand_text_impl）；解析失败返回 nullopt。
    [[nodiscard]] static std::optional<Up1MnExpr> parse(std::string_view text) {
        auto m = smnshare::from_display(text);
        if (!m) return std::nullopt;
        for (const auto& col : *m) {
            for (const auto& e : col) {
                if (e.sep != 0) return std::nullopt;  // TS from_nMN 抛错
            }
        }
        Up1MnExpr e;
        e.reserve(m->size());
        for (const auto& col : *m) {
            std::vector<int> nc;
            nc.reserve(col.size());
            for (const auto& en : col) nc.push_back(en.value - 1);
            e.push_back(std::move(nc));
        }
        return e;
    }

    [[nodiscard]] static std::string to_string(const Up1MnExpr& e) {
        smnshare::Mountain m;
        m.reserve(e.size());
        for (const auto& col : e) {
            smnshare::Column mc;
            mc.reserve(col.size());
            for (int v : col) mc.push_back(smnshare::Entry{ v + 1, 0 });
            m.push_back(std::move(mc));
        }
        return smnshare::mountain_display(m, false);
    }

private:
    using Expr = Up1MnExpr;

    // TS infinity_FS(index) = [[], [0]×(index+1)]。
    [[nodiscard]] static Expr infinity_fs(int index) {
        Expr result;
        result.emplace_back();
        result.emplace_back(static_cast<std::size_t>(index < 0 ? 0 : index + 1), 0);
        return result;
    }

    [[nodiscard]] static std::vector<int> compute_up(const Expr& expr, int r, int b) {
        const int right = static_cast<int>(expr.size()) - 1;
        std::vector<int> result(expr.size(), -1);
        for (int k = 0; k < r; ++k) result[static_cast<std::size_t>(k)] = 0;
        result[static_cast<std::size_t>(r)] = 1;
        for (int i = r + 1; i < static_cast<int>(expr.size()); ++i) {
            const std::vector<int>& col = expr[static_cast<std::size_t>(i)];
            if (static_cast<int>(col.size()) <= b) {
                result[static_cast<std::size_t>(i)] = 0;
                continue;
            }
            if (col[static_cast<std::size_t>(b)] != r) {
                result[static_cast<std::size_t>(i)] =
                    result[static_cast<std::size_t>(col[static_cast<std::size_t>(b)])];
                continue;
            }
            int j = -1;
            for (std::size_t k = 0; k < col.size(); ++k) {
                if (col[k] == r) {
                    j = static_cast<int>(k);
                    break;
                }
            }
            if (j < 0) {  // TS findIndex 返回 -1 后会死循环；安全兜底。
                result[static_cast<std::size_t>(i)] = 0;
                continue;
            }
            if (j > 0) {
                int p = i;
                while (expr[static_cast<std::size_t>(p)][static_cast<std::size_t>(j - 1)] != r) {
                    p = expr[static_cast<std::size_t>(p)][static_cast<std::size_t>(j - 1)];
                }
                if (!result[static_cast<std::size_t>(p)]) {
                    result[static_cast<std::size_t>(i)] = 0;
                    continue;
                }
            }
            const int X_start = i;
            int Y_start = right;
            while (expr[static_cast<std::size_t>(Y_start)][static_cast<std::size_t>(j)] != r) {
                Y_start = expr[static_cast<std::size_t>(Y_start)][static_cast<std::size_t>(j)];
            }
            if (Y_start <= X_start) {
                result[static_cast<std::size_t>(i)] = X_start == Y_start ? 1 : 0;
                continue;
            }
            const std::vector<int> X0(expr[static_cast<std::size_t>(X_start)].begin() + j,
                                      expr[static_cast<std::size_t>(X_start)].end());
            const std::vector<int> Y0(expr[static_cast<std::size_t>(Y_start)].begin() + j,
                                      expr[static_cast<std::size_t>(Y_start)].end());
            const std::size_t n0 = X0.size() < Y0.size() ? X0.size() : Y0.size();
            int cmp_0 = 0;
            for (std::size_t k = 0; k < n0; ++k) {
                if (X0[k] != Y0[k]) {
                    cmp_0 = X0[k] < Y0[k] ? -1 : 1;
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
                std::vector<std::pair<bool, int>> Xk, Yk;
                for (int v : expr[static_cast<std::size_t>(X_start + k)]) {
                    Xk.emplace_back(v >= X_start, v >= X_start ? v - X_start : v);
                }
                for (int v : expr[static_cast<std::size_t>(Y_start + k)]) {
                    Yk.emplace_back(v >= Y_start, v >= Y_start ? v - Y_start : v);
                }
                const std::size_t nk = Xk.size() < Yk.size() ? Xk.size() : Yk.size();
                int cmp = 0;
                for (std::size_t t = 0; t < nk; ++t) {
                    const auto& a = Xk[t];
                    const auto& b = Yk[t];
                    if (a.first != b.first) {
                        cmp = a.first ? 1 : -1;
                        break;
                    }
                    if (a.second != b.second) {
                        cmp = a.second < b.second ? -1 : 1;
                        break;
                    }
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

    [[nodiscard]] static std::vector<int> copy_column(const std::vector<int>& col, int r, int offset, bool up) {
        std::vector<int> result;
        result.reserve(col.size());
        for (int v : col) {
            result.push_back(v > r ? v + offset : v < r ? v : up ? v + offset : v);
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
        const int r = expr[static_cast<std::size_t>(right)][static_cast<std::size_t>(top)];

        const std::vector<int> up_list = compute_up(expr, r, top);

        Expr result(expr.begin(), expr.end() - 1);
        {
            const std::vector<int>& rc = expr[static_cast<std::size_t>(right)];
            const std::vector<int>& ri = expr[static_cast<std::size_t>(r)];
            std::vector<int> new_col(rc.begin(), rc.end() - 1);
            new_col.insert(new_col.end(), ri.begin() + top, ri.end());
            result.push_back(std::move(new_col));
        }
        for (int w = 1; w <= index; ++w) {
            for (int i = r + 1; i <= right; ++i) {
                result.push_back(copy_column(result[static_cast<std::size_t>(i)], r,
                                             (right - r) * w, up_list[static_cast<std::size_t>(i)] != 0));
            }
        }
        if (shorter) result.pop_back();
        return result;
    }

    static std::string expand_text_impl(std::string_view text, int term, bool longer) {
        if (term < 1) term = 1;
        const bool is_inf = text == "Limit" || text == "limit" || text == "LIMIT";
        if (is_inf) return to_string(infinity_fs(term));
        auto parsed = parse(text);
        if (!parsed) {
            return "解析失败，请确认你没有使用全角括号，逗号，分号";
        }
        const Expr& e = *parsed;
        if (e.empty()) return {};
        // 值域校验：value 作为列索引使用，越界时 TS 在 expand 中抛错。
        for (const auto& col : e) {
            for (int v : col) {
                if (v < 0 || v >= static_cast<int>(e.size())) {
                    return "解析失败，请确认你没有使用全角括号，逗号，分号";
                }
            }
        }
        if (e.back().empty()) return to_string(Expr(e.begin(), e.end() - 1));
        return to_string(expand(e, term, !longer));
    }
};

}  // namespace omegay::notation

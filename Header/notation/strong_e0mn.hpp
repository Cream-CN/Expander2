// Header/notation/strong_e0mn.hpp
// strong e0MN 记号（display 名 "strong e0MN"，TS id "strong-e0mn"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/strong_e0MN.ts
// （家族/类别：category-ta0-mn，credit.e0mn；原作者 test_alpha0）。
// 与 e0MN 共享 mnordshare.hpp 核心；仅三处实质差异（见 mnordshare.hpp 头注）：
//   1. shiftOrdNumbers：strong 版把指数项也递归移位，判据为 coeff >= cutoff；
//   2. down 的 omit 判据：ordEq(xnL, last[n-2].x)（相等）而非 <= 0；
//   3. shortExpansion 的 omit 判据：ordEq(xnL, prev) 而非 <= 0。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mnordshare.hpp"

namespace omegay::notation {

// Expr：非 ∞ 时持有 Expr；∞（显示 "Limit"）用标志表示。
struct StrongE0MnExpr {
    bool infinity = false;
    mnordshare::Expr expr;
};

struct StrongE0MNNotation {
    static constexpr const char* kName = "strong e0MN";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/false);
    }
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<StrongE0MnExpr> parse(std::string_view text) {
        auto parsed = mnordshare::parse_expr(text);
        if (!parsed) return std::nullopt;
        return StrongE0MnExpr{ parsed->infinity, std::move(parsed->expr) };
    }

    [[nodiscard]] static std::string to_string(const StrongE0MnExpr& e) {
        if (e.infinity) return mnordshare::infinity_display();
        return mnordshare::expr_to_plain(e.expr);
    }

private:
    static std::string expand_text_impl(std::string_view text, int term, bool longer) {
        if (term < 1) term = 1;
        const auto parsed = parse(text);
        if (!parsed) {
            return "解析失败，请确认你没有使用全角括号，逗号，分号";
        }
        if (parsed->infinity) {
            return mnordshare::expr_to_plain(mnordshare::infinity_fs(term));
        }
        const mnordshare::Expr& e = parsed->expr;
        if (e.empty()) return {};
        if (!mnordshare::is_limit_expr(e)) {
            return mnordshare::expr_to_plain(mnordshare::predecessor(e));
        }
        return mnordshare::expr_to_plain(mnordshare::fs(e, term, !longer, /*strong=*/true));
    }
};

} // namespace omegay::notation

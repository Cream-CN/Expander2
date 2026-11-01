
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mnordshare.hpp"

namespace omegay::notation {

// Expr：非 ∞ 时持有 Expr；∞（显示 "Limit"）用标志表示。
struct E0MnExpr {
    bool infinity = false;
    mnordshare::Expr expr;
};

struct E0MNNotation {
    static constexpr const char* kName = "e0MN";

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
    [[nodiscard]] static std::optional<E0MnExpr> parse(std::string_view text) {
        auto parsed = mnordshare::parse_expr(text);
        if (!parsed) return std::nullopt;
        return E0MnExpr{ parsed->infinity, std::move(parsed->expr) };
    }

    [[nodiscard]] static std::string to_string(const E0MnExpr& e) {
        if (e.infinity) return mnordshare::infinity_display();
        return mnordshare::expr_to_plain(e.expr);
    }

    // 非 ∞ 表达式的比较（供测试用）。
    [[nodiscard]] static int compare(const E0MnExpr& a, const E0MnExpr& b) {
        if (a.infinity || b.infinity) {
            return a.infinity == b.infinity ? 0 : (a.infinity ? 1 : -1);
        }
        return mnordshare::expr_compare(a.expr, b.expr);
    }

private:
    // TS MN_FS_variants(...).FS / .FS_alter：FS 用 expand(shorter=true)，
    // FS_alter 用 expand(shorter=false)。纯函数语义，省略 display 键缓存。
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
        return mnordshare::expr_to_plain(mnordshare::fs(e, term, !longer, /*strong=*/false));
    }
};

} // namespace omegay::notation

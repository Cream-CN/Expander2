// Header/notation/e0mn.hpp
// e0MN 记号（display 名 "e0MN"，TS id "e0mn"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/e0MN.ts
// （家族/类别：category-ta0-mn，credit.e0mn；原作者 test_alpha0）。
// 表达式结构（TS）：Term={exp:Ord, coeff:int}；Ord=Term[]（[]=0）；
// Entry={a:int, x:Ord}；Column=Entry[]；Expr=Column[]；∞ 用标志表示
// （display 为 "Limit"）。
// 共享核心见 mnordshare.hpp（strong e0MN 同族复用）。
// 对外文本接口：expand_text = FS（较短）、expand_text_alter = FS_alter（较长），
// 与 UI 的 NotationEntry::expand_text / expand_text_alter 槽位对接。
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

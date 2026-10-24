// Header/notation/up2mn_v1b.hpp
// UP2MN v1B 记号（display 名 "UP2MN v1B"，TS id "up2mn-v1b"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/UPMN/UP2MN-v1b.ts
// （家族/类别：category-upmn-test，credit.upmn）。
// 表达式结构（TS）：Entry=[value(0-based), sep]；Column=Entry[]；Expr=Column[]。
// 显示/解析复用 mnup2share（n-MN 转换）。∞ 用 bool 标志表示。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mnup2share.hpp"

namespace omegay::notation {

struct Up2MnV1bExpr {
    bool infinity = false;
    mnup2::Expr expr;
};

struct UP2MNV1BNotation {
    static constexpr const char* kName = "UP2MN v1B";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/false);
    }
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::optional<Up2MnV1bExpr> parse(std::string_view text) {
        if (text == "Limit" || text == "limit" || text == "LIMIT") {
            return Up2MnV1bExpr{ true, {} };
        }
        auto e = mnup2::from_display(text);
        if (!e) return std::nullopt;
        return Up2MnV1bExpr{ false, std::move(*e) };
    }

    [[nodiscard]] static std::string to_string(const Up2MnV1bExpr& e) {
        if (e.infinity) return "Limit";
        return mnup2::display(e.expr);
    }

private:
    static std::string expand_text_impl(std::string_view text, int term, bool longer) {
        if (term < 1) term = 1;
        const auto parsed = parse(text);
        if (!parsed) {
            return "解析失败，请确认你没有使用全角括号，逗号，分号";
        }
        if (parsed->infinity) return mnup2::display(mnup2::up2mn_infinity_fs(term, /*plus=*/false));
        const mnup2::Expr& e = parsed->expr;
        if (e.empty()) return {};
        if (!mnup2::expr_valid(e)) {
            return "解析失败，请确认你没有使用全角括号，逗号，分号";
        }
        if (e.back().empty()) return mnup2::display(mnup2::Expr(e.begin(), e.end() - 1));
        return mnup2::display(mnup2::up2mn_expand(e, term, !longer, /*plus=*/false));
    }
};

}  // namespace omegay::notation

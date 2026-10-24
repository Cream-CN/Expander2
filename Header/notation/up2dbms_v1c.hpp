// Header/notation/up2dbms_v1c.hpp
// UP2DBMS v1C 记号（display 名 "UP2DBMS v1C"，TS id "up2dbms-v1c"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/UPMN/UP2DBMS-v1c.ts
// （家族/类别：category-upmn，credit.upmn）。
// 表达式结构（TS）：Entry=[value(0-based), sep]；Column=Entry[]；Expr=Column[]。
// 共享核心见 mnup2share.hpp（与 v1b+ 唯一差异：compute_up_2mn 的 while 循环
// 用 Rj 索引且追加 P[Y][Rj][0] !== Ri 条件，即 v1c=true）。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mnup2share.hpp"

namespace omegay::notation {

struct Up2DbmsV1cExpr {
    bool infinity = false;
    mnup2::Expr expr;
};

struct UP2DBMSV1CNotation {
    static constexpr const char* kName = "UP2DBMS v1C";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/false);
    }
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::optional<Up2DbmsV1cExpr> parse(std::string_view text) {
        if (text == "Limit" || text == "limit" || text == "LIMIT") {
            return Up2DbmsV1cExpr{ true, {} };
        }
        auto e = mnup2::from_display(text);
        if (!e) return std::nullopt;
        return Up2DbmsV1cExpr{ false, std::move(*e) };
    }

    [[nodiscard]] static std::string to_string(const Up2DbmsV1cExpr& e) {
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
        if (parsed->infinity) return mnup2::display(mnup2::up2dbms_infinity_fs(term));
        const mnup2::Expr& e = parsed->expr;
        if (e.empty()) return {};
        if (!mnup2::expr_valid(e)) {
            return "解析失败，请确认你没有使用全角括号，逗号，分号";
        }
        if (e.back().empty()) return mnup2::display(mnup2::Expr(e.begin(), e.end() - 1));
        return mnup2::display(mnup2::up2dbms_expand(e, term, !longer, /*v1_flag=*/false, /*v1c=*/true));
    }
};

}  // namespace omegay::notation

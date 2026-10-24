// Header/notation/waw2mn3.hpp
// wAω2MN3 记号（display 名 "wAω2MN3"，TS id "weak-a-omega2-mn-3"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/Aw2MN3.ts
// （家族/类别：category-hypcos-w2mn，credit.hypcos_mn）。
// 表达式结构：与 Aw2MN2 相同 —— Sep=Expr（递归）；Vertical=Sep[]；
// Entry=[value(1-based), Sep, mark?]；Column=Entry[]；Expr=Column[]；∞ 用 bool 标志表示。
// 核心实现与普通版/版本2 共享 Header/notation/aw2mn_share.hpp
// （本记号：subtract1 版3、弱版 —— stretch 时 V0[x][y] 不低于 top_vertical 的条目保持原分隔符）。
#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "Header/notation/aw2mn_share.hpp"

namespace omegay::notation {

struct WAw2MN3Notation {
    static constexpr const char* kName = "wAω2MN3";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/false);
    }
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::optional<aw2mn_share::Parsed> parse(std::string_view text) {
        return aw2mn_share::parse(text);
    }

    [[nodiscard]] static std::string to_string(const aw2mn_share::Parsed& p) {
        if (p.infinity) return "Limit";
        return aw2mn_share::mountain_display(p.expr);
    }

private:
    static std::string expand_text_impl(std::string_view text, int term, bool longer) {
        if (term < 1) term = 1;
        const auto parsed = aw2mn_share::parse(text);
        if (!parsed) return aw2mn_share::kParseFail;
        if (parsed->infinity) return aw2mn_share::mountain_display(aw2mn_share::infinity_fs(term));
        const aw2mn_share::Expr& e = parsed->expr;
        if (e.empty()) return {};
        if (e.back().empty()) {
            return aw2mn_share::mountain_display(aw2mn_share::Expr(e.begin(), e.end() - 1));
        }
        // 值域校验：value(1-based) 作为父列索引 value-1 须在 [0, size)
        for (const auto& col : e) {
            for (const auto& en : col) {
                if (en.value < 1 || en.value > static_cast<int>(e.size())) return aw2mn_share::kParseFail;
            }
        }
        return aw2mn_share::mountain_display(
            aw2mn_share::expand(e, term, /*shorter=*/!longer, /*weak=*/true,
                                aw2mn_share::SubtractKind::V3));
    }
};

} // namespace omegay::notation

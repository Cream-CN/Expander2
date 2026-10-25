#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "Header/notation/aw2mn_share.hpp"

namespace omegay::notation {

struct WAw2MN2Notation {
    static constexpr const char* kName = "wAω2MN2";

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
                                aw2mn_share::SubtractKind::V2));
    }
};

} // namespace omegay::notation

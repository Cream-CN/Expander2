
#pragma once

#include "notation_base.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "smnshare.hpp"

namespace omegay::notation {

// Expr：解析结果（顶层 ∞ / "Limit" 已拆除）。
struct NMnExpr {
    smnshare::Mountain mountain;
};

template <int N>
struct NMnNotation : NotationBase {
    // 具体显示名为 N 后接 "MN"（如 1MN、2MN…）。
    static constexpr const char* kName = "nMN";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<NMnExpr> parse(std::string_view text) {
        auto m = smnshare::from_display(text);
        if (!m) return std::nullopt;
        return NMnExpr{ std::move(*m) };
    }

    [[nodiscard]] static std::string to_string(const NMnExpr& e) {
        return smnshare::mountain_display(e.mountain, false);
    }

private:
    // TS MN_FS_variants(expand, is_limit, to_data_key) 的两支：较短支用
    // expand(shorter=true)，较长支用 expand(shorter=false)；对外只走较长支。
    static std::string expand_text_impl(std::string_view text, int term, bool longer) {
        if (term < 1) term = 1;
        const auto expr = parse(text);
        if (!expr) {
            return "解析失败，请确认你没有使用全角括号，逗号，分号";
        }
        const smnshare::Mountain& m = expr->mountain;
        if (m.empty()) return {};
        if (m.back().empty()) {
            // 后继：去掉末列。
            return smnshare::mountain_display(
                smnshare::Mountain(m.begin(), m.end() - 1), false);
        }
        const auto out = smnshare::expand(m, term, !longer);
        return smnshare::mountain_display(out, false);
    }
};

} // namespace omegay::notation

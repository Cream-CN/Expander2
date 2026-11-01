
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "smnshare.hpp"

namespace omegay::notation {

// Expr：非 ∞ 时持有 Mountain；∞（显示 "Limit"）用标志表示。
struct NMnExpr {
    bool infinity = false;
    smnshare::Mountain mountain;
};

template <int N>
struct NMnNotation {
    // 具体显示名为 N 后接 "MN"（如 1MN、2MN…）。
    static constexpr const char* kName = "nMN";

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
    [[nodiscard]] static std::optional<NMnExpr> parse(std::string_view text) {
        if (text == "Limit") return NMnExpr{ true, {} };
        auto m = smnshare::from_display(text);
        if (!m) return std::nullopt;
        return NMnExpr{ false, std::move(*m) };
    }

    [[nodiscard]] static std::string to_string(const NMnExpr& e) {
        if (e.infinity) return "Limit";
        return smnshare::mountain_display(e.mountain, false);
    }

private:
    // TS MN_FS_variants(expand, is_infinity, NT_infinity_FS(n), is_limit,
    // to_data_key).FS / .FS_alter：FS 用 expand(shorter=true)，FS_alter 用
    // expand(shorter=false)。纯函数语义，省略 display 键缓存。
    static std::string expand_text_impl(std::string_view text, int term, bool longer) {
        if (term < 1) term = 1;
        const auto expr = parse(text);
        if (!expr) {
            return "解析失败，请确认你没有使用全角括号，逗号，分号";
        }
        if (expr->infinity) {
            const auto m = smnshare::nt_infinity_fs(N, term);
            return smnshare::mountain_display(m, false);
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

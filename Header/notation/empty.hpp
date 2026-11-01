#pragma once
// ============================================================
// 记号: "空记号" (Passthrough / Empty Notation)
// ============================================================
#include <string>
#include <vector>

namespace omegay::notation {

struct EmptyNotation {
    static constexpr const char* kName        = "空记号 (Passthrough)";
    static constexpr const char* kDescription = "";
    [[nodiscard]] static std::vector<int> expand(
        const std::vector<int>& seq, int /*FSterm*/) {
        return seq;
    }
    [[nodiscard]] static std::string suffix() {
        return "  (尚在重构中)";
    }
};

} // namespace omegay::notation
#pragma once
// ============================================================
// 记号: "空记号" (Passthrough / Empty Notation)
// 行为: 原样输出输入序列，并追加"尚在重构中"的提示。
// 这是当前唯一可用的记号，其它记号将在后续版本中重新加入。
// ============================================================
#include <string>
#include <vector>

namespace omegay::notation {

struct EmptyNotation {
    static constexpr const char* kName        = "空记号 (Passthrough)";
    static constexpr const char* kDescription = "原样输出，尚在重构中";

    // 展开接口与旧记号保持一致的调用形态，方便日后替换。
    [[nodiscard]] static std::vector<int> expand(
        const std::vector<int>& seq, int /*FSterm*/) {
        return seq; // 原样输出
    }

    // 追加给 UI 的提示
    [[nodiscard]] static std::string suffix() {
        return "  (尚在重构中)";
    }
};

} // namespace omegay::notation
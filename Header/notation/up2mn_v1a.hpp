// Header/notation/up2mn_v1a.hpp
// UP2MN v1A 记号（display 名 "UP2MN v1A"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/UPMN/UP2MN-v1a.ts
// （TS id "up2mn-v1a"，家族/类别：category-upmn-test，credit.upmn）。
// 表达式结构（TS）：与 nMN 相同 —— Sep=int；Vertical=Sep[]；Entry=[int,Sep]；
// Column=Entry[]；Mountain=Column[]。
// 注意：本文件为接口骨架 —— 函数定义暂时留空（TODO），尚未在
// Expander.cpp 的 notationTable() 注册；待算法移植后补全。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

// Expr 的占位类型（定义待补全）。
struct Up2MnV1aExpr;

struct UP2MNV1ANotation {
    static constexpr const char* kName = "UP2MN v1A";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view text, int term);
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term);
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<Up2MnV1aExpr> parse(std::string_view text);
    [[nodiscard]] static std::string to_string(const Up2MnV1aExpr& expr);
};

} // namespace omegay::notation

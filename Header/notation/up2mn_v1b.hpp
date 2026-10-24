// Header/notation/up2mn_v1b.hpp
// UP2MN v1B 记号（display 名 "UP2MN v1B"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/UPMN/UP2MN-v1b.ts
// （TS id "up2mn-v1b"，家族/类别：category-upmn-test，credit.upmn）。
// 表达式结构（TS）：Entry=[int,int]；Column=Entry[]；Expr=Column[]；
// Vertical=number[]。
// 注意：本文件为接口骨架 —— 函数定义暂时留空（TODO），尚未在
// Expander.cpp 的 notationTable() 注册；待算法移植后补全。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

// Expr 的占位类型（定义待补全）。
struct Up2MnV1bExpr;

struct UP2MNV1BNotation {
    static constexpr const char* kName = "UP2MN v1B";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view text, int term);
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term);
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<Up2MnV1bExpr> parse(std::string_view text);
    [[nodiscard]] static std::string to_string(const Up2MnV1bExpr& expr);
};

} // namespace omegay::notation

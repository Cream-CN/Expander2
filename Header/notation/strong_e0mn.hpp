// Header/notation/strong_e0mn.hpp
// strong e0MN 记号（display 名 "strong e0MN"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/strong_e0MN.ts
// （TS id "strong-e0mn"，家族/类别：category-ta0-mn，credit.e0mn；原作者 test_alpha0）。
// 表达式结构（TS）：与 e0MN 相同 —— Term={exp:Ord, coeff:int}；Ord=Term[]；
// Entry={a:int, x:Ord}；Column=Entry[]；Expr=Column[]。
// 注意：本文件为接口骨架 —— 函数定义仅返回记号名字（占位），尚未在
// Expander.cpp 的 notationTable() 注册；算法逻辑待移植后补全。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

// Expr 的占位类型（定义待补全）。
struct StrongE0MnExpr {};

struct StrongE0MNNotation {
    static constexpr const char* kName = "strong e0MN";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view, int) { return kName; }
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view, int) { return kName; }
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<StrongE0MnExpr> parse(std::string_view) { return std::nullopt; }
    [[nodiscard]] static std::string to_string(const StrongE0MnExpr&) { return kName; }
};

} // namespace omegay::notation

// Header/notation/t_omega_mn.hpp
// TωMN 记号（display 名 "TωMN"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/T_omega_MN.ts
// （TS id "t-omega-mn"，家族/类别：category-mn，credit.hypcos_mn）。
// 表达式结构（TS）：Sep=Expr（递归）；Vertical=Sep[]；Entry=[int,Sep]；
// Column=Entry[]；Expr=Column[]。
// 注意：本文件为接口骨架 —— 函数定义仅返回记号名字（占位），尚未在
// Expander.cpp 的 notationTable() 注册；算法逻辑待移植后补全。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

// Expr 的占位类型（定义待补全）。
struct TOmegaMnExpr {};

struct TOmegaMNNotation {
    static constexpr const char* kName = "TωMN";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view, int) { return kName; }
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view, int) { return kName; }
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<TOmegaMnExpr> parse(std::string_view) { return std::nullopt; }
    [[nodiscard]] static std::string to_string(const TOmegaMnExpr&) { return kName; }
};

} // namespace omegay::notation

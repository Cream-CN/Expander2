// Header/notation/pcf_omega_mn.hpp
// PCF's ωMN 记号（display 名 "PCF's ωMN"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/pcf-omega_MN.ts
// （TS id "pcf-omega-mn"，家族/类别：category-mn，credit.pcf-omega-mn）。
// 表达式结构（TS）：Vertical=number[]；Entry=[int,Vertical]；
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
struct PcfOmegaMnExpr {};

struct PcfOmegaMNNotation {
    static constexpr const char* kName = "PCF's ωMN";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view, int) { return kName; }
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view, int) { return kName; }
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<PcfOmegaMnExpr> parse(std::string_view) { return std::nullopt; }
    [[nodiscard]] static std::string to_string(const PcfOmegaMnExpr&) { return kName; }
};

} // namespace omegay::notation

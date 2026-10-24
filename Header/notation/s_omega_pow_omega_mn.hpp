// Header/notation/s_omega_pow_omega_mn.hpp
// Sω^ωMN 记号（display 名 "Sω^ωMN"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/SMN/S_omega_pow_omega_MN.ts
// （TS id "S-omega^omega-MN"，家族/类别：category-smile-mn，credit.n_mn）。
// 表达式结构（TS）：Sep=number[]；Vertical=Sep[]；Entry=[int,Sep]；
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
struct SOmegaPowOmegaMnExpr;

struct SOmegaPowOmegaMNNotation {
    static constexpr const char* kName = "Sω^ωMN";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view text, int term);
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term);
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<SOmegaPowOmegaMnExpr> parse(std::string_view text);
    [[nodiscard]] static std::string to_string(const SOmegaPowOmegaMnExpr& expr);
};

} // namespace omegay::notation

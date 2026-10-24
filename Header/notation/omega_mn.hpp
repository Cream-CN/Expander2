// Header/notation/omega_mn.hpp
// ωMN 记号（display 名 "ωMN"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/Omega_MN.ts
// （TS id "omega-mn"，家族/类别：category-mn，credit.hypcos_mn）。
// 表达式结构（TS）：Sep=int；Vertical=Sep[]；Entry=[int,Sep]；
// Column=Entry[]；Expr=Column[]。
// 注意：本文件为接口骨架 —— 函数定义暂时留空（TODO），尚未在
// Expander.cpp 的 notationTable() 注册；待算法移植后补全。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

// Expr 的占位类型（定义待补全）。
struct OmegaMnExpr;

struct OmegaMNNotation {
    static constexpr const char* kName = "ωMN";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view text, int term);
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term);
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<OmegaMnExpr> parse(std::string_view text);
    [[nodiscard]] static std::string to_string(const OmegaMnExpr& expr);
};

} // namespace omegay::notation

// Header/notation/s_omega_dbms_v3.hpp
// SωDBMS v3 记号（display 名 "SωDBMS v3"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/SDBMS/S_omega_DBMS-v3.ts
// （TS id "s-omega-dbms-v3"，家族/类别：category-sdbms）。
// 表达式结构（TS）：Entry=[int,int]；Column=Entry[]；Expr=Column[]。
// 注意：本文件为接口骨架 —— 函数定义暂时留空（TODO），尚未在
// Expander.cpp 的 notationTable() 注册；待算法移植后补全。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

// Expr 的占位类型（定义待补全）。
struct SOmegaDbmsV3Expr;

struct SOmegaDBMSV3Notation {
    static constexpr const char* kName = "SωDBMS v3";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view text, int term);
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term);
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<SOmegaDbmsV3Expr> parse(std::string_view text);
    [[nodiscard]] static std::string to_string(const SOmegaDbmsV3Expr& expr);
};

} // namespace omegay::notation

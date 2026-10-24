// Header/notation/s1dbms.hpp
// S1DBMS 记号（display 名 "S1DBMS"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/SDBMS/S1DBMS.ts
// （TS id "s1dbms"，家族/类别：category-sdbms）。
// 表达式结构（TS）：Entry=[int,int]；Column=Entry[]；Expr=Column[]；
// 另含 DBMS 矩阵视图 Expr_DBMS=number[][]（显示时列标 +1）。
// 注意：本文件为接口骨架 —— 函数定义仅返回记号名字（占位），尚未在
// Expander.cpp 的 notationTable() 注册；算法逻辑待移植后补全。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

// Expr 的占位类型（定义待补全）。
struct S1DbmsExpr {};

struct S1DBMSNotation {
    static constexpr const char* kName = "S1DBMS";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view, int) { return kName; }
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view, int) { return kName; }
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<S1DbmsExpr> parse(std::string_view) { return std::nullopt; }
    [[nodiscard]] static std::string to_string(const S1DbmsExpr&) { return kName; }
};

} // namespace omegay::notation

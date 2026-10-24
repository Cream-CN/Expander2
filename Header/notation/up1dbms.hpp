// Header/notation/up1dbms.hpp
// UP1DBMS 记号（display 名 "UP1DBMS"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/UPMN/UP1DBMS.ts
// （TS id "up1dbms"，家族/类别：category-upmn-test）。
// 表达式结构（TS）：Entry=[int,int]；Column=Entry[]；Expr=Column[]；
// 另含 DBMS 矩阵视图 Expr_DBMS=number[][]。
// 注意：本文件为接口骨架 —— 函数定义仅返回记号名字（占位），尚未在
// Expander.cpp 的 notationTable() 注册；算法逻辑待移植后补全。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

// Expr 的占位类型（定义待补全）。
struct Up1DbmsExpr {};

struct UP1DBMSNotation {
    static constexpr const char* kName = "UP1DBMS";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view, int) { return kName; }
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view, int) { return kName; }
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<Up1DbmsExpr> parse(std::string_view) { return std::nullopt; }
    [[nodiscard]] static std::string to_string(const Up1DbmsExpr&) { return kName; }
};

} // namespace omegay::notation

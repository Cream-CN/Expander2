// Header/notation/up2dbms_v2.hpp
// UP2DBMS v2 记号（display 名 "UP2DBMS v2"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/UPMN/UP2DBMS-v2.ts
// （TS id "up2dbms-v2"，家族/类别：category-upmn，credit.upmn）。
// 表达式结构（TS）：Height=[int,int]；Entry=[int,Height]；Column=Entry[]；
// Expr=Column[]；另含 DBMS 视图 Entry_DBMS=[int,int]、Expr_DBMS=Column_DBMS[]。
// 注意：本文件为接口骨架 —— 函数定义暂时留空（TODO），尚未在
// Expander.cpp 的 notationTable() 注册；待算法移植后补全。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

// Expr 的占位类型（定义待补全）。
struct Up2DbmsV2Expr;

struct UP2DBMSV2Notation {
    static constexpr const char* kName = "UP2DBMS v2";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view text, int term);
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term);
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<Up2DbmsV2Expr> parse(std::string_view text);
    [[nodiscard]] static std::string to_string(const Up2DbmsV2Expr& expr);
};

} // namespace omegay::notation

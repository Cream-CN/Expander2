// Header/notation/waw2mn2.hpp
// wAω2MN2 记号（display 名 "wAω2MN2"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/Aw2MN2.ts
// （TS id "weak-a-omega2-mn-2"，家族/类别：category-hypcos-w2mn，credit.hypcos_mn）。
// 表达式结构（TS）：Sep=Expr（递归）；Vertical=Expr[]；
// Entry=[int,Sep,bool?]（第三项为标记）；Column=Entry[]；Expr=Column[]。
// 注意：本文件为接口骨架 —— 函数定义暂时留空（TODO），尚未在
// Expander.cpp 的 notationTable() 注册；待算法移植后补全。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

// Expr 的占位类型（定义待补全）。
struct WAw2Mn2Expr;

struct WAw2MN2Notation {
    static constexpr const char* kName = "wAω2MN2";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view text, int term);
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term);
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<WAw2Mn2Expr> parse(std::string_view text);
    [[nodiscard]] static std::string to_string(const WAw2Mn2Expr& expr);
};

} // namespace omegay::notation

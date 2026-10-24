// Header/notation/aw2mn3.hpp
// Aω2MN3 记号（display 名 "Aω2MN3"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/Aw2MN3.ts
// （TS id "a-omega2-mn-3"，家族/类别：category-hypcos-w2mn，credit.hypcos_mn）。
// 表达式结构（TS）：与 Aw2MN2 相同 —— Sep=Expr（递归）；Vertical=Expr[]；
// Entry=[int,Sep,bool?]（第三项为标记）；Column=Entry[]；Expr=Column[]。
// 注意：本文件为接口骨架 —— 函数定义仅返回记号名字（占位），尚未在
// Expander.cpp 的 notationTable() 注册；算法逻辑待移植后补全。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

// Expr 的占位类型（定义待补全）。
struct Aw2Mn3Expr {};

struct Aw2MN3Notation {
    static constexpr const char* kName = "Aω2MN3";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view, int) { return kName; }
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view, int) { return kName; }
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<Aw2Mn3Expr> parse(std::string_view) { return std::nullopt; }
    [[nodiscard]] static std::string to_string(const Aw2Mn3Expr&) { return kName; }
};

} // namespace omegay::notation

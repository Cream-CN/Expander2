// Header/notation/n_mn.hpp
// nMN 家族记号（display 名 "<n>MN"，如 1MN / 2MN / 3MN …）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/SMN/n_MN.ts
// （TS 生成器 category_n_mn：start=1、initial=3；TS id "<n>-MN"，
// 家族/类别：category-n-mn，credit.n_mn）。
// 表达式结构（TS）：Sep=int；Vertical=Sep[]；Entry=[int,Sep]；
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
struct NMnExpr;

template <int N>
struct NMnNotation {
    // 具体显示名为 N 后接 "MN"（如 1MN、2MN…）。
    static constexpr const char* kName = "nMN";

    // FS：较短展开。
    [[nodiscard]] static std::string expand_text(std::string_view text, int term);
    // FS_alter：较长展开。
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term);
    [[nodiscard]] static std::string suffix() { return {}; }

    // 供测试 / 调试使用的解析与格式化。
    [[nodiscard]] static std::optional<NMnExpr> parse(std::string_view text);
    [[nodiscard]] static std::string to_string(const NMnExpr& expr);
};

} // namespace omegay::notation

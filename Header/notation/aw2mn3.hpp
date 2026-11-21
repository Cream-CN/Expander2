// Header/notation/aw2mn3.hpp
// Aω2MN3 记号（display 名 "Aω2MN3"，TS id "a-omega2-mn-3"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/Aw2MN3.ts
// （家族/类别：category-hypcos-w2mn，credit.hypcos_mn）。
// 表达式结构：与 Aw2MN2 相同 —— Sep=Expr（递归）；Vertical=Sep[]；
// Entry=[value(1-based), Sep, mark?]；Column=Entry[]；Expr=Column[]。
// 核心实现与普通版/版本2 共享 Header/notation/aw2mn_share.hpp
// （本记号：subtract1 版3 —— is_one 提前返回；limit 分支用 while 循环加长 J 至两侧 vertical 相等）。
// 与另三个同族记号的差异仅 (weak, subtract1 版本) 两个编译期开关，统一在
// aw2mn_share::Aw2MnFamilyNotation 模板里。
#pragma once

#include "aw2mn_share.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace omegay::notation {

struct Aw2MN3Notation
    : aw2mn_share::Aw2MnFamilyNotation</*kWeak=*/false, aw2mn_share::SubtractKind::V3> {
    static constexpr const char* kName = "Aω2MN3";
};

} // namespace omegay::notation

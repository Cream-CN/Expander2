// Header/notation/strong_e0mn.hpp
// strong e0MN 记号（strong=true）。与 e0MN 的唯一差异是 strong 开关，
// 骨架见 mnordshare::E0MnFamilyNotation 模板。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mnordshare.hpp"

namespace omegay::notation {

// 兼容旧名：解析结果（表达式）。
using StrongE0MnExpr = mnordshare::E0MnExpr;

struct StrongE0MNNotation : mnordshare::E0MnFamilyNotation</*kStrong=*/true> {
    static constexpr const char* kName = "strong e0MN";
};

} // namespace omegay::notation

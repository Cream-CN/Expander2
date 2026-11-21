// Header/notation/e0mn.hpp
// e0MN 记号（strong=false）。骨架见 mnordshare::E0MnFamilyNotation 模板；
// 本文件只保留记号名与“供测试用的表达式比较”。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mnordshare.hpp"

namespace omegay::notation {

// 兼容旧名：解析结果（表达式）。
using E0MnExpr = mnordshare::E0MnExpr;

struct E0MNNotation : mnordshare::E0MnFamilyNotation</*kStrong=*/false> {
    static constexpr const char* kName = "e0MN";

    // 表达式的比较（供测试用）。
    [[nodiscard]] static int compare(const mnordshare::E0MnExpr& a, const mnordshare::E0MnExpr& b) {
        return mnordshare::expr_compare(a.expr, b.expr);
    }
};

} // namespace omegay::notation

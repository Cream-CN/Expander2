// Header/notation/up2mn_v1b_plus.hpp
// UP2MN v1B+（plus=true）。
// 骨架与 UP2MN v1B 相同，见 mnup2share::Up2MnFamilyNotation 模板。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mnup2share.hpp"

namespace omegay::notation {

// 兼容旧名（测试/调试代码沿用它）。
using Up2MnV1bPlusExpr = mnup2::Up2Expr;

struct UP2MNV1BPlusNotation : mnup2::Up2MnFamilyNotation</*kPlus=*/true> {
    static constexpr const char* kName = "UP2MN v1B+";
};

}  // namespace omegay::notation

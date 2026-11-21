// Header/notation/up2dbms_v1b_plus.hpp
// UP2DBMS v1B+（减法变体 v1_flag=false、v1c=false）。
// 骨架与 UP2DBMS v1 / v1C 相同，见 mnup2share::Up2DbmsFamilyNotation 模板。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mnup2share.hpp"

namespace omegay::notation {

// 兼容旧名（测试/调试代码沿用它）。
using Up2DbmsV1bPlusExpr = mnup2::Up2Expr;

struct UP2DBMSV1BPlusNotation
    : mnup2::Up2DbmsFamilyNotation</*kV1Flag=*/false, /*kV1c=*/false> {
    static constexpr const char* kName = "UP2DBMS v1B+";
};

}  // namespace omegay::notation

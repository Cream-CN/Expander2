// Header/notation/up2dbms_v1.hpp
// UP2DBMS v1（减法变体 v1_flag=true、v1c=false）。
// 骨架与 UP2DBMS v1B+ / v1C 相同，见 mnup2share::Up2DbmsFamilyNotation 模板。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mnup2share.hpp"

namespace omegay::notation {

// 兼容旧名（测试/调试代码沿用它）。
using Up2DbmsV1Expr = mnup2::Up2Expr;

struct UP2DBMSV1Notation
    : mnup2::Up2DbmsFamilyNotation</*kV1Flag=*/true, /*kV1c=*/false> {
    static constexpr const char* kName = "UP2DBMS v1";
};

}  // namespace omegay::notation

// Header/notation/waw2mn2.hpp
// wAω2MN2（弱版 Aω2MN2）。与 Aω2MN2 的唯一差异是 weak 开关，
// 实现统一在 aw2mn_share::Aw2MnFamilyNotation 模板里。
#pragma once

#include "aw2mn_share.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace omegay::notation {

struct WAw2MN2Notation
    : aw2mn_share::Aw2MnFamilyNotation</*kWeak=*/true, aw2mn_share::SubtractKind::V2> {
    static constexpr const char* kName = "wAω2MN2";
};

} // namespace omegay::notation

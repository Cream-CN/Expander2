// Header/notation/waw2mn3.hpp
// wAω2MN3（弱版 Aω2MN3）。与 Aω2MN3 的唯一差异是 weak 开关，
// 实现统一在 aw2mn_share::Aw2MnFamilyNotation 模板里。
#pragma once

#include "Header/notation/aw2mn_share.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace omegay::notation {

struct WAw2MN3Notation
    : aw2mn_share::Aw2MnFamilyNotation</*kWeak=*/true, aw2mn_share::SubtractKind::V3> {
    static constexpr const char* kName = "wAω2MN3";
};

} // namespace omegay::notation

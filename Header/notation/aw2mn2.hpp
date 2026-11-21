// Header/notation/aw2mn2.hpp
// Aω2MN2 记号（display 名 "Aω2MN2"，TS id "a-omega2-mn-2"）。
// 与 Aω2MN3 / wAω2MN2 / wAω2MN3 同构，差异只有两个编译期开关（weak、subtract1 版本），
// 共享实现见 Header/notation/aw2mn_share.hpp 的 Aw2MnFamilyNotation 模板。
#pragma once

#include "aw2mn_share.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace omegay::notation {

struct Aw2MN2Notation
    : aw2mn_share::Aw2MnFamilyNotation</*kWeak=*/false, aw2mn_share::SubtractKind::V2> {
    static constexpr const char* kName = "Aω2MN2";
};

} // namespace omegay::notation

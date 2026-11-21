// Header/notation/ton.hpp
// Taranovsky's ordinal notation
// 文本：main 家族格式（Ω<数字> / 0 / Limit / C 后缀），展开入口为 expand_text（唯一展开）。
#pragma once

#include "tonshare.hpp"

#include <map>
#include <string>
#include <string_view>

namespace omegay::notation {

namespace ton_detail {

// TS: BuiltQ(n, b, a, x)（TON_main 版，n 实为系统高度 sys，随递归递减）。
[[nodiscard]] inline bool builtq_main(tonshare::TonValue n, const tonshare::TonNode* b,
                                      const tonshare::TonNode* a, const tonshare::TonNode* x) {
    if (n) {
        return builtq_main(n - 1, b, x, x) ||
               (tonshare::compare_raise(x, a) <= 0 &&
                (tonshare::is_leaf(x) ? x->v >= 0
                                      : builtq_main(n, b, a, x->r) && builtq_main(n, b, a, x->l)));
    }
    return tonshare::compare_raise(a, b) < 0;
}

// TS: StandardQ(n, a)。StdTrue 只缓存“真”，且缓存忽略 n（与 TS 一致）。
[[nodiscard]] inline bool standard_q(tonshare::TonValue n, const tonshare::TonNode* a,
                                     std::map<std::string, bool>& memo) {
    return tonshare::std_memoized(memo, a, [&]() {
        return standard_q(n, a->r, memo) && standard_q(n, a->l, memo) &&
               (tonshare::is_leaf(a->l) || tonshare::compare_raise(a->r, a->l->r) <= 0) &&
               builtq_main(n, a, a->r, a->r);
    });
}

// 策略：main 风格（Ω<n>）、无 passthrough；mark 用 mark_main。
struct TonPolicy {
    static constexpr bool kMainStyle = true;
    static constexpr bool kIbpStyleLimit = false;
    static constexpr bool kHasMark = true;
    static constexpr tonshare::MarkFn mark_fn() { return &tonshare::mark_main; }

    [[nodiscard]] static bool is_standard(const tonshare::TonNode* a, tonshare::TonValue sys,
                                          std::map<std::string, bool>& memo) {
        return standard_q(sys, a, memo);
    }
};

} // namespace ton_detail

struct TonNotation : tonshare::TonNotationBase<ton_detail::TonPolicy> {
    static constexpr const char* kName = "TON";
};

} // namespace omegay::notation

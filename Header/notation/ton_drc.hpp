// Header/notation/ton_drc.hpp
// TON_DRC —— "Degrees of Reflection (reflection configuration)"。
// 文本：noraise 家族格式（Ω / 0 / Limit / C 后缀）。
#pragma once

#include "tonshare.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

namespace ton_drc_detail {

// TS: BuiltQ(a, ap, b, x, xp)（5 参，含反射构型 r()）。
[[nodiscard]] inline bool builtq_drc(const tonshare::TonNode* a, const tonshare::TonNode* ap,
                                     const tonshare::TonNode* b, const tonshare::TonNode* x,
                                     const tonshare::TonNode* xp) {
    if (tonshare::is_leaf(x)) {
        const tonshare::TonValue v = x->v;
        if (v < 0) {
            return tonshare::compare_noraise(x, b) < 0 ||
                   (tonshare::compare_noraise(x, a) <= 0 && false);
        }
        return v == 0;
    }
    if (tonshare::compare_noraise(x, tonshare::zero_leaf()) < 0) {
        tonshare::TonArena ra;
        const tonshare::TonNode* rxxp = tonshare::r(ra, x, xp);
        const tonshare::TonNode* raap = tonshare::r(ra, a, ap);
        return tonshare::compare_noraise(x, b) < 0 ||
               (tonshare::compare_noraise(rxxp, raap) <= 0 && builtq_drc(a, ap, b, x->l, x) &&
                builtq_drc(a, ap, b, x->r, x));
    }
    return builtq_drc(a, ap, b, x->l, xp) && builtq_drc(a, ap, b, x->r, xp);
}

// TS: StandardQ(a)。DRCStd 只缓存“真”。
[[nodiscard]] inline bool standard_q_drc(const tonshare::TonNode* a, std::map<std::string, bool>& memo) {
    return tonshare::std_memoized(memo, a, [&]() {
        if (!(standard_q_drc(a->r, memo) && standard_q_drc(a->l, memo) &&
              (tonshare::is_leaf(a->l) || tonshare::compare_noraise(a->r, a->l->r) <= 0)))
            return false;
        std::vector<const tonshare::TonNode*> small_terms;
        tonshare::collect_smallpart(a->r, small_terms);
        for (const tonshare::TonNode* x : small_terms) {
            if (!builtq_drc(x, a, a, x, a)) return false;
        }
        return true;
    });
}

// 策略：noraise 风格（裸 Ω）、DoR 极限项、无 mark 函数。
struct TonDRCPolicy {
    static constexpr bool kMainStyle = false;
    static constexpr bool kHasMark = false;

    [[nodiscard]] static bool is_standard(const tonshare::TonNode* a, tonshare::TonValue /*sys*/,
                                          std::map<std::string, bool>& memo) {
        return standard_q_drc(a, memo);
    }
};

} // namespace ton_drc_detail

struct TonDRCNotation : tonshare::TonNotationBase<ton_drc_detail::TonDRCPolicy> {
    static constexpr const char* kName = "TON_DRC";
};

} // namespace omegay::notation

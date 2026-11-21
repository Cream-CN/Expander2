// Header/notation/ton_dor.hpp
// TON_DoR —— "Degrees of Reflection"。
// 文本：noraise 家族格式（Ω / 0 / Limit / C 后缀）。
#pragma once

#include "tonshare.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

namespace ton_dor_detail {

// TS: BuiltQ(a, b, x)（3 参）。叶子处理对齐 TS 的 undefined/NaN 语义（-1 走首分支、
// 0 恒真、其余非负叶子恒假）。
[[nodiscard]] inline bool builtq_dor(const tonshare::TonNode* a, const tonshare::TonNode* b,
                                     const tonshare::TonNode* x) {
    if (tonshare::is_leaf(x)) {
        const tonshare::TonValue v = x->v;
        if (v < 0) {
            return tonshare::compare_noraise(x, b) < 0 ||
                   (tonshare::compare_noraise(x, a) <= 0 && false);
        }
        return v == 0;
    }
    if (tonshare::compare_noraise(x, tonshare::zero_leaf()) < 0) {
        return tonshare::compare_noraise(x, b) < 0 ||
               (tonshare::compare_noraise(x, a) <= 0 && builtq_dor(a, b, x->l) && builtq_dor(a, b, x->r));
    }
    return builtq_dor(a, b, x->l) && builtq_dor(a, b, x->r);
}

// TS: StandardQ(a)。DRStd 只缓存“真”。
[[nodiscard]] inline bool standard_q_dor(const tonshare::TonNode* a, std::map<std::string, bool>& memo) {
    return tonshare::std_memoized(memo, a, [&]() {
        if (!(standard_q_dor(a->r, memo) && standard_q_dor(a->l, memo) &&
              (tonshare::is_leaf(a->l) || tonshare::compare_noraise(a->r, a->l->r) <= 0)))
            return false;
        std::vector<const tonshare::TonNode*> small_terms;
        tonshare::collect_smallpart(a->r, small_terms);
        for (const tonshare::TonNode* x : small_terms) {
            if (!builtq_dor(x, a, x)) return false;
        }
        return true;
    });
}

// 策略：noraise 风格（裸 Ω）、DoR 极限项、无 mark 函数。
struct TonDoRPolicy {
    static constexpr bool kMainStyle = false;
    static constexpr bool kHasMark = false;

    [[nodiscard]] static bool is_standard(const tonshare::TonNode* a, tonshare::TonValue /*sys*/,
                                          std::map<std::string, bool>& memo) {
        return standard_q_dor(a, memo);
    }
};

} // namespace ton_dor_detail

struct TonDoRNotation : tonshare::TonNotationBase<ton_dor_detail::TonDoRPolicy> {
    static constexpr const char* kName = "TON_DoR";
};

} // namespace omegay::notation

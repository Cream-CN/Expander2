// Header/notation/ton_drpc.hpp
// TON_DRPC —— "Degrees of Reflection with Passthrough (reflection configuration)"。
// 文本：noraise 家族格式（Ω / 0 / Limit / C 后缀）。
#pragma once

#include "tonshare.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

namespace ton_drpc_detail {

// TS: BuiltQ(a, ap, ai, b, a0, d)（6 参）。d 状态参数同 DRP。
[[nodiscard]] inline bool builtq_drpc(const tonshare::TonNode* a, const tonshare::TonNode* ap,
                                      const tonshare::TonNode* ai, const tonshare::TonNode* b,
                                      const tonshare::TonNode* a0, const tonshare::TonNode* d) {
    if (tonshare::is_leaf(a)) {
        const tonshare::TonValue v = a->v;
        if (v == 0) return true;
        return tonshare::compare_noraise(a, b) < 0;
    }
    if (tonshare::compare_noraise(a, b) < 0) return true;
    if (d == tonshare::neg_one_leaf() && tonshare::compare_noraise(a, tonshare::zero_leaf()) < 0) {
        tonshare::TonArena ra;
        const tonshare::TonNode* raap = tonshare::r(ra, a, ap);
        const tonshare::TonNode* raib = tonshare::r(ra, ai, b);
        if (tonshare::compare_noraise(raap, raib) > 0) return false;
    }
    if (tonshare::compare_noraise(a, d) < 0) return builtq_drpc(a, ap, ai, b, a0, tonshare::neg_one_leaf());
    // x2 = compare(a, 0) < 0 ? a : ap
    const tonshare::TonNode* x2 =
        tonshare::compare_noraise(a, tonshare::zero_leaf()) < 0 ? a : ap;
    if (d == tonshare::neg_one_leaf() && tonshare::compare_noraise(a->l, tonshare::zero_leaf()) < 0) {
        tonshare::TonArena ra;
        const tonshare::TonNode* r1 = tonshare::r(ra, a->r, x2);
        const tonshare::TonNode* r2 = tonshare::r(ra, a0, b);
        if (tonshare::compare_noraise(r1, r2) < 0) return builtq_drpc(a, ap, ai, b, a0, a);
    }
    return builtq_drpc(a->l, x2, ai, b, a0, d) && builtq_drpc(a->r, x2, ai, b, a0, d);
}

// TS: StandardQ(a)。DRPCStd 只缓存“真”。
[[nodiscard]] inline bool standard_q_drpc(const tonshare::TonNode* a, std::map<std::string, bool>& memo) {
    return tonshare::std_memoized(memo, a, [&]() {
        if (!(standard_q_drpc(a->r, memo) && standard_q_drpc(a->l, memo) &&
              (tonshare::is_leaf(a->l) || tonshare::compare_noraise(a->r, a->l->r) <= 0)))
            return false;
        std::vector<const tonshare::TonNode*> small_terms;
        tonshare::collect_smallpart(a->r, small_terms);
        for (const tonshare::TonNode* x : small_terms) {
            if (!builtq_drpc(x, a, x, a, a->r, tonshare::neg_one_leaf())) return false;
        }
        return true;
    });
}

// 策略：noraise 风格（裸 Ω）、DoR 极限项、无 mark 函数。
struct TonDRPCPolicy {
    static constexpr bool kMainStyle = false;
    static constexpr bool kHasMark = false;

    [[nodiscard]] static bool is_standard(const tonshare::TonNode* a, tonshare::TonValue /*sys*/,
                                          std::map<std::string, bool>& memo) {
        return standard_q_drpc(a, memo);
    }
};

} // namespace ton_drpc_detail

struct TonDRPCNotation : tonshare::TonNotationBase<ton_drpc_detail::TonDRPCPolicy> {
    static constexpr const char* kName = "TON_DRPC";
};

} // namespace omegay::notation

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

} // namespace ton_drc_detail

struct TonDRCNotation {
    static constexpr const char* kName = "TON_DRC";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        static std::map<std::string, bool> memo;
        static tonshare::TonFsCache cache;
        const tonshare::StdFn std_fn = [](const tonshare::TonNode* a,
                                          tonshare::TonValue /*sys*/) -> bool {
            return ton_drc_detail::standard_q_drc(a, memo);
        };
        return tonshare::ton_expand_text(text, term, /*main_style=*/false,
                                         /*ibp_style_limit=*/false, std_fn, cache, nullptr);
    }

    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text(text, term);
    }

    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::string parse(std::string_view text) {
        tonshare::TonArena arena;
        const tonshare::TonNode* t = tonshare::parse_impl(arena, text, false);
        return t ? tonshare::display_noraise(t) : "解析失败";
    }
};

} // namespace omegay::notation

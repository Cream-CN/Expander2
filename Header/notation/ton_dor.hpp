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

} // namespace ton_dor_detail

struct TonDoRNotation {
    static constexpr const char* kName = "TON_DoR";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        static std::map<std::string, bool> memo;
        static tonshare::TonFsCache cache;
        const tonshare::StdFn std_fn = [](const tonshare::TonNode* a,
                                          tonshare::TonValue /*sys*/) -> bool {
            return ton_dor_detail::standard_q_dor(a, memo);
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

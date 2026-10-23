// Header/notation/ton_drp.hpp
// TON_DRP —— "Degrees of Reflection with Passthrough"。
// 文本：noraise 家族格式（Ω / 0 / Limit / C 后缀）。
#pragma once

#include "tonshare.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

namespace ton_drp_detail {

// TS: BuiltQ(a, ai, b, a0, d)。d 为状态参数：-1（用 neg_one_leaf 哨兵表示）
// 或上一层的 a（节点）。语句式早退语义逐条照搬。
[[nodiscard]] inline bool builtq_drp(const tonshare::TonNode* a, const tonshare::TonNode* ai,
                                     const tonshare::TonNode* b, const tonshare::TonNode* a0,
                                     const tonshare::TonNode* d) {
    if (tonshare::is_leaf(a)) {
        const tonshare::TonValue v = a->v;
        if (v == 0) return true;
        // 负数叶子（-1）：TS 中 compare(a, b) < 0 对任意 b（节点）恒真；
        // 其余非负叶子（kInf）按防御处理，不访问 a[0]/a[1]。
        return tonshare::compare_noraise(a, b) < 0;
    }
    if (tonshare::compare_noraise(a, b) < 0) return true;
    if (d == tonshare::neg_one_leaf() && tonshare::compare_noraise(a, tonshare::zero_leaf()) < 0 &&
        tonshare::compare_noraise(a, ai) > 0)
        return false;
    if (tonshare::compare_noraise(a, d) < 0) return builtq_drp(a, ai, b, a0, tonshare::neg_one_leaf());
    if (d == tonshare::neg_one_leaf() && tonshare::compare_noraise(a->l, tonshare::zero_leaf()) < 0 &&
        tonshare::compare_noraise(a->r, a0) < 0)
        return builtq_drp(a, ai, b, a0, a);
    return builtq_drp(a->l, ai, b, a0, d) && builtq_drp(a->r, ai, b, a0, d);
}

// TS: StandardQ(a)。DRPStd 只缓存“真”。
[[nodiscard]] inline bool standard_q_drp(const tonshare::TonNode* a, std::map<std::string, bool>& memo) {
    return tonshare::std_memoized(memo, a, [&]() {
        if (!(standard_q_drp(a->r, memo) && standard_q_drp(a->l, memo) &&
              (tonshare::is_leaf(a->l) || tonshare::compare_noraise(a->r, a->l->r) <= 0)))
            return false;
        std::vector<const tonshare::TonNode*> small_terms;
        tonshare::collect_smallpart(a->r, small_terms);
        for (const tonshare::TonNode* x : small_terms) {
            if (!builtq_drp(x, x, a, a->r, tonshare::neg_one_leaf())) return false;
        }
        return true;
    });
}

} // namespace ton_drp_detail

struct TonDRPNotation {
    static constexpr const char* kName = "TON_DRP";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        static std::map<std::string, bool> memo;
        static tonshare::TonFsCache cache;
        const tonshare::StdFn std_fn = [](const tonshare::TonNode* a,
                                          tonshare::TonValue /*sys*/) -> bool {
            return ton_drp_detail::standard_q_drp(a, memo);
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

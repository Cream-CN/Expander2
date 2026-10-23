// Header/notation/ton_i.hpp
// TON_I —— "Iteration of n-built from below (no passthrough)"。
// 文本：noraise 家族格式（Ω / 0 / Limit / C 后缀）。
#pragma once

#include "tonshare.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

namespace ton_i_detail {

// TS: BuiltQ(a, b, n, x)（TON_I 版，4 参；n 递减）。
// 叶子处理说明：TS 在 x 为叶子时对 x[0]/x[1] 取 undefined 并经由 NaN 比较得“假”；
// 本移植显式短路（-1 叶子走首分支递归、0 叶子恒真、其余非负叶子恒假），结果一致。
[[nodiscard]] inline bool builtq_i(const tonshare::TonNode* a, const tonshare::TonNode* b,
                                   tonshare::TonValue n, const tonshare::TonNode* x) {
    if (n) {
        if (tonshare::is_leaf(x)) {
            const tonshare::TonValue v = x->v;
            if (v < 0) return builtq_i(x, b, n - 1, x);   // 第二分支的叶子子树为假
            if (v == 0) return true;                       // (x>=0||...) 且 x===0
            return false;                                   // kInf 等非零非负叶子：子树 undefined
        }
        return (tonshare::compare_noraise(x, tonshare::zero_leaf()) < 0 &&
                builtq_i(x, b, n - 1, x)) ||
               ((tonshare::compare_noraise(x, tonshare::zero_leaf()) >= 0 ||
                 tonshare::compare_noraise(x, a) <= 0) &&
                builtq_i(a, b, n, x->l) && builtq_i(a, b, n, x->r));
    }
    return tonshare::compare_noraise(a, b) < 0;
}

// TS: StandardQ(a)。IStd 只缓存“真”。
[[nodiscard]] inline bool standard_q_i(const tonshare::TonNode* a, std::map<std::string, bool>& memo) {
    return tonshare::std_memoized(memo, a, [&]() {
        if (!(standard_q_i(a->r, memo) && standard_q_i(a->l, memo) &&
              (tonshare::is_leaf(a->l) || tonshare::compare_noraise(a->r, a->l->r) <= 0)))
            return false;
        std::vector<std::vector<int>> small_paths;
        tonshare::collect_smallindex(a->r, {}, small_paths);
        for (const std::vector<int>& index : small_paths) {
            tonshare::TonArena arena;
            const tonshare::TonNode* ex = tonshare::extract(a->r, index);
            if (!builtq_i(ex, a, static_cast<tonshare::TonValue>(tonshare::get_n(arena, a->r, index)), ex))
                return false;
        }
        return true;
    });
}

} // namespace ton_i_detail

struct TonINotation {
    static constexpr const char* kName = "TON_I";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        static std::map<std::string, bool> memo;
        static tonshare::TonFsCache cache;
        const tonshare::StdFn std_fn = [](const tonshare::TonNode* a,
                                          tonshare::TonValue /*sys*/) -> bool {
            return ton_i_detail::standard_q_i(a, memo);
        };
        return tonshare::ton_expand_text(text, term, /*main_style=*/false,
                                         /*ibp_style_limit=*/true, std_fn, cache, nullptr);
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

// Header/notation/ton.hpp
// Taranovsky's ordinal notation
// 文本：main 家族格式（Ω<数字> / 0 / Limit / C 后缀），FS 入口为 expand_text/expand_text_alter。
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

} // namespace ton_detail

struct TonNotation {
    static constexpr const char* kName = "TON";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        static std::map<std::string, bool> memo;
        static tonshare::TonFsCache cache;
        const tonshare::StdFn std_fn = [](const tonshare::TonNode* a,
                                          tonshare::TonValue sys) -> bool {
            return ton_detail::standard_q(sys, a, memo);
        };
        return tonshare::ton_expand_text(text, term, /*main_style=*/true,
                                         /*ibp_style_limit=*/false, std_fn, cache,
                                         tonshare::mark_main);
    }

    // 与 expand_text 同语义（避免 UI 对非 BMS/UPMS 文本走通用截断分支）。
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text(text, term);
    }

    [[nodiscard]] static std::string suffix() { return {}; }

    // 测试辅助：文本 -> 术语（原样显示）。
    [[nodiscard]] static std::string parse(std::string_view text) {
        tonshare::TonArena arena;
        const tonshare::TonNode* t = tonshare::parse_impl(arena, text, true);
        return t ? tonshare::display_main(t) : "解析失败";
    }
};

} // namespace omegay::notation

// Header/notation/ton_ibp.hpp
// TON_IBP —— "Iteration of n-built from below"。
// 文本：noraise 家族格式（Ω / 0 / Limit / C 后缀）。
#pragma once

#include "tonshare.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

namespace ton_ibp_detail {

// TS: BuiltQ(a, b, c, n)。c 为 get_a2 得到的公共前缀向量。
[[nodiscard]] inline bool builtq_ibp(const tonshare::TonNode* a, const tonshare::TonNode* b,
                                     const std::vector<tonshare::TonValue>& c, tonshare::TonValue n) {
    if (!n) return tonshare::compare_noraise(a, b) < 0;
    std::vector<std::vector<int>> subterms;
    tonshare::collect_subterm_index(a, {}, subterms);
    for (const std::vector<int>& x : subterms) {
        const tonshare::TonNode* ex = tonshare::extract(a, x);
        if (ex == nullptr) continue;
        if (tonshare::compare_noraise(ex, a) <= 0) continue;
        if (tonshare::compare_noraise(ex, tonshare::zero_leaf()) >= 0) continue;
        if (builtq_ibp(ex, b, c, n - 1)) continue;
        bool ok = false;
        for (std::size_t yindex = 0; yindex < x.size(); ++yindex) {
            std::vector<int> y(x.begin(), x.begin() + static_cast<std::ptrdiff_t>(yindex));
            const tonshare::TonNode* ey = tonshare::extract(a, y);
            if (ey == nullptr) continue;
            if (tonshare::compare_noraise(ey, tonshare::zero_leaf()) >= 0) continue;
            if (builtq_ibp(ey, b, c, n - 1)) { ok = true; break; }
            if (tonshare::is_leaf(ey)) continue;
            if (tonshare::compare_noraise_vec(ey->r, c) >= 0) continue;
            bool zok = true;
            for (std::size_t zindex = x.size() + 1; zindex > yindex; --zindex) {
                std::vector<int> z(x.begin(), x.begin() + static_cast<std::ptrdiff_t>(zindex - 1));
                const tonshare::TonNode* ez = tonshare::extract(a, z);
                if (ez == nullptr || tonshare::compare_noraise(ez, ey) < 0) { zok = false; break; }
            }
            if (zok) { ok = true; break; }
        }
        if (!ok) return false;
    }
    return true;
}

// TS: StandardQ(a)。IBPStd 缓存“真”（false 不缓存，与 TS 的 IBPStd[str]=result 语义一致）。
[[nodiscard]] inline bool standard_q_ibp(const tonshare::TonNode* a, std::map<std::string, bool>& memo) {
    return tonshare::std_memoized(memo, a, [&]() {
        if (!(standard_q_ibp(a->r, memo) && standard_q_ibp(a->l, memo) &&
              (tonshare::is_leaf(a->l) || tonshare::compare_noraise(a->r, a->l->r) <= 0)))
            return false;
        std::vector<std::vector<int>> small_paths;
        tonshare::collect_smallindex(a->r, {}, small_paths);
        for (const std::vector<int>& a1index : small_paths) {
            tonshare::TonArena arena;
            std::vector<tonshare::TonValue> a2 = tonshare::get_a2(arena, a->r, a1index);
            const tonshare::TonNode* ex = tonshare::extract(a->r, a1index);
            if (!builtq_ibp(ex, a, a2, static_cast<tonshare::TonValue>(tonshare::get_n(a2))))
                return false;
        }
        return true;
    });
}

} // namespace ton_ibp_detail

struct TonIbpNotation {
    static constexpr const char* kName = "TON_IBP";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        static std::map<std::string, bool> memo;
        static tonshare::TonFsCache cache;
        const tonshare::StdFn std_fn = [](const tonshare::TonNode* a,
                                          tonshare::TonValue /*sys*/) -> bool {
            return ton_ibp_detail::standard_q_ibp(a, memo);
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

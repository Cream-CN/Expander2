// Header/notation/ton_mc.hpp
// TON_MC —— "TON (reflection configuration) without passthrough"。
#pragma once

#include "tonshare.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

namespace ton_mc_detail {

[[nodiscard]] inline const tonshare::TonNode* extractparent(const tonshare::TonNode* a,
                                                            const tonshare::TonNode* b,
                                                            const std::vector<int>& x) {
    if (x.empty()) return b;
    std::vector<int> pre(x.begin(), x.end() - 1);
    return tonshare::extract(a, pre);
}

// TS: refresh_totest(d, e) —— 收集“不比 extractparent(e) 小”且非叶子的子项路径。
inline void refresh_totest(const tonshare::TonNode* a, const tonshare::TonNode* b,
                           std::vector<std::vector<int>>& totest, const std::vector<int>& d,
                           const std::vector<int>& e) {
    const tonshare::TonNode* ex = tonshare::extract(a, d);
    if (ex == nullptr || tonshare::is_leaf(ex) ||
        tonshare::compare_raise(ex, extractparent(a, b, e)) < 0)
        return;
    totest.push_back(d);
    std::vector<int> d0 = d;
    d0.push_back(0);
    std::vector<int> d1 = d;
    d1.push_back(1);
    refresh_totest(a, b, totest, d0, e);
    refresh_totest(a, b, totest, d1, e);
}

// TS: BuiltQ(a, b, n)（3 参）。注：x.some(...) 的 t 参数未使用。
[[nodiscard]] inline bool builtq_mc(const tonshare::TonNode* a, const tonshare::TonNode* b,
                                    tonshare::TonValue n) {
    if (!(n > 0)) return tonshare::compare_raise(a, b) < 0;
    std::vector<std::vector<int>> subterms;
    tonshare::collect_subterm_index(a, {}, subterms);
    for (const std::vector<int>& x : subterms) {
        const tonshare::TonNode* ex = tonshare::extract(a, x);
        if (ex == nullptr) continue;
        {
            tonshare::TonArena ra;
            if (tonshare::compare_raise(tonshare::r(ra, ex, extractparent(a, b, x)),
                                        tonshare::r(ra, a, b)) <= 0)
                continue;
        }
        bool some = false;
        for (std::size_t zindex = 0; zindex < x.size(); ++zindex) {
            std::vector<int> pre(x.begin(), x.begin() + static_cast<std::ptrdiff_t>(zindex));
            if (tonshare::compare_raise(tonshare::extract(a, pre), b) < 0) {
                some = true;
                break;
            }
        }
        if (some) continue;
        std::vector<std::vector<int>> totest;
        refresh_totest(a, b, totest, x, x);
        bool found = false;
        std::vector<int> y = x;
        while (!y.empty()) {
            bool cond1 = true;
            for (std::size_t dz = 0; dz < x.size() - y.size(); ++dz) {
                std::vector<int> pre(x.begin(),
                                     x.begin() + static_cast<std::ptrdiff_t>(y.size() + dz));
                if (tonshare::compare_raise(tonshare::extract(a, pre), extractparent(a, b, y)) < 0) {
                    cond1 = false;
                    break;
                }
            }
            if (cond1) {
                bool cond2 = true;
                for (const std::vector<int>& z : totest) {
                    if (tonshare::compare_raise(tonshare::extract(a, z), extractparent(a, b, y)) < 0) {
                        cond2 = false;
                        break;
                    }
                }
                if (cond2 && builtq_mc(tonshare::extract(a, y), extractparent(a, b, y), n - 1)) {
                    found = true;
                    break;
                }
            }
            y.pop_back();
        }
        if (!found) return false;
    }
    return true;
}

// TS: StandardQ(n, a)。MCStd 只缓存“真”。
[[nodiscard]] inline bool standard_q_mc(tonshare::TonValue n, const tonshare::TonNode* a,
                                        std::map<std::string, bool>& memo) {
    return tonshare::std_memoized(memo, a, [&]() {
        return standard_q_mc(n, a->r, memo) && standard_q_mc(n, a->l, memo) &&
               (tonshare::is_leaf(a->l) || tonshare::compare_raise(a->r, a->l->r) <= 0) &&
               builtq_mc(a->r, a, n);
    });
}

} // namespace ton_mc_detail

struct TonMcNotation {
    static constexpr const char* kName = "TON_MC";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        static std::map<std::string, bool> memo;
        static tonshare::TonFsCache cache;
        const tonshare::StdFn std_fn = [](const tonshare::TonNode* a,
                                          tonshare::TonValue sys) -> bool {
            return ton_mc_detail::standard_q_mc(sys, a, memo);
        };
        return tonshare::ton_expand_text(text, term, /*main_style=*/true,
                                         /*ibp_style_limit=*/false, std_fn, cache,
                                         tonshare::mark_mc);
    }

    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text(text, term);
    }

    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::string parse(std::string_view text) {
        tonshare::TonArena arena;
        const tonshare::TonNode* t = tonshare::parse_impl(arena, text, true);
        return t ? tonshare::display_main(t) : "解析失败";
    }
};

} // namespace omegay::notation

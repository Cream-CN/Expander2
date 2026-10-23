// Header/notation/ton_mpc.hpp
// TON_MPC —— "TON with passthrough (reflection configuration)"。
#pragma once

#include "tonshare.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

namespace ton_mpc_detail {

[[nodiscard]] inline const tonshare::TonNode* extractparent(const tonshare::TonNode* a,
                                                            const tonshare::TonNode* b,
                                                            const std::vector<int>& x) {
    if (x.empty()) return b;
    std::vector<int> pre(x.begin(), x.end() - 1);
    return tonshare::extract(a, pre);
}

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

// TS: BuiltQ(a, b, rc, n)（4 参）。rc = r(a[1], a)（由 StandardQ 传入）。
[[nodiscard]] inline bool builtq_mpc(const tonshare::TonNode* a, const tonshare::TonNode* b,
                                     const tonshare::TonNode* rc, tonshare::TonValue n) {
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
                if (cond2) {
                    tonshare::TonArena ra;
                    if (tonshare::compare_raise(tonshare::r(ra, tonshare::extract(a, y),
                                                            extractparent(a, b, y)),
                                                rc) < 0 ||
                        builtq_mpc(tonshare::extract(a, y), extractparent(a, b, y), rc, n - 1)) {
                        found = true;
                        break;
                    }
                }
            }
            y.pop_back();
        }
        if (!found) return false;
    }
    return true;
}

// TS: StandardQ(n, a)。MPCStd 只缓存“真”。
[[nodiscard]] inline bool standard_q_mpc(tonshare::TonValue n, const tonshare::TonNode* a,
                                         std::map<std::string, bool>& memo) {
    return tonshare::std_memoized(memo, a, [&]() {
        tonshare::TonArena ra;
        const tonshare::TonNode* rc = tonshare::r(ra, a->r, a);
        return standard_q_mpc(n, a->r, memo) && standard_q_mpc(n, a->l, memo) &&
               (tonshare::is_leaf(a->l) || tonshare::compare_raise(a->r, a->l->r) <= 0) &&
               builtq_mpc(a->r, a, rc, n);
    });
}

} // namespace ton_mpc_detail

struct TonMpcNotation {
    static constexpr const char* kName = "TON_MPC";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        static std::map<std::string, bool> memo;
        static tonshare::TonFsCache cache;
        const tonshare::StdFn std_fn = [](const tonshare::TonNode* a,
                                          tonshare::TonValue sys) -> bool {
            return ton_mpc_detail::standard_q_mpc(sys, a, memo);
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

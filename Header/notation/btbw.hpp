// BTBM-weak（weak Bubby3's Transfinite BMS v2）—— BM-like 分层树形记号。
//
// 移植自 ne-rewritten 的 BTBM-weak.ts。它与标准 BTBM 共用全部层导航、special、
// 高度比较等原语，区别仅在后继展开：weak 版 ascend_replace 额外带 start_index
// 与一条“临界竖直序列（critical vertical）”，在越过该临界位置时补入剩余高度，
// 提升基准也从 r 改为 r+1。这里复用 btbm_detail，只重写提升与 FS。
#pragma once

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Header/notation/btbm.hpp"

namespace omegay::notation
{
namespace btbw_detail
{
using namespace btbm_detail;

inline Expr w_ascend_replace(const Expr& e, int start_index, int r, int diff,
                             std::optional<int> tl, const Expr& new_tail,
                             const Vertical& critical_vert)
{
    Expr result;
    for (size_t ii = 0; ii < e.cols.size(); ++ii)
    {
        const int i = (int)ii;
        if (tl.has_value() && *tl == 0 && i == (int)e.cols.size() - 1)
        {
            for (const Column& c : new_tail.cols) result.cols.push_back(c);
            continue;
        }
        const Column& col = e.cols[ii];
        const int col_index = start_index + i;
        Vertical current_vert;  // TS 中每列初始化为 []，且从不更新
        bool above_crit = false;
        bool ignore_check = false;
        Column result_col;

        for (size_t jj = 0; jj < col.size(); ++jj)
        {
            const int j = (int)jj;
            const Entry& entry = col[jj];
            const bool is_layer_tail =
                tl.has_value() && i == (int)e.cols.size() - 1 &&
                j == (int)col.size() - 1;

            if (!above_crit)
            {
                const HExpr height = to_height(*entry.height, col_index);
                const Vertical new_vert =
                    vertical_increase(current_vert, height);
                const int cmp = vertical_cmp(new_vert, critical_vert);
                auto plain_value = [&]() {
                    return entry.value >= r ? entry.value + diff : entry.value;
                };
                if (cmp > 0)
                {
                    const Vertical crit_remaining =
                        vertical_diff(critical_vert, current_vert);
                    for (const HExpr& h : crit_remaining)
                    {
                        Entry ce;
                        ce.value = plain_value();
                        ce.height = std::make_shared<Expr>(
                            from_height(h, start_index + diff));
                        result_col.push_back(std::move(ce));
                    }
                    above_crit = true;
                }
                else
                {
                    Entry ce;
                    ce.value = plain_value();
                    ce.height = std::make_shared<Expr>(
                        from_height(height, start_index + diff));
                    result_col.push_back(std::move(ce));
                    if (cmp == 0) above_crit = true;
                    if (!is_layer_tail) continue;
                }
            }

            std::optional<int> new_tl;
            if (is_layer_tail) new_tl = *tl - 1;

            Entry new_entry;
            new_entry.value = entry.value >= r ? entry.value + diff : entry.value;
            new_entry.height = std::make_shared<Expr>(w_ascend_replace(
                *entry.height, col_index + 1, r, diff, new_tl, new_tail,
                critical_vert));
            if (!ignore_check)
            {
                while (!result_col.empty() &&
                       result_col.back().value == new_entry.value &&
                       compare(*result_col.back().height,
                               *new_entry.height) < 0)
                {
#ifdef BW_TRACE
                    bw_tr_ev("i" + std::to_string(i) + "j" + std::to_string(j) +
                             " POP v" + std::to_string(result_col.back().value));
#endif
                    result_col.pop_back();
                }
                ignore_check = true;
            }
            result_col.push_back(std::move(new_entry));
        }
        result.cols.push_back(std::move(result_col));
    }
    return result;
}

inline Expr w_fs(const Expr& e, int index)
{
    if (is_infinity(e)) return infinity_fs(index);
    if (e.cols.empty()) return e;
    const int t_layer = tail_layer(e);
    if (t_layer < 0)
    {
        Expr r;
        for (size_t i = 0; i + 1 < e.cols.size(); ++i)
            r.cols.push_back(e.cols[i]);
        return r;
    }
    if (is_special(e, t_layer))
        return expand_special(e, t_layer, index);

    const int t = tail(e, t_layer);
    const int r = root(e, t_layer);
    const auto [r_layer, ri] = root_layer(e, r);

    const Expr& expr_root = skip_layers(e, r_layer);
    const Column& col_root = expr_root.cols[ri];
    const Expr& expr_tail = skip_layers(expr_root, t_layer - r_layer);
    const Column& col_tail = expr_tail.cols.back();
    const int appending = root_appending_start(col_root, r, col_tail, t);

    std::vector<HExpr> critical_heights;
    for (size_t k = 0; k + 1 < col_tail.size(); ++k)
        critical_heights.push_back(to_height(*col_tail[k].height, t));
    Vertical critical_vert;
    for (const HExpr& h : critical_heights)
        critical_vert = vertical_increase(critical_vert, h);

    Expr new_tail;
    for (int j = index; j >= 1; --j)
    {
        if (ri != (int)expr_root.cols.size() - 1)
        {
            Expr tail1;
            for (size_t k = ri + 1; k < expr_root.cols.size(); ++k)
                tail1.cols.push_back(expr_root.cols[k]);
            Expr replaced1 = w_ascend_replace(tail1, r + 1, r,
                                              j * (t - r),
                                              t_layer - r_layer, new_tail,
                                              critical_vert);
            Column new_col;
            for (size_t k = 0; k + 1 < col_tail.size(); ++k)
            {
                Entry ce;
                ce.value = col_tail[k].value + (j - 1) * (t - r);
                ce.height = std::make_shared<Expr>(
                    from_height(critical_heights[k], r + j * (t - r)));
                new_col.push_back(std::move(ce));
            }
            for (int k = appending; k < (int)col_root.size(); ++k)
            {
                Entry ne;
                ne.value = col_root[k].value;
                ne.height = std::make_shared<Expr>(w_ascend_replace(
                    *col_root[k].height, r + 1, r, j * (t - r),
                    std::nullopt, Expr{}, critical_vert));
                new_col.push_back(std::move(ne));
            }
            Expr combined;
            combined.cols.push_back(std::move(new_col));
            for (const Column& c : replaced1.cols)
                combined.cols.push_back(c);
            new_tail = std::move(combined);
        }
        else
        {
            if (appending == (int)col_root.size())
                throw std::runtime_error("Illegal state");
            Column new_col;
            for (size_t k = 0; k + 1 < col_tail.size(); ++k)
            {
                Entry ce;
                ce.value = col_tail[k].value + (j - 1) * (t - r);
                ce.height = std::make_shared<Expr>(
                    from_height(critical_heights[k], r + j * (t - r)));
                new_col.push_back(std::move(ce));
            }
            for (int k = appending; k < (int)col_root.size(); ++k)
            {
                std::optional<int> htl;
                if (k == (int)col_root.size() - 1)
                    htl = t_layer - r_layer - 1;
                Entry ne;
                ne.value = col_root[k].value;
                ne.height = std::make_shared<Expr>(w_ascend_replace(
                    *col_root[k].height, r + 1, r, j * (t - r), htl,
                    new_tail, critical_vert));
                new_col.push_back(std::move(ne));
            }
            Expr combined;
            combined.cols.push_back(std::move(new_col));
            new_tail = std::move(combined);
        }
    }
    return w_ascend_replace(e, 0, 0, 0, t_layer, new_tail, Vertical{});
}
}  // namespace btbw_detail

// BTBM-weak 记号接口：仅 FS（weak 的 FS 内部直接处理后继删末列）。
struct BTBMWeakNotation
{
    static std::string suffix() { return std::string{}; }
    static std::string expand_text(std::string_view text, int term)
    {
        using namespace btbm_detail;
        using namespace btbw_detail;
        try
        {
            Parser parser(text);
            const Expr m = parser.parse();
            return display(btbw_detail::w_fs(m, term));
        }
        catch (...)
        {
            return bmlike::kParseError;
        }
    }
};
}  // namespace omegay::notation

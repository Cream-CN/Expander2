// BBM（Branching BMS，BBMS，分支巴斯丘矩阵）—— BM-like 分层树形记号。
//
// 移植自 ne-rewritten 的 BBM.ts。它与 BTBM 的结构、层导航、提升（ascend_replace）、
// 追加起点、FS 主流程完全一致，唯一区别在“特殊形状（special）”的判定与展开：
//   * BBM 只要尾层之下一层的末列是空列即判 special（BTBM 还要求该层仅一项且
//     值为当前层左界 - 1）；
//   * special 复制在 t_layer > 0 时就递归进高度（BTBM 要 t_layer > 1）。
// 因此这里复用 btbm_detail 的全部原语，只重写 is_special / expand_special 与 FS。
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Header/notation/btbm.hpp"

namespace omegay::notation
{
namespace bbm_detail
{
using namespace btbm_detail;

// BBM special：尾层（t_layer 所指层）下一层的末列为空列。
inline bool b_is_special(const Expr& e, int t_layer)
{
    if (t_layer < 0) return false;
    const Expr& current = skip_layers(e, t_layer);
    if (!has_next_layer(current)) return false;
    const Expr& nxt = next_layer(current);
    // TS: next[next.length-1]?.length === 0 —— 空数组时 next[-1] 为 undefined。
    if (nxt.cols.empty()) return false;
    return nxt.cols.back().empty();
}

inline Expr b_expand_special(const Expr& e, int t_layer, int index)
{
    Expr result;
    for (size_t i = 0; i + 1 < e.cols.size(); ++i)
        result.cols.push_back(e.cols[i]);
    const Column& col = e.cols.back();
    Column result_col;
    for (size_t k = 0; k + 1 < col.size(); ++k)
        result_col.push_back(col[k]);
    const Entry& entry = col.back();
    if (t_layer > 0)
    {
        Entry ne;
        ne.value = entry.value;
        ne.height = std::make_shared<Expr>(
            b_expand_special(*entry.height, t_layer - 1, index));
        result_col.push_back(std::move(ne));
    }
    else
    {
        Entry ne;
        ne.value = entry.value;
        auto h = std::make_shared<Expr>();
        for (size_t k = 0; k + 1 < entry.height->cols.size(); ++k)
            h->cols.push_back(entry.height->cols[k]);
        ne.height = h;
        if (result_col.empty()) result_col.push_back(ne);
        // TS: Array(index).fill(new_entry) —— 同一对象填入 index 份（只读共享）。
        for (int k = 0; k < index; ++k) result_col.push_back(ne);
    }
    result.cols.push_back(std::move(result_col));
    return result;
}

// 与 btbm_detail::fs_core 逐字相同，仅把 special 判定/展开换成 BBM 版本。
inline Expr b_fs_core(const Expr& e, int index)
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
    if (b_is_special(e, t_layer))
        return b_expand_special(e, t_layer, index);

    const int t = tail(e, t_layer);
    const int r = root(e, t_layer);
    const auto [r_layer, ri] = root_layer(e, r);

    const Expr& expr_root = skip_layers(e, r_layer);
    const Column& col_root = expr_root.cols[ri];
    const Expr& expr_tail = skip_layers(expr_root, t_layer - r_layer);
    const Column& col_tail = expr_tail.cols.back();

    const int appending = root_appending_start(col_root, r, col_tail, t);

    Expr new_tail;
    for (int j = index; j >= 1; --j)
    {
        if (ri != (int)expr_root.cols.size() - 1)
        {
            Expr tail1;
            for (size_t k = ri + 1; k < expr_root.cols.size(); ++k)
                tail1.cols.push_back(expr_root.cols[k]);
            Expr replaced1 = ascend_replace(tail1, r, j * (t - r),
                                            t_layer - r_layer, new_tail);

            Column tail_prefix;
            for (size_t k = 0; k + 1 < col_tail.size(); ++k)
                tail_prefix.push_back(col_tail[k]);
            Expr prefix_expr;
            prefix_expr.cols.push_back(tail_prefix);
            Column new_col = ascend_replace(prefix_expr, r,
                                            (j - 1) * (t - r),
                                            std::nullopt, Expr{}).cols[0];
            for (int k = appending; k < (int)col_root.size(); ++k)
            {
                Entry ne;
                ne.value = col_root[k].value;
                ne.height = std::make_shared<Expr>(ascend_replace(
                    *col_root[k].height, r, j * (t - r),
                    std::nullopt, Expr{}));
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
            Column tail_prefix;
            for (size_t k = 0; k + 1 < col_tail.size(); ++k)
                tail_prefix.push_back(col_tail[k]);
            Expr prefix_expr;
            prefix_expr.cols.push_back(tail_prefix);
            Column new_col = ascend_replace(prefix_expr, r,
                                            (j - 1) * (t - r),
                                            std::nullopt, Expr{}).cols[0];
            for (int k = appending; k < (int)col_root.size(); ++k)
            {
                std::optional<int> htl;
                if (k == (int)col_root.size() - 1)
                    htl = t_layer - r_layer - 1;
                Entry ne;
                ne.value = col_root[k].value;
                ne.height = std::make_shared<Expr>(ascend_replace(
                    *col_root[k].height, r, j * (t - r), htl, new_tail));
                new_col.push_back(std::move(ne));
            }
            Expr combined;
            combined.cols.push_back(std::move(new_col));
            new_tail = std::move(combined);
        }
    }
    return ascend_replace(e, 0, 0, t_layer, new_tail);
}

// FS_default_LNZ_variant 分派。
inline Expr b_fs(const Expr& e, int index)
{
    if (is_infinity(e)) return infinity_fs(index);
    if (!is_limit(e)) return b_fs_core(e, 0);
    return b_fs_core(e, index);
}
}  // namespace bbm_detail

// BBM 记号接口：仅 FS。
struct BBMNotation
{
    static std::string suffix() { return std::string{}; }
    static std::string expand_text(std::string_view text, int term)
    {
        using namespace btbm_detail;
        try
        {
            Parser parser(text);
            const Expr m = parser.parse();
            return display(bbm_detail::b_fs(m, term));
        }
        catch (...)
        {
            return bmlike::kParseError;
        }
    }
};
}  // namespace omegay::notation

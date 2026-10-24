// TUPMS（Transfinite UPMS，超限 UPMS）—— BM-like 树形记号。
//
// 移植自 ne-rewritten 的 TUPMS.ts。TUPMS 与 TBM 共用同一套树形数据结构
// （Entry=[数值,高度表达式]、列、表达式、竖直序列）与绝大多数展开原语，唯一的
// 区别在后继展开求“提升阈值（ascension threshold）”时，TUPMS 用一个按竖直位置
// 排序的最小堆做“延迟 UPMS 提升判定”（含 X/Y 段变换比较），而不是 TBM 的直接扫描。
// 因此这里直接复用 tbm_detail 的全部原语，只重写 ascension_threshold 与 FS 包装。
#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Header/notation/tbm.hpp"

namespace omegay::notation
{
namespace tupms_detail
{
using namespace tbm_detail;

// const_column(value, vertical)：竖直序列中每一段都配上同一个数值的常数列。
inline Column const_column(int value, const Vertical& vertical)
{
    Column c;
    c.reserve(vertical.size());
    for (const Expr& s : vertical)
        c.push_back(Entry{ value, std::make_shared<Expr>(s) });
    return c;
}

// 以“延迟竖直位置为主键、列号为次键”的最小堆，复刻 TS 的 MinHeap<number>。
class DelayedHeap
{
public:
    explicit DelayedHeap(const std::vector<Vertical>* delayed_pos)
        : pos_(delayed_pos) {}
    bool empty() const { return items_.empty(); }
    void push(int i)
    {
        items_.push_back(i);
        size_t k = items_.size() - 1;
        while (k > 0)
        {
            const size_t p = (k - 1) >> 1;
            if (cmp(items_[k], items_[p]) >= 0) break;
            std::swap(items_[k], items_[p]);
            k = p;
        }
    }
    int pop_min()
    {
        const int top = items_[0];
        const int last = items_.back();
        items_.pop_back();
        if (!items_.empty())
        {
            items_[0] = last;
            size_t k = 0;
            for (;;)
            {
                const size_t l = k * 2 + 1, rr = l + 1;
                size_t s = k;
                if (l < items_.size() && cmp(items_[l], items_[s]) < 0) s = l;
                if (rr < items_.size() && cmp(items_[rr], items_[s]) < 0) s = rr;
                if (s == k) break;
                std::swap(items_[k], items_[s]);
                k = s;
            }
        }
        return top;
    }
private:
    int cmp(int i, int j) const
    {
        const int c = vertical_cmp((*pos_)[i], (*pos_)[j]);
        return c != 0 ? c : number_cmp(i, j);
    }
    std::vector<int> items_;
    const std::vector<Vertical>* pos_;
};

inline std::vector<Vertical> ascension_threshold(
    const Expr& m,
    const std::vector<std::vector<Vertical>>& V,
    const std::vector<ParentRow>& P, int r, const Vertical& b)
{
    const int right = static_cast<int>(m.cols.size()) - 1;
    const int cols = static_cast<int>(m.cols.size());

    std::vector<Vertical> A(cols);
    std::vector<char> A_set(cols, false);
    for (int k = 0; k < r; ++k)
    {
        A[k] = Vertical{};
        A_set[k] = true;
    }
    A[r] = b;
    A_set[r] = true;

    std::vector<Vertical> delayed_pos(cols);
    std::vector<char> delayed_vr(cols, false);
    std::vector<char> delayed_present(cols, false);

    DelayedHeap heap(&delayed_pos);
    for (int i = r + 1; i < cols; ++i)
    {
        delayed_pos[i] = Vertical{};
        delayed_vr[i] = false;
        delayed_present[i] = true;
        heap.push(i);
    }

    std::vector<std::vector<Vertical>> root_cache(cols);
    std::vector<char> root_set(cols, false);

    while (!heap.empty())
    {
        const int i = heap.pop_min();
        const Vertical prev_pos = delayed_pos[i];
        const bool vr = delayed_vr[i];
        delayed_present[i] = false;

        if (!vr)
        {
            const int j0 = index_after(V[i], prev_pos);
            for (int j = j0; j < static_cast<int>(V[i].size()); ++j)
            {
                Vertical pos;
                if (j != 0) pos = V[i][j - 1];
                const int col_p = P[i][j].first;
                if (col_p < r)
                {
                    A[i] = pos;
                    A_set[i] = true;
                    break;
                }
                if (col_p == r)
                {
                    // 延迟，等待 UPMS 提升判定。
                    delayed_pos[i] = pos;
                    delayed_vr[i] = true;
                    delayed_present[i] = true;
                    break;
                }
                const bool p_delayed = !A_set[col_p];
                const Vertical& Ap =
                    A_set[col_p] ? A[col_p] : delayed_pos[col_p];
                if (vertical_cmp(pos, Ap) >= 0)
                {
                    if (p_delayed)
                    {
                        delayed_pos[i] = pos;
                        delayed_vr[i] = false;
                        delayed_present[i] = true;
                    }
                    else
                    {
                        A[i] = pos;
                        A_set[i] = true;
                    }
                    break;
                }
                const Vertical& new_pos = V[i][j];
                if (vertical_cmp(new_pos, Ap) >= 0)
                {
                    if (p_delayed)
                    {
                        delayed_pos[i] = Ap;
                        delayed_vr[i] = false;
                        delayed_present[i] = true;
                    }
                    else
                    {
                        A[i] = Ap;
                        A_set[i] = true;
                    }
                    break;
                }
            }
            if (!A_set[i] && !delayed_present[i])
            {
                A[i] = V[i].back();
                A_set[i] = true;
            }
        }
        else
        {
            bool upgrading = false;
            {
                // 临界竖直序列为空则总是提升。
                if (prev_pos.empty())
                {
                    upgrading = true;
                }
                else
                {
                    const int ji0 = index_after(V[i], prev_pos);
                    Column threshold_column = column_add(
                        column_truncate(m.cols[i], prev_pos),
                        const_column(1, prev_pos));
                    threshold_column.push_back(
                        Entry{ m.cols[i][ji0].value + 1, one_expr() });
                    const int offset2 =
                        m.cols[i][ji0].height->cols.empty() ? 1 : 0;
                    threshold_column.push_back(
                        Entry{ m.cols[i][ji0 + offset2].value, one_expr() });

                    int X_end = i + 1;
                    while (X_end < cols)
                    {
                        if (column_cmp(m.cols[X_end], threshold_column) >= 0)
                            ++X_end;
                        else
                            break;
                    }
                    if (X_end != cols)
                    {
                        // Y 段起点。
                        const int jr0 = index_after(V[r], prev_pos);
                        const int value_r =
                            (jr0 < static_cast<int>(m.cols[r].size()))
                                ? m.cols[r][jr0].value
                                : 0;
                        const int jt0 = index_after(V[right], prev_pos);
                        const int value_t = m.cols[right][jt0].value;
                        int Y_start = right;
                        for (int k = 0; k < value_t - value_r - 1; ++k)
                            Y_start = P[Y_start]
                                [index_after(V[Y_start], prev_pos)].first;

                        Column X_init = column_sub(
                            column_truncate(m.cols[i], prev_pos), m.cols[r]);
                        Column Y_init = column_sub(
                            column_truncate(m.cols[Y_start], prev_pos),
                            m.cols[r]);
                        const int M = (std::max)(m.cols[i][0].value,
                                                 m.cols[Y_start][0].value);
                        for (Entry& e : X_init) e.value += M;
                        for (Entry& e : Y_init) e.value += M;

                        if (!root_set[Y_start])
                        {
                            root_cache[Y_start] =
                                tbm_detail::ascension_threshold(V, P,
                                                                 Y_start, b);
                            root_set[Y_start] = true;
                        }

                        for (int k = 0;; ++k)
                        {
                            if (Y_start + k > right) { upgrading = true; break; }
                            if (i + k > X_end) { upgrading = false; break; }

                            Vertical X_threshold;
                            if (A_set[i + k]) X_threshold = A[i + k];
                            if (!A_set[i + k] ||
                                vertical_cmp(X_threshold, prev_pos) > 0)
                                X_threshold = prev_pos;
                            const Column X_transformed = column_add(
                                m.cols[i + k],
                                column_truncate(Y_init, X_threshold));

                            Vertical Y_threshold =
                                root_cache[Y_start][Y_start + k];
                            if (vertical_cmp(Y_threshold, prev_pos) > 0)
                                Y_threshold = prev_pos;
                            const Column Y_transformed = column_add(
                                m.cols[Y_start + k],
                                column_truncate(X_init, Y_threshold));

                            const int cmp =
                                column_cmp(X_transformed, Y_transformed);
                            if (cmp != 0)
                            {
                                upgrading = cmp > 0;
                                break;
                            }
                        }
                    }
                    // X_end == cols：upgrading 已为 true。
                    else
                    {
                        upgrading = true;
                    }
                }
            }
            A[i] = upgrading ? b : prev_pos;
            A_set[i] = true;
        }

        if (delayed_present[i]) heap.push(i);
    }

    return A;
}

Expr tupms_fs(const Expr& m, int index);

inline Expr tu_expand_successor(const Expr& m, int index)
{
    std::vector<std::vector<Vertical>> V;
    V.reserve(m.cols.size());
    for (const Column& c : m.cols) V.push_back(column_verticals(c));
    const std::vector<ParentRow> P = parents(m, V);
    const int N = static_cast<int>(m.cols.size()) - 1;
    const int r = P[N][m.cols[N].size() - 1].first;

    Expr result;
    for (int i = 0; i < N; ++i) result.cols.push_back(m.cols[i]);

    Vertical b;
    if (m.cols[N].size() > 1) b = V[N][m.cols[N].size() - 2];

    const Column offset = column_sub(m.cols[N], m.cols[r]);
    const std::vector<Vertical> A =
        tupms_detail::ascension_threshold(m, V, P, r, b);

    for (int w = 1; w <= index; ++w)
        for (int i = r; i < N; ++i)
            result.cols.push_back(copy_column(m.cols[i], offset, A[i], w));
    return result;
}
inline Expr tu_expand_limit(const Expr& m, int index)
{
    const int right = static_cast<int>(m.cols.size()) - 1;
    const Column& col = m.cols[right];
    const int last_idx = static_cast<int>(col.size()) - 1;
    const int v = col[last_idx].value;
    const Expr new_h = tupms_fs(*col[last_idx].height, index);
    const std::vector<Expr> segs = to_vertical(new_h);

    Expr result = m;
    Column new_entries;
    for (int k = 0; k < last_idx; ++k) new_entries.push_back(col[k]);
    for (const Expr& seg : segs)
        new_entries.push_back(Entry{ v, std::make_shared<Expr>(seg) });
    result.cols[right] = std::move(new_entries);
    return result;
}
inline Expr tu_expand(const Expr& m, int index)
{
    if (m.cols.empty()) return m;
    const int N = static_cast<int>(m.cols.size()) - 1;
    const Column& last_col = m.cols[N];
    if (last_col.empty())
    {
        Expr r;
        for (int i = 0; i < N; ++i) r.cols.push_back(m.cols[i]);
        return r;
    }
    if (is_one(*last_col.back().height)) return tu_expand_successor(m, index);
    return tu_expand_limit(m, index);
}
inline Expr tupms_fs(const Expr& m, int index)
{
    if (is_infinity(m)) return infinity_fs(index);
    if (m.cols.empty()) return m;
    return tu_expand(m, index);
}
}  // namespace tupms_detail

// TUPMS 记号接口：仅 FS。
struct TUPMSNotation
{
    static std::string suffix() { return std::string{}; }
    static std::string expand_text(std::string_view text, int term)
    {
        using namespace tbm_detail;
        using namespace tupms_detail;
        try
        {
            Parser parser(text);
            const Expr m = parser.parse();
            return display(tupms_detail::tupms_fs(m, term));
        }
        catch (...)
        {
            return bmlike::kParseError;
        }
    }
};
}  // namespace omegay::notation

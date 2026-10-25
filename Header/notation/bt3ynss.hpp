// BT_star_Minus1_Y_nSS-v3（weak）C++ 移植。
//
// 与 v1/v2 差异：
//  - 显示：无 higher 的列走 BM column_display（normalize 截尾零，全 0 → (0)）；
//    有 higher 的列显示全量 lower + *^ 项（不裁剪）。
//  - 顶层父：vertical 比较（to_height + height_column_compare + vertical_compare）。
//  - compute_tail 返回全局栈下标 t 与层数 t_layer；root 走 skip_layer。
//  - ascend_replace 带 critical_vert：每列重算 compute_vertical，m 之前从
//    vertical 直接 from_height 重建，m 之后递归。
//  - 同时提供 FS（expand_text_n）与 FS_short（expand_text_n_alter，lnz_m1 分支）。
#pragma once

#include <algorithm>
#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "bmlike_share.hpp"

namespace omegay::notation
{
namespace bt3_detail
{
struct NodeC;
using NodePtr = std::shared_ptr<NodeC>;
struct ColumnC { std::vector<int> lower; std::vector<NodePtr> higher; };
struct NodeC { bool inf = false; std::vector<ColumnC> cols; };

inline NodePtr bt3_make_node() { return std::make_shared<NodeC>(); }
inline NodePtr bt3_infinity() { auto e = bt3_make_node(); e->inf = true; return e; }
inline bool bt3_is_infinity(const NodePtr& e) { return e->inf; }

inline NodePtr bt3_infinity_fs(int index, int n)
{
    NodePtr chain = bt3_make_node();
    for (int i = index; i > 0; --i)
    {
        auto wrapped = bt3_make_node();
        ColumnC c; c.lower.assign(n, i); c.higher.push_back(chain);
        wrapped->cols.push_back(std::move(c));
        chain = wrapped;
    }
    auto full = bt3_make_node();
    full->cols.push_back(ColumnC{ std::vector<int>(n, 0), {} });
    for (const ColumnC& cc : chain->cols) full->cols.push_back(cc);
    return full;
}
inline bool bt3_is_zero_column(const ColumnC& c)
{
    for (int x : c.lower) if (x != 0) return false;
    return c.higher.empty();
}
inline int bt3_last_nonneg(const std::vector<int>& v)
{
    for (int i = (int)v.size() - 1; i >= 0; --i) if (v[i] >= 0) return i;
    return -1;
}
inline int bt3_last_pos(const std::vector<int>& v)
{
    for (int i = (int)v.size() - 1; i >= 0; --i) if (v[i] > 0) return i;
    return -1;
}
inline bool bt3_contains(const std::deque<int>& d, int v)
{
    for (int x : d) if (x == v) return true;
    return false;
}

// ---- 显示（plain）----
inline std::string bt3_top_display(const NodePtr& e);
inline std::string bt3_display(const NodePtr& e);
inline std::string bt3_bm_column_display(const std::vector<int>& lower)
{
    int N = 0;
    for (size_t i = 0; i < lower.size(); ++i) if (lower[i] > 0) N = (int)i + 1;
    if (N == 0) return "(0)";
    std::string s = "(";
    for (int i = 0; i < N; ++i) { if (i) s += ","; s += std::to_string(lower[(size_t)i]); }
    s += ")";
    return s;
}
inline std::string bt3_column_display(const ColumnC& c)
{
    if (c.higher.empty()) return bt3_bm_column_display(c.lower);
    std::string s = "(";
    bool first = true;
    for (int x : c.lower) { if (!first) s += ","; first = false; s += std::to_string(x); }
    for (const NodePtr& x : c.higher) { if (!first) s += ","; first = false; s += bt3_top_display(x); }
    s += ")";
    return s;
}
inline std::string bt3_top_display(const NodePtr& e)
{
    if (e->cols.empty()) return "*";
    return "*^" + bt3_display(e);
}
inline std::string bt3_display(const NodePtr& e)
{
    if (bt3_is_infinity(e)) return "Limit";
    std::string s;
    for (const ColumnC& c : e->cols) s += bt3_column_display(c);
    return s;
}
inline bool bt3_is_limit(const NodePtr& e)
{
    if (bt3_is_infinity(e)) return true;
    if (e->cols.empty()) return false;
    return !bt3_is_zero_column(e->cols.back());
}

// ---- 比较 ----
inline int bt3_number_cmp(int a, int b) { return a == b ? 0 : a < b ? -1 : 1; }
inline int bt3_bool_cmp(bool a, bool b) { return (a ? 1 : 0) - (b ? 1 : 0); }
inline int bt3_compare(const NodePtr& a, const NodePtr& b);
inline int bt3_column_compare(const ColumnC& a, const ColumnC& b);
inline int bt3_higher_compare(const std::vector<NodePtr>& a, const std::vector<NodePtr>& b);
inline int bt3_column_compare(const ColumnC& a, const ColumnC& b)
{
    const size_t m = (std::min)(a.lower.size(), b.lower.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = bt3_number_cmp(a.lower[i], b.lower[i]); c != 0) return c;
    if (const int c = bt3_number_cmp((int)a.lower.size(), (int)b.lower.size()); c != 0) return c;
    return bt3_higher_compare(a.higher, b.higher);
}
inline int bt3_higher_compare(const std::vector<NodePtr>& a, const std::vector<NodePtr>& b)
{
    const size_t m = (std::min)(a.size(), b.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = bt3_compare(a[i], b[i]); c != 0) return c;
    return bt3_number_cmp((int)a.size(), (int)b.size());
}
inline int bt3_compare(const NodePtr& a, const NodePtr& b)
{
    const size_t m = (std::min)(a->cols.size(), b->cols.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = bt3_column_compare(a->cols[i], b->cols[i]); c != 0) return c;
    return bt3_number_cmp((int)a->cols.size(), (int)b->cols.size());
}

// ---- 父树 ----
struct PNodeC;
using PNodePtr = std::shared_ptr<PNodeC>;
struct PColC { std::shared_ptr<std::vector<int>> parr; std::vector<PNodePtr> higher; };
struct PNodeC { std::vector<PColC> cols; };
using ColStack = std::vector<const ColumnC*>;
using ParStack = std::vector<std::shared_ptr<std::vector<int>>>;

PNodePtr bt3_lower(const NodePtr& e, int n, ColStack& stack, ParStack& pstack)
{
    const size_t lS0 = stack.size();
    auto P = std::make_shared<PNodeC>();
    for (size_t i = 0; i < e->cols.size(); ++i)
    {
        const ColumnC& col = e->cols[i];
        const int iS = (int)stack.size();
        stack.push_back(&col);
        auto parr = std::make_shared<std::vector<int>>(n + 1, -1);
        pstack.push_back(parr);
        for (int j = 0; j < n; ++j)
        {
            int p = iS;
            while (p >= 0)
            {
                if (stack[(size_t)p]->lower[j] < col.lower[j]) break;
                p = (j == 0) ? p - 1 : (*(pstack[(size_t)p]))[j - 1];
            }
            if (p < 0) break;
            (*parr)[j] = p;
        }
        PColC pc; pc.parr = parr;
        for (const NodePtr& x : col.higher) pc.higher.push_back(bt3_lower(x, n, stack, pstack));
        P->cols.push_back(std::move(pc));
    }
    stack.resize(lS0);
    pstack.resize(lS0);
    return P;
}

// ---- 阈值 ----
struct ANodeC;
using ANodePtr = std::shared_ptr<ANodeC>;
struct AColC { int threshold = 0; std::vector<ANodePtr> higher; };
struct ANodeC { std::vector<AColC> cols; };

ANodePtr bt3_empty_at(const NodePtr& e)
{
    auto A = std::make_shared<ANodeC>();
    for (const ColumnC& col : e->cols)
    {
        AColC ac; ac.threshold = 0;
        for (const NodePtr& x : col.higher) ac.higher.push_back(bt3_empty_at(x));
        A->cols.push_back(std::move(ac));
    }
    return A;
}
ANodePtr bt3_thresholds(const NodePtr& e, const PNodePtr& P, int r, int b,
                        std::vector<int>& stk)
{
    auto A = std::make_shared<ANodeC>();
    const size_t lS0 = stk.size();
    for (size_t i = 0; i < e->cols.size(); ++i)
    {
        const int iS = (int)stk.size();
        AColC ac;
        if (iS < r && i + 1 != e->cols.size())
        {
            stk.push_back(0);
            ac.threshold = 0;
            for (const NodePtr& x : e->cols[i].higher) ac.higher.push_back(bt3_empty_at(x));
        }
        else
        {
            int Ai = 0;
            if (iS == r) Ai = b;
            else if (iS > r)
            {
                const std::vector<int>& parents = P->cols[i].parr ? *P->cols[i].parr : P->cols[i].parr ? *P->cols[i].parr : std::vector<int>{};
                const std::vector<int>& parents2 = *P->cols[i].parr;
                while (Ai < (int)parents2.size() && parents2[(size_t)Ai] >= r &&
                       parents2[(size_t)Ai] < (int)stk.size() && stk[(size_t)parents2[(size_t)Ai]] > Ai) ++Ai;
            }
            stk.push_back(Ai);
            ac.threshold = Ai;
            for (size_t ix = 0; ix < e->cols[i].higher.size(); ++ix)
                ac.higher.push_back(bt3_thresholds(e->cols[i].higher[ix],
                    P->cols[i].higher[ix], r, b, stk));
        }
        A->cols.push_back(std::move(ac));
    }
    stk.resize(lS0);
    return A;
}

// ---- vertical / height ----
struct HNodeC;
using HNodePtr = std::shared_ptr<HNodeC>;
struct HColC { std::vector<std::pair<int, bool>> low; int threshold = 0; std::vector<HNodePtr> higher; };
struct HNodeC { std::vector<HColC> cols; };

HNodePtr bt3_to_height(const std::vector<int>& base, const NodePtr& cur, const ANodePtr& AT, int n);
int bt3_height_cmp(const HNodePtr& a, const HNodePtr& b);
int bt3_hheight_cmp(const std::vector<HNodePtr>& a, const std::vector<HNodePtr>& b);
int bt3_hcol_cmp(const HColC& a, const HColC& b)
{
    const size_t m = (std::min)(a.low.size(), b.low.size());
    for (size_t i = 0; i < m; ++i)
    {
        if (const int c = bt3_number_cmp(a.low[i].first, b.low[i].first); c != 0) return c;
        if (const int c = bt3_bool_cmp(a.low[i].second, b.low[i].second); c != 0) return c;
    }
    if (const int c = bt3_number_cmp((int)a.low.size(), (int)b.low.size()); c != 0) return c;
    return bt3_hheight_cmp(a.higher, b.higher);
}
int bt3_hheight_cmp(const std::vector<HNodePtr>& a, const std::vector<HNodePtr>& b)
{
    const size_t m = (std::min)(a.size(), b.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = bt3_height_cmp(a[i], b[i]); c != 0) return c;
    return bt3_number_cmp((int)a.size(), (int)b.size());
}
int bt3_height_cmp(const HNodePtr& a, const HNodePtr& b)
{
    const size_t m = (std::min)(a->cols.size(), b->cols.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = bt3_hcol_cmp(a->cols[i], b->cols[i]); c != 0) return c;
    return bt3_number_cmp((int)a->cols.size(), (int)b->cols.size());
}
int bt3_vertical_cmp(const std::vector<HNodePtr>& a, const std::vector<HNodePtr>& b)
{
    return bt3_hheight_cmp(a, b);
}
HNodePtr bt3_to_height(const std::vector<int>& base, const NodePtr& cur, const ANodePtr& AT, int n)
{
    auto H = std::make_shared<HNodeC>();
    for (size_t i = 0; i < cur->cols.size(); ++i)
    {
        const ColumnC& col = cur->cols[i];
        const int A_col = (AT && i < AT->cols.size()) ? AT->cols[i].threshold : 0;
        HColC hc;
        hc.threshold = A_col;
        for (int j = 0; j < n; ++j)
            hc.low.push_back({ (j < A_col) ? col.lower[(size_t)j] - base[(size_t)j] : col.lower[(size_t)j],
                               j < A_col });
        for (size_t j = 0; j < col.higher.size(); ++j)
        {
            const ANodePtr ac = (AT && i < AT->cols.size() && j < AT->cols[i].higher.size())
                ? AT->cols[i].higher[j] : nullptr;
            hc.higher.push_back(bt3_to_height(base, col.higher[j], ac, n));
        }
        H->cols.push_back(std::move(hc));
    }
    return H;
}
std::vector<HNodePtr> bt3_compute_vertical(const ColumnC& col, int n)
{
    auto one = bt3_make_node();
    one->cols.push_back(col);
    ColStack stack; ParStack pstack;
    PNodePtr P = bt3_lower(one, n, stack, pstack);
    std::vector<int> stk;
    ANodePtr AT = bt3_thresholds(one, P, 0, n, stk);
    std::vector<HNodePtr> out;
    for (size_t i = 0; i < col.higher.size(); ++i)
        out.push_back(bt3_to_height(col.lower, col.higher[i], AT->cols[0].higher[i], n));
    return out;
}
NodePtr bt3_from_height(const std::vector<int>& base, const HNodePtr& height, int n)
{
    auto out = bt3_make_node();
    for (size_t i = 0; i < height->cols.size(); ++i)
    {
        const HColC& hc = height->cols[i];
        ColumnC nc;
        nc.lower.reserve(n);
        for (int j = 0; j < n; ++j)
            nc.lower.push_back((j < hc.threshold) ? hc.low[(size_t)j].first + base[(size_t)j]
                                                  : hc.low[(size_t)j].first);
        for (const HNodePtr& x : hc.higher) nc.higher.push_back(bt3_from_height(base, x, n));
        out->cols.push_back(std::move(nc));
    }
    return out;
}

// ---- 顶层父 ----
void bt3_compute_top_parents(const NodePtr& e, const PNodePtr& P, int n,
                             ParStack& pstack, std::vector<std::vector<HNodePtr>>& vstack,
                             std::deque<int>& outer)
{
    const size_t lS0 = pstack.size();
    for (size_t i = 0; i < e->cols.size(); ++i)
    {
        const ColumnC& col = e->cols[i];
        const int iS = (int)pstack.size();
        pstack.push_back(P->cols[i].parr);

        auto wrapE = bt3_make_node();
        wrapE->cols.push_back(col);
        auto wrapP = std::make_shared<PNodeC>();
        PColC wpc;
        wpc.parr = P->cols[i].parr;
        wpc.higher = P->cols[i].higher;
        wrapP->cols.push_back(std::move(wpc));
        std::vector<int> stk((size_t)iS, 0);
        ANodePtr oneTree = bt3_thresholds(wrapE, wrapP, iS, n, stk);

        std::vector<HNodePtr> vertical;
        for (size_t j = 0; j < col.higher.size(); ++j)
            vertical.push_back(bt3_to_height(col.lower, col.higher[j],
                oneTree->cols[0].higher[j], n));
        vstack.push_back(vertical);

        int p = iS;
        while (p >= 0)
        {
            if (!bt3_contains(outer, p) &&
                bt3_vertical_cmp(vstack[(size_t)p], vertical) < 0) break;
            p = (*(pstack[(size_t)p]))[n - 1];
        }
        (*(P->cols[i].parr))[n] = p;

        outer.push_back(iS);
        for (size_t j = 0; j < col.higher.size(); ++j)
            bt3_compute_top_parents(col.higher[j], P->cols[i].higher[j], n,
                pstack, vstack, outer);
        outer.pop_back();
    }
    pstack.resize(lS0);
    vstack.resize(lS0);
}
PNodePtr bt3_compute_parents(const NodePtr& e, int n)
{
    ColStack stack; ParStack pstack;
    PNodePtr P = bt3_lower(e, n, stack, pstack);
    std::vector<std::vector<HNodePtr>> vstack;
    std::deque<int> outer;
    bt3_compute_top_parents(e, P, n, pstack, vstack, outer);
    return P;
}

// ---- 层 / 根 ----
template <class T>
const T* bt3_next_layer(const T* cur)
{
    const int right = (int)cur->cols.size() - 1;
    const int higher_right = (int)cur->cols[(size_t)right].higher.size() - 1;
    return cur->cols[(size_t)right].higher[(size_t)higher_right].get();
}
template <class T>
const T* bt3_skip_layer(const T* cur, int layer)
{
    for (int i = 0; i < layer; ++i) cur = bt3_next_layer(cur);
    return cur;
}
std::pair<int, int> bt3_compute_tail(const NodePtr& e)  // {t, t_layer}
{
    if (e->cols.empty() || bt3_is_zero_column(e->cols.back())) return { -1, -1 };
    const NodeC* cur = e.get();
    int layer = 0, len = 0;
    while (true)
    {
        const int right = (int)cur->cols.size() - 1;
        const ColumnC& col = cur->cols[(size_t)right];
        const int higher_right = (int)col.higher.size() - 1;
        if (higher_right == -1) return { len + right, layer };
        if (col.higher[(size_t)higher_right]->cols.empty()) return { len + right, layer };
        len += (int)cur->cols.size();
        ++layer;
        cur = col.higher[(size_t)higher_right].get();
    }
}
std::pair<int, int> bt3_root_layer(const NodePtr& e, int r)
{
    int layer = 0, len = 0;
    const NodeC* cur = e.get();
    while (len + (int)cur->cols.size() <= r)
    {
        ++layer;
        len += (int)cur->cols.size();
        cur = bt3_next_layer(cur);
    }
    return { layer, r - len };
}
std::pair<int, int> bt3_root(const NodePtr& e, const PNodePtr& Pin, int t_layer)
{
    if (e->cols.empty() || bt3_is_zero_column(e->cols.back())) return { -1, -1 };
    const PNodeC* tail_P = bt3_skip_layer(Pin.get(), t_layer);
    const int right = (int)tail_P->cols.size() - 1;
    const std::vector<int>& parents = *tail_P->cols[(size_t)right].parr;
    const int b = bt3_last_nonneg(parents);
    if (b < 0) return { -1, -1 };
    return { parents[(size_t)b], b };
}
std::vector<int> bt3_ascension_vector(const NodePtr& e, int b, int r_layer, int r_index, int t_layer)
{
    const NodeC* r_e = bt3_skip_layer(e.get(), r_layer);
    const NodeC* t_e = bt3_skip_layer(r_e, t_layer - r_layer);
    const std::vector<int>& col_r = r_e->cols[(size_t)r_index].lower;
    const std::vector<int>& col_t = t_e->cols.back().lower;
    std::vector<int> v((size_t)b);
    for (int j = 0; j < b; ++j) v[(size_t)j] = col_t[(size_t)j] - col_r[(size_t)j];
    return v;
}
inline std::vector<int> bt3_ascend_vector(const std::vector<int>& v, int A, const std::vector<int>& V, int w)
{
    std::vector<int> out = v;
    for (int j = 0; j < A && j < (int)V.size(); ++j) out[(size_t)j] += V[(size_t)j] * w;
    return out;
}

// ---- special ----
bool bt3_is_special(const NodeC* e, int tail_layer)
{
    if (tail_layer == 0) return false;
    if (tail_layer > 1) return bt3_is_special(bt3_skip_layer(e, tail_layer - 1), 1);
    const ColumnC& tail = e->cols.back();
    const NodeC* next = bt3_next_layer(e);
    const ColumnC& next_tail = next->cols.back();
    if (!next_tail.higher.empty()) return false;
    if (bt3_last_pos(next_tail.lower) != 0) return false;
    return next_tail.lower[0] == tail.lower[0] + 1;
}
inline bool bt3_is_special(const NodePtr& e, int tail_layer)
{
    return bt3_is_special(e.get(), tail_layer);
}
NodePtr bt3_fs_special(const NodePtr& e, int tail_layer, int index)
{
    const int right = (int)e->cols.size() - 1;
    const int higher_right = (int)e->cols[(size_t)right].higher.size() - 1;
    auto out = bt3_make_node();
    for (int i = 0; i < right; ++i) out->cols.push_back(e->cols[(size_t)i]);
    ColumnC nc;
    nc.lower = e->cols[(size_t)right].lower;
    for (int k = 0; k < higher_right; ++k) nc.higher.push_back(e->cols[(size_t)right].higher[(size_t)k]);
    if (tail_layer == 1)
    {
        const NodePtr& h = e->cols[(size_t)right].higher[(size_t)higher_right];
        const int vert_right = (int)h->cols.size() - 1;
        auto new_vert = bt3_make_node();
        for (int k = 0; k < vert_right; ++k) new_vert->cols.push_back(h->cols[(size_t)k]);
        for (int k = 0; k < index; ++k) nc.higher.push_back(new_vert);
    }
    else
    {
        nc.higher.push_back(bt3_fs_special(
            e->cols[(size_t)right].higher[(size_t)higher_right], tail_layer - 1, index));
    }
    out->cols.push_back(std::move(nc));
    return out;
}

// ---- 复制提升 ----
inline NodePtr bt3_clone_slice_from_raw(const NodeC* e, int from)
{
    auto out = bt3_make_node();
    out->inf = e->inf;
    for (size_t i = from; i < e->cols.size(); ++i) out->cols.push_back(e->cols[i]);
    return out;
}
inline NodePtr bt3_pop_last(const NodePtr& e)
{
    auto out = bt3_make_node();
    out->inf = e->inf;
    for (size_t i = 0; i + 1 < e->cols.size(); ++i) out->cols.push_back(e->cols[i]);
    return out;
}
inline ANodePtr bt3_a_slice(const ANodeC* A, int from)
{
    auto out = std::make_shared<ANodeC>();
    for (size_t i = from; i < A->cols.size(); ++i) out->cols.push_back(A->cols[i]);
    return out;
}
NodePtr bt3_ascend_replace(const NodePtr& e, const NodePtr& tail,
                           std::optional<int> tl, const ANodePtr& A,
                           const std::vector<int>& V, int w,
                           const std::vector<HNodePtr>& critical_vert)
{
    auto out = bt3_make_node();
    for (size_t i = 0; i < e->cols.size(); ++i)
    {
        if (tl.has_value() && *tl == 0 && i + 1 == e->cols.size())
        {
            for (const ColumnC& tc : tail->cols) out->cols.push_back(tc);
        }
        else
        {
            const ColumnC& col = e->cols[i];
            const int Ai = (A && i < A->cols.size()) ? A->cols[i].threshold : 0;
            const int higher_right = (int)col.higher.size() - 1;
            const std::vector<int> new_lower = bt3_ascend_vector(col.lower, Ai, V, w);
            std::optional<int> ntl;
            if (i + 1 == e->cols.size() && tl.has_value()) ntl = *tl - 1;
            const int n = (int)col.lower.size();
            const std::vector<HNodePtr> col_vertical = bt3_compute_vertical(col, n);
            int m = 0;
            while (m < (int)col_vertical.size() && m < (int)critical_vert.size())
            {
                if (m == (int)col_vertical.size() - 1 && ntl.has_value()) break;
                const int cmp = bt3_height_cmp(col_vertical[(size_t)m], critical_vert[(size_t)m]);
                if (cmp > 0) break;
                if (cmp < 0)
                {
                    if (ntl.has_value()) m = (int)col_vertical.size() - 1;
                    else m = (int)col_vertical.size();
                    break;
                }
                ++m;
            }
            ColumnC nc;
            nc.lower = new_lower;
            for (int k = 0; k < m; ++k)
                nc.higher.push_back(bt3_from_height(new_lower, col_vertical[(size_t)k], n));
            for (int j = m; j < (int)col_vertical.size(); ++j)
            {
                std::optional<int> jtl;
                if (j == higher_right && ntl.has_value()) jtl = ntl;
                nc.higher.push_back(bt3_ascend_replace(col.higher[(size_t)j], tail, jtl,
                    (A && i < A->cols.size() && (size_t)j < A->cols[i].higher.size())
                        ? A->cols[i].higher[(size_t)j] : nullptr,
                    V, w, critical_vert));
            }
            out->cols.push_back(std::move(nc));
        }
    }
    return out;
}

NodePtr bt3_expand(const NodePtr& e, int index, int n, bool lnz_m1)
{
    if (bt3_is_infinity(e)) return bt3_infinity_fs(index, n);
    if (e->cols.empty()) return e;
    if (!bt3_is_limit(e)) return bt3_pop_last(e);

    PNodePtr P = bt3_compute_parents(e, n);
    const auto [t, t_layer] = bt3_compute_tail(e);
    (void)t;
    const auto rb = bt3_root(e, P, t_layer);
    const int r = rb.first;
    const int b = rb.second;
    if (bt3_is_special(e, t_layer)) return bt3_fs_special(e, t_layer, index);
    const auto rl = bt3_root_layer(e, r);
    const int r_layer = rl.first;
    const int r_index = rl.second;
    std::vector<int> stk;
    const ANodePtr A = bt3_thresholds(e, P, r, b, stk);
    const std::vector<int> V = bt3_ascension_vector(e, b, r_layer, r_index, t_layer);

    const NodeC* r_e = bt3_skip_layer(e.get(), r_layer);
    NodePtr copy_part = bt3_clone_slice_from_raw(r_e, r_index);
    const ANodeC* r_A = bt3_skip_layer(A.get(), r_layer);
    ANodePtr copy_part_A = bt3_a_slice(r_A, r_index);

    const NodeC* e_t = bt3_skip_layer(e.get(), t_layer);
    const int tail_right = (int)e_t->cols.size() - 1;
    const ColumnC& tail = e_t->cols[(size_t)tail_right];
    std::vector<HNodePtr> critical_vert = bt3_compute_vertical(tail, n);
    if (!critical_vert.empty()) critical_vert.pop_back();  // slice(0, -1)

    NodePtr result = bt3_make_node();
    if (lnz_m1 && index > 0)
    {
        const bool skip_2 = copy_part->cols.size() == 1 ||
            (copy_part->cols.size() == 2 && r_layer == t_layer);
        bool skip_1;
        if (b == n) skip_1 = true;
        else
        {
            const ColumnC& root_column = copy_part->cols[0];
            if (b == n - 1) skip_1 = root_column.higher.empty();
            else skip_1 = root_column.lower[(size_t)(b + 1)] == 0;
        }
        if (!skip_1 && index > 0)
        {
            if (index == 1)
            {
                if (b == n)
                {
                    ColumnC c;
                    c.lower = tail.lower;
                    for (size_t k = 0; k + 1 < tail.higher.size(); ++k)
                        c.higher.push_back(tail.higher[k]);
                    result->cols.push_back(std::move(c));
                }
                else
                {
                    ColumnC c;
                    c.lower = tail.lower;
                    if (b < (int)c.lower.size()) c.lower[(size_t)b] -= 1;
                    result->cols.push_back(std::move(c));
                }
            }
            --index;
        }
        if (!skip_2 && index > 0)
        {
            if (index == 1)
            {
                auto one = bt3_make_node();
                one->cols.push_back(copy_part->cols[0]);
                auto oneA = std::make_shared<ANodeC>();
                oneA->cols.push_back(copy_part_A->cols[0]);
                result = bt3_ascend_replace(one, bt3_make_node(), std::nullopt, oneA, V, 1, {});
                if (b == n)
                {
                    ColumnC c0;
                    c0.lower = result->cols[0].lower;
                    for (const HNodePtr& x : critical_vert)
                        c0.higher.push_back(bt3_from_height(result->cols[0].lower, x, n));
                    result->cols[0] = std::move(c0);
                }
            }
            --index;
        }
    }
    for (int w = index; w > 0; --w)
    {
        result = bt3_ascend_replace(copy_part, result, t_layer - r_layer, copy_part_A, V, w, critical_vert);
        if (b == n)
        {
            ColumnC c0;
            c0.lower = result->cols[0].lower;
            for (const HNodePtr& x : critical_vert)
                c0.higher.push_back(bt3_from_height(result->cols[0].lower, x, n));
            result->cols[0] = std::move(c0);
        }
    }
    result = bt3_ascend_replace(e, result, t_layer, A, V, 0, {});
    return result;
}

inline NodePtr bt3_lnz(bool short_, const NodePtr& e, int index, int n)
{
    if (bt3_is_infinity(e)) return bt3_infinity_fs(index, n);
    if (!bt3_is_limit(e)) return bt3_expand(e, 0, n, short_);
    return bt3_expand(e, index, n, short_);
}

// ---- 解析（* / ∗ 与 ^ 语法；数字部分必须凑满 n 个）----
struct Parser
{
    explicit Parser(std::string_view s) : s_(s) {}
    std::string_view s_;
    size_t i_ = 0;
    int n_ = 0;
    [[noreturn]] void err() { throw std::runtime_error("illegal"); }
    void sp() { while (i_ < s_.size() && s_[i_] == ' ') ++i_; }
    int num_tok()
    {
        sp();
        const size_t st = i_;
        while (i_ < s_.size() && s_[i_] >= '0' && s_[i_] <= '9') ++i_;
        if (st == i_) err();
        int v = 0;
        for (size_t k = st; k < i_; ++k) v = v * 10 + (s_[k] - '0');
        return v;
    }
    NodePtr parse_higher()
    {
        if (i_ >= s_.size() || (s_[i_] != '*' && s_[i_] != '\u2217')) err();
        ++i_;
        sp();
        if (i_ < s_.size() && s_[i_] == '^') { ++i_; return parse_expr(); }
        return bt3_make_node();
    }
    ColumnC parse_column()
    {
        sp();
        if (i_ >= s_.size() || s_[i_] != '(') err();
        ++i_;
        std::vector<int> numbers;
        std::vector<NodePtr> higher;
        sp();
        while (i_ < s_.size() && s_[i_] != ')' && s_[i_] >= '0' && s_[i_] <= '9' &&
               numbers.size() < (size_t)n_)
        {
            numbers.push_back(num_tok());
            sp();
            if (i_ < s_.size() && s_[i_] == ',') ++i_;
            sp();
        }
        while (i_ < s_.size() && s_[i_] != ')')
        {
            sp();
            if (s_[i_] == '*' || s_[i_] == '\u2217')
            {
                if (numbers.size() != (size_t)n_) err();
                higher.push_back(parse_higher());
            }
            else err();
            sp();
            if (i_ < s_.size() && s_[i_] == ',') ++i_;
        }
        if (i_ >= s_.size()) err();
        ++i_;
        ColumnC c;
        c.lower = numbers;
        while (c.lower.size() < (size_t)n_) c.lower.push_back(0);
        c.higher = std::move(higher);
        return c;
    }
    NodePtr parse_expr()
    {
        auto out = bt3_make_node();
        sp();
        while (i_ < s_.size() && s_[i_] == '(')
        {
            out->cols.push_back(parse_column());
            sp();
        }
        return out;
    }
    NodePtr parse(int n)
    {
        n_ = n;
        sp();
        if (i_ + 5 <= s_.size() && s_.substr(i_, 5) == "Limit")
        {
            i_ += 5;
            sp();
            if (i_ != s_.size()) err();
            return bt3_infinity();
        }
        NodePtr out = parse_expr();
        sp();
        if (i_ != s_.size()) err();
        return out;
    }
};

struct BT3Notation
{
    static std::string suffix() { return std::string{}; }
    static constexpr int kParamMin = 1;
    static constexpr int kParamMax = 25;
    static std::string expand_text_n(std::string_view text, int term, int n)
    {
        try
        {
            n = (std::max)(kParamMin, (std::min)(kParamMax, n));
            if (term < 1) term = 1;
            Parser parser(text);
            NodePtr m = parser.parse(n);
            return bt3_display(bt3_lnz(false, m, term, n));
        }
        catch (...)
        {
            return bmlike::kParseError;
        }
    }
    static std::string expand_text_n_alter(std::string_view text, int term, int n)
    {
        try
        {
            n = (std::max)(kParamMin, (std::min)(kParamMax, n));
            if (term < 1) term = 1;
            Parser parser(text);
            NodePtr m = parser.parse(n);
            return bt3_display(bt3_lnz(true, m, term, n));
        }
        catch (...)
        {
            return bmlike::kParseError;
        }
    }
};
}  // namespace bt3_detail

using bt3_detail::BT3Notation;
}  // namespace omegay::notation

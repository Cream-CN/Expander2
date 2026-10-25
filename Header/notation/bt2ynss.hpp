// BT_star_Minus1_Y_nSS-v2（strong）C++ 移植。
//
// 结构 / 显示 / 层 / 根 / 提升 / FS_special 与 v1 相同（higher[] 为多个子表达式）；
// 唯一区别是顶层父求法：先 compute_lower_parents 得到下层父，再 fill_top_parents
// 按“相对差分 key”（to_relative + rel_compare，排除 outer_stack）回填第 n 个父。
// start=1（n>=1），无 n===0 步进分支。
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
namespace bt2_detail
{
struct NodeC;
using NodePtr = std::shared_ptr<NodeC>;
struct ColumnC { std::vector<int> arr; std::vector<NodePtr> higher; };
struct NodeC { bool inf = false; std::vector<ColumnC> cols; };

inline NodePtr bt2_make_node() { return std::make_shared<NodeC>(); }
inline NodePtr bt2_infinity() { auto e = bt2_make_node(); e->inf = true; return e; }
inline bool bt2_is_infinity(const NodePtr& e) { return e->inf; }

inline NodePtr bt2_infinity_fs(int index, int n)
{
    NodePtr chain = bt2_make_node();
    for (int i = index; i > 0; --i)
    {
        auto wrapped = bt2_make_node();
        ColumnC c; c.arr.assign(n, i); c.higher.push_back(chain);
        wrapped->cols.push_back(std::move(c));
        chain = wrapped;
    }
    auto full = bt2_make_node();
    full->cols.push_back(ColumnC{ std::vector<int>(n, 0), {} });
    for (const ColumnC& cc : chain->cols) full->cols.push_back(cc);
    return full;
}

inline bool bt2_is_zero_column(const ColumnC& c)
{
    for (int x : c.arr) if (x != 0) return false;
    return c.higher.empty();
}

inline int bt2_number_cmp(int a, int b) { return a == b ? 0 : a < b ? -1 : 1; }
inline int bt2_compare(const NodePtr& a, const NodePtr& b);
inline int bt2_column_compare(const ColumnC& a, const ColumnC& b);
inline int bt2_compare_shared_higher(const std::vector<NodePtr>& a, const std::vector<NodePtr>& b);
inline int bt2_compare(const NodePtr& a, const NodePtr& b)
{
    const size_t m = (std::min)(a->cols.size(), b->cols.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = bt2_column_compare(a->cols[i], b->cols[i]); c != 0) return c;
    return bt2_number_cmp((int)a->cols.size(), (int)b->cols.size());
}
inline int bt2_compare_shared_higher(const std::vector<NodePtr>& a, const std::vector<NodePtr>& b)
{
    const size_t m = (std::min)(a.size(), b.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = bt2_compare(a[i], b[i]); c != 0) return c;
    return bt2_number_cmp((int)a.size(), (int)b.size());
}
inline int bt2_column_compare(const ColumnC& a, const ColumnC& b)
{
    const size_t m = (std::min)(a.arr.size(), b.arr.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = bt2_number_cmp(a.arr[i], b.arr[i]); c != 0) return c;
    if (const int c = bt2_number_cmp((int)a.arr.size(), (int)b.arr.size()); c != 0) return c;
    return bt2_compare_shared_higher(a.higher, b.higher);
}

// ---- 显示（plain）----
inline std::string bt2_top_display(const NodePtr& e);
inline std::string bt2_display(const NodePtr& e);
inline std::string bt2_column_display(const ColumnC& c)
{
    std::vector<std::string> list;
    for (int x : c.arr) list.push_back(std::to_string(x));
    for (const NodePtr& x : c.higher) list.push_back(bt2_top_display(x));
    while (!list.empty() && list.back() == "0") list.pop_back();
    std::string s = "(";
    for (size_t i = 0; i < list.size(); ++i) { if (i) s += ","; s += list[i]; }
    s += ")";
    return s;
}
inline std::string bt2_top_display(const NodePtr& e)
{
    if (e->cols.empty()) return "*";
    return "*^" + bt2_display(e);
}
inline std::string bt2_display(const NodePtr& e)
{
    if (bt2_is_infinity(e)) return "Limit";
    std::string s;
    for (const ColumnC& c : e->cols) s += bt2_column_display(c);
    return s;
}

inline bool bt2_is_limit(const NodePtr& e)
{
    if (bt2_is_infinity(e)) return true;
    if (e->cols.empty()) return false;
    return !bt2_is_zero_column(e->cols.back());
}

inline bool bt2_is_one_line_column(const ColumnC& c, int value)
{
    if (c.arr.empty() || c.arr[0] != value) return false;
    for (size_t i = 1; i < c.arr.size(); ++i) if (c.arr[i] != 0) return false;
    return c.higher.empty();
}
inline bool bt2_is_special_column(const ColumnC& c)
{
    const int higher_right = (int)c.higher.size() - 1;
    if (higher_right < 0) return false;
    const NodePtr& h = c.higher[(size_t)higher_right];
    const int vert_right = (int)h->cols.size() - 1;
    if (vert_right < 0) return false;
    return bt2_is_one_line_column(h->cols[(size_t)vert_right],
        c.arr.empty() ? 0 : c.arr[0] + 1);
}

// ---- 父树 ----
struct PNodeC;
using PNodePtr = std::shared_ptr<PNodeC>;
struct PPColC { std::shared_ptr<std::vector<int>> parr; std::vector<PNodePtr> subs; };
struct PNodeC { std::vector<PPColC> cols; };

using ColStack = std::vector<const ColumnC*>;
using ParStack = std::vector<std::shared_ptr<std::vector<int>>>;

inline bool bt2_contains(const std::deque<int>& d, int v)
{
    for (int x : d) if (x == v) return true;
    return false;
}
inline int bt2_last_nonneg(const std::vector<int>& v)
{
    for (int i = (int)v.size() - 1; i >= 0; --i) if (v[i] >= 0) return i;
    return -1;
}

PNodePtr bt2_lower(const NodePtr& e, int n, ColStack& stack, ParStack& pstack)
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
                if (stack[(size_t)p]->arr[j] < col.arr[j]) break;
                p = (j == 0) ? p - 1 : (*(pstack[(size_t)p]))[j - 1];
            }
            if (p < 0) break;
            (*parr)[j] = p;
        }
        PPColC pc;
        pc.parr = parr;
        for (const NodePtr& x : col.higher)
            pc.subs.push_back(bt2_lower(x, n, stack, pstack));
        P->cols.push_back(std::move(pc));
    }
    stack.resize(lS0);
    pstack.resize(lS0);
    return P;
}

// ---- 相对 key ----
struct ANodeC;
using ANodePtr = std::shared_ptr<ANodeC>;
struct AColC { std::optional<int> a; std::vector<ANodePtr> subs; };
struct ANodeC { std::vector<AColC> cols; };

ANodePtr bt2_undefined_at(const NodePtr& e)
{
    auto A = std::make_shared<ANodeC>();
    for (const ColumnC& col : e->cols)
    {
        AColC ac; ac.a = std::nullopt;
        for (const NodePtr& x : col.higher) ac.subs.push_back(bt2_undefined_at(x));
        A->cols.push_back(std::move(ac));
    }
    return A;
}
ANodePtr bt2_thresholds(const NodePtr& e, const PNodePtr& P,
                        std::optional<int> ropt, int b,
                        std::vector<std::optional<int>>& stk)
{
    auto A = std::make_shared<ANodeC>();
    if (!ropt.has_value()) return bt2_undefined_at(e);
    const int r = *ropt;
    const size_t lS0 = stk.size();
    for (size_t i = 0; i < e->cols.size(); ++i)
    {
        const int iS = (int)stk.size();
        AColC ac;
        if (iS < r && i + 1 != e->cols.size())
        {
            stk.push_back(std::nullopt);
            ac.a = std::nullopt;
            for (const NodePtr& x : e->cols[i].higher)
                ac.subs.push_back(bt2_undefined_at(x));
        }
        else
        {
            std::optional<int> Ai;
            if (iS == r) Ai = b;
            else if (iS > r)
            {
                int ai = 0;
                const std::vector<int>& parr = *P->cols[i].parr;
                while (ai < (int)parr.size() && parr[(size_t)ai] >= r &&
                       parr[(size_t)ai] < (int)stk.size() &&
                       stk[(size_t)parr[(size_t)ai]].has_value() &&
                       *stk[(size_t)parr[(size_t)ai]] > ai) ++ai;
                Ai = ai;
            }
            stk.push_back(Ai);
            ac.a = Ai;
            for (size_t ix = 0; ix < e->cols[i].higher.size(); ++ix)
                ac.subs.push_back(bt2_thresholds(e->cols[i].higher[ix],
                    P && i < P->cols.size() && ix < P->cols[i].subs.size() ? P->cols[i].subs[ix] : nullptr,
                    r, b, stk));
        }
        A->cols.push_back(std::move(ac));
    }
    stk.resize(lS0);
    return A;
}

struct RKeyC;
using RKeyPtr = std::shared_ptr<RKeyC>;
struct RKeyColC { std::vector<std::pair<bool, int>> low; std::vector<RKeyPtr> subs; };
struct RKeyC { std::vector<RKeyColC> cols; };

inline int bt2_bool_cmp(bool a, bool b) { return (a ? 1 : 0) - (b ? 1 : 0); }
inline int bt2_rel_compare(const RKeyPtr& a, const RKeyPtr& b);
inline int bt2_rel_compare_higher(const std::vector<RKeyPtr>& a, const std::vector<RKeyPtr>& b);
inline int bt2_rel_compare(const RKeyPtr& a, const RKeyPtr& b)
{
    const size_t m = (std::min)(a->cols.size(), b->cols.size());
    for (size_t i = 0; i < m; ++i)
    {
        const RKeyColC& x = a->cols[i];
        const RKeyColC& y = b->cols[i];
        const size_t km = (std::min)(x.low.size(), y.low.size());
        for (size_t j = 0; j < km; ++j)
        {
            if (const int c = bt2_bool_cmp(x.low[j].first, y.low[j].first); c != 0) return c;
            if (const int c = bt2_number_cmp(x.low[j].second, y.low[j].second); c != 0) return c;
        }
        if (const int c = bt2_number_cmp((int)x.low.size(), (int)y.low.size()); c != 0) return c;
        if (const int c = bt2_rel_compare_higher(x.subs, y.subs); c != 0) return c;
    }
    return bt2_number_cmp((int)a->cols.size(), (int)b->cols.size());
}
inline int bt2_rel_compare_higher(const std::vector<RKeyPtr>& a, const std::vector<RKeyPtr>& b)
{
    const size_t m = (std::min)(a.size(), b.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = bt2_rel_compare(a[i], b[i]); c != 0) return c;
    return bt2_number_cmp((int)a.size(), (int)b.size());
}

RKeyPtr bt2_to_relative(const ColumnC& base, const NodePtr& cur, const ANodePtr& T, int n)
{
    auto K = std::make_shared<RKeyC>();
    for (size_t i = 0; i < cur->cols.size(); ++i)
    {
        const ColumnC& col = cur->cols[i];
        const std::optional<int> t =
            (T && i < T->cols.size()) ? T->cols[i].a : std::nullopt;
        const int tv = t.value_or(-1);
        RKeyColC kc;
        kc.low.reserve(n);
        for (int j = 0; j < n; ++j)
        {
            if (j < tv) kc.low.push_back({ true, col.arr[j] - base.arr[j] });
            else kc.low.push_back({ false, col.arr[j] });
        }
        for (size_t j = 0; j < col.higher.size(); ++j)
        {
            const ANodePtr tc = (T && i < T->cols.size() && j < T->cols[i].subs.size())
                ? T->cols[i].subs[j] : nullptr;
            kc.subs.push_back(bt2_to_relative(base, col.higher[j], tc, n));
        }
        K->cols.push_back(std::move(kc));
    }
    return K;
}

void bt2_fill_top(const NodePtr& e, const PNodePtr& P, int n,
                  ParStack& pstack, std::vector<std::vector<RKeyPtr>>& keys,
                  std::deque<int>& outer)
{
    const size_t lS0 = pstack.size();
    for (size_t i = 0; i < e->cols.size(); ++i)
    {
        const ColumnC& col = e->cols[i];
        const int iS = (int)pstack.size();
        pstack.push_back(P->cols[i].parr);

        // oneTree = ascension_thresholds([col], [Pi], iS, n, Array(iS).fill(undefined))
        auto wrapE = bt2_make_node();
        wrapE->cols.push_back(col);
        auto wrapP = std::make_shared<PNodeC>();
        PPColC wpc;
        wpc.parr = P->cols[i].parr;
        wpc.subs = P->cols[i].subs;
        wrapP->cols.push_back(std::move(wpc));
        std::vector<std::optional<int>> stk((size_t)iS, std::nullopt);
        const ANodePtr oneTree = bt2_thresholds(wrapE, wrapP, iS, n, stk);
        // AT = oneTree[0][1]（higher 数组的阈值子树）
        std::vector<RKeyPtr> key;
        for (size_t j = 0; j < col.higher.size(); ++j)
        {
            const ANodePtr atj = oneTree->cols[0].subs[j];
            key.push_back(bt2_to_relative(col, col.higher[j], atj, n));
        }
        keys.push_back(std::move(key));

        int p = iS;
        while (p >= 0)
        {
            if (!bt2_contains(outer, p) &&
                bt2_rel_compare_higher(keys[(size_t)p], keys[(size_t)iS]) < 0) break;
            p = (*(pstack[(size_t)p]))[n - 1];
        }
        (*(P->cols[i].parr))[n] = p;

        outer.push_back(iS);
        for (size_t j = 0; j < col.higher.size(); ++j)
            bt2_fill_top(col.higher[j], P->cols[i].subs[j], n, pstack, keys, outer);
        outer.pop_back();
    }
    pstack.resize(lS0);
    keys.resize(lS0);
}

PNodePtr bt2_compute_parents(const NodePtr& e, int n)
{
    ColStack stack; ParStack pstack;
    PNodePtr P = bt2_lower(e, n, stack, pstack);
    std::vector<std::vector<RKeyPtr>> keys;
    std::deque<int> outer;
    bt2_fill_top(e, P, n, pstack, keys, outer);
    return P;
}

// ---- 层 / 根 / 提升向量 ----
std::pair<int, bool> bt2_tail_layer(const NodePtr& e)
{
    if (e->cols.empty() || bt2_is_zero_column(e->cols.back())) return { -1, false };
    const NodeC* cur = e.get();
    int layer = 0;
    while (true)
    {
        const int right = (int)cur->cols.size() - 1;
        const ColumnC& col = cur->cols[(size_t)right];
        const int higher_right = (int)col.higher.size() - 1;
        if (col.higher.empty()) return { layer, false };
        if (bt2_is_special_column(col)) return { layer, true };
        if (col.higher[(size_t)higher_right]->cols.empty()) return { layer, false };
        cur = col.higher[(size_t)higher_right].get();
        ++layer;
    }
}
std::pair<int, int> bt2_root_layer(const NodePtr& e, int r)
{
    int layer = 0, len = (int)e->cols.size();
    const NodeC* cur = e.get();
    while (len <= r)
    {
        ++layer;
        const int right = (int)cur->cols.size() - 1;
        const ColumnC& col = cur->cols[(size_t)right];
        const int higher_right = (int)col.higher.size() - 1;
        cur = col.higher[(size_t)higher_right].get();
        len += (int)cur->cols.size();
    }
    return { layer, r - (len - (int)cur->cols.size()) };
}
PNodePtr bt2_descend_p(PNodePtr P, int tailLayer)
{
    for (int k = 0; k < tailLayer; ++k)
    {
        const int right = (int)P->cols.size() - 1;
        const PPColC& pc = P->cols[(size_t)right];
        const int higher_right = (int)pc.subs.size() - 1;
        P = pc.subs[(size_t)higher_right];
    }
    return P;
}
std::pair<int, int> bt2_root(const NodePtr& e, const PNodePtr& Pin, int tailLayer)
{
    if (e->cols.empty() || bt2_is_zero_column(e->cols.back())) return { -1, -1 };
    PNodePtr P = bt2_descend_p(Pin, tailLayer);
    const int right = (int)P->cols.size() - 1;
    const std::vector<int>& parr = *P->cols[(size_t)right].parr;
    const int b = bt2_last_nonneg(parr);
    if (b < 0) return { -1, -1 };
    return { parr[(size_t)b], b };
}
std::vector<int> bt2_ascension_vector(const NodePtr& e, int r, int b, int tailLayer)
{
    std::vector<const ColumnC*> st;
    for (const ColumnC& c : e->cols) st.push_back(&c);
    const NodeC* cur = e.get();
    for (int k = 0; k < tailLayer; ++k)
    {
        const int right = (int)cur->cols.size() - 1;
        const ColumnC& col = cur->cols[(size_t)right];
        const int higher_right = (int)col.higher.size() - 1;
        cur = col.higher[(size_t)higher_right].get();
        for (const ColumnC& c : cur->cols) st.push_back(&c);
    }
    std::vector<int> v(b, 0);
    for (int j = 0; j < b; ++j)
        v[j] = st.back()->arr[j] - st[(size_t)r]->arr[j];
    return v;
}

inline std::vector<int> bt2_ascend_vector(const std::vector<int>& v, int A, const std::vector<int>& V, int w)
{
    std::vector<int> out = v;
    for (int j = 0; j < A && j < (int)V.size(); ++j) out[(size_t)j] += V[(size_t)j] * w;
    return out;
}

// ---- 复制提升 ----
inline NodePtr bt2_clone_slice_from_raw(const NodeC* e, int from)
{
    auto out = bt2_make_node();
    out->inf = e->inf;
    for (size_t i = from; i < e->cols.size(); ++i) out->cols.push_back(e->cols[i]);
    return out;
}
inline NodePtr bt2_clone_slice(const NodePtr& e, int from)
{
    return bt2_clone_slice_from_raw(e.get(), from);
}
inline NodePtr bt2_pop_last(const NodePtr& e)
{
    auto out = bt2_make_node();
    out->inf = e->inf;
    for (size_t i = 0; i + 1 < e->cols.size(); ++i) out->cols.push_back(e->cols[i]);
    return out;
}
inline ANodePtr bt2_a_slice(const ANodePtr& A, int from)
{
    auto out = std::make_shared<ANodeC>();
    for (size_t i = from; i < A->cols.size(); ++i) out->cols.push_back(A->cols[i]);
    return out;
}
inline ANodePtr bt2_a_pop_last(const ANodePtr& A)
{
    auto out = std::make_shared<ANodeC>();
    for (size_t i = 0; i + 1 < A->cols.size(); ++i) out->cols.push_back(A->cols[i]);
    return out;
}
NodePtr bt2_ascend_replace(const NodePtr& e, const NodePtr& tail,
                           std::optional<int> tl, const ANodePtr& A,
                           const std::vector<int>& V, int w)
{
    auto out = bt2_make_node();
    for (size_t i = 0; i < e->cols.size(); ++i)
    {
        if (tl.has_value() && *tl == 0 && i + 1 == e->cols.size())
        {
            for (const ColumnC& tc : tail->cols) out->cols.push_back(tc);
        }
        else
        {
            const ColumnC& col = e->cols[i];
            ColumnC nc;
            nc.arr = col.arr;
            const int Ai = (A && i < A->cols.size() && A->cols[i].a.has_value())
                ? *A->cols[i].a : 0;
            nc.arr = bt2_ascend_vector(col.arr, Ai, V, w);
            std::optional<int> ntl;
            if (i + 1 == e->cols.size() && tl.has_value()) ntl = *tl - 1;
            const int higher_right = (int)col.higher.size() - 1;
            for (size_t ix = 0; ix < col.higher.size(); ++ix)
            {
                std::optional<int> xntl;
                if ((int)ix == higher_right && ntl.has_value()) xntl = ntl;
                nc.higher.push_back(bt2_ascend_replace(col.higher[ix], tail, xntl,
                    (A && i < A->cols.size() && ix < A->cols[i].subs.size())
                        ? A->cols[i].subs[ix] : nullptr, V, w));
            }
            out->cols.push_back(std::move(nc));
        }
    }
    return out;
}

NodePtr bt2_fs_special(const NodePtr& e, int tail_layer, int index)
{
    const int right = (int)e->cols.size() - 1;
    const int higher_right = (int)e->cols[(size_t)right].higher.size() - 1;
    auto out = bt2_make_node();
    for (int i = 0; i < right; ++i) out->cols.push_back(e->cols[(size_t)i]);
    ColumnC nc;
    nc.arr = e->cols[(size_t)right].arr;
    for (int k = 0; k < higher_right; ++k)
        nc.higher.push_back(e->cols[(size_t)right].higher[(size_t)k]);
    if (tail_layer == 0)
    {
        const NodePtr& h = e->cols[(size_t)right].higher[(size_t)higher_right];
        const int vert_right = (int)h->cols.size() - 1;
        NodePtr new_vert = bt2_clone_slice(h, 0);
        new_vert->cols.resize((size_t)vert_right);
        for (int k = 0; k < index; ++k) nc.higher.push_back(new_vert);
    }
    else
    {
        nc.higher.push_back(bt2_fs_special(
            e->cols[(size_t)right].higher[(size_t)higher_right], tail_layer - 1, index));
    }
    out->cols.push_back(std::move(nc));
    return out;
}

NodePtr bt2_fs(const NodePtr& e, int index, int n)
{
    if (bt2_is_infinity(e)) return bt2_infinity_fs(index, n);
    if (e->cols.empty()) return e;
    if (!bt2_is_limit(e)) return bt2_pop_last(e);

    PNodePtr P = bt2_compute_parents(e, n);
    const auto tl = bt2_tail_layer(e);
    const int t_layer = tl.first;
    const bool is_special = tl.second;
    if (is_special) return bt2_fs_special(e, t_layer, index);

    const auto rb = bt2_root(e, P, t_layer);
    const int r = rb.first;
    const int b = rb.second;
    const bool hasRoot = (b >= 0);

    int r_layer = 0, ri = 0;
    if (hasRoot)
    {
        const auto rl = bt2_root_layer(e, r);
        r_layer = rl.first;
        ri = rl.second;
    }
    std::vector<std::optional<int>> stk;
    const ANodePtr A = bt2_thresholds(e, P,
        hasRoot ? std::optional<int>(r) : std::nullopt, b, stk);
    const std::vector<int> V = hasRoot
        ? bt2_ascension_vector(e, r, b, t_layer) : std::vector<int>{};

    const NodeC* cur = e.get();
    ANodePtr curA = A;
    for (int k = 0; k < r_layer; ++k)
    {
        const int right = (int)cur->cols.size() - 1;
        const int higher_right = (int)cur->cols[(size_t)right].higher.size() - 1;
        cur = cur->cols[(size_t)right].higher[(size_t)higher_right].get();
        curA = curA->cols[(size_t)right].subs[(size_t)higher_right];
    }
    NodePtr copy_part = bt2_clone_slice_from_raw(cur, ri);
    ANodePtr copy_partA = bt2_a_slice(curA, ri);
    for (int k = r_layer; k < t_layer; ++k)
    {
        const int right = (int)cur->cols.size() - 1;
        const int higher_right = (int)cur->cols[(size_t)right].higher.size() - 1;
        cur = cur->cols[(size_t)right].higher[(size_t)higher_right].get();
        curA = curA->cols[(size_t)right].subs[(size_t)higher_right];
    }
    const int right = (int)cur->cols.size() - 1;
    std::vector<NodePtr> tail_top = cur->cols[(size_t)right].higher;
    if (!tail_top.empty()) tail_top.pop_back();
    std::vector<ANodePtr> tail_topA = curA->cols[(size_t)right].subs;
    if (!tail_topA.empty()) tail_topA.pop_back();

    NodePtr result = bt2_make_node();
    for (int w = index; w > 0; --w)
    {
        result = bt2_ascend_replace(copy_part, result, t_layer - r_layer,
                                    copy_partA, V, w);
        if (b == n)
        {
            ColumnC c0;
            c0.arr = result->cols[0].arr;
            for (size_t ix = 0; ix < tail_top.size(); ++ix)
                c0.higher.push_back(bt2_ascend_replace(tail_top[ix], bt2_make_node(),
                    std::nullopt, tail_topA[ix], V, w - 1));
            result->cols[0] = std::move(c0);
        }
    }
    result = bt2_ascend_replace(e, result, t_layer, A, V, 0);
    return result;
}

inline NodePtr bt2_lnz(const NodePtr& e, int index, int n)
{
    if (bt2_is_infinity(e)) return bt2_infinity_fs(index, n);
    if (!bt2_is_limit(e)) return bt2_fs(e, 0, n);
    return bt2_fs(e, index, n);
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
        return bt2_make_node();
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
        c.arr = numbers;
        while (c.arr.size() < (size_t)n_) c.arr.push_back(0);
        c.higher = std::move(higher);
        return c;
    }
    NodePtr parse_expr()
    {
        auto out = bt2_make_node();
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
            return bt2_infinity();
        }
        NodePtr out = parse_expr();
        sp();
        if (i_ != s_.size()) err();
        return out;
    }
};

struct BT2Notation
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
            return bt2_display(bt2_lnz(m, term, n));
        }
        catch (...)
        {
            return bmlike::kParseError;
        }
    }
};
}  // namespace bt2_detail

using bt2_detail::BT2Notation;
}  // namespace omegay::notation

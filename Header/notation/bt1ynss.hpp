// BT_star_Minus1_Y_nSS（Bubby3's Transfinite* -1Y-nSS v1）C++ 移植。
//
// 与 T/Abs/Rel 的关键结构差异：每列的子结构是“多个子表达式”的数组（higher[]），
// 而不是单个 step。显示用 *^expr 前缀（或裸 * 表示空子表达式），无 ω/数字简写。
// 第 n 个（顶层）父的比较用 highest_without_base（对 higher 去基化后的字典序），
// 并排除祖先链；is_special_column（末 higher 的末列是“基线+1”的单行列）触发
// FS_special 分支。start=1（n>=1）。
#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "bmlike_share.hpp"

namespace omegay::notation
{
namespace bt1_detail
{
struct NodeC;
using NodePtr = std::shared_ptr<NodeC>;
struct ColumnC { std::vector<int> arr; std::vector<NodePtr> higher; };
struct NodeC { bool inf = false; std::vector<ColumnC> cols; };

inline NodePtr bt1_make_node() { return std::make_shared<NodeC>(); }
inline NodePtr bt1_infinity() { auto e = bt1_make_node(); e->inf = true; return e; }
inline bool bt1_is_infinity(const NodePtr& e) { return e->inf; }

inline NodePtr bt1_infinity_fs(int index, int n)
{
    NodePtr chain = bt1_make_node();
    for (int i = index; i > 0; --i)
    {
        auto wrapped = bt1_make_node();
        ColumnC c; c.arr.assign(n, i); c.higher.push_back(chain);
        wrapped->cols.push_back(std::move(c));
        chain = wrapped;
    }
    auto full = bt1_make_node();
    full->cols.push_back(ColumnC{ std::vector<int>(n, 0), {} });
    for (const ColumnC& cc : chain->cols) full->cols.push_back(cc);
    return full;
}

inline bool bt1_is_zero_column(const ColumnC& c)
{
    for (int x : c.arr) if (x != 0) return false;
    return c.higher.empty();
}

inline int bt1_number_cmp(int a, int b) { return a == b ? 0 : a < b ? -1 : 1; }
inline int bt1_compare(const NodePtr& a, const NodePtr& b);
inline int bt1_column_compare(const ColumnC& a, const ColumnC& b);
inline int bt1_compare_shared_higher(const std::vector<NodePtr>& a, const std::vector<NodePtr>& b);
inline int bt1_compare(const NodePtr& a, const NodePtr& b)
{
    const size_t m = (std::min)(a->cols.size(), b->cols.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = bt1_column_compare(a->cols[i], b->cols[i]); c != 0) return c;
    return bt1_number_cmp((int)a->cols.size(), (int)b->cols.size());
}
inline int bt1_compare_shared_higher(const std::vector<NodePtr>& a, const std::vector<NodePtr>& b)
{
    const size_t m = (std::min)(a.size(), b.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = bt1_compare(a[i], b[i]); c != 0) return c;
    return bt1_number_cmp((int)a.size(), (int)b.size());
}
inline int bt1_column_compare(const ColumnC& a, const ColumnC& b)
{
    const size_t m = (std::min)(a.arr.size(), b.arr.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = bt1_number_cmp(a.arr[i], b.arr[i]); c != 0) return c;
    if (const int c = bt1_number_cmp((int)a.arr.size(), (int)b.arr.size()); c != 0) return c;
    return bt1_compare_shared_higher(a.higher, b.higher);
}

// ---- 显示（plain）----
inline std::string bt1_top_display(const NodePtr& e);
inline std::string bt1_display(const NodePtr& e);
inline std::string bt1_column_display(const ColumnC& c)
{
    std::vector<std::string> list;
    for (int x : c.arr) list.push_back(std::to_string(x));
    for (const NodePtr& x : c.higher) list.push_back(bt1_top_display(x));
    while (!list.empty() && list.back() == "0") list.pop_back();
    std::string s = "(";
    for (size_t i = 0; i < list.size(); ++i) { if (i) s += ","; s += list[i]; }
    s += ")";
    return s;
}
inline std::string bt1_top_display(const NodePtr& e)
{
    if (e->cols.empty()) return "*";
    return "*^" + bt1_display(e);
}
inline std::string bt1_display(const NodePtr& e)
{
    if (bt1_is_infinity(e)) return "Limit";
    std::string s;
    for (const ColumnC& c : e->cols) s += bt1_column_display(c);
    return s;
}

inline bool bt1_is_limit(const NodePtr& e)
{
    if (bt1_is_infinity(e)) return true;
    if (e->cols.empty()) return false;
    return !bt1_is_zero_column(e->cols.back());
}

// ---- 去基化 / 特判列 ----
inline NodePtr bt1_remove_base(const NodePtr& a, int base)
{
    auto out = bt1_make_node();
    out->inf = a->inf;
    for (const ColumnC& col : a->cols)
    {
        ColumnC nc;
        nc.arr = col.arr;
        if (!nc.arr.empty()) nc.arr[0] -= base;
        for (const NodePtr& x : col.higher) nc.higher.push_back(bt1_remove_base(x, base));
        out->cols.push_back(std::move(nc));
    }
    return out;
}
inline std::vector<NodePtr> bt1_highest_without_base(const ColumnC& c)
{
    std::vector<NodePtr> out;
    for (const NodePtr& x : c.higher) out.push_back(bt1_remove_base(x, c.arr.empty() ? 0 : c.arr[0] + 1));
    return out;
}
inline bool bt1_is_one_line_column(const ColumnC& c, int value)
{
    if (c.arr.empty() || c.arr[0] != value) return false;
    for (size_t i = 1; i < c.arr.size(); ++i) if (c.arr[i] != 0) return false;
    return c.higher.empty();
}
inline bool bt1_is_special_column(const ColumnC& c)
{
    const int higher_right = (int)c.higher.size() - 1;
    if (higher_right < 0) return false;
    const NodePtr& h = c.higher[(size_t)higher_right];
    const int vert_right = (int)h->cols.size() - 1;
    if (vert_right < 0) return false;
    return bt1_is_one_line_column(h->cols[(size_t)vert_right],
        c.arr.empty() ? 0 : c.arr[0] + 1);
}

// ---- 父树（higher 为数组，sub 为多个子树）----
struct PNodeC;
using PNodePtr = std::shared_ptr<PNodeC>;
struct PPColC { std::shared_ptr<std::vector<int>> parr; std::vector<PNodePtr> subs; };
struct PNodeC { std::vector<PPColC> cols; };

using ColStack = std::vector<const ColumnC*>;
using ParStack = std::vector<std::shared_ptr<std::vector<int>>>;

inline bool bt1_contains(const std::vector<int>& v, int x)
{
    for (int t : v) if (t == x) return true;
    return false;
}
inline int bt1_last_nonneg(const std::vector<int>& v)
{
    for (int i = (int)v.size() - 1; i >= 0; --i) if (v[i] >= 0) return i;
    return -1;
}

PNodePtr bt1_compute_parents(const NodePtr& e, int n, ColStack& stack,
                             ParStack& pstack, std::vector<int>& forbidden)
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
        int p = iS;
        while (p >= 0)
        {
            if (!bt1_contains(forbidden, p) &&
                bt1_compare_shared_higher(bt1_highest_without_base(*stack[(size_t)p]),
                                          bt1_highest_without_base(col)) < 0) break;
            p = (n == 0) ? p - 1 : (*(pstack[(size_t)p]))[n - 1];
        }
        (*parr)[n] = p;

        forbidden.push_back(iS);
        PPColC pc;
        pc.parr = parr;
        for (const NodePtr& x : col.higher)
            pc.subs.push_back(bt1_compute_parents(x, n, stack, pstack, forbidden));
        forbidden.pop_back();
        P->cols.push_back(std::move(pc));
    }
    stack.resize(lS0);
    pstack.resize(lS0);
    return P;
}

// ---- 层 / 根 / 提升向量 ----
std::pair<int, bool> bt1_tail_layer(const NodePtr& e)
{
    if (e->cols.empty() || bt1_is_zero_column(e->cols.back())) return { -1, false };
    const NodeC* cur = e.get();
    int layer = 0;
    while (true)
    {
        const int right = (int)cur->cols.size() - 1;
        const ColumnC& col = cur->cols[(size_t)right];
        const int higher_right = (int)col.higher.size() - 1;
        if (col.higher.empty()) return { layer, false };
        if (bt1_is_special_column(col)) return { layer, true };
        if (col.higher[(size_t)higher_right]->cols.empty()) return { layer, false };
        cur = col.higher[(size_t)higher_right].get();
        ++layer;
    }
}
std::pair<int, int> bt1_root_layer(const NodePtr& e, int r)
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
PNodePtr bt1_descend_p(PNodePtr P, int tailLayer)
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
std::pair<int, int> bt1_root(const NodePtr& e, const PNodePtr& Pin, int tailLayer)
{
    if (e->cols.empty() || bt1_is_zero_column(e->cols.back())) return { -1, -1 };
    PNodePtr P = bt1_descend_p(Pin, tailLayer);
    const int right = (int)P->cols.size() - 1;
    const std::vector<int>& parr = *P->cols[(size_t)right].parr;
    const int b = bt1_last_nonneg(parr);
    if (b < 0) return { -1, -1 };
    return { parr[(size_t)b], b };
}
std::vector<int> bt1_ascension_vector(const NodePtr& e, int r, int b, int tailLayer)
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

// ---- 阈值树 ----
struct ANodeC;
using ANodePtr = std::shared_ptr<ANodeC>;
struct AColC { std::optional<int> a; std::vector<ANodePtr> subs; };
struct ANodeC { std::vector<AColC> cols; };

ANodePtr bt1_undefined_at(const NodePtr& e)
{
    auto A = std::make_shared<ANodeC>();
    for (const ColumnC& col : e->cols)
    {
        AColC ac; ac.a = std::nullopt;
        for (const NodePtr& x : col.higher) ac.subs.push_back(bt1_undefined_at(x));
        A->cols.push_back(std::move(ac));
    }
    return A;
}
ANodePtr bt1_thresholds(const NodePtr& e, const PNodePtr& P,
                        std::optional<int> ropt, int b,
                        std::vector<std::optional<int>>& stk)
{
    auto A = std::make_shared<ANodeC>();
    if (!ropt.has_value()) return bt1_undefined_at(e);
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
                ac.subs.push_back(bt1_undefined_at(x));
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
                ac.subs.push_back(bt1_thresholds(e->cols[i].higher[ix],
                    P && i < P->cols.size() && ix < P->cols[i].subs.size() ? P->cols[i].subs[ix] : nullptr,
                    r, b, stk));
        }
        A->cols.push_back(std::move(ac));
    }
    stk.resize(lS0);
    return A;
}

inline std::vector<int> bt1_ascend_vector(const std::vector<int>& v, int A, const std::vector<int>& V, int w)
{
    std::vector<int> out = v;
    for (int j = 0; j < A && j < (int)V.size(); ++j) out[(size_t)j] += V[(size_t)j] * w;
    return out;
}

// ---- 复制提升 ----
inline NodePtr bt1_clone_slice_from_raw(const NodeC* e, int from)
{
    auto out = bt1_make_node();
    out->inf = e->inf;
    for (size_t i = from; i < e->cols.size(); ++i) out->cols.push_back(e->cols[i]);
    return out;
}
inline NodePtr bt1_clone_slice(const NodePtr& e, int from)
{
    return bt1_clone_slice_from_raw(e.get(), from);
}
inline NodePtr bt1_pop_last(const NodePtr& e)
{
    auto out = bt1_make_node();
    out->inf = e->inf;
    for (size_t i = 0; i + 1 < e->cols.size(); ++i) out->cols.push_back(e->cols[i]);
    return out;
}
inline ANodePtr bt1_a_slice(const ANodePtr& A, int from)
{
    auto out = std::make_shared<ANodeC>();
    for (size_t i = from; i < A->cols.size(); ++i) out->cols.push_back(A->cols[i]);
    return out;
}
NodePtr bt1_ascend_replace(const NodePtr& e, const NodePtr& tail,
                           std::optional<int> tl, const ANodePtr& A,
                           const std::vector<int>& V, int w)
{
    auto out = bt1_make_node();
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
            nc.arr = bt1_ascend_vector(col.arr, Ai, V, w);
            std::optional<int> ntl;
            if (i + 1 == e->cols.size() && tl.has_value()) ntl = *tl - 1;
            const int higher_right = (int)col.higher.size() - 1;
            for (size_t ix = 0; ix < col.higher.size(); ++ix)
            {
                std::optional<int> xntl;
                if ((int)ix == higher_right && ntl.has_value()) xntl = ntl;
                nc.higher.push_back(bt1_ascend_replace(col.higher[ix], tail, xntl,
                    (A && i < A->cols.size() && ix < A->cols[i].subs.size())
                        ? A->cols[i].subs[ix] : nullptr, V, w));
            }
            out->cols.push_back(std::move(nc));
        }
    }
    return out;
}

NodePtr bt1_fs_special(const NodePtr& e, int tail_layer, int index)
{
    const int right = (int)e->cols.size() - 1;
    const int higher_right = (int)e->cols[(size_t)right].higher.size() - 1;
    auto out = bt1_make_node();
    for (int i = 0; i < right; ++i) out->cols.push_back(e->cols[(size_t)i]);
    ColumnC nc;
    nc.arr = e->cols[(size_t)right].arr;
    for (int k = 0; k < higher_right; ++k)
        nc.higher.push_back(e->cols[(size_t)right].higher[(size_t)k]);
    if (tail_layer == 0)
    {
        const NodePtr& h = e->cols[(size_t)right].higher[(size_t)higher_right];
        const int vert_right = (int)h->cols.size() - 1;
        NodePtr new_vert = bt1_clone_slice(h, 0);
        new_vert->cols.resize((size_t)vert_right);
        for (int k = 0; k < index; ++k) nc.higher.push_back(new_vert);
    }
    else
    {
        nc.higher.push_back(bt1_fs_special(
            e->cols[(size_t)right].higher[(size_t)higher_right], tail_layer - 1, index));
    }
    out->cols.push_back(std::move(nc));
    return out;
}

NodePtr bt1_fs(const NodePtr& e, int index, int n)
{
    if (bt1_is_infinity(e)) return bt1_infinity_fs(index, n);
    if (e->cols.empty()) return e;
    if (!bt1_is_limit(e)) return bt1_pop_last(e);

    ColStack stack; ParStack pstack; std::vector<int> forbidden;
    PNodePtr P = bt1_compute_parents(e, n, stack, pstack, forbidden);
    const auto tl = bt1_tail_layer(e);
    const int t_layer = tl.first;
    const bool is_special = tl.second;
    if (is_special) return bt1_fs_special(e, t_layer, index);

    const auto rb = bt1_root(e, P, t_layer);
    const int r = rb.first;
    const int b = rb.second;
    const bool hasRoot = (b >= 0);

    int r_layer = 0, ri = 0;
    if (hasRoot)
    {
        const auto rl = bt1_root_layer(e, r);
        r_layer = rl.first;
        ri = rl.second;
    }
    std::vector<std::optional<int>> stk;
    const ANodePtr A = bt1_thresholds(e, P,
        hasRoot ? std::optional<int>(r) : std::nullopt, b, stk);
    const std::vector<int> V = hasRoot
        ? bt1_ascension_vector(e, r, b, t_layer) : std::vector<int>{};

    const NodeC* cur = e.get();
    ANodePtr curA = A;
    for (int k = 0; k < r_layer; ++k)
    {
        const int right = (int)cur->cols.size() - 1;
        const int higher_right = (int)cur->cols[(size_t)right].higher.size() - 1;
        cur = cur->cols[(size_t)right].higher[(size_t)higher_right].get();
        curA = curA->cols[(size_t)right].subs[(size_t)higher_right];
    }
    NodePtr copy_part = bt1_clone_slice_from_raw(cur, ri);
    ANodePtr copy_partA = bt1_a_slice(curA, ri);
    for (int k = r_layer; k < t_layer; ++k)
    {
        const int right = (int)cur->cols.size() - 1;
        const int higher_right = (int)cur->cols[(size_t)right].higher.size() - 1;
        cur = cur->cols[(size_t)right].higher[(size_t)higher_right].get();
        curA = curA->cols[(size_t)right].subs[(size_t)higher_right];
    }
    const int right = (int)cur->cols.size() - 1;
    std::vector<NodePtr> tail_top = cur->cols[(size_t)right].higher;  // slice(0,-1)
    if (!tail_top.empty()) tail_top.pop_back();
    std::vector<ANodePtr> tail_topA = curA->cols[(size_t)right].subs;  // slice(0,-1)
    if (!tail_topA.empty()) tail_topA.pop_back();

    NodePtr result = bt1_make_node();
    for (int w = index; w > 0; --w)
    {
        result = bt1_ascend_replace(copy_part, result, t_layer - r_layer,
                                    copy_partA, V, w);
        if (b == n)
        {
            ColumnC c0;
            c0.arr = result->cols[0].arr;
            for (size_t ix = 0; ix < tail_top.size(); ++ix)
                c0.higher.push_back(bt1_ascend_replace(tail_top[ix], bt1_make_node(),
                    std::nullopt, tail_topA[ix], V, w - 1));
            result->cols[0] = std::move(c0);
        }
    }
    result = bt1_ascend_replace(e, result, t_layer, A, V, 0);
    return result;
}

inline NodePtr bt1_lnz(const NodePtr& e, int index, int n)
{
    if (bt1_is_infinity(e)) return bt1_infinity_fs(index, n);
    if (!bt1_is_limit(e)) return bt1_fs(e, 0, n);
    return bt1_fs(e, index, n);
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
        return bt1_make_node();
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
        auto out = bt1_make_node();
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
            return bt1_infinity();
        }
        NodePtr out = parse_expr();
        sp();
        if (i_ != s_.size()) err();
        return out;
    }
};

struct BT1Notation
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
            return bt1_display(bt1_lnz(m, term, n));
        }
        catch (...)
        {
            return bmlike::kParseError;
        }
    }
};
}  // namespace bt1_detail

using bt1_detail::BT1Notation;
}  // namespace omegay::notation

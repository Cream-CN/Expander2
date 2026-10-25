// BTL_Minus1_Y_nSS（Asheep's Transfinite nSS）C++ 移植。
//
// 每列除 lower（父标号分量）外还有 mark；有子表达式时 mark>0 且子表达式以
// `mark^expr` 显示（空子表达式显示为裸 mark 数字）。
// 显示 use_sc=true：有 higher → (lower 全量; u^e0,u^e1)；无 higher → (lower 截尾零)。
// 父计算同时求 entry_parents 与展开类型 type（lower/higher/star/mark/mark*/undefined）
// 与 is_tail；FS 按 type 分派：star → FS_star；mark → copy_part 重组后复制提升；
// higher/mark 复制时回填 tail_top；undefined → 删末列。
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
namespace btl_detail
{
struct NodeC;
using NodePtr = std::shared_ptr<NodeC>;
struct ColumnC { std::vector<int> lower; int mark = 0; std::vector<NodePtr> higher; };
struct NodeC { bool inf = false; std::vector<ColumnC> cols; };

inline NodePtr btl_make_node() { return std::make_shared<NodeC>(); }
inline NodePtr btl_infinity() { auto e = btl_make_node(); e->inf = true; return e; }
inline bool btl_is_infinity(const NodePtr& e) { return e->inf; }

inline NodePtr btl_infinity_fs(int index, int n)
{
    NodePtr chain = btl_make_node();
    for (int i = index; i > 0; --i)
    {
        auto wrapped = btl_make_node();
        ColumnC c; c.lower.assign(n, i); c.mark = i; c.higher.push_back(chain);
        wrapped->cols.push_back(std::move(c));
        chain = wrapped;
    }
    auto full = btl_make_node();
    full->cols.push_back(ColumnC{ std::vector<int>(n, 0), 0, {} });
    for (const ColumnC& cc : chain->cols) full->cols.push_back(cc);
    return full;
}
inline bool btl_is_zero_column(const ColumnC& c)
{
    for (int x : c.lower) if (x != 0) return false;
    return c.higher.empty();
}
inline int btl_last_nonneg(const std::vector<int>& v)
{
    for (int i = (int)v.size() - 1; i >= 0; --i) if (v[i] >= 0) return i;
    return -1;
}
inline bool btl_contains(const std::deque<int>& d, int v)
{
    for (int x : d) if (x == v) return true;
    return false;
}

// ---- 显示（use_sc=true）----
inline std::string btl_display(const NodePtr& e);
inline std::string btl_top_display(const NodePtr& e, int u)
{
    if (e->cols.empty()) return std::to_string(u);
    return std::to_string(u) + "^" + btl_display(e);
}
inline std::string btl_display(const NodePtr& e);
inline std::string btl_column_display(const ColumnC& c)
{
    if (!c.higher.empty())
    {
        std::string s = "(";
        for (size_t k = 0; k < c.lower.size(); ++k) { if (k) s += ","; s += std::to_string(c.lower[k]); }
        s += ";";
        for (size_t k = 0; k < c.higher.size(); ++k)
        {
            if (k) s += ",";
            s += btl_top_display(c.higher[k], c.mark);
        }
        s += ")";
        return s;
    }
    int j = 0;
    for (size_t k = 0; k < c.lower.size(); ++k) if (c.lower[k] > 0) j = (int)k + 1;
    std::string s = "(";
    for (int k = 0; k < j; ++k) { if (k) s += ","; s += std::to_string(c.lower[(size_t)k]); }
    s += ")";
    return s;
}
inline std::string btl_display(const NodePtr& e)
{
    if (btl_is_infinity(e)) return "Limit";
    std::string s;
    for (const ColumnC& c : e->cols) s += btl_column_display(c);
    return s;
}
inline bool btl_is_limit(const NodePtr& e)
{
    if (btl_is_infinity(e)) return true;
    if (e->cols.empty()) return false;
    return !btl_is_zero_column(e->cols.back());
}

// ---- 比较 ----
inline int btl_number_cmp(int a, int b) { return a == b ? 0 : a < b ? -1 : 1; }
inline int btl_compare(const NodePtr& a, const NodePtr& b);
inline int btl_column_compare(const ColumnC& a, const ColumnC& b);
inline int btl_higher_compare(const std::vector<NodePtr>& a, const std::vector<NodePtr>& b);
inline int btl_column_compare(const ColumnC& a, const ColumnC& b)
{
    const size_t m = (std::min)(a.lower.size(), b.lower.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = btl_number_cmp(a.lower[i], b.lower[i]); c != 0) return c;
    if (const int c = btl_number_cmp((int)a.lower.size(), (int)b.lower.size()); c != 0) return c;
    if (const int c = btl_number_cmp(a.mark, b.mark); c != 0) return c;
    return btl_higher_compare(a.higher, b.higher);
}
inline int btl_higher_compare(const std::vector<NodePtr>& a, const std::vector<NodePtr>& b)
{
    const size_t m = (std::min)(a.size(), b.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = btl_compare(a[i], b[i]); c != 0) return c;
    return btl_number_cmp((int)a.size(), (int)b.size());
}
inline int btl_compare(const NodePtr& a, const NodePtr& b)
{
    const size_t m = (std::min)(a->cols.size(), b->cols.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = btl_column_compare(a->cols[i], b->cols[i]); c != 0) return c;
    return btl_number_cmp((int)a->cols.size(), (int)b->cols.size());
}

NodePtr btl_remove_base(const NodePtr& e, int base)
{
    auto out = btl_make_node();
    for (const ColumnC& c : e->cols)
    {
        ColumnC nc;
        nc.lower = c.lower;
        if (!nc.lower.empty()) nc.lower[0] -= base;
        nc.mark = c.mark;
        for (const NodePtr& x : c.higher) nc.higher.push_back(btl_remove_base(x, base));
        out->cols.push_back(std::move(nc));
    }
    return out;
}
std::vector<NodePtr> btl_higher_remove_base(const ColumnC& c)
{
    std::vector<NodePtr> out;
    for (const NodePtr& x : c.higher) out.push_back(btl_remove_base(x, c.lower[0] + 1));
    return out;
}
inline bool btl_is_one_line_column(const ColumnC& c, int value)
{
    if (c.lower.empty() || c.lower[0] != value) return false;
    for (size_t i = 1; i < c.lower.size(); ++i) if (c.lower[i] != 0) return false;
    return c.higher.empty();
}

// ---- 父计算 ----
enum class BtlType { Undef, Lower, Higher, Star, Mark, MarkStar };
struct PNodeC;
using PNodePtr = std::shared_ptr<PNodeC>;
struct PColC
{
    std::shared_ptr<std::vector<int>> entry_parents;
    BtlType type = BtlType::Undef;
    bool is_tail = false;
    std::vector<PNodePtr> higher;
};
struct PNodeC { std::vector<PColC> cols; };
using ColStack = std::vector<const ColumnC*>;
using PColStack = std::vector<std::shared_ptr<PColC>>;

PNodePtr btl_compute_parents(const NodePtr& e, int n, ColStack& column_stack,
                             PColStack& parent_stack, std::deque<int>& outer_stack)
{
    const size_t lS0 = column_stack.size();
    auto P = std::make_shared<PNodeC>();
    for (size_t i = 0; i < e->cols.size(); ++i)
    {
        const ColumnC& col = e->cols[i];
        const int iS = (int)column_stack.size();
        column_stack.push_back(&col);
        auto entry = std::make_shared<std::vector<int>>(n + 1, -1);
        auto result_i = std::make_shared<PColC>();
        result_i->entry_parents = entry;
        parent_stack.push_back(result_i);
        for (int j = 0; j < n; ++j)
        {
            int p = iS;
            while (p >= 0)
            {
                if (column_stack[(size_t)p]->lower[j] < col.lower[j]) break;
                p = (j == 0) ? p - 1 : (*(*parent_stack[(size_t)p]).entry_parents)[j - 1];
            }
            if (p < 0) break;
            (*entry)[j] = p;
        }
        {
            int p = iS;
            while (p >= 0)
            {
                const ColumnC& pc = *column_stack[(size_t)p];
                if (pc.mark < col.mark && btl_contains(outer_stack, p))
                {
                    result_i->type = BtlType::Mark;
                    if (col.higher.size() == 1 && col.higher[0]->cols.empty())
                        result_i->type = BtlType::MarkStar;
                    break;
                }
                else if (pc.mark <= col.mark && !btl_contains(outer_stack, p) &&
                         btl_higher_compare(btl_higher_remove_base(pc), btl_higher_remove_base(col)) < 0)
                {
                    result_i->type = BtlType::Higher;
                    break;
                }
                p = (*(*parent_stack[(size_t)p]).entry_parents)[n - 1];
            }
            (*entry)[n] = p;
            if (p < 0)
            {
                if (lS0 > 0 &&
                    btl_is_one_line_column(col, column_stack[lS0 - 1]->lower[0] + 1))
                    result_i->type = BtlType::Star;
                else
                    result_i->type = BtlType::Lower;
                result_i->is_tail = true;
            }
            else
            {
                const int higher_right = (int)col.higher.size() - 1;
                if (higher_right < 0)
                    throw std::runtime_error("illegal tail");
                if (col.higher[(size_t)higher_right]->cols.empty()) result_i->is_tail = true;
            }
        }
        outer_stack.push_back(iS);
        for (const NodePtr& x : col.higher)
            result_i->higher.push_back(btl_compute_parents(x, n, column_stack, parent_stack, outer_stack));
        outer_stack.pop_back();
        P->cols.push_back(*result_i);
    }
    column_stack.resize(lS0);
    parent_stack.resize(lS0);
    return P;
}
PNodePtr btl_compute_parents(const NodePtr& e, int n)
{
    ColStack cs; PColStack ps; std::deque<int> outer;
    return btl_compute_parents(e, n, cs, ps, outer);
}

// ---- 层 / 根 ----
template <class T>
const T* btl_next_layer(const T* cur)
{
    const int right = (int)cur->cols.size() - 1;
    if (right < 0) throw std::runtime_error("illegal layer");
    const int higher_right = (int)cur->cols[(size_t)right].higher.size() - 1;
    if (higher_right < 0) throw std::runtime_error("illegal layer");
    return cur->cols[(size_t)right].higher[(size_t)higher_right].get();
}
template <class T>
const T* btl_skip_layer(const T* cur, int layer)
{
    for (int k = 0; k < layer; ++k) cur = btl_next_layer(cur);
    return cur;
}
std::pair<int, BtlType> btl_tail_info(const PNodePtr& P)
{
    const PNodeC* cur_P = P.get();
    int layer = 0;
    while (true)
    {
        const int right = (int)cur_P->cols.size() - 1;
        if (cur_P->cols[(size_t)right].is_tail)
            return { layer, cur_P->cols[(size_t)right].type };
        cur_P = btl_next_layer(cur_P);
        ++layer;
    }
}
std::pair<int, int> btl_root(const PNodePtr& P, int t_layer)
{
    const PNodeC* cur_P = P.get();
    for (int k = 0; k < t_layer; ++k) cur_P = btl_next_layer(cur_P);
    const int right = (int)cur_P->cols.size() - 1;
    const std::vector<int>& entry = *cur_P->cols[(size_t)right].entry_parents;
    const int b = btl_last_nonneg(entry);
    if (b < 0) return { -1, -1 };
    return { entry[(size_t)b], b };
}
std::pair<int, int> btl_root_layer(const NodePtr& e, int r)
{
    int layer = 0, len = (int)e->cols.size();
    const NodeC* cur = e.get();
    while (len <= r)
    {
        ++layer;
        cur = btl_next_layer(cur);
        len += (int)cur->cols.size();
    }
    return { layer, r - (len - (int)cur->cols.size()) };
}
std::vector<int> btl_ascension_vector(const NodePtr& e, int r, int b, int t_layer)
{
    std::vector<const ColumnC*> st;
    for (const ColumnC& c : e->cols) st.push_back(&c);
    const NodeC* cur = e.get();
    for (int k = 0; k < t_layer; ++k)
    {
        const int right = (int)cur->cols.size() - 1;
        const ColumnC& col = cur->cols[(size_t)right];
        const int higher_right = (int)col.higher.size() - 1;
        cur = col.higher[(size_t)higher_right].get();
        for (const ColumnC& c : cur->cols) st.push_back(&c);
    }
    std::vector<int> v((size_t)b);
    for (int j = 0; j < b; ++j)
        v[(size_t)j] = st.back()->lower[(size_t)j] - st[(size_t)r]->lower[(size_t)j];
    return v;
}
inline std::vector<int> btl_ascend_vector(const std::vector<int>& v, int A, const std::vector<int>& V, int w)
{
    std::vector<int> out = v;
    for (int j = 0; j < A && j < (int)V.size(); ++j) out[(size_t)j] += V[(size_t)j] * w;
    return out;
}

// ---- 阈值 ----
struct ANodeC;
using ANodePtr = std::shared_ptr<ANodeC>;
struct AColC { std::optional<int> threshold; std::vector<ANodePtr> higher; };
struct ANodeC { std::vector<AColC> cols; };

ANodePtr btl_undefined_at(const NodePtr& e)
{
    auto A = std::make_shared<ANodeC>();
    for (const ColumnC& col : e->cols)
    {
        AColC ac; ac.threshold = std::nullopt;
        for (const NodePtr& x : col.higher) ac.higher.push_back(btl_undefined_at(x));
        A->cols.push_back(std::move(ac));
    }
    return A;
}
ANodePtr btl_thresholds(const NodePtr& e, const PNodePtr& P,
                        std::optional<int> ropt, int b,
                        std::vector<std::optional<int>>& stk)
{
    auto A = std::make_shared<ANodeC>();
    if (!ropt.has_value()) return btl_undefined_at(e);
    const int r = *ropt;
    const size_t lS0 = stk.size();
    for (size_t i = 0; i < e->cols.size(); ++i)
    {
        const int iS = (int)stk.size();
        AColC ac;
        if (iS < r && i + 1 != e->cols.size())
        {
            stk.push_back(std::nullopt);
            ac.threshold = std::nullopt;
            for (const NodePtr& x : e->cols[i].higher) ac.higher.push_back(btl_undefined_at(x));
        }
        else
        {
            std::optional<int> Ai;
            if (iS == r) Ai = b;
            else if (iS > r)
            {
                int ai = 0;
                const std::vector<int>& entry = *P->cols[i].entry_parents;
                while (ai < (int)entry.size() && entry[(size_t)ai] >= r &&
                       entry[(size_t)ai] < (int)stk.size() &&
                       stk[(size_t)entry[(size_t)ai]].has_value() &&
                       *stk[(size_t)entry[(size_t)ai]] > ai) ++ai;
                Ai = ai;
            }
            stk.push_back(Ai);
            ac.threshold = Ai;
            for (size_t ix = 0; ix < e->cols[i].higher.size(); ++ix)
                ac.higher.push_back(btl_thresholds(e->cols[i].higher[ix],
                    P->cols[i].higher[ix], r, b, stk));
        }
        A->cols.push_back(std::move(ac));
    }
    stk.resize(lS0);
    return A;
}

// ---- 复制提升 ----
inline NodePtr btl_clone_slice_from_raw(const NodeC* e, int from)
{
    auto out = btl_make_node();
    out->inf = e->inf;
    for (size_t i = from; i < e->cols.size(); ++i) out->cols.push_back(e->cols[i]);
    return out;
}
inline NodePtr btl_pop_last(const NodePtr& e)
{
    auto out = btl_make_node();
    out->inf = e->inf;
    for (size_t i = 0; i + 1 < e->cols.size(); ++i) out->cols.push_back(e->cols[i]);
    return out;
}
inline ANodePtr btl_a_slice(const ANodeC* A, int from)
{
    auto out = std::make_shared<ANodeC>();
    for (size_t i = from; i < A->cols.size(); ++i) out->cols.push_back(A->cols[i]);
    return out;
}
NodePtr btl_ascend_replace(const NodePtr& e, const NodePtr& tail,
                           std::optional<int> t_layer, const ANodePtr& A,
                           const std::vector<int>& V, int w)
{
    auto out = btl_make_node();
    for (size_t i = 0; i < e->cols.size(); ++i)
    {
        if (t_layer.has_value() && *t_layer == 0 && i + 1 == e->cols.size())
        {
            for (const ColumnC& tc : tail->cols) out->cols.push_back(tc);
        }
        else
        {
            const ColumnC& col = e->cols[i];
            const int Ai = (A && i < A->cols.size() && A->cols[i].threshold.has_value())
                ? *A->cols[i].threshold : 0;
            const int higher_right = (int)col.higher.size() - 1;
            ColumnC nc;
            nc.lower = btl_ascend_vector(col.lower, Ai, V, w);
            nc.mark = col.mark;
            std::optional<int> ntl;
            if (i + 1 == e->cols.size() && t_layer.has_value()) ntl = *t_layer - 1;
            for (size_t ix = 0; ix < col.higher.size(); ++ix)
            {
                std::optional<int> xntl;
                if ((int)ix == higher_right && ntl.has_value()) xntl = ntl;
                nc.higher.push_back(btl_ascend_replace(col.higher[ix], tail, xntl,
                    (A && i < A->cols.size() && ix < A->cols[i].higher.size())
                        ? A->cols[i].higher[ix] : nullptr, V, w));
            }
            if (nc.higher.empty()) nc.mark = 0;
            out->cols.push_back(std::move(nc));
        }
    }
    return out;
}

NodePtr btl_fs_star(const NodePtr& e, int tail_layer, int index)
{
    const int right = (int)e->cols.size() - 1;
    const int higher_right = (int)e->cols[(size_t)right].higher.size() - 1;
    auto out = btl_make_node();
    for (int i = 0; i < right; ++i) out->cols.push_back(e->cols[(size_t)i]);
    ColumnC nc;
    nc.lower = e->cols[(size_t)right].lower;
    for (int k = 0; k < higher_right; ++k) nc.higher.push_back(e->cols[(size_t)right].higher[(size_t)k]);
    if (tail_layer == 1)
    {
        const NodePtr& h = e->cols[(size_t)right].higher[(size_t)higher_right];
        const int vert_right = (int)h->cols.size() - 1;
        auto new_vert = btl_make_node();
        for (int k = 0; k < vert_right; ++k) new_vert->cols.push_back(h->cols[(size_t)k]);
        nc.mark = (higher_right == 0 && index == 0) ? 0 : e->cols[(size_t)right].mark;
        for (int k = 0; k < index; ++k) nc.higher.push_back(new_vert);
    }
    else
    {
        nc.mark = e->cols[(size_t)right].mark;
        nc.higher.push_back(btl_fs_star(
            e->cols[(size_t)right].higher[(size_t)higher_right], tail_layer - 1, index));
    }
    out->cols.push_back(std::move(nc));
    return out;
}

NodePtr btl_fs(const NodePtr& e, int index, int n)
{
    if (btl_is_infinity(e)) return btl_infinity_fs(index, n);
    if (e->cols.empty()) return e;
    if (!btl_is_limit(e)) return btl_pop_last(e);

    PNodePtr P = btl_compute_parents(e, n);
    const auto ti = btl_tail_info(P);
    const int t_layer = ti.first;
    const BtlType type = ti.second;
    if (type == BtlType::Undef) return btl_pop_last(e);
    if (type == BtlType::Star) return btl_fs_star(e, t_layer, index);

    const auto rb = btl_root(P, t_layer);
    const int r = rb.first;
    const int b = rb.second;
    const auto rl = btl_root_layer(e, r);
    const int r_layer = rl.first;
    const int ri = rl.second;
    const std::vector<int> V = btl_ascension_vector(e, r, b, t_layer);
    std::vector<std::optional<int>> stk;
    const ANodePtr A = btl_thresholds(e, P, r, b, stk);

    NodePtr copy_part = btl_clone_slice_from_raw(btl_skip_layer(e.get(), r_layer), ri);
    ANodePtr copy_part_A = btl_a_slice(btl_skip_layer(A.get(), r_layer), ri);
    int diff_layer = t_layer - r_layer;
    if (type == BtlType::Mark)
    {
        const int higher_right = (int)copy_part->cols[0].higher.size() - 1;
        auto new_cp = btl_make_node();
        ColumnC c0;
        c0.lower = copy_part->cols[0].lower;
        c0.mark = 0;
        new_cp->cols.push_back(std::move(c0));
        for (const ColumnC& cc : copy_part->cols[0].higher[(size_t)higher_right]->cols)
            new_cp->cols.push_back(cc);
        copy_part = new_cp;
        auto new_cpA = std::make_shared<ANodeC>();
        AColC a0;
        a0.threshold = b;
        new_cpA->cols.push_back(std::move(a0));
        for (const AColC& ac : copy_part_A->cols[0].higher[(size_t)higher_right]->cols)
            new_cpA->cols.push_back(ac);
        copy_part_A = new_cpA;
        --diff_layer;
    }
    const NodeC* current = btl_skip_layer(e.get(), t_layer);
    const ANodeC* current_A = btl_skip_layer(A.get(), t_layer);
    const int right = (int)current->cols.size() - 1;
    std::vector<NodePtr> tail_top;
    for (size_t k = 0; k + 1 < current->cols[(size_t)right].higher.size(); ++k)
        tail_top.push_back(current->cols[(size_t)right].higher[k]);
    std::vector<ANodePtr> tail_top_A;
    for (size_t k = 0; k + 1 < current_A->cols[(size_t)right].higher.size(); ++k)
        tail_top_A.push_back(current_A->cols[(size_t)right].higher[k]);
    const int tail_mark = tail_top.empty() ? 0 : current->cols[(size_t)right].mark;

    NodePtr result = btl_make_node();
    for (int w = index; w > 0; --w)
    {
        result = btl_ascend_replace(copy_part, result, diff_layer, copy_part_A, V, w);
        if (type == BtlType::Higher || type == BtlType::Mark)
        {
            ColumnC c0;
            c0.lower = result->cols[0].lower;
            c0.mark = tail_mark;
            for (size_t ix = 0; ix < tail_top.size(); ++ix)
                c0.higher.push_back(btl_ascend_replace(tail_top[ix], btl_make_node(),
                    std::nullopt, tail_top_A[ix], V, w - 1));
            result->cols[0] = std::move(c0);
        }
    }
    result = btl_ascend_replace(e, result, t_layer, A, V, 0);
    return result;
}

inline NodePtr btl_lnz(const NodePtr& e, int index, int n)
{
    if (btl_is_infinity(e)) return btl_infinity_fs(index, n);
    if (!btl_is_limit(e)) return btl_fs(e, 0, n);
    return btl_fs(e, index, n);
}

// ---- 解析（mark^expr 语法；',' 或 ';' 分隔；裸数字作空子表达式）----
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
    ColumnC parse_column()
    {
        sp();
        if (i_ >= s_.size() || s_[i_] != '(') err();
        ++i_;
        std::vector<int> numbers;
        std::vector<NodePtr> higher;
        std::optional<int> mark;
        bool in_higher = false;
        bool expect_value = true;
        while (i_ < s_.size() && s_[i_] != ')')
        {
            sp();
            if (i_ >= s_.size()) break;
            if (s_[i_] == ',' || s_[i_] == ';')
            {
                if (expect_value) err();
                if (s_[i_] == ';')
                {
                    if (numbers.size() != (size_t)n_) err();
                    in_higher = true;
                }
                ++i_;
                expect_value = true;
                continue;
            }
            if (!expect_value) err();
            const int m = num_tok();
            sp();
            if (i_ < s_.size() && s_[i_] == '^')
            {
                in_higher = true;
                if (!mark.has_value()) mark = m;
                else if (m != *mark) err();
                ++i_;
                higher.push_back(parse_expr());
            }
            else if (in_higher || numbers.size() >= (size_t)n_)
            {
                if (!mark.has_value()) mark = m;
                else if (m != *mark) err();
                higher.push_back(btl_make_node());
            }
            else
            {
                numbers.push_back(m);
            }
            expect_value = false;
        }
        if (i_ >= s_.size()) err();
        ++i_;
        while (numbers.size() < (size_t)n_) numbers.push_back(mark.value_or(0));
        const int final_mark = higher.empty() ? 0 : mark.value_or(0);
        if (!higher.empty() && final_mark == 0) err();
        ColumnC c;
        c.lower = std::move(numbers);
        c.mark = final_mark;
        c.higher = std::move(higher);
        return c;
    }
    NodePtr parse_expr()
    {
        auto out = btl_make_node();
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
            return btl_infinity();
        }
        NodePtr out = parse_expr();
        sp();
        if (i_ != s_.size()) err();
        return out;
    }
};

struct BTlNotation
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
            return btl_display(btl_lnz(m, term, n));
        }
        catch (...)
        {
            return bmlike::kParseError;
        }
    }
};
}  // namespace btl_detail

using btl_detail::BTlNotation;
}  // namespace omegay::notation

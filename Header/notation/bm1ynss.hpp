// Abs_B / Rel_B_Minus1_Y_nSS（abs B(n+1)SS / rel B(n+1)SS，绝对/相对分支 -1Y-nSS）。
//
// 移植自 ne-rewritten 的 Minus1_Y_nSS-series/{Abs,Rel}_B_Minus1_Y_nSS.ts。
// 列结构、解析与显示与 T_Minus1_Y_nSS 完全相同（复用 tm1_detail）；二者唯一区别是
// 第 n 个（步长 / 顶层）父的求法：
//   abs —— 直接按“绝对 step 子表达式”比较，并排除祖先链（forbidden_stack）；
//   rel —— 先把候选列相对根列做差分得到布尔/数值 key，按 key 比较并排除祖先链。
// 其余下层父、提升阈值、尾层定位、整体复制提升（ascend_replace）两族逐字相同。
//
// 父下标坐标系（重要）：TS 的 stack / parent_stack 是“活动深度路径”，随子树返回而
// splice（每层 for 结束后统一弹出），父数组存的是该路径中的活动下标，不是全局
// preorder 序号。C++ 侧以 vector 活栈 + push/resize(lS0) 精确复刻，每个 PCol 自带
// 父数组（shared_ptr 与活栈共享同一对象，fill_top 回填第 n 个父时活栈同步可见）。
#pragma once

#include <algorithm>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Header/notation/tm1ynss.hpp"

namespace omegay::notation
{
namespace bm1_detail
{
using namespace tm1_detail;  // Node / Column / NodePtr / Parser / render / compare_node ...

struct PNodeC;
using PNodePtr = std::shared_ptr<PNodeC>;
struct PColC { std::shared_ptr<std::vector<int>> parr; PNodePtr sub; };
struct PNodeC { std::vector<PColC> cols; };

struct ANodeC;
using ANodePtr = std::shared_ptr<ANodeC>;
struct AColC { std::optional<int> a; ANodePtr sub; };
struct ANodeC { std::vector<AColC> cols; };

struct RKeyC;
using RKeyPtr = std::shared_ptr<RKeyC>;
struct RKeyColC { std::vector<std::pair<bool, int>> low; RKeyPtr sub; };
struct RKeyC { std::vector<RKeyColC> cols; };

// 活动栈：列指针栈 + 父数组栈（与 TS stack / parent_stack 同步 push、每层 for 后 splice）。
using ColStack = std::vector<const Column*>;
using ParStack = std::vector<std::shared_ptr<std::vector<int>>>;

inline bool br_contains(const std::deque<int>& d, int v)
{
    for (int x : d) if (x == v) return true;
    return false;
}
inline int br_last_nonneg(const std::vector<int>& v)
{
    for (int i = (int)v.size() - 1; i >= 0; --i)
        if (v[i] >= 0) return i;
    return -1;
}

inline bool br_is_limit(const NodePtr& e)
{
    if (is_infinity(e)) return true;
    if (e->cols.empty()) return false;
    return !is_zero_column(e->cols.back());
}
inline NodePtr br_zero_column(int n)
{
    auto e = std::make_shared<Node>();
    Column c; c.arr.assign(n, 0); c.step = make_node();
    e->cols.push_back(std::move(c));
    return e;
}
inline NodePtr br_infinity_fs(int index, int n)
{
    NodePtr chain = make_node();
    for (int i = index; i > 0; --i)
    {
        auto wrapped = std::make_shared<Node>();
        Column c; c.arr.assign(n, i); c.step = chain;
        wrapped->cols.push_back(std::move(c));
        chain = wrapped;
    }
    auto full = std::make_shared<Node>();
    Column z; z.arr.assign(n, 0); z.step = make_node();
    full->cols.push_back(std::move(z));
    for (const Column& cc : chain->cols) full->cols.push_back(cc);
    return full;
}

// 下层父（j=0..n-1），两族共用。活动栈 push / 每层 for 后 resize（splice）与 TS 逐字一致。
PNodePtr br_lower(const NodePtr& e, int n, ColStack& stack, ParStack& pstack)
{
    const size_t lS0 = stack.size();
    auto P = std::make_shared<PNodeC>();
    for (size_t i = 0; i < e->cols.size(); ++i)
    {
        const Column& col = e->cols[i];
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
        PColC pc;
        pc.parr = parr;
        pc.sub = br_lower(col.step, n, stack, pstack);
        P->cols.push_back(std::move(pc));
    }
    stack.resize(lS0);
    pstack.resize(lS0);
    return P;
}

// abs：同趟 DFS 求第 n 个父，排除祖先链。
PNodePtr br_abs(const NodePtr& e, int n, ColStack& stack, ParStack& pstack,
                std::deque<int>& forbidden)
{
    const size_t lS0 = stack.size();
    auto P = std::make_shared<PNodeC>();
    for (size_t i = 0; i < e->cols.size(); ++i)
    {
        const Column& col = e->cols[i];
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
            if (compare_node(stack[(size_t)p]->step, col.step) < 0 &&
                !br_contains(forbidden, p)) break;
            p = (n == 0) ? p - 1 : (*(pstack[(size_t)p]))[n - 1];
        }
        (*parr)[n] = p;

        PColC pc;
        pc.parr = parr;
        forbidden.push_back(iS);
        pc.sub = br_abs(col.step, n, stack, pstack, forbidden);
        forbidden.pop_back();
        P->cols.push_back(std::move(pc));
    }
    stack.resize(lS0);
    pstack.resize(lS0);
    return P;
}

// 提升阈值树。thresholds_stack 与 TS 一致：iS<r 的非末列 push undefined 并以新空栈
// 递归（子树内 r=undefined 走 map 分支，不访问栈）；其余 push Ai 并共享栈递归。
ANodePtr br_thresholds(const NodePtr& e, const PNodePtr& P,
                       std::optional<int> ropt, int b,
                       std::deque<std::optional<int>>& stk)
{
    auto A = std::make_shared<ANodeC>();
    auto psub = [&](size_t i) -> PNodePtr {
        return (P && i < P->cols.size()) ? P->cols[i].sub : nullptr;
    };
    if (!ropt.has_value())
    {
        for (size_t i = 0; i < e->cols.size(); ++i)
        {
            std::deque<std::optional<int>> fresh;
            AColC ac;
            ac.a = std::nullopt;
            ac.sub = br_thresholds(e->cols[i].step, psub(i), std::nullopt, b, fresh);
            A->cols.push_back(std::move(ac));
        }
        return A;
    }
    const int r = *ropt;
    const size_t lS0 = stk.size();
    for (size_t i = 0; i < e->cols.size(); ++i)
    {
        const int iS = (int)stk.size();
        AColC ac;
        if (iS < r && i + 1 != e->cols.size())
        {
            std::deque<std::optional<int>> fresh;
            stk.push_back(std::nullopt);
            ac.a = std::nullopt;
            ac.sub = br_thresholds(e->cols[i].step, psub(i), std::nullopt, b, fresh);
        }
        else
        {
            std::optional<int> Ai;
            if (iS == r) Ai = b;
            else if (iS > r)
            {
                int ai = 0;
                const std::vector<int>& parr = *P->cols[i].parr;
                // TS 越界读 undefined 时比较恒 false（停）；C++ 以边界条件等价停止。
                while (ai < (int)parr.size() &&
                       parr[ai] >= r &&
                       parr[ai] < (int)stk.size() &&
                       stk[(size_t)parr[ai]].has_value() &&
                       *stk[(size_t)parr[ai]] > ai) ++ai;
                Ai = ai;
            }
            stk.push_back(Ai);
            ac.a = Ai;
            ac.sub = br_thresholds(e->cols[i].step, psub(i), r, b, stk);
        }
        A->cols.push_back(std::move(ac));
    }
    stk.resize(lS0);
    return A;
}

inline int br_bool_cmp(bool a, bool b) { return (a ? 1 : 0) - (b ? 1 : 0); }

RKeyPtr br_to_relative(const Column& base, const NodePtr& cur,
                       const ANodePtr& T, int n)
{
    auto K = std::make_shared<RKeyC>();
    for (size_t i = 0; i < cur->cols.size(); ++i)
    {
        const Column& col = cur->cols[i];
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
        kc.sub = br_to_relative(base, col.step,
            (T && i < T->cols.size()) ? T->cols[i].sub : nullptr, n);
        K->cols.push_back(std::move(kc));
    }
    return K;
}
int br_key_cmp(const RKeyPtr& a, const RKeyPtr& b)
{
    const size_t m = (std::min)(a->cols.size(), b->cols.size());
    for (size_t i = 0; i < m; ++i)
    {
        const RKeyColC& x = a->cols[i];
        const RKeyColC& y = b->cols[i];
        const size_t km = (std::min)(x.low.size(), y.low.size());
        for (size_t j = 0; j < km; ++j)
        {
            if (const int c = br_bool_cmp(x.low[j].first, y.low[j].first); c != 0)
                return c;
            if (const int c = number_cmp(x.low[j].second, y.low[j].second); c != 0)
                return c;
        }
        if (const int c = number_cmp((int)x.low.size(), (int)y.low.size()); c != 0)
            return c;
        if (const int c = br_key_cmp(x.sub, y.sub); c != 0) return c;
    }
    return number_cmp((int)a->cols.size(), (int)b->cols.size());
}

// rel：在已建好的下层父上回填第 n 个父。key 栈与 pstack 同步（活动下标）。
void br_fill_top(const NodePtr& e, const PNodePtr& P, int n,
                 ParStack& pstack, std::vector<RKeyPtr>& keys,
                 std::deque<int>& outer)
{
    const size_t lS0 = pstack.size();
    for (size_t i = 0; i < e->cols.size(); ++i)
    {
        const Column& col = e->cols[i];
        const int iS = (int)pstack.size();
        pstack.push_back(P->cols[i].parr);

        // oneTree = ascension_thresholds([col], [Pi], iS, n,
        //                               Array.from({length: iS}, () => undefined))
        auto wrapE = std::make_shared<Node>();
        wrapE->cols.push_back(col);
        auto wrapP = std::make_shared<PNodeC>();
        PColC wpc;
        wpc.parr = P->cols[i].parr;
        wpc.sub = P->cols[i].sub;
        wrapP->cols.push_back(std::move(wpc));
        std::deque<std::optional<int>> stk((size_t)iS, std::nullopt);
        const ANodePtr oneTree = br_thresholds(wrapE, wrapP, iS, n, stk);
        const ANodePtr AT = oneTree->cols[0].sub;
        const RKeyPtr key = br_to_relative(col, col.step, AT, n);
        keys.push_back(key);

        int p = iS;
        while (p >= 0)
        {
            if (!br_contains(outer, p) && br_key_cmp(keys[(size_t)p], key) < 0) break;
            p = (n == 0) ? p - 1 : (*(pstack[(size_t)p]))[n - 1];
        }
        (*(P->cols[i].parr))[n] = p;  // 回填；pstack 中共享同一对象，同步可见

        outer.push_back(iS);
        br_fill_top(col.step, P->cols[i].sub, n, pstack, keys, outer);
        outer.pop_back();
    }
    pstack.resize(lS0);
    keys.resize(lS0);
}

inline PNodePtr br_compute_parents(const NodePtr& e, int n, bool rel)
{
    ColStack stack;
    ParStack pstack;
    if (!rel)
    {
        std::deque<int> forbidden;
        return br_abs(e, n, stack, pstack, forbidden);
    }
    PNodePtr P = br_lower(e, n, stack, pstack);
    std::vector<RKeyPtr> keys;
    std::deque<int> outer;
    br_fill_top(e, P, n, pstack, keys, outer);
    return P;
}

// ---- 层 / 根 / 提升向量 ----
int br_tail_layer(const NodePtr& e)
{
    if (e->cols.empty() || is_zero_column(e->cols.back())) return -1;
    const Node* cur = e.get();
    int layer = 0;
    while (true)
    {
        const int right = (int)cur->cols.size() - 1;
        if (cur->cols[right].step->cols.empty()) return layer;
        if (!br_is_limit(cur->cols[right].step)) return layer;
        cur = cur->cols[right].step.get();
        ++layer;
    }
}
std::pair<int, int> br_root_layer(const NodePtr& e, int r)
{
    int layer = 0, len = (int)e->cols.size();
    const Node* cur = e.get();
    while (len <= r)
    {
        ++layer;
        const int right = (int)cur->cols.size() - 1;
        cur = cur->cols[right].step.get();
        len += (int)cur->cols.size();
    }
    return { layer, r - (len - (int)cur->cols.size()) };
}
// 沿尾层下沉 P 树到最深一层，返回该层 PNode。
PNodePtr br_descend_p(PNodePtr P, int tailLayer)
{
    for (int k = 0; k < tailLayer; ++k)
    {
        const int right = (int)P->cols.size() - 1;
        P = P->cols[right].sub;
    }
    return P;
}
std::pair<int, int> br_root(const NodePtr& e, const PNodePtr& Pin, int tailLayer)
{
    if (e->cols.empty() || is_zero_column(e->cols.back()))
        return { -1, -1 };
    PNodePtr P = br_descend_p(Pin, tailLayer);
    const int right = (int)P->cols.size() - 1;
    const std::vector<int>& parr = *P->cols[right].parr;
    const int b = br_last_nonneg(parr);
    if (b < 0) return { -1, -1 };  // TS: r=parr[-1]=undefined, b=-1
    const int r = parr[b];
    return { r, b };
}
std::vector<int> br_ascension_vector(const NodePtr& e, int r, int b, int tailLayer)
{
    std::vector<const Column*> st;
    for (const Column& c : e->cols) st.push_back(&c);
    const Node* cur = e.get();
    for (int k = 0; k < tailLayer; ++k)
    {
        const int right = (int)cur->cols.size() - 1;
        cur = cur->cols[right].step.get();
        for (const Column& c : cur->cols) st.push_back(&c);
    }
    std::vector<int> v(b, 0);
    for (int j = 0; j < b; ++j)
        v[j] = st.back()->arr[j] - st[(size_t)r]->arr[j];
    return v;
}

// ---- 树复制（顶层列复制 arr、step/sub 共享只读）----
NodePtr br_clone_slice_from_raw(const Node* e, int from)
{
    auto out = std::make_shared<Node>();
    out->inf = e->inf;
    for (size_t i = from; i < e->cols.size(); ++i)
        out->cols.push_back(Column{ e->cols[i].arr, e->cols[i].step });
    return out;
}
NodePtr br_clone_slice(const NodePtr& e, int from)
{
    return br_clone_slice_from_raw(e.get(), from);
}
ANodePtr br_a_slice(const ANodePtr& A, int from)
{
    auto out = std::make_shared<ANodeC>();
    for (size_t i = from; i < A->cols.size(); ++i)
        out->cols.push_back(AColC{ A->cols[i].a, A->cols[i].sub });
    return out;
}
ANodePtr br_a_pop_last(const ANodePtr& A)
{
    auto out = std::make_shared<ANodeC>();
    if (A) for (size_t i = 0; i + 1 < A->cols.size(); ++i)
        out->cols.push_back(AColC{ A->cols[i].a, A->cols[i].sub });
    return out;
}
NodePtr br_node_pop_last(const NodePtr& e)
{
    auto out = std::make_shared<Node>();
    out->inf = e->inf;
    for (size_t i = 0; i + 1 < e->cols.size(); ++i)
        out->cols.push_back(Column{ e->cols[i].arr, e->cols[i].step });
    return out;
}

NodePtr br_ascend_replace(const NodePtr& e, const NodePtr& tail,
                          std::optional<int> tl, const ANodePtr& A,
                          const std::vector<int>& V, int w)
{
    auto out = std::make_shared<Node>();
    for (size_t i = 0; i < e->cols.size(); ++i)
    {
        if (tl.has_value() && *tl == 0 && i + 1 == e->cols.size())
        {
            for (const Column& tc : tail->cols)
                out->cols.push_back(Column{ tc.arr, tc.step });
        }
        else
        {
            const Column& col = e->cols[i];
            Column nc;
            nc.arr = col.arr;
            const int Ai = (A && i < A->cols.size() && A->cols[i].a.has_value())
                ? *A->cols[i].a : 0;
            for (int j = 0; j < Ai; ++j) nc.arr[j] += V[j] * w;
            std::optional<int> ntl;
            if (i + 1 == e->cols.size() && tl.has_value()) ntl = *tl - 1;
            nc.step = br_ascend_replace(col.step, tail, ntl,
                (A && i < A->cols.size()) ? A->cols[i].sub : nullptr, V, w);
            out->cols.push_back(std::move(nc));
        }
    }
    return out;
}

NodePtr br_fs(const NodePtr& e, int index, int n, bool rel)
{
    if (is_infinity(e)) return br_infinity_fs(index, n);
    if (e->cols.empty()) return e;
    if (!br_is_limit(e)) return br_node_pop_last(e);  // 后继：删去末列

    PNodePtr P = br_compute_parents(e, n, rel);
    const int t_layer = br_tail_layer(e);
    const auto rb = br_root(e, P, t_layer);
    const int r = rb.first;
    const int b = rb.second;
    const bool hasRoot = (b >= 0);
    // TS：b<0 时 r=undefined → compute_root_layer 得 [0, NaN]（slice(NaN)=slice(0)）、
    // A 走 r=undefined 的 map 分支、V=[]、b===n 永不触发。

    int r_layer = 0, ri = 0;
    if (hasRoot)
    {
        const auto rl = br_root_layer(e, r);
        r_layer = rl.first;
        ri = rl.second;
    }
    std::deque<std::optional<int>> stk;
    const ANodePtr A = br_thresholds(e, P,
        hasRoot ? std::optional<int>(r) : std::nullopt, b, stk);
    const std::vector<int> V = hasRoot
        ? br_ascension_vector(e, r, b, t_layer) : std::vector<int>{};

    const Node* cur = e.get();
    ANodePtr curA = A;
    for (int k = 0; k < r_layer; ++k)
    {
        const int right = (int)cur->cols.size() - 1;
        cur = cur->cols[right].step.get();
        curA = curA->cols[right].sub;
    }
    NodePtr copy_part = br_clone_slice_from_raw(cur, ri);
    ANodePtr copy_partA = br_a_slice(curA, ri);
    for (int k = r_layer; k < t_layer; ++k)
    {
        const int right = (int)cur->cols.size() - 1;
        cur = cur->cols[right].step.get();
        curA = curA->cols[right].sub;
    }
    const int right = (int)cur->cols.size() - 1;
    NodePtr tail_top = br_node_pop_last(cur->cols[right].step);
    ANodePtr tail_topA = br_a_pop_last(curA->cols[right].sub);

    NodePtr result = make_node();
    for (int w = index; w > 0; --w)
    {
        result = br_ascend_replace(copy_part, result, t_layer - r_layer,
                                   copy_partA, V, w);
        if (b == n)
            result->cols[0].step =
                br_ascend_replace(tail_top, make_node(), std::nullopt,
                                  tail_topA, V, w - 1);
    }
    result = br_ascend_replace(e, result, t_layer, A, V, 0);
    return result;
}

inline NodePtr br_lnz(const NodePtr& e, int index, int n, bool rel)
{
    if (is_infinity(e)) return br_infinity_fs(index, n);
    if (!br_is_limit(e)) return br_fs(e, 0, n, rel);
    return br_fs(e, index, n, rel);
}

// ---- branch 语义显示（严格版，勿改 tm1_detail 的宽松版：T 族 golden 依赖）----
// TS branch 引擎：is_one_column n==0 → step 恰 1 列且该列为零列；
// display 的 ω 简写条件 = e.length===2 && is_zero_column(e[0]) && is_one_column(e[1])。
inline bool br_is_one_column(const Column& c)
{
    const int n = (int)c.arr.size();
    if (n == 0)
        return c.step->cols.size() == 1 && is_zero_column(c.step->cols[0]);
    if (c.arr[0] != 1 || !c.step->cols.empty()) return false;
    for (size_t i = 1; i < c.arr.size(); ++i)
        if (c.arr[i] != 0) return false;
    return true;
}
inline std::string br_render(const NodePtr& e, bool top_level);
inline std::string br_column_display(const Column& c)
{
    std::vector<std::string> list;
    for (int x : c.arr) list.push_back(std::to_string(x));
    list.push_back(br_render(c.step, false));
    while (!list.empty() && list.back() == "0") list.pop_back();
    std::string s = "(";
    for (size_t i = 0; i < list.size(); ++i) { if (i) s += ","; s += list[i]; }
    s += ")";
    return s;
}
inline std::string br_render(const NodePtr& e, bool top_level)
{
    if (is_infinity(e)) return "Limit";
    if (!top_level)
    {
        bool allZero = true;
        for (const Column& c : e->cols) if (!is_zero_column(c)) { allZero = false; break; }
        if (allZero) return std::to_string(e->cols.size());
        if (e->cols.size() == 2 && is_zero_column(e->cols[0]) &&
            br_is_one_column(e->cols[1])) return "\xCF\x89";  // ω UTF-8
    }
    std::string s;
    for (const Column& c : e->cols) s += br_column_display(c);
    return s;
}

template <bool kRel>
struct BranchNotation
{
    static std::string suffix() { return std::string{}; }
    static constexpr int kParamMin = 0;
    static constexpr int kParamMax = 25;
    static std::string expand_text_n(std::string_view text, int term, int n)
    {
        try
        {
            n = (std::max)(kParamMin, (std::min)(kParamMax, n));
            if (term < 1) term = 1;
            Parser parser(text);
            NodePtr m = parser.parse(n);
            return br_render(br_lnz(m, term, n, kRel), true);
        }
        catch (...)
        {
            return bmlike::kParseError;
        }
    }
};

using AbsBM1Notation = BranchNotation<false>;
using RelBM1Notation = BranchNotation<true>;
}  // namespace bm1_detail

using bm1_detail::AbsBM1Notation;
using bm1_detail::RelBM1Notation;
}  // namespace omegay::notation

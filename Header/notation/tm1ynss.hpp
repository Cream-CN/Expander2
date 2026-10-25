// T_Minus1_Y_nSS（T(n+1)SS，超限 -1Y 序列系统）—— BM-like 递归嵌套矩阵。
//
// 移植自 ne-rewritten 的 Minus1_Y_nSS-series/T_Minus1_Y_nSS.ts。
// 参数化族：n 为元组宽度（UI 输入框，0 <= n <= 25）。
// 与基础族 m1ynss 的区别：一列 = [长度 n 的父标号数组 arr, 递归子表达式 step]，
// 即步长不是整数而是另一棵表达式树；取父、比较、提升都递归进行。
// 显示时嵌套子树用“纯空列数量”的数字或 ω 简写。舍弃 BOCF 等价显示。
#pragma once

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Header/notation/bmlike_share.hpp"

namespace omegay::notation
{
namespace tm1_detail
{
struct Node;
struct Column
{
    std::vector<int> arr;
    std::shared_ptr<Node> step;
};
struct Node
{
    bool inf = false;
    std::vector<Column> cols;
};

using NodePtr = std::shared_ptr<Node>;

inline NodePtr make_node() { return std::make_shared<Node>(); }
inline bool is_infinity(const NodePtr& e) { return e && e->inf; }

inline NodePtr empty_column(int n)
{
    auto c = std::make_shared<Node>();
    Column col;
    col.arr.assign(n, 0);
    col.step = make_node();
    c->cols.push_back(std::move(col));
    return c;
}
inline Column one_column(int n)
{
    Column c;
    if (n == 0)
    {
        c.step = std::make_shared<Node>();
        c.step->cols.push_back(Column{});
        c.step->cols.back().arr.assign(0, 0);
        c.step->cols.back().step = make_node();
    }
    else
    {
        c.arr.assign(n, 0);
        c.arr[0] = 1;
        c.step = make_node();
    }
    return c;
}

inline int number_cmp(int a, int b) { return a < b ? -1 : (a > b ? 1 : 0); }
inline int lex_num(const std::vector<int>& a, const std::vector<int>& b)
{
    const size_t m = (std::min)(a.size(), b.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = number_cmp(a[i], b[i]); c != 0) return c;
    return number_cmp((int)a.size(), (int)b.size());
}
int compare_node(const NodePtr& a, const NodePtr& b);
inline int column_compare(const Column& a, const Column& b)
{
    if (const int c = lex_num(a.arr, b.arr); c != 0) return c;
    return compare_node(a.step, b.step);
}
inline int compare_node(const NodePtr& a, const NodePtr& b)
{
    const bool ai = is_infinity(a), bi = is_infinity(b);
    if (ai || bi) return number_cmp(ai ? 1 : 0, bi ? 1 : 0);
    const size_t m = (std::min)(a->cols.size(), b->cols.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = column_compare(a->cols[i], b->cols[i]); c != 0) return c;
    return number_cmp((int)a->cols.size(), (int)b->cols.size());
}

inline int index_of_last_nonneg(const std::vector<int>& v)
{
    for (int i = (int)v.size() - 1; i >= 0; --i)
        if (v[i] >= 0) return i;
    return -1;
}

inline bool is_limit(const NodePtr& e, int n)
{
    if (is_infinity(e)) return true;
    if (e->cols.empty()) return false;
    const Column& last = e->cols.back();
    return n == 0 ? !last.step->cols.empty() : (!last.arr.empty() && last.arr[0] > 0);
}

inline NodePtr infinity_fs(int index, int n)
{
    auto e = std::make_shared<Node>();
    Column c0;
    c0.arr.assign(n, 0);
    c0.step = make_node();
    e->cols.push_back(std::move(c0));
    if (index > 0)
    {
        Column c1;
        c1.arr.assign(n, 1);
        c1.step = infinity_fs(index - 1, n);
        e->cols.push_back(std::move(c1));
    }
    return e;
}

// 父标号表用扁平 Node 表示：每列 arr 存父列下标，step 列存一个下标。
struct PCol { std::vector<int> arr; int step; };
struct PNode { std::vector<PCol> cols; };

inline PNode parents(const NodePtr& e, int n)
{
    PNode P;
    if (is_infinity(e)) return P;
    P.cols.resize(e->cols.size());
    for (int i = 0; i < (int)e->cols.size(); ++i)
    {
        PCol pc;
        pc.arr.assign(n, -1);
        pc.step = -1;
        for (int j = 0; j < n; ++j)
        {
            const int v = j < (int)e->cols[i].arr.size() ? e->cols[i].arr[j] : 0;
            int p = (j == 0) ? i - 1 : pc.arr[j - 1];
            while (p >= 0)
            {
                if (e->cols[p].arr[j] < v) break;
                p = (j == 0) ? p - 1 : P.cols[p].arr[j - 1];
            }
            if (p < 0) break;
            pc.arr[j] = p;
        }
        const NodePtr& v = e->cols[i].step;
        int p = (n == 0) ? i - 1 : pc.arr[n - 1];
        while (p >= 0)
        {
            if (compare_node(e->cols[p].step, v) < 0) break;
            p = (n == 0) ? p - 1 : P.cols[p].arr[n - 1];
        }
        pc.step = p;
        P.cols[i] = std::move(pc);
    }
    return P;
}

inline std::optional<std::pair<int, int>> root(const PNode& P, int n)
{
    if (P.cols.empty()) return std::nullopt;
    const int right = (int)P.cols.size() - 1;
    if (P.cols[right].step >= 0)
        return std::pair<int, int>{ P.cols[right].step, n };
    const int b = index_of_last_nonneg(P.cols[right].arr);
    if (b == -1) return std::nullopt;
    return std::pair<int, int>{ P.cols[right].arr[b], b };
}

inline std::vector<int> ascension_vector(const NodePtr& e, int r, int b)
{
    std::vector<int> v(b, 0);
    const Column& tail = e->cols.back();
    const Column& base = e->cols[r];
    for (int i = 0; i < b; ++i) v[i] = tail.arr[i] - base.arr[i];
    return v;
}
inline std::vector<int> ascension_thresholds(const PNode& P, int r, int b)
{
    std::vector<int> result(P.cols.size(), 0);
    result[r] = b;
    for (int i = r + 1; i < (int)P.cols.size(); ++i)
    {
        int ai = 0;
        while (ai < b)
        {
            int p = i;
            while (p > r) p = P.cols[p].arr[ai];
            if (p < r) break;
            ++ai;
        }
        result[i] = ai;
    }
    return result;
}
inline Column ascend(const Column& ei, const std::vector<int>& delta, int b, int w)
{
    Column out;
    out.arr = ei.arr;             // 深拷贝父标号数组
    out.step = ei.step;           // 子表达式共享（不可变）
    for (int i = 0; i < b; ++i) out.arr[i] += delta[i] * w;
    return out;
}

inline NodePtr pop_last(const NodePtr& e)
{
    auto r = std::make_shared<Node>();
    for (size_t i = 0; i + 1 < e->cols.size(); ++i) r->cols.push_back(e->cols[i]);
    return r;
}

NodePtr fs(const NodePtr& e, int index, int n)
{
    if (is_infinity(e)) return infinity_fs(index, n);
    if (e->cols.empty()) return e;
    const int right = (int)e->cols.size() - 1;
    if (is_limit(e->cols[right].step, n))
    {
        auto r = std::make_shared<Node>();
        for (int i = 0; i < right; ++i) r->cols.push_back(e->cols[i]);
        Column c;
        c.arr = e->cols[right].arr;
        c.step = fs(e->cols[right].step, index, n);
        r->cols.push_back(std::move(c));
        return r;
    }
    const PNode P = parents(e, n);
    const auto rb = root(P, n);
    if (!rb.has_value()) return pop_last(e);
    const int r0 = rb->first;
    const int b = rb->second;
    const int width = right - r0;
    const std::vector<int> V = ascension_vector(e, r0, b);
    const std::vector<int> A = ascension_thresholds(P, r0, b);

    auto result = std::make_shared<Node>();
    for (int i = 0; i < right; ++i)
    {
        Column c;
        c.arr = e->cols[i].arr;
        c.step = e->cols[i].step;
        result->cols.push_back(std::move(c));
    }
    for (int w = 1; w <= index; ++w)
    {
        for (int i = r0; i < right; ++i)
            result->cols.push_back(ascend(e->cols[i], V, A[i], w));
        if (b == n)
            result->cols[r0 + w * width].step = pop_last(e->cols[right].step);
    }
    return result;
}

// LNZ 外壳。
inline NodePtr lnz(const NodePtr& e, int index, int n)
{
    if (is_infinity(e)) return infinity_fs(index, n);
    if (!is_limit(e, n)) return fs(e, 0, n);
    return fs(e, index, n);
}

inline bool is_zero_column(const Column& c)
{
    for (int x : c.arr) if (x != 0) return false;
    return c.step->cols.empty();
}
inline bool is_one_column(const Column& c)
{
    const int n = (int)c.arr.size();
    if (n == 0) return c.step->cols.size() == 1;
    if (c.arr[0] != 1 || !c.step->cols.empty()) return false;
    for (size_t i = 1; i < c.arr.size(); ++i)
        if (c.arr[i] != 0) return false;
    return true;
}
std::string render(const NodePtr& e, bool top_level);
inline std::string column_display(const Column& c)
{
    std::vector<std::string> list;
    for (int x : c.arr) list.push_back(std::to_string(x));
    list.push_back(render(c.step, false));
    while (!list.empty() && list.back() == "0") list.pop_back();
    std::string s = "(";
    for (size_t i = 0; i < list.size(); ++i) { if (i) s += ","; s += list[i]; }
    s += ")";
    return s;
}
inline std::string render(const NodePtr& e, bool top_level)
{
    if (is_infinity(e)) return "Limit";
    if (!top_level)
    {
        bool allZero = true;
        for (const Column& c : e->cols) if (!is_zero_column(c)) { allZero = false; break; }
        if (allZero) return std::to_string(e->cols.size());
        if (e->cols.size() == 2 && is_one_column(e->cols[1])) return "\xCF\x89";  // ω UTF-8
    }
    std::string s;
    for (const Column& c : e->cols) s += column_display(c);
    return s;
}

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
    NodePtr parse_expr(bool top_level)
    {
        sp();
        if (i_ + 5 <= s_.size() && s_.substr(i_, 5) == "Limit")
        {
            i_ += 5;
            auto e = std::make_shared<Node>();
            e->inf = true;
            return e;
        }
        if (!top_level)
        {
            // 裸数字 = 该数量个空列；ω/w = [空列, 一列]。
            if (i_ < s_.size() && s_[i_] >= '0' && s_[i_] <= '9')
            {
                const int cnt = num_tok();
                auto e = std::make_shared<Node>();
                for (int k = 0; k < cnt; ++k)
                {
                    Column c;
                    c.arr.assign(n_, 0);
                    c.step = make_node();
                    e->cols.push_back(std::move(c));
                }
                return e;
            }
            // ω = 0xCF 0x89
            if (i_ + 1 < s_.size() &&
                static_cast<unsigned char>(s_[i_]) == 0xCF &&
                static_cast<unsigned char>(s_[i_ + 1]) == 0x89)
            {
                i_ += 2;
                auto e = std::make_shared<Node>();
                Column c0; c0.arr.assign(n_, 0); c0.step = make_node();
                e->cols.push_back(std::move(c0));
                e->cols.push_back(one_column(n_));
                return e;
            }
            if (i_ < s_.size() && s_[i_] == 'w')
            {
                ++i_;
                auto e = std::make_shared<Node>();
                Column c0; c0.arr.assign(n_, 0); c0.step = make_node();
                e->cols.push_back(std::move(c0));
                e->cols.push_back(one_column(n_));
                return e;
            }
        }
        auto e = std::make_shared<Node>();
        sp();
        while (i_ < s_.size() && s_[i_] == '(')
        {
            e->cols.push_back(parse_column());
            sp();
        }
        return e;
    }
    Column parse_column()
    {
        sp();
        if (i_ >= s_.size() || s_[i_] != '(') err();
        ++i_;
        sp();
        Column c;
        NodePtr step = make_node();
        if (n_ == 0)
        {
            step = parse_expr(false);
        }
        else
        {
            c.arr.assign(n_, 0);
            for (int j = 0; j < n_; ++j)
            {
                if (j > 0)
                {
                    sp();
                    if (i_ >= s_.size() || s_[i_] != ',') continue;
                    ++i_;
                }
                sp();
                if (i_ < s_.size() && s_[i_] >= '0' && s_[i_] <= '9')
                    c.arr[j] = num_tok();
            }
            sp();
            if (i_ < s_.size() && s_[i_] == ',')
            {
                ++i_;
                step = parse_expr(false);
            }
        }
        c.step = step;
        sp();
        if (i_ >= s_.size() || s_[i_] != ')') err();
        ++i_;
        return c;
    }
    NodePtr parse(int n)
    {
        n_ = n;
        NodePtr e = parse_expr(true);
        sp();
        if (i_ != s_.size()) err();
        return e;
    }
};
}  // namespace tm1_detail

struct TM1YnssNotation
{
    static std::string suffix() { return std::string{}; }
    static constexpr int kParamMin = 0;
    static constexpr int kParamMax = 25;
    static std::string expand_text_n(std::string_view text, int term, int n)
    {
        using namespace tm1_detail;
        try
        {
            n = (std::max)(kParamMin, (std::min)(kParamMax, n));
            if (term < 1) term = 1;
            Parser parser(text);
            NodePtr m = parser.parse(n);
            return render(lnz(m, term, n), true);
        }
        catch (...)
        {
            return bmlike::kParseError;
        }
    }
};
}  // namespace omegay::notation

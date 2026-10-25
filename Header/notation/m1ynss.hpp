// Minus1_Y_nSS（(-1)Y-(n+1)SS，-1Y n-元组序列系统）—— BM-like 平面矩阵。
//
// 移植自 ne-rewritten 的 Minus1_Y_nSS-series/Minus1_Y_nSS.ts。
// 这是“参数化族”：n 为元组宽度（UI 提供输入框，0 <= n <= 25）。
// 表达式 Expr = Column[]；每个 Column = [长度为 n 的父标号数组 arr, 末尾 step]。
// 显示时把 arr 与 step 依次列出并去掉尾随 0，再用圆括号包成一列；空列即 ()。
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
namespace m1ynss_detail
{
struct Column
{
    std::vector<int> arr;
    int step = 0;
};
struct Expr
{
    bool inf = false;
    std::vector<Column> cols;
};

inline bool is_infinity(const Expr& e) { return e.inf; }

inline int number_cmp(int a, int b) { return a < b ? -1 : (a > b ? 1 : 0); }
inline int lex_num(const std::vector<int>& a, const std::vector<int>& b)
{
    const size_t m = (std::min)(a.size(), b.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = number_cmp(a[i], b[i]); c != 0) return c;
    return number_cmp((int)a.size(), (int)b.size());
}
inline int column_cmp(const Column& a, const Column& b)
{
    if (const int c = lex_num(a.arr, b.arr); c != 0) return c;
    return number_cmp(a.step, b.step);
}
inline int compare(const Expr& a, const Expr& b)
{
    const size_t m = (std::min)(a.cols.size(), b.cols.size());
    for (size_t i = 0; i < m; ++i)
        if (const int c = column_cmp(a.cols[i], b.cols[i]); c != 0) return c;
    return number_cmp((int)a.cols.size(), (int)b.cols.size());
}
inline int index_of_last_nonneg(const std::vector<int>& v)
{
    for (int i = (int)v.size() - 1; i >= 0; --i)
        if (v[i] >= 0) return i;
    return -1;
}

// 父标号表：与 Expr 同形，但每个位置存放的是“父列下标”。
inline Expr parents(const Expr& e, int n)
{
    if (is_infinity(e)) return Expr{};
    Expr P;
    P.cols.resize(e.cols.size());
    for (int i = 0; i < (int)e.cols.size(); ++i)
    {
        Column pc;
        pc.arr.assign(n, -1);
        pc.step = -1;
        for (int j = 0; j < n; ++j)
        {
            const int v = j < (int)e.cols[i].arr.size() ? e.cols[i].arr[j] : 0;
            int p = (j == 0) ? i - 1 : pc.arr[j - 1];
            while (p >= 0)
            {
                if (e.cols[p].arr[j] < v) break;
                p = (j == 0) ? p - 1 : P.cols[p].arr[j - 1];
            }
            if (p < 0) break;
            pc.arr[j] = p;
        }
        const int v = e.cols[i].step;
        int p = (n == 0) ? i - 1 : pc.arr[n - 1];
        while (p >= 0)
        {
            if (e.cols[p].step < v) break;
            p = (n == 0) ? p - 1 : P.cols[p].arr[n - 1];
        }
        pc.step = p;
        P.cols[i] = std::move(pc);
    }
    return P;
}

// 返回 {r, b}；无根时返回 nullopt。
inline std::optional<std::pair<int, int>> root(const Expr& P, int n)
{
    if (P.cols.empty()) return std::nullopt;
    const int right = (int)P.cols.size() - 1;
    if (P.cols[right].step >= 0)
        return std::pair<int, int>{ P.cols[right].step, n };
    const int b = index_of_last_nonneg(P.cols[right].arr);
    if (b == -1) return std::nullopt;
    return std::pair<int, int>{ P.cols[right].arr[b], b };
}

inline std::vector<int> ascension_vector(const Expr& e, int r, int b)
{
    std::vector<int> v(b, 0);
    const Column& tail = e.cols.back();
    const Column& base = e.cols[r];
    for (int i = 0; i < b; ++i) v[i] = tail.arr[i] - base.arr[i];
    return v;
}

inline std::vector<int> ascension_thresholds(const Expr& P, int r, int b)
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

inline Column ascend(const Column& ei, const std::vector<int>& delta,
                     int b, int w)
{
    Column out = ei;  // 深拷贝 arr；step 为标量
    for (int i = 0; i < b; ++i) out.arr[i] += delta[i] * w;
    return out;
}

inline Expr infinity_fs(int index, int n)
{
    Expr e;
    Column c0;
    c0.arr.assign(n, 0);
    c0.step = 0;
    Column c1;
    c1.arr.assign(n, 1);
    c1.step = index;
    e.cols.push_back(std::move(c0));
    e.cols.push_back(std::move(c1));
    return e;
}

inline Expr fs(Expr e, int index, int n)
{
    if (is_infinity(e)) return infinity_fs(index, n);
    if (e.cols.empty()) return e;
    const Expr P = parents(e, n);
    const auto rb = root(P, n);
    if (!rb.has_value())
    {
        Expr r;
        for (size_t i = 0; i + 1 < e.cols.size(); ++i)
            r.cols.push_back(e.cols[i]);
        return r;
    }
    const int right = (int)e.cols.size() - 1;
    const int r = rb->first;
    const int b = rb->second;
    const int width = right - r;
    const std::vector<int> V = ascension_vector(e, r, b);
    const std::vector<int> A = ascension_thresholds(P, r, b);

    Expr result;
    for (size_t i = 0; i + 1 < e.cols.size(); ++i)
        result.cols.push_back(e.cols[i]);  // 后续仅按值读出，浅拷贝即可

    for (int w = 1; w <= index; ++w)
    {
        for (int i = r; i < right; ++i)
            result.cols.push_back(ascend(e.cols[i], V, A[i], w));
        if (b == n)
            result.cols[r + w * width].step = e.cols[right].step - 1;
    }
    return result;
}

inline std::string column_display(const Column& c)
{
    std::vector<int> list = c.arr;
    list.push_back(c.step);
    while (!list.empty() && list.back() == 0) list.pop_back();
    std::string s = "(";
    for (size_t i = 0; i < list.size(); ++i)
    {
        if (i) s += ",";
        s += std::to_string(list[i]);
    }
    s += ")";
    return s;
}
inline std::string display(const Expr& e)
{
    if (is_infinity(e)) return "Limit";
    std::string s;
    for (const Column& c : e.cols) s += column_display(c);
    return s;
}

struct Parser
{
    explicit Parser(std::string_view s) : s_(s) {}
    std::string_view s_;
    size_t i_ = 0;
    [[noreturn]] void err() { throw std::runtime_error("illegal"); }
    void sp() { while (i_ < s_.size() && s_[i_] == ' ') ++i_; }
    int num()
    {
        sp();
        const size_t start = i_;
        while (i_ < s_.size() && s_[i_] >= '0' && s_[i_] <= '9') ++i_;
        if (start == i_) err();
        int v = 0;
        for (size_t k = start; k < i_; ++k) v = v * 10 + (s_[k] - '0');
        return v;
    }
    Column parse_column(int n)
    {
        sp();
        if (i_ >= s_.size() || s_[i_] != '(') err();
        ++i_;
        sp();
        std::vector<int> values;
        if (i_ < s_.size() && s_[i_] != ')')
        {
            values.push_back(num());
            while (true)
            {
                sp();
                if (i_ >= s_.size() || s_[i_] != ',') break;
                ++i_;
                sp();
                if (i_ < s_.size() && s_[i_] == ')') break;
                values.push_back(num());
            }
        }
        sp();
        if (i_ >= s_.size() || s_[i_] != ')') err();
        ++i_;
        Column c;
        for (int k = 0; k < n; ++k)
            c.arr.push_back(k < (int)values.size() ? values[k] : 0);
        c.step = (int)values.size() > n ? values[n] : 0;
        return c;
    }
    Expr parse(int n)
    {
        sp();
        if (i_ + 5 <= s_.size() && s_.substr(i_, 5) == "Limit")
        {
            i_ += 5;
            sp();
            if (i_ != s_.size()) err();
            Expr e; e.inf = true;
            return e;
        }
        Expr e;
        sp();
        while (i_ < s_.size())
        {
            if (s_[i_] != '(') break;
            e.cols.push_back(parse_column(n));
            sp();
        }
        if (i_ != s_.size()) err();
        return e;
    }
};
}  // namespace m1ynss_detail

// Minus1_Y_nSS 记号接口：参数 n 经 UI 输入（0 <= n <= 25）。
struct M1YnssNotation
{
    static std::string suffix() { return std::string{}; }
    static constexpr int kParamMin = 0;
    static constexpr int kParamMax = 25;
    static std::string expand_text_n(std::string_view text, int term, int n)
    {
        using namespace m1ynss_detail;
        try
        {
            n = (std::max)(kParamMin, (std::min)(kParamMax, n));
            if (term < 1) term = 1;
            Parser parser(text);
            const Expr m = parser.parse(n);
            return display(fs(m, term, n));
        }
        catch (...)
        {
            return bmlike::kParseError;
        }
    }
};
}  // namespace omegay::notation

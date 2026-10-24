// BTBM（Bubby3's Transfinite BMS，Bubby3 超限巴斯丘矩阵）—— BM-like 分层树形记号。
//
// 移植自 ne-rewritten 的 BTBM.ts，仅保留核心展开（FS）与 plain 文本解析/显示。
// 舍弃 convert_to_layer/from_layer（“层”等价表示）、display_marked（列号标记）、
// html/latex 渲染等纯显示层。
//
// 结构：Expr=Column[]，Column=Entry[]，Entry={value,height:Expr}。内部 value 为
// 0-based，显示时 +1；空列显示 (0)，输入也用 (0) 表示空列。末元素的 height 嵌套
// 即“下一层（layer）”，FS 通过在各层间定位 tail/root 并整体替换末层来展开。
// 另需一套带 mark 的 Height 结构，仅用于根列追加位置（root_appending_start）比较；
// vertical_increase/vertical_diff/from_height 供 BTBM-weak 复用。
#pragma once

#include <algorithm>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Header/notation/bmlike_share.hpp"

namespace omegay::notation
{
namespace btbm_detail
{

struct Expr;
struct Entry
{
    int value = 0;
    std::shared_ptr<Expr> height;  // 缺省为空表达式（height.length === 0）
};
using Column = std::vector<Entry>;
struct Expr
{
    bool inf = false;  // INFINITY 哨兵
    std::vector<Column> cols;
};

// 带标记的 Height（to_height 的结果，仅用于比较）。
struct HExpr;
struct HEntry
{
    bool mark = false;
    int value = 0;
    std::shared_ptr<HExpr> height;
};
using HColumn = std::vector<HEntry>;
struct HExpr
{
    std::vector<HColumn> cols;
};

inline std::shared_ptr<Expr> empty_height()
{
    return std::make_shared<Expr>();
}
inline Expr make_inf() { Expr e; e.inf = true; return e; }
inline bool is_infinity(const Expr& e) { return e.inf; }

inline int number_cmp(int a, int b) { return a == b ? 0 : (a < b ? -1 : 1); }
inline int boolean_cmp(bool a, bool b)
{
    return (a ? 1 : 0) - (b ? 1 : 0);
}

// ---- 极限基本列 ----
inline Expr infinity_fs(int index)
{
    if (index == 0)
    {
        Expr e;
        e.cols.push_back(Column{});
        return e;
    }
    Column col;
    {
        Entry en;
        en.value = index - 1;
        en.height = empty_height();
        col.push_back(std::move(en));
    }
    for (int i = index - 1; i > 0; --i)
    {
        Column next;
        Entry en;
        en.value = i - 1;
        auto inner = std::make_shared<Expr>();
        inner->cols.push_back(col);
        en.height = inner;
        next.push_back(std::move(en));
        col = std::move(next);
    }
    Expr e;
    e.cols.push_back(Column{});
    e.cols.push_back(std::move(col));
    return e;
}

// ---- to_height / from_height ----
inline HExpr to_height(const Expr& e, int r);
inline HExpr to_height(const Expr& e, int r)
{
    HExpr result;
    for (const Column& col : e.cols)
    {
        HColumn rc;
        for (const Entry& entry : col)
        {
            const int p = entry.value;
            HEntry he;
            if (p < r) { he.mark = false; he.value = p; }
            else { he.mark = true; he.value = p - r; }
            he.height = std::make_shared<HExpr>(to_height(*entry.height, r));
            rc.push_back(std::move(he));
        }
        result.cols.push_back(std::move(rc));
    }
    return result;
}
inline Expr from_height(const HExpr& h, int r)
{
    Expr result;
    for (const HColumn& col : h.cols)
    {
        Column rc;
        for (const HEntry& entry : col)
        {
            Entry e;
            e.value = entry.value + (entry.mark ? r : 0);
            e.height = std::make_shared<Expr>(from_height(*entry.height, r));
            rc.push_back(std::move(e));
        }
        result.cols.push_back(std::move(rc));
    }
    return result;
}

inline int height_cmp(const HExpr& a, const HExpr& b);
inline int height_entry_cmp(const HEntry& a, const HEntry& b)
{
    if (const int c = boolean_cmp(a.mark, b.mark); c != 0) return c;
    if (const int c = number_cmp(a.value, b.value); c != 0) return c;
    return height_cmp(*a.height, *b.height);
}
inline int height_cmp(const HExpr& a, const HExpr& b)
{
    const size_t n = (std::min)(a.cols.size(), b.cols.size());
    for (size_t i = 0; i < n; ++i)
    {
        const HColumn& ca = a.cols[i];
        const HColumn& cb = b.cols[i];
        const size_t m = (std::min)(ca.size(), cb.size());
        for (size_t j = 0; j < m; ++j)
        {
            const int c = height_entry_cmp(ca[j], cb[j]);
            if (c != 0) return c;
        }
        if (const int c = number_cmp((int)ca.size(), (int)cb.size()); c != 0)
            return c;
    }
    return number_cmp((int)a.cols.size(), (int)b.cols.size());
}
using Vertical = std::vector<HExpr>;
inline int vertical_cmp(const Vertical& a, const Vertical& b)
{
    const size_t n = (std::min)(a.size(), b.size());
    for (size_t i = 0; i < n; ++i)
    {
        const int c = height_cmp(a[i], b[i]);
        if (c != 0) return c;
    }
    return number_cmp((int)a.size(), (int)b.size());
}

// ---- 层（layer）导航 ----
inline bool has_next_layer(const Expr& e)
{
    const int right = (int)e.cols.size() - 1;
    if (right == -1) return false;
    const int top = (int)e.cols[right].size() - 1;
    return top != -1;
}
inline const Expr& next_layer(const Expr& e)
{
    const int right = (int)e.cols.size() - 1;
    const int top = (int)e.cols[right].size() - 1;
    return *e.cols[right][top].height;
}
inline const Expr& skip_layers(const Expr& e, int l)
{
    const Expr* cur = &e;
    for (int i = 0; i < l; ++i) cur = &next_layer(*cur);
    return *cur;
}
inline int tail_layer(const Expr& e)
{
    if (!has_next_layer(e)) return -1;
    return 1 + tail_layer(next_layer(e));
}
inline int tail(const Expr& eIn, int t_layer)
{
    int current_left = 0;
    const Expr* cur = &eIn;
    for (int i = 0; i < t_layer; ++i)
    {
        current_left += (int)cur->cols.size();
        cur = &next_layer(*cur);
    }
    return current_left + (int)cur->cols.size() - 1;
}
inline int root(const Expr& eIn, int t_layer)
{
    const Expr& cur = skip_layers(eIn, t_layer);
    const Column& last = cur.cols.back();
    return last.back().value;
}
// 返回 {layer, ri}
inline std::pair<int, int> root_layer(const Expr& eIn, int r)
{
    const Expr* cur = &eIn;
    int current_left = 0;
    int current_layer = 0;
    for (;;)
    {
        if (r < current_left + (int)cur->cols.size())
            return { current_layer, r - current_left };
        current_left += (int)cur->cols.size();
        cur = &next_layer(*cur);
        ++current_layer;
    }
}

// BTBM 版整体替换提升。tl 为 nullopt 表示 TS 的 undefined。
inline Expr ascend_replace(const Expr& e, int r, int diff,
                           std::optional<int> tl, const Expr& new_tail)
{
    Expr result;
    for (size_t ii = 0; ii < e.cols.size(); ++ii)
    {
        const int i = (int)ii;
        if (tl.has_value() && *tl == 0 && i == (int)e.cols.size() - 1)
        {
            for (const Column& c : new_tail.cols) result.cols.push_back(c);
        }
        else
        {
            const Column& col = e.cols[ii];
            Column rc;
            for (size_t jj = 0; jj < col.size(); ++jj)
            {
                const int j = (int)jj;
                std::optional<int> new_tl;
                if (tl.has_value() && i == (int)e.cols.size() - 1 &&
                    j == (int)col.size() - 1)
                    new_tl = *tl - 1;
                const Entry& entry = col[jj];
                Entry ne;
                ne.value = entry.value >= r ? entry.value + diff : entry.value;
                ne.height = std::make_shared<Expr>(
                    ascend_replace(*entry.height, r, diff, new_tl, new_tail));
                rc.push_back(std::move(ne));
            }
            result.cols.push_back(std::move(rc));
        }
    }
    return result;
}

inline bool is_special(const Expr& eIn, int t_layer)
{
    if (t_layer == 0) return false;
    const Expr* cur = &eIn;
    int current_left = 0;
    for (int i = 0; i < t_layer; ++i)
    {
        current_left += (int)cur->cols.size();
        cur = &next_layer(*cur);
    }
    if (cur->cols.back().size() != 1) return false;
    const Entry& entry = cur->cols.back()[0];
    return entry.height->cols.empty() && entry.value == current_left - 1;
}
inline Expr expand_special(const Expr& e, int t_layer, int index)
{
    Expr result;
    for (size_t i = 0; i + 1 < e.cols.size(); ++i)
        result.cols.push_back(e.cols[i]);
    const Column& col = e.cols.back();
    Column result_col;
    for (size_t k = 0; k + 1 < col.size(); ++k)
        result_col.push_back(col[k]);
    const Entry& entry = col.back();
    if (t_layer > 1)
    {
        Entry ne;
        ne.value = entry.value;
        ne.height = std::make_shared<Expr>(
            expand_special(*entry.height, t_layer - 1, index));
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

inline int root_appending_start(const Column& col_root, int r,
                                const Column& col_tail, int t)
{
    std::vector<HExpr> heights_root;
    for (const Entry& en : col_root)
        heights_root.push_back(to_height(*en.height, r));
    std::vector<HExpr> heights_tail;
    for (size_t k = 0; k + 1 < col_tail.size(); ++k)
        heights_tail.push_back(to_height(*col_tail[k].height, t));
    int ir = 0, it = 0;
    while (ir != (int)heights_root.size() && it != (int)heights_tail.size())
    {
        const int cmp = height_cmp(heights_root[ir], heights_tail[it]);
        if (cmp >= 0) ++it;
        if (cmp <= 0) ++ir;
    }
    return ir;
}

inline bool is_limit(const Expr& e)
{
    return is_infinity(e) ||
           (!e.cols.empty() && !e.cols.back().empty());
}

// 供 weak 使用的竖直序列运算。
inline Vertical vertical_increase(const Vertical& vert, const HExpr& h_diff)
{
    Vertical result = vert;
    while (!result.empty() && height_cmp(result.back(), h_diff) < 0)
        result.pop_back();
    result.push_back(h_diff);
    return result;
}
inline Vertical vertical_diff(const Vertical& v1, const Vertical& v2)
{
    size_t i = 0;
    while (i < v1.size() && i < v2.size() &&
           height_cmp(v1[i], v2[i]) == 0)
        ++i;
    Vertical r;
    for (size_t k = i; k < v1.size(); ++k) r.push_back(v1[k]);
    return r;
}

// ---- 核心 FS（内部，参数 index 直接使用）----
inline Expr fs_core(const Expr& e, int index)
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

// 导出记号暴露的 FS（非极限强制 index=0）。
inline Expr fs(const Expr& e, int index)
{
    if (is_infinity(e)) return infinity_fs(index);
    if (!is_limit(e)) return fs_core(e, 0);
    return fs_core(e, index);
}

// ---- 比较（plain Expr）----
inline int compare(const Expr& a, const Expr& b);
inline int entry_cmp(const Entry& a, const Entry& b)
{
    if (const int c = number_cmp(a.value, b.value); c != 0) return c;
    return compare(*a.height, *b.height);
}
inline int compare(const Expr& a, const Expr& b)
{
    if (a.inf || b.inf) return boolean_cmp(a.inf, b.inf);
    const size_t n = (std::min)(a.cols.size(), b.cols.size());
    for (size_t i = 0; i < n; ++i)
    {
        const Column& ca = a.cols[i];
        const Column& cb = b.cols[i];
        const size_t m = (std::min)(ca.size(), cb.size());
        for (size_t j = 0; j < m; ++j)
        {
            const int c = entry_cmp(ca[j], cb[j]);
            if (c != 0) return c;
        }
        if (const int c = number_cmp((int)ca.size(), (int)cb.size()); c != 0)
            return c;
    }
    return number_cmp((int)a.cols.size(), (int)b.cols.size());
}

// ---- plain 显示 ----
inline std::string display(const Expr& e);
inline std::string display_entry(const Entry& en)
{
    std::string v = std::to_string(en.value + 1);
    if (en.height->cols.empty()) return v;
    return v + "^" + display(*en.height);
}
inline std::string display_column(const Column& c)
{
    if (c.empty()) return "(0)";
    std::string r = "(";
    for (size_t i = 0; i < c.size(); ++i)
    {
        if (i) r += ",";
        r += display_entry(c[i]);
    }
    r += ")";
    return r;
}
inline std::string display(const Expr& e)
{
    if (is_infinity(e)) return "Limit";
    std::string r;
    for (const Column& c : e.cols) r += display_column(c);
    return r;
}

// ---- 解析（1-based，支持 (0) 空列、^高度、:列号）----
class ParseError : public std::exception
{
public:
    const char* what() const noexcept override { return "illegal BTBM input"; }
};
class Parser
{
public:
    explicit Parser(std::string_view s) : s_(s) {}
    Expr parse()
    {
        skip_spaces();
        if (i_ + 5 <= s_.size() && s_.substr(i_, 5) == "Limit")
        {
            i_ += 5;
            skip_spaces();
            if (i_ != s_.size()) throw ParseError{};
            return make_inf();
        }
        Expr result = parse_expr();
        skip_spaces();
        if (i_ != s_.size()) throw ParseError{};
        return result;
    }
private:
    std::string_view s_;
    size_t i_ = 0;

    void skip_spaces()
    {
        while (i_ < s_.size() && s_[i_] == ' ') ++i_;
    }
    void skip_index()
    {
        if (i_ < s_.size() && s_[i_] == ':')
        {
            ++i_;
            skip_spaces();
            while (i_ < s_.size() && s_[i_] >= '0' && s_[i_] <= '9')
                ++i_;
        }
    }
    int parse_number()
    {
        skip_spaces();
        const size_t start = i_;
        while (i_ < s_.size() && s_[i_] >= '0' && s_[i_] <= '9')
            ++i_;
        if (start == i_) throw ParseError{};
        int v = 0;
        for (size_t k = start; k < i_; ++k) v = v * 10 + (s_[k] - '0');
        return v;
    }
    Expr parse_expr()
    {
        Expr result;
        skip_spaces();
        while (i_ < s_.size() && s_[i_] == '(')
        {
            result.cols.push_back(parse_column());
            skip_spaces();
        }
        return result;
    }
    Column parse_column()
    {
        skip_spaces();
        if (i_ >= s_.size() || s_[i_] != '(') throw ParseError{};
        ++i_;
        Column entries;
        skip_spaces();
        if (i_ < s_.size() && s_[i_] != ')' && s_[i_] != ':')
        {
            entries.push_back(parse_entry());
            skip_spaces();
            while (i_ < s_.size() && s_[i_] == ',')
            {
                ++i_;
                skip_spaces();
                if (i_ < s_.size() && s_[i_] == ')') break;
                entries.push_back(parse_entry());
                skip_spaces();
            }
        }
        skip_spaces();
        skip_index();
        skip_spaces();
        if (i_ >= s_.size() || s_[i_] != ')') throw ParseError{};
        ++i_;
        while (!entries.empty() && entries.back().value == -1)
            entries.pop_back();
        return entries;
    }
    Entry parse_entry()
    {
        const int v = parse_number() - 1;  // 显示 1-based → 内部 0-based
        skip_spaces();
        Entry e;
        e.value = v;
        if (i_ < s_.size() && s_[i_] == '^')
        {
            ++i_;
            e.height = std::make_shared<Expr>(parse_expr());
        }
        else
        {
            e.height = empty_height();
        }
        return e;
    }
};
}  // namespace btbm_detail

// BTBM 记号接口：仅 FS。
struct BTBMNotation
{
    static std::string suffix() { return std::string{}; }
    static std::string expand_text(std::string_view text, int term)
    {
        using namespace btbm_detail;
        try
        {
            Parser parser(text);
            const Expr m = parser.parse();
            return display(fs(m, term));
        }
        catch (...)
        {
            return bmlike::kParseError;
        }
    }
};
}  // namespace omegay::notation

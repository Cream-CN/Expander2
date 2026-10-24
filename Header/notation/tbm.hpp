// TBM（Transfinite Bashicu matrix，超限巴斯丘矩阵）—— BM-like 树形记号。
//
// 移植自 ne-rewritten 的 TBM.ts，仅保留核心展开逻辑（舍弃 HTML/山脉图等显示层）。
// 与平面矩阵不同，TBM 的每个元素 Entry = [数值, 高度表达式]，高度本身又是一棵
// 嵌套矩阵；极限型展开时会对末元素的高度递归调用一次本记号的 FS，再按空列切段。
//
// 数据层级（自外向内）：
//   Expr     = Column[]   一个矩阵
//   Column   = Entry[]    一列
//   Entry    = [value, Expr]  一个带“高度”的数
//   Vertical = Expr[]     由若干列段组成的“竖直序列”
#pragma once

#include <algorithm>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Header/notation/bmlike_share.hpp"

namespace omegay::notation
{
namespace tbm_detail
{
struct Expr;
struct Entry
{
    int value = 0;
    std::shared_ptr<Expr> height;  // 缺省为 ONE
};
using Column = std::vector<Entry>;
struct Expr
{
    std::vector<Column> cols;
    Expr() = default;
    Expr(std::initializer_list<Column> init) : cols(init) {}
};
using Vertical = std::vector<Expr>;

constexpr int kInfinity = (1 << 30);

inline const std::shared_ptr<Expr>& one_expr()
{
    static const std::shared_ptr<Expr> one = std::make_shared<Expr>(Expr{ {Column{} } });
    return one;
}
inline Expr make_omega()
{
    // [[], [[1, ONE]]]
    Expr e;
    e.cols.push_back(Column{});
    Column c;
    c.push_back(Entry{ 1, one_expr() });
    e.cols.push_back(std::move(c));
    return e;
}

// ---- 比较链：entry（数值优先，高度次序）→ column（字典序）→ expr → vertical ----
inline int number_cmp(int a, int b)
{
    return a == b ? 0 : (a < b ? -1 : 1);
}
int expr_cmp(const Expr& a, const Expr& b);
inline int entry_cmp(const Entry& a, const Entry& b)
{
    if (a.value != b.value) return number_cmp(a.value, b.value);
    return expr_cmp(*a.height, *b.height);
}
inline int column_cmp(const Column& a, const Column& b)
{
    const size_t n = (std::min)(a.size(), b.size());
    for (size_t i = 0; i < n; ++i)
    {
        const int r = entry_cmp(a[i], b[i]);
        if (r != 0) return r;
    }
    return number_cmp(static_cast<int>(a.size()), static_cast<int>(b.size()));
}
inline int expr_cmp(const Expr& a, const Expr& b)
{
    const size_t n = (std::min)(a.cols.size(), b.cols.size());
    for (size_t i = 0; i < n; ++i)
    {
        const int r = column_cmp(a.cols[i], b.cols[i]);
        if (r != 0) return r;
    }
    return number_cmp(static_cast<int>(a.cols.size()),
                      static_cast<int>(b.cols.size()));
}
inline int vertical_cmp(const Vertical& a, const Vertical& b)
{
    const size_t n = (std::min)(a.size(), b.size());
    for (size_t i = 0; i < n; ++i)
    {
        const int r = expr_cmp(a[i], b[i]);
        if (r != 0) return r;
    }
    return number_cmp(static_cast<int>(a.size()), static_cast<int>(b.size()));
}

inline bool is_one(const Expr& e)
{
    return e.cols.size() == 1 && e.cols[0].empty();
}
inline bool is_infinity(const Expr& a)
{
    return !a.cols.empty() && !a.cols[0].empty() &&
           a.cols[0][0].value == kInfinity;
}

// ---- vertical（竖直序列）运算 ----
inline int index_after(const std::vector<Vertical>& V, const Vertical& pos)
{
    for (size_t k = 0; k < V.size(); ++k)
        if (vertical_cmp(V[k], pos) > 0) return static_cast<int>(k);
    return static_cast<int>(V.size());
}
inline bool vertical_parent(const Vertical& v,
                            const std::vector<std::pair<int, int>>& Pi,
                            const std::vector<Vertical>& Vi,
                            std::pair<int, int>& out)
{
    for (size_t k = 0; k < Vi.size(); ++k)
    {
        if (vertical_cmp(Vi[k], v) >= 0)
        {
            out = Pi[k];
            return true;
        }
    }
    return false;
}
inline Vertical vertical_add(const Vertical& v1, const Vertical& v2)
{
    if (v1.empty()) return v2;
    if (v2.empty()) return v1;
    const Expr& first2 = v2[0];
    size_t i = v1.size();
    while (i > 0 && expr_cmp(v1[i - 1], first2) < 0) --i;
    Vertical r;
    r.reserve(i + v2.size());
    for (size_t k = 0; k < i; ++k) r.push_back(v1[k]);
    for (const Expr& e : v2) r.push_back(e);
    return r;
}
inline std::vector<Vertical> column_verticals(const Column& col)
{
    std::vector<Vertical> result;
    result.reserve(col.size());
    Vertical acc;
    for (const Entry& en : col)
    {
        acc = vertical_add(acc, Vertical{ *en.height });
        result.push_back(acc);
    }
    return result;
}

using ParentRow = std::vector<std::pair<int, int>>;
inline std::vector<ParentRow> parents(
    const Expr& m, const std::vector<std::vector<Vertical>>& V)
{
    std::vector<ParentRow> P;
    P.reserve(m.cols.size());
    for (size_t ii = 0; ii < m.cols.size(); ++ii)
    {
        const int i = static_cast<int>(ii);
        ParentRow Pi;
        bool broken = false;
        for (size_t jj = 0; jj < m.cols[ii].size(); ++jj)
        {
            const int j = static_cast<int>(jj);
            const int value = m.cols[ii][jj].value;
            Vertical pos;
            if (j != 0) pos = V[ii][jj - 1];
            int p = (j == 0) ? i - 1 : Pi[j - 1].first;
            while (p >= 0)
            {
                if (m.cols[p].empty())
                {
                    if (0 < value)
                    {
                        Pi.emplace_back(p, 0);
                        break;
                    }
                    p = -1;
                    break;
                }
                const int j_p = (j == 0) ? 0 : index_after(V[p], pos);
                if (j_p >= static_cast<int>(m.cols[p].size()))
                {
                    Pi.emplace_back(p, j_p);
                    break;
                }
                const int value_p = m.cols[p][j_p].value;
                if (value_p < value)
                {
                    Pi.emplace_back(p, j_p);
                    break;
                }
                int np0 = 0, np1 = 0;
                bool has;
                if (j == 0)
                {
                    np0 = p - 1;
                    np1 = 0;
                    has = true;
                }
                else
                {
                    std::pair<int, int> nx;
                    has = vertical_parent(pos, P[p], V[p], nx);
                    if (has) { np0 = nx.first; np1 = nx.second; }
                }
                if (!has) { p = -1; break; }
                p = np0;
            }
            if (p < 0) { broken = true; break; }
        }
        (void)broken;
        P.push_back(std::move(Pi));
    }
    return P;
}

// 把一个高度表达式按空列切成若干竖直段。
inline std::vector<Expr> to_vertical(const Expr& m)
{
    std::vector<Expr> v;
    size_t prev = 0;
    for (size_t i = 1; i <= m.cols.size(); ++i)
    {
        if (i == m.cols.size() || m.cols[i].empty())
        {
            Expr seg;
            for (size_t k = prev; k < i; ++k) seg.cols.push_back(m.cols[k]);
            v.push_back(std::move(seg));
            prev = i;
        }
    }
    return v;
}

// ---- 列运算 ----
inline Column column_sub(const Column& a, const Column& b)
{
    Column off;
    const std::vector<Vertical> Va = column_verticals(a);
    const std::vector<Vertical> Vb = column_verticals(b);
    for (size_t j = 0; j < a.size(); ++j)
    {
        Vertical pos;
        if (j != 0) pos = Va[j - 1];
        const int j_r = index_after(Vb, pos);
        const int delta = a[j].value -
                          (j_r < static_cast<int>(Vb.size()) ? b[j_r].value : 0);
        if (delta <= 0) break;
        off.push_back(Entry{ delta, a[j].height });
    }
    return off;
}
inline std::vector<Vertical> ascension_threshold(
    const std::vector<std::vector<Vertical>>& V,
    const std::vector<ParentRow>& P, int r, const Vertical& b)
{
    std::vector<Vertical> A;
    A.reserve(V.size());
    for (size_t ii = 0; ii < V.size(); ++ii)
    {
        const int i = static_cast<int>(ii);
        if (i < r) { A.emplace_back(); continue; }
        if (i == r) { A.push_back(b); continue; }
        bool have = false;
        Vertical found;
        for (size_t j = 0; j < V[i].size(); ++j)
        {
            Vertical pos;
            if (j != 0) pos = V[i][j - 1];
            const int col_p = P[i][j].first;
            if (col_p < r) { found = pos; have = true; break; }
            if (vertical_cmp(pos, A[col_p]) >= 0) { found = pos; have = true; break; }
            const Vertical& new_pos = V[i][j];
            if (vertical_cmp(new_pos, A[col_p]) >= 0)
            {
                found = A[col_p];
                have = true;
                break;
            }
        }
        if (!have) found = V[i].back();
        A.push_back(std::move(found));
    }
    return A;
}
inline Column column_add(const Column& a, const Column& b)
{
    Column res;
    size_t ai = 0, bi = 0;
    while (ai < a.size() || bi < b.size())
    {
        if (ai >= a.size())
        {
            res.push_back(b[bi++]);
        }
        else if (bi >= b.size())
        {
            res.push_back(a[ai++]);
        }
        else
        {
            const Entry& ea = a[ai];
            const Entry& eb = b[bi];
            const int cmp = expr_cmp(*ea.height, *eb.height);
            const std::shared_ptr<Expr>& h = (cmp < 0) ? ea.height : eb.height;
            res.push_back(Entry{ ea.value + eb.value, h });
            if (cmp <= 0) ++ai;
            if (cmp >= 0) ++bi;
        }
    }
    return res;
}
inline Column column_truncate(const Column& col, const Vertical& b)
{
    Column res;
    size_t ci = 0, vi = 0;
    while (ci < col.size() && vi < b.size())
    {
        const Entry& e = col[ci];
        const Expr& vh = b[vi];
        const int cmp = expr_cmp(*e.height, vh);
        res.push_back(Entry{ e.value,
                             cmp < 0 ? e.height : std::make_shared<Expr>(vh) });
        if (cmp <= 0) ++ci;
        if (cmp >= 0) ++vi;
    }
    return res;
}
inline Column column_mul(const Column& col, int w)
{
    Column res;
    res.reserve(col.size());
    for (const Entry& e : col) res.push_back(Entry{ e.value * w, e.height });
    return res;
}
inline Column copy_column(const Column& col_i, const Column& offset,
                          const Vertical& A_i, int w)
{
    return column_add(col_i, column_mul(column_truncate(offset, A_i), w));
}

// ---- 展开 ----
Expr tbm_fs(const Expr& expr, int index);

inline Expr expand_successor(const Expr& m, int index)
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
    const std::vector<Vertical> A = ascension_threshold(V, P, r, b);

    for (int w = 1; w <= index; ++w)
        for (int i = r; i < N; ++i)
            result.cols.push_back(copy_column(m.cols[i], offset, A[i], w));
    return result;
}
inline Expr expand_limit(const Expr& m, int index)
{
    const int right = static_cast<int>(m.cols.size()) - 1;
    const Column& col = m.cols[right];
    const int last_idx = static_cast<int>(col.size()) - 1;
    const int v = col[last_idx].value;
    const Expr& h = *col[last_idx].height;
    const Expr new_h = tbm_fs(h, index);
    const std::vector<Expr> segs = to_vertical(new_h);

    Expr result = m;
    Column new_entries;
    for (int k = 0; k < last_idx; ++k) new_entries.push_back(col[k]);
    for (const Expr& seg : segs)
        new_entries.push_back(Entry{ v, std::make_shared<Expr>(seg) });
    result.cols[right] = std::move(new_entries);
    return result;
}
inline Expr expand(const Expr& m, int index)
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
    const Expr& last_height = *last_col.back().height;
    if (is_one(last_height)) return expand_successor(m, index);
    return expand_limit(m, index);
}
inline Expr infinity_fs(int index)
{
    if (index == 0) return Expr{ {Column{} } };
    Expr e;
    e.cols.push_back(Column{});
    Column c;
    c.push_back(Entry{ 1, std::make_shared<Expr>(infinity_fs(index - 1)) });
    e.cols.push_back(std::move(c));
    return e;
}
inline bool is_limit(const Expr& m)
{
    if (is_infinity(m)) return true;
    if (m.cols.empty()) return false;
    return !m.cols.back().empty();
}
inline Expr tbm_fs(const Expr& expr, int index)
{
    if (is_infinity(expr)) return infinity_fs(index);
    if (!is_limit(expr)) return expand(expr, 0);
    return expand(expr, index);
}

// ---- 显示（纯文本，空列显示 ()，高度为 1 省略上标，OMEGA 显示 ω）----
inline std::string display(const Expr& m);
inline bool height_display(const Expr& s, std::string& out)
{
    if (s.cols.size() == 1) return false;  // 高度为 ONE：无上标
    static const Expr omega = make_omega();
    if (expr_cmp(s, omega) == 0) { out = "\xCF\x89"; return true; }
    out = display(s);
    return true;
}
inline std::string entry_display(const Entry& e)
{
    std::string sd;
    if (!height_display(*e.height, sd)) return std::to_string(e.value);
    return std::to_string(e.value) + "^" + sd;
}
inline std::string column_display(const Column& c)
{
    std::string r = "(";
    for (size_t i = 0; i < c.size(); ++i)
    {
        if (i) r += ",";
        r += entry_display(c[i]);
    }
    r += ")";
    return r;
}
inline std::string display(const Expr& m)
{
    if (is_infinity(m)) return "Limit";
    std::string r;
    for (const Column& c : m.cols) r += column_display(c);
    return r;
}

// ---- 解析 ----
class ParseError : public std::exception
{
public:
    const char* what() const noexcept override { return "illegal TBM input"; }
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
            Expr inf;
            Column c;
            c.push_back(Entry{ kInfinity, std::make_shared<Expr>() });
            inf.cols.push_back(std::move(c));
            return inf;
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
    int parse_number()
    {
        skip_spaces();
        const size_t start = i_;
        while (i_ < s_.size() && s_[i_] >= '0' && s_[i_] <= '9') ++i_;
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
        if (i_ < s_.size() && s_[i_] != ')')
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
        if (i_ >= s_.size() || s_[i_] != ')') throw ParseError{};
        ++i_;
        return entries;
    }
    bool match_omega()
    {
        // ω = UTF-8 CF 89，或 ASCII w
        if (i_ + 1 < s_.size() &&
            static_cast<unsigned char>(s_[i_]) == 0xCF &&
            static_cast<unsigned char>(s_[i_ + 1]) == 0x89)
        {
            i_ += 2;
            return true;
        }
        if (i_ < s_.size() && s_[i_] == 'w') { ++i_; return true; }
        return false;
    }
    Expr parse_height()
    {
        skip_spaces();
        if (match_omega()) return make_omega();
        return parse_expr();
    }
    Entry parse_entry()
    {
        const int v = parse_number();
        skip_spaces();
        if (i_ < s_.size() && s_[i_] == '^')
        {
            ++i_;
            return Entry{ v, std::make_shared<Expr>(parse_height()) };
        }
        return Entry{ v, one_expr() };
    }
};
}  // namespace tbm_detail

// TBM 记号接口：仅 FS（FS_default_LNZ_variant 只注册 FS；非极限走 expand(_,0)）。
struct TBMNotation
{
    static std::string suffix() { return std::string{}; }
    static std::string expand_text(std::string_view text, int term)
    {
        using namespace tbm_detail;
        try
        {
            Parser parser(text);
            const Expr m = parser.parse();
            return display(tbm_fs(m, term));
        }
        catch (...)
        {
            return bmlike::kParseError;
        }
    }
};
}  // namespace omegay::notation

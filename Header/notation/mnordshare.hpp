// Header/notation/mnordshare.hpp
// e0MN 家族共享核心：e0 Mountain Notation 的序数(Ord)运算、显示/解析与基本列机制。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/e0MN.ts 与
// src/notations/MN/strong_e0MN.ts（两者为各自独立的副本，仅三处实质差异，
// 本共享头用 bool strong 参数区分，见 shift_ord_numbers / down / short_expansion）。
// 表达式结构（TS）：Term={exp:Ord, coeff:int}；Ord=Term[]（[]=0）；
// Entry={a:int, x:Ord}；Column=Entry[]；Expr=Column[]；∞ 由外层 E0MnExpr 的标志表示
// （display 为 "Limit"）。
// 说明：纯函数、不抛异常；非法输入在解析层返回 nullopt。行高差显示等 UI 等价
// 表示未移植（C++ 侧仅需 plain 显示往返）。
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation::mnordshare {

struct Term;
using Ord = std::vector<Term>;
struct Term {
    Ord exp;     // 指数（递归）
    int coeff = 0;
};

struct Entry {
    int a = 0;   // 左腿列标（1 起）
    Ord x;       // 行高序数
};
using Column = std::vector<Entry>;
using Expr = std::vector<Column>;

// ---------------------------------------------------------------------
// 基础序数构造与判定（TS ordNat / ordOne / isZero / isOne / isPositive）
// ---------------------------------------------------------------------

inline Ord ord_nat(int n) {
    if (n < 0) return {};
    return n ? Ord{ Term{ {}, n } } : Ord{};
}

inline Ord ord_one() { return ord_nat(1); }

inline bool is_zero(const Ord& a) { return a.empty(); }
inline bool is_one(const Ord& a) { return a.size() == 1 && is_zero(a[0].exp) && a[0].coeff == 1; }
inline bool is_positive(const Ord& a) { return !a.empty(); }

// ---------------------------------------------------------------------
// 序数比较（TS ordCompare / ordEq / ordIsSuccessor / ordIsStandard）
// ---------------------------------------------------------------------

inline int ord_compare(const Ord& a, const Ord& b);

inline bool ord_eq(const Ord& a, const Ord& b) { return ord_compare(a, b) == 0; }

inline int ord_compare(const Ord& a, const Ord& b) {
    const std::size_t n = a.size() < b.size() ? a.size() : b.size();
    for (std::size_t i = 0; i < n; ++i) {
        const int c = ord_compare(a[i].exp, b[i].exp);
        if (c != 0) return c;
        if (a[i].coeff != b[i].coeff) return a[i].coeff > b[i].coeff ? 1 : -1;
    }
    return a.size() == b.size() ? 0 : (a.size() > b.size() ? 1 : -1);
}

inline bool ord_is_successor(const Ord& a) {
    return is_positive(a) && is_zero(a[a.size() - 1].exp);
}

inline bool ord_is_standard(const Ord& a) {
    for (std::size_t i = 0; i < a.size(); ++i) {
        const Term& t = a[i];
        if (t.coeff <= 0) return false;
        if (!ord_is_standard(t.exp)) return false;
        if (i > 0 && ord_compare(a[i - 1].exp, t.exp) <= 0) return false;
    }
    return true;
}

// ---------------------------------------------------------------------
// 序数加法与右差（TS ordAdd / ordRightDiff；作用于标准 Cantor 范式）
// ---------------------------------------------------------------------

inline Ord ord_add(const Ord& a, const Ord& b) {
    if (b.empty()) return a;
    const Ord& lead = b[0].exp;
    std::size_t i = 0;
    while (i < a.size() && ord_compare(a[i].exp, lead) > 0) ++i;
    Ord out(a.begin(), a.begin() + static_cast<std::ptrdiff_t>(i));
    if (i < a.size() && ord_eq(a[i].exp, lead)) {
        out.push_back(Term{ lead, a[i].coeff + b[0].coeff });
        for (std::size_t j = 1; j < b.size(); ++j) out.push_back(b[j]);
    } else {
        for (const Term& t : b) out.push_back(t);
    }
    return out;
}

// 返回 std::nullopt 表示右差不存在（TS 抛错）。
inline std::optional<Ord> ord_right_diff(const Ord& a, const Ord& b) {
    const int cmp = ord_compare(a, b);
    if (cmp > 0) return std::nullopt;
    if (cmp == 0) return Ord{};
    std::size_t i = 0;
    while (i < a.size() && i < b.size() && ord_eq(a[i].exp, b[i].exp) && a[i].coeff == b[i].coeff) ++i;
    if (i == a.size()) return Ord(b.begin() + static_cast<std::ptrdiff_t>(i), b.end());
    if (i >= b.size()) return std::nullopt;
    const int ec = ord_compare(a[i].exp, b[i].exp);
    if (ec < 0) return Ord(b.begin() + static_cast<std::ptrdiff_t>(i), b.end());
    if (ec == 0 && a[i].coeff < b[i].coeff) {
        Ord out;
        out.reserve(b.size() - i);
        out.push_back(Term{ b[i].exp, b[i].coeff - a[i].coeff });
        for (std::size_t j = i + 1; j < b.size(); ++j) out.push_back(b[j]);
        return out;
    }
    return std::nullopt;
}

// ---------------------------------------------------------------------
// e0 基本列（TS ordFS，情形 1--6 递归）
// ---------------------------------------------------------------------

inline Ord ord_fs(const Ord& src0, int m) {
    if (src0.empty()) return {};
    Ord a = src0;
    Term& last = a[a.size() - 1];
    if (is_zero(last.exp)) {
        // cases 1, 2
        if (last.coeff == 1) a.pop_back();
        else --last.coeff;
        return a;
    }
    const Ord exp = last.exp;
    const int coeff = last.coeff;
    a.pop_back();
    if (coeff > 1) a.push_back(Term{ exp, coeff - 1 });
    const Ord next_exp = ord_fs(exp, m);
    if (ord_is_successor(exp)) {
        // cases 3, 4
        if (m) a.push_back(Term{ next_exp, m });
    } else {
        // cases 5, 6
        a.push_back(Term{ next_exp, 1 });
    }
    return a;
}

inline Ord omega_power(const Ord& exp) { return Ord{ Term{ exp, 1 } }; }

inline Ord omega_tower(int n) {
    if (n < 0) return {};
    Ord r = ord_one();
    while (n--) r = omega_power(r);
    return r;
}

// rfl 中可见正整数移位：strong=false 时仅当 q > cutoff 且系数 ≠ 1（或整体是
// 纯数字项）才移位；strong=true 时指数项也递归移位，判据为 coeff >= cutoff
// （含省略的 1，如 ω = ω^1·1 在 cutoff=1,d=2 下变成 ω^3·3）。
inline Ord shift_ord_numbers(const Ord& a, int cutoff, int d, bool strong) {
    Ord out;
    out.reserve(a.size());
    for (const Term& t : a) {
        if (!strong && is_zero(t.exp)) {
            out.push_back(Term{ {}, t.coeff > cutoff ? t.coeff + d : t.coeff });
            continue;
        }
        Term nt;
        if (strong) {
            nt.exp = shift_ord_numbers(t.exp, cutoff, d, strong);
            nt.coeff = t.coeff >= cutoff ? t.coeff + d : t.coeff;
        } else {
            nt.exp = is_one(t.exp) ? t.exp : shift_ord_numbers(t.exp, cutoff, d, strong);
            nt.coeff = t.coeff != 1 && t.coeff > cutoff ? t.coeff + d : t.coeff;
        }
        out.push_back(std::move(nt));
    }
    return out;
}

// ---------------------------------------------------------------------
// 序数解析与显示（TS parseOrd / ordTo；仅 plain 模式）
// ---------------------------------------------------------------------

// 'ω' 的 UTF-8 是 0xCF 0x89；解析期用 ASCII 哨兵代替，避免多字节逐字符处理。
inline constexpr char kW = '\x7F';

inline std::string normalize_omega(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size();) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (c == 0xCF && i + 1 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0x89) {
            out.push_back(kW);
            i += 2;
        } else if (c == 'w' || c == 'W') {
            out.push_back(kW);
            ++i;
        } else if ((c == 'o' || c == 'O') && i + 4 < s.size()) {
            // omega / Omega / OMeGa ... 不区分大小写
            const std::string_view rest = s.substr(i, 5);
            const std::string lower = [&]() {
                std::string l;
                l.reserve(5);
                for (char ch : rest) l.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
                return l;
            }();
            if (lower == "omega") {
                out.push_back(kW);
                i += 5;
            } else {
                out.push_back(s[i]);
                ++i;
            }
        } else {
            out.push_back(s[i]);
            ++i;
        }
    }
    return out;
}

struct OrdParser {
    const std::string& s;
    std::size_t i = 0;
    bool allow_zero = false;
    bool failed = false;

    bool at_end() const { return i >= s.size(); }
    char cur() const { return s[i]; }

    std::optional<int> number() {
        const std::size_t start = i;
        while (i < s.size() && s[i] >= '0' && s[i] <= '9') ++i;
        if (start == i) return std::nullopt;
        long long v = 0;
        for (std::size_t k = start; k < i; ++k) v = v * 10 + (s[k] - '0');
        if (v > 2147483647LL) return std::nullopt;
        return static_cast<int>(v);
    }

    std::optional<Ord> expr(char end = 0) {
        if (s[i] == '0' && (i + 1 == s.size() || (end && s[i + 1] == end))) {
            ++i;
            return Ord{};
        }
        Ord out;
        while (i < s.size() && (!end || s[i] != end)) {
            auto t = term();
            if (!t) return std::nullopt;
            out.push_back(std::move(*t));
            if (s[i] != '+') break;
            ++i;
            if (i >= s.size() || (end && s[i] == end)) return std::nullopt;
        }
        return out;
    }

    std::optional<Ord> exponent() {
        if (i >= s.size()) return std::nullopt;
        const char open = s[i];
        if (open == '{' || open == '(') {
            const char close = open == '{' ? '}' : ')';
            ++i;
            if (s[i] == close) {
                ++i;
                return Ord{};
            }
            auto e = expr(close);
            if (!e) return std::nullopt;
            if (s[i] != close) return std::nullopt;
            ++i;
            return e;
        }
        if (s[i] >= '0' && s[i] <= '9') {
            auto n = number();
            if (!n) return std::nullopt;
            return ord_nat(*n);
        }
        if (s[i] == kW) {
            auto t = term();
            if (!t) return std::nullopt;
            return Ord{ std::move(*t) };  // TS: [term()]
        }
        return std::nullopt;
    }

    std::optional<Term> term() {
        if (i >= s.size()) return std::nullopt;
        if (s[i] >= '0' && s[i] <= '9') {
            auto n = number();
            if (!n || *n <= 0) return std::nullopt;
            return Term{ {}, *n };
        }
        if (s[i] != kW) return std::nullopt;
        ++i;
        Ord exp = ord_one();
        if (s[i] == '^') {
            ++i;
            auto e = exponent();
            if (!e) return std::nullopt;
            exp = std::move(*e);
        }
        int coeff = 1;
        if (i < s.size() && s[i] >= '0' && s[i] <= '9') {
            auto n = number();
            if (!n || *n <= 0) return std::nullopt;
            coeff = *n;
        }
        return Term{ std::move(exp), coeff };
    }
};

// TS parseOrd。tower 捷径 ω^^n 仅在全串匹配时生效。
inline std::optional<Ord> parse_ord(std::string_view text, bool allowZero = false) {
    const std::string s0 = normalize_omega(std::string(text));
    // 去除全部空白
    std::string s;
    s.reserve(s0.size());
    for (char c : s0) {
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') s.push_back(c);
    }
    if (s.empty()) return std::nullopt;
    if (s[0] == '0' && s.size() == 1) {
        return allowZero ? std::optional<Ord>(Ord{}) : std::nullopt;
    }
    // ω^^n
    if (s.size() >= 4 && s[0] == kW && s[1] == '^' && s[2] == '^') {
        int n = 0;
        std::size_t k = 3;
        bool ok = k < s.size();
        for (; k < s.size(); ++k) {
            if (s[k] < '0' || s[k] > '9') { ok = false; break; }
            n = n * 10 + (s[k] - '0');
            if (n > 1000000) { ok = false; break; }
        }
        if (ok) return omega_tower(n);
    }
    OrdParser p{ s, 0, allowZero, false };
    auto out = p.expr();
    if (!out) return std::nullopt;
    if (p.i != s.size()) return std::nullopt;
    if (!allowZero && out->empty()) return std::nullopt;
    return out;
}

inline std::string ord_to_plain(const Ord& a) {
    if (a.empty()) return "0";
    std::string out;
    for (std::size_t idx = 0; idx < a.size(); ++idx) {
        const Term& t = a[idx];
        if (idx) out += '+';
        if (is_zero(t.exp)) {
            out += std::to_string(t.coeff);
            continue;
        }
        out += "ω";
        if (!is_one(t.exp)) {
            out += "^{";
            out += ord_to_plain(t.exp);
            out += '}';
        }
        if (t.coeff != 1) out += std::to_string(t.coeff);
    }
    return out;
}

// TS towerIndex：ω^^k 的 k；非纯塔返回 -1。
inline int tower_index(const Ord& a) {
    if (is_one(a)) return 0;
    if (a.size() != 1 || a[0].coeff != 1 || is_zero(a[0].exp)) return -1;
    const int k = tower_index(a[0].exp);
    return k < 0 ? -1 : k + 1;
}

// ---------------------------------------------------------------------
// e0MN 表达式比较 / 合法性 / 显示 / 解析
// ---------------------------------------------------------------------

inline int entry_compare(const Entry& p, const Entry& q) {
    if (p.a != q.a) return p.a > q.a ? 1 : -1;
    return ord_compare(p.x, q.x);
}

template <typename T, typename Cmp>
inline int array_lex_compare(const std::vector<T>& a, const std::vector<T>& b, Cmp cmp) {
    const std::size_t n = a.size() < b.size() ? a.size() : b.size();
    for (std::size_t i = 0; i < n; ++i) {
        const int c = cmp(a[i], b[i]);
        if (c != 0) return c;
    }
    return a.size() == b.size() ? 0 : (a.size() > b.size() ? 1 : -1);
}

inline int col_compare(const Column& a, const Column& b) {
    return array_lex_compare(a, b, [](const Entry& x, const Entry& y) { return entry_compare(x, y); });
}

inline int expr_compare(const Expr& a, const Expr& b) {
    return array_lex_compare(a, b, [](const Column& x, const Column& y) { return col_compare(x, y); });
}

inline bool is_legal_column(const Column& c, int colNo) {
    for (std::size_t i = 0; i < c.size(); ++i) {
        const Entry& e = c[i];
        if (e.a <= 0 || e.a >= colNo) return false;
        if (!is_positive(e.x)) return false;
        if (i > 0 && !(c[i - 1].a > e.a && ord_compare(c[i - 1].x, e.x) < 0)) return false;
    }
    return true;
}

inline bool is_legal_expr(const Expr& e) {
    for (std::size_t i = 0; i < e.size(); ++i) {
        if (!is_legal_column(e[i], static_cast<int>(i + 1))) return false;
    }
    return true;
}

inline bool is_limit_expr(const Expr& e) {
    return !e.empty() && !e.back().empty();
}

inline Expr predecessor(const Expr& e) {
    if (e.empty()) return {};
    return Expr(e.begin(), e.end() - 1);
}

// TS exprLimitIndex：()(1:ω^^k) 形式的 k；否则 -1。
inline int expr_limit_index(const Expr& e) {
    if (e.size() == 2 && e[0].empty() && e[1].size() == 1 && e[1][0].a == 1) {
        return tower_index(e[1][0].x);
    }
    return -1;
}

// TS splitTop / topColon：在顶层（括号深度 0）按 sep / ':' 切分。
inline std::vector<std::string> split_top(std::string_view s, char sep) {
    std::vector<std::string> out;
    std::size_t start = 0;
    int p = 0, b = 0;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const char ch = s[i];
        if (ch == '(') ++p;
        else if (ch == ')') --p;
        else if (ch == '{') ++b;
        else if (ch == '}') --b;
        else if (ch == sep && !p && !b) {
            out.push_back(std::string(s.substr(start, i - start)));
            start = i + 1;
        }
    }
    out.push_back(std::string(s.substr(start)));
    return out;
}

inline int top_colon(std::string_view s) {
    int p = 0, b = 0;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const char ch = s[i];
        if (ch == '(') ++p;
        else if (ch == ')') --p;
        else if (ch == '{') ++b;
        else if (ch == '}') --b;
        else if (ch == ':' && !p && !b) return static_cast<int>(i);
    }
    return -1;
}

inline std::optional<Column> parse_column(std::string_view s) {
    // 去首尾空白
    std::size_t begin = 0, end = s.size();
    while (begin < end && (s[begin] == ' ' || s[begin] == '\t')) ++begin;
    while (end > begin && (s[end - 1] == ' ' || s[end - 1] == '\t')) --end;
    if (begin == end) return Column{};
    const std::string_view body = s.substr(begin, end - begin);
    Column col;
    for (const std::string& piece : split_top(body, ';')) {
        const int colon = top_colon(piece);
        if (colon < 0) return std::nullopt;
        const std::string a_text = [&]() {
            std::size_t b = 0, e = static_cast<std::size_t>(colon);
            while (b < e && (piece[b] == ' ' || piece[b] == '\t')) ++b;
            while (e > b && (piece[e - 1] == ' ' || piece[e - 1] == '\t')) --e;
            return piece.substr(b, e - b);
        }();
        if (a_text.empty() || a_text.find_first_not_of("0123456789") != std::string::npos) return std::nullopt;
        int a = 0;
        for (char ch : a_text) {
            a = a * 10 + (ch - '0');
            if (a > 1000000) return std::nullopt;
        }
        if (a <= 0) return std::nullopt;
        std::string x_text = piece.substr(static_cast<std::size_t>(colon) + 1);
        // 去首尾空白
        std::size_t xb = 0, xe = x_text.size();
        while (xb < xe && (x_text[xb] == ' ' || x_text[xb] == '\t')) ++xb;
        while (xe > xb && (x_text[xe - 1] == ' ' || x_text[xe - 1] == '\t')) --xe;
        x_text = x_text.substr(xb, xe - xb);
        auto x = parse_ord(x_text);
        if (!x) return std::nullopt;
        col.push_back(Entry{ a, std::move(*x) });
    }
    return col;
}

struct ParseResult {
    bool infinity = false;
    Expr expr;
};

// TS parseExpr。注意 "Limit" 返回 ∞ 标志、"Limit[n]" 返回具体的第 n 个基本列。
inline std::optional<ParseResult> parse_expr(std::string_view text) {
    // 去首尾空白
    std::size_t begin = 0, end = text.size();
    while (begin < end && (text[begin] == ' ' || text[begin] == '\t')) ++begin;
    while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t')) --end;
    const std::string_view raw = text.substr(begin, end - begin);
    if (raw.empty() || raw == "0" || raw == "∅") return ParseResult{ false, {} };
    if (raw == "Limit" || raw == "limit" || raw == "LIMIT") return ParseResult{ true, {} };
    // Limit[n]
    if (raw.size() >= 7 && (raw[0] == 'L' || raw[0] == 'l')) {
        const std::string lower = [&]() {
            std::string l;
            l.reserve(raw.size());
            for (char ch : raw) l.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
            return l;
        }();
        if (lower.rfind("limit[", 0) == 0 && lower.back() == ']') {
            int n = 0;
            bool ok = lower.size() > 7;
            for (std::size_t k = 6; k + 1 < lower.size(); ++k) {
                if (lower[k] < '0' || lower[k] > '9') { ok = false; break; }
                n = n * 10 + (lower[k] - '0');
                if (n > 1000000) { ok = false; break; }
            }
            if (ok) return ParseResult{ false, Expr{ Column{}, Column{ Entry{ 1, omega_tower(n) } } } };
        }
        return std::nullopt;
    }
    Expr out;
    std::size_t i = 0;
    while (i < raw.size()) {
        while (i < raw.size() && (raw[i] == ' ' || raw[i] == '\t')) ++i;
        if (i >= raw.size()) break;
        if (raw[i] != '(') return std::nullopt;
        int p = 0, b = 0;
        std::size_t endpos = std::string_view::npos;
        for (std::size_t j = i; j < raw.size(); ++j) {
            const char ch = raw[j];
            if (ch == '(') ++p;
            else if (ch == ')') {
                --p;
                if (!p && !b) { endpos = j; break; }
            } else if (ch == '{') ++b;
            else if (ch == '}') --b;
        }
        if (endpos == std::string_view::npos) return std::nullopt;
        auto col = parse_column(raw.substr(i + 1, endpos - i - 1));
        if (!col) return std::nullopt;
        out.push_back(std::move(*col));
        i = endpos + 1;
    }
    return ParseResult{ false, std::move(out) };
}

// ---------------------------------------------------------------------
// 显示（TS colTo / exprToPlain）
// ---------------------------------------------------------------------

inline std::string col_to_plain(const Column& c) {
    if (c.empty()) return "()";
    std::string out = "(";
    for (std::size_t i = 0; i < c.size(); ++i) {
        if (i) out += ';';
        out += std::to_string(c[i].a);
        out += ':';
        out += ord_to_plain(c[i].x);
    }
    out += ')';
    return out;
}

inline std::string expr_to_plain(const Expr& e) {
    std::string out;
    for (const Column& c : e) out += col_to_plain(c);
    return out;
}

inline std::string infinity_display() { return "Limit"; }

// ---------------------------------------------------------------------
// 基本列机制（TS down / processRflCol / rfl / FS / shortExpansion / FSShort）
// strong：见文件头注释的三处差异。
// ---------------------------------------------------------------------

// TS down：对极限表达式实施下降；omit 判据随 strong 区分。
inline Expr down(const Expr& src0, bool strong) {
    if (!is_limit_expr(src0)) return predecessor(src0);
    Expr e = src0;
    const int l = static_cast<int>(e.size());
    Column& last = e[static_cast<std::size_t>(l - 1)];
    const int n = static_cast<int>(last.size());
    const int an = last[static_cast<std::size_t>(n - 1)].a;
    const Ord& xn = last[static_cast<std::size_t>(n - 1)].x;
    const int d = l - an;
    if (d <= 0) return src0;  // 非法表达式：原样返回（TS 抛错）

    const Column& source = e[static_cast<std::size_t>(an - 1)];
    std::size_t s = 0;
    while (s < source.size() && ord_compare(source[s].x, xn) < 0) ++s;

    Column new_last(last.begin(), last.end() - 1);
    const Ord xn_l = ord_fs(xn, l - 1);
    const bool omit = is_one(xn) ||
                      (n > 1 && (strong ? ord_eq(xn_l, last[static_cast<std::size_t>(n - 2)].x)
                                        : ord_compare(xn_l, last[static_cast<std::size_t>(n - 2)].x) <= 0));
    if (!omit && is_positive(xn_l)) new_last.push_back(Entry{ an, xn_l });
    for (std::size_t j = s; j < source.size(); ++j) new_last.push_back(source[j]);
    e[static_cast<std::size_t>(l - 1)] = std::move(new_last);
    return e;
}

inline Column process_rfl_col(const Column& c, int cutoff, int d, bool strong) {
    Column out;
    out.reserve(c.size());
    for (const Entry& z : c) {
        out.push_back(Entry{ z.a >= cutoff ? z.a + d : z.a, shift_ord_numbers(z.x, cutoff, d, strong) });
    }
    return out;
}

inline Expr rfl(const Expr& src0, bool strong) {
    if (!is_limit_expr(src0)) return predecessor(src0);
    const int l = static_cast<int>(src0.size());
    const int an = src0[static_cast<std::size_t>(l - 1)].back().a;
    const int d = l - an;
    if (d <= 0) return src0;
    Expr out = down(src0, strong);
    for (int col = an + 1; col <= l; ++col) {
        out.push_back(process_rfl_col(src0[static_cast<std::size_t>(col - 1)], an, d, strong));
    }
    return out;
}

// TS FS(src, m, shorter)。m ≥ 1 且 src 为极限（调用方保证）。
inline Expr fs(const Expr& src0, int m, bool shorter, bool strong) {
    if (m <= 0 || !is_limit_expr(src0)) return predecessor(src0);
    Expr r = src0;
    while (m--) r = rfl(r, strong);
    if (shorter) {
        return predecessor(r);
    }
    const int l = static_cast<int>(r.size());
    r = rfl(r, strong);
    if (static_cast<int>(r.size()) > l) r.resize(static_cast<std::size_t>(l));
    return r;
}

inline Expr infinity_fs(int index) {
    Expr e;
    e.push_back(Column{});
    e.push_back(Column{ Entry{ 1, omega_tower(index) } });
    return e;
}

// TS shortExpansion（FS_equiv.fast 的截断用）。
inline Expr short_expansion(const Expr& src0, bool strong) {
    if (!is_limit_expr(src0)) return predecessor(src0);
    const int l = static_cast<int>(src0.size());
    const Column& last = src0[static_cast<std::size_t>(l - 1)];
    const int n = static_cast<int>(last.size());
    const int an = last[static_cast<std::size_t>(n - 1)].a;
    const Ord& xn = last[static_cast<std::size_t>(n - 1)].x;
    const Ord prev = n > 1 ? last[static_cast<std::size_t>(n - 2)].x : Ord{};
    const Ord xn_l = ord_fs(xn, l - 1);
    const bool omit = is_one(xn) ||
                      (n > 1 && (strong ? ord_eq(xn_l, prev) : ord_compare(xn_l, prev) <= 0));
    if (omit) return down(src0, strong);

    int s = 0;
    Ord xn_s = ord_fs(xn, 0);
    while (ord_compare(xn_s, prev) <= 0) {
        ++s;
        if (s > 1000000) return src0;
        xn_s = ord_fs(xn, s);
    }

    Expr e = src0;
    Column& last2 = e[static_cast<std::size_t>(l - 1)];
    last2 = Column(last2.begin(), last2.end() - 1);
    last2.push_back(Entry{ an, xn_s });
    return e;
}

// TS FSShort(src, m)。
inline Expr fs_short(const Expr& src0, int m, bool strong) {
    if (m <= 0 || !is_limit_expr(src0)) return predecessor(src0);
    const Expr short_ex = short_expansion(src0, strong);
    if (m == 1) return short_ex;
    const Expr first = fs(src0, 1, true, strong);
    return fs(src0, expr_compare(short_ex, first) == 0 ? m : m - 1, true, strong);
}

} // namespace omegay::notation::mnordshare

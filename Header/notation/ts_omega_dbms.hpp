// Header/notation/ts_omega_dbms.hpp
// TSωDBMS —— 超限 ω 二元 DBMS（Transfinite SωDBMS）。
// 逻辑移植自 notation-explorer（ne-rewritten-master）的
//   src/notations/MN/SDBMS/TS_omega_DBMS.ts
// 并按用户要求去掉 ∞ 顶层元素（Limit）相关逻辑：不再接受 "Limit"/"limit"/"LIMIT"
// 输入、不生成 "Limit" 显示、去掉 infinity_FS 与 init 的 INFINITY 种子；
// 保留表达式内部“极限/后继”判定与 limit_approximation / compute_new_height 的
// 极限高度展开逻辑（本记号核心）。
// 仅移植 C++ 展开器所需的 plain 显示/解析往返；RMN/DBMS/Y/TωY 等 display_equiv
// 未移植。纯函数；非法输入在 parse 层返回 nullopt；资源上限超出返回错误提示串。
#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

struct TsOmegaDbmsHeight;
struct TsOmegaDbmsEntry;
struct TsOmegaDbmsCnfTerm;

using TsOmegaDbmsColumn = std::vector<TsOmegaDbmsEntry>;
using TsOmegaDbmsExpr = std::vector<TsOmegaDbmsColumn>;

// HeightPos = Expr；HeightEntry = [anchor:number, position:HeightPos]
struct TsOmegaDbmsHeightEntry {
    int anchor = 0;
    TsOmegaDbmsExpr position;
};

// Height = HeightEntry[]
struct TsOmegaDbmsHeight {
    std::vector<TsOmegaDbmsHeightEntry> entries;
};

// Entry = [parent:number, height:Height]
struct TsOmegaDbmsEntry {
    int parent = 0;
    TsOmegaDbmsHeight height;
};

// Ordinal = {kind:'epsilon', expr} | {kind:'cnf', terms}
struct TsOmegaDbmsOrdinal {
    bool epsilon = false;
    TsOmegaDbmsExpr epsilon_expr;
    std::vector<TsOmegaDbmsCnfTerm> terms;
};

struct TsOmegaDbmsCnfTerm {
    TsOmegaDbmsOrdinal exponent;
    long long coefficient = 0;
};

struct TsOmegaDbmsParsed {
    TsOmegaDbmsExpr expr;
};

struct TsOmegaDbmsNotation {
    static constexpr const char* kName = "TSωDBMS";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*alter=*/false);
    }
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*alter=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::optional<TsOmegaDbmsParsed> parse(std::string_view s) {
        try {
            return TsOmegaDbmsParsed{ from_display(std::string(s)) };
        } catch (...) {
            return std::nullopt;
        }
    }

    [[nodiscard]] static std::string to_string(const TsOmegaDbmsParsed& p) { return display(p.expr); }

private:
    using HeightPos = TsOmegaDbmsExpr;
    using HeightEntry = TsOmegaDbmsHeightEntry;
    using Height = TsOmegaDbmsHeight;
    using Entry = TsOmegaDbmsEntry;
    using Column = TsOmegaDbmsColumn;
    using Expr = TsOmegaDbmsExpr;
    using Ordinal = TsOmegaDbmsOrdinal;
    using CnfTerm = TsOmegaDbmsCnfTerm;

    static constexpr long long kMaxMaterializedLength = 100000;
    static constexpr const char* kParseFail = "解析失败，请确认你没有使用全角括号，逗号，分号";
    static constexpr const char* kResourceFail = "展开超出资源上限（表达式过大或下标过大）";

    static int sgn(int v) { return v < 0 ? -1 : v > 0 ? 1 : 0; }

    static const Ordinal& ord_zero() {
        static const Ordinal v{};
        return v;
    }
    static const Ordinal& ord_one() {
        static const Ordinal v = [] {
            Ordinal o;
            o.terms = { CnfTerm{ ord_zero(), 1 } };
            return o;
        }();
        return v;
    }
    static Ordinal ord_omega() { return ord_pow_omega(ord_one()); }
    static const Expr& low() {
        static const Expr v = [] {
            // [[], [[0, [[0, []]]]]]
            Expr e;
            e.emplace_back();
            e.push_back(Column{ Entry{ 0, Height{ { HeightEntry{ 0, {} } } } } });
            return e;
        }();
        return v;
    }

    // -----------------------------------------------------------------
    // 比较
    // -----------------------------------------------------------------
    static int compare_height(const Height& a, const Height& b) {
        // lex_compare(a, b, (x,y)=>sign(x.anchor-y.anchor)||compare(x.position,y.position), longer_last=false)
        const std::size_t n = a.entries.size() < b.entries.size() ? a.entries.size() : b.entries.size();
        for (std::size_t i = 0; i < n; ++i) {
            int c = sgn(a.entries[i].anchor - b.entries[i].anchor);
            if (c == 0) c = compare(a.entries[i].position, b.entries[i].position);
            if (c != 0) return c;
        }
        if (a.entries.size() != b.entries.size())
            return a.entries.size() > b.entries.size() ? -1 : 1;  // 更短者更大
        return 0;
    }

    static int compare_entry(const Entry& a, const Entry& b) {
        const int c = sgn(a.parent - b.parent);
        if (c != 0) return c;
        return compare_height(a.height, b.height);
    }

    static int compare_column(const Column& a, const Column& b) {
        const std::size_t n = a.size() < b.size() ? a.size() : b.size();
        for (std::size_t i = 0; i < n; ++i) {
            const int c = compare_entry(a[i], b[i]);
            if (c != 0) return c;
        }
        return sgn(static_cast<int>(a.size()) - static_cast<int>(b.size()));
    }

    static int compare(const Expr& a, const Expr& b) {
        const std::size_t n = a.size() < b.size() ? a.size() : b.size();
        for (std::size_t i = 0; i < n; ++i) {
            const int c = compare_column(a[i], b[i]);
            if (c != 0) return c;
        }
        return sgn(static_cast<int>(a.size()) - static_cast<int>(b.size()));
    }

    static bool is_limit(const Expr& e) {
        return !e.empty() && !e.back().empty();
    }

    // -----------------------------------------------------------------
    // 序数运算（CNF，与 TS2DBMS 一致）
    // -----------------------------------------------------------------
    static std::vector<CnfTerm> ord_terms(const Ordinal& value) {
        if (value.epsilon) return { CnfTerm{ value, 1 } };
        return value.terms;
    }

    static Ordinal ord_from_terms(std::vector<CnfTerm> terms) {
        if (terms.empty()) return ord_zero();
        if (terms.size() == 1 && terms[0].coefficient == 1 && terms[0].exponent.epsilon)
            return terms[0].exponent;
        Ordinal o;
        o.terms = std::move(terms);
        return o;
    }

    static bool ord_is_zero(const Ordinal& v) { return !v.epsilon && v.terms.empty(); }

    static Ordinal ord_natural(long long value) {
        if (value < 0) throw std::runtime_error("Negative ordinal");
        if (value == 0) return ord_zero();
        return ord_from_terms({ CnfTerm{ ord_zero(), value } });
    }

    static int ord_compare(const Ordinal& a, const Ordinal& b) {
        if (a.epsilon && b.epsilon) return compare(a.epsilon_expr, b.epsilon_expr);
        if (a.epsilon) return -ord_compare(b, a);
        if (b.epsilon) {
            if (a.terms.empty()) return -1;
            const int c = ord_compare(a.terms[0].exponent, b);
            if (c != 0) return c;
            return (a.terms.size() == 1 && a.terms[0].coefficient == 1) ? 0 : 1;
        }
        const std::size_t n = a.terms.size() < b.terms.size() ? a.terms.size() : b.terms.size();
        for (std::size_t i = 0; i < n; ++i) {
            const int c = ord_compare(a.terms[i].exponent, b.terms[i].exponent);
            if (c != 0) return c;
            if (a.terms[i].coefficient != b.terms[i].coefficient)
                return a.terms[i].coefficient < b.terms[i].coefficient ? -1 : 1;
        }
        return sgn(static_cast<int>(a.terms.size()) - static_cast<int>(b.terms.size()));
    }

    static Ordinal ord_add(const Ordinal& a, const Ordinal& b) {
        std::vector<CnfTerm> right = ord_terms(b);
        if (right.empty()) return a;
        std::vector<CnfTerm> left = ord_terms(a);
        while (!left.empty() && ord_compare(left.back().exponent, right[0].exponent) < 0) left.pop_back();
        if (!left.empty() && ord_compare(left.back().exponent, right[0].exponent) == 0) {
            const CnfTerm last = left.back();
            left.pop_back();
            left.push_back(CnfTerm{ last.exponent, last.coefficient + right[0].coefficient });
            left.insert(left.end(), right.begin() + 1, right.end());
            return ord_from_terms(std::move(left));
        }
        left.insert(left.end(), right.begin(), right.end());
        return ord_from_terms(std::move(left));
    }

    static Ordinal ord_pow_omega(const Ordinal& exponent) {
        return ord_from_terms({ CnfTerm{ exponent, 1 } });
    }

    static Ordinal ord_mul(const Ordinal& a, const Ordinal& b) {
        if (ord_is_zero(a) || ord_is_zero(b)) return ord_zero();
        const std::vector<CnfTerm> left = ord_terms(a);
        Ordinal result = ord_zero();
        for (const CnfTerm& t : ord_terms(b)) {
            Ordinal term;
            if (ord_is_zero(t.exponent)) {
                std::vector<CnfTerm> terms;
                terms.push_back(CnfTerm{ left[0].exponent, left[0].coefficient * t.coefficient });
                terms.insert(terms.end(), left.begin() + 1, left.end());
                term = ord_from_terms(std::move(terms));
            } else {
                term = ord_from_terms({ CnfTerm{ ord_add(left[0].exponent, t.exponent), t.coefficient } });
            }
            result = ord_add(result, term);
        }
        return result;
    }

    static Ordinal ord_tail_after(const Ordinal& a, const Ordinal& b) {
        if (ord_compare(a, b) > 0) throw std::runtime_error("Negative ordinal tail");
        const std::vector<CnfTerm> left = ord_terms(a);
        const std::vector<CnfTerm> right = ord_terms(b);
        std::size_t i = 0;
        while (i < left.size() && i < right.size() &&
               ord_compare(left[i].exponent, right[i].exponent) == 0 &&
               left[i].coefficient == right[i].coefficient)
            ++i;
        if (i == left.size() || ord_compare(left[i].exponent, right[i].exponent) < 0)
            return ord_from_terms(std::vector<CnfTerm>(right.begin() + static_cast<std::ptrdiff_t>(i), right.end()));
        std::vector<CnfTerm> out;
        out.push_back(CnfTerm{ right[i].exponent, right[i].coefficient - left[i].coefficient });
        out.insert(out.end(), right.begin() + static_cast<std::ptrdiff_t>(i) + 1, right.end());
        return ord_from_terms(std::move(out));
    }

    static std::optional<Ordinal> max_epsilon_atom(const Ordinal& value) {
        std::optional<Ordinal> result;
        if (value.epsilon) {
            result = value;
        } else {
            for (const CnfTerm& term : value.terms) {
                std::optional<Ordinal> atom = max_epsilon_atom(term.exponent);
                if (atom && (!result || ord_compare(*atom, *result) > 0)) result = *atom;
            }
        }
        return result;
    }

    static int materialized_count(long long count, int width, int previous) {
        if (count * width + previous > kMaxMaterializedLength)
            throw std::runtime_error("TSωDBMS materialization resource limit exceeded");
        return static_cast<int>(count);
    }

    // -----------------------------------------------------------------
    // 矩阵列机制
    // -----------------------------------------------------------------
    static std::optional<Height> height(const Column& col) {
        if (col.empty()) return std::nullopt;
        return col.back().height;
    }

    static Column merge_column(const Column& a, const Column& b) {
        Column out = a;
        std::optional<Height> h = height(a);
        for (const Entry& e : b)
            if (!h || compare_height(e.height, *h) > 0) out.push_back(e);
        return out;
    }

    static Expr predecessor(const Expr& position) {
        if (position.empty() || is_limit(position))
            throw std::runtime_error("Not a successor TSωDBMS position");
        return Expr(position.begin(), position.end() - 1);
    }

    static Height height_fill_dec(const Height& h, const Expr& position, int anchor) {
        Height result = h;
        while (!result.entries.empty() && compare(result.entries.back().position, position) <= 0)
            result.entries.pop_back();
        if (!result.entries.empty() && result.entries.back().anchor == anchor) result.entries.pop_back();
        result.entries.push_back(HeightEntry{ anchor, position });
        return result;
    }

    static std::optional<Height> compute_new_height(const Height& h, const Expr& expr, int root) {
        const HeightEntry& last = h.entries.back();
        const int anchor = last.anchor;
        const Expr& position = last.position;
        if (!position.empty()) return height_fill_dec(h, predecessor(position), root);
        if (h.entries.size() == 1) return height(expr[static_cast<std::size_t>(anchor)]);
        const Expr& bound = h.entries[h.entries.size() - 2].position;
        const Column& col = expr[static_cast<std::size_t>(anchor)];
        std::size_t j = 0;
        for (; j < col.size(); ++j)
            if (compare_height(h, col[j].height) <= 0) break;
        if (j >= col.size()) return height(col);
        Height result = height_fill_dec(h, predecessor(bound), col[j].parent);
        if (j > 0 && compare_height(col[j - 1].height, result) > 0) result = col[j - 1].height;
        return result;
    }

    static Column remap_column(const Column& col,
                               const std::function<int(int)>& map_parent,
                               const std::function<int(int, const Expr&)>& map_anchor = {}) {
        Column out;
        out.reserve(col.size());
        for (const Entry& e : col) {
            Height nh;
            nh.entries.reserve(e.height.entries.size());
            for (const HeightEntry& he : e.height.entries) {
                const int anchor = map_anchor ? map_anchor(he.anchor, he.position)
                                              : map_parent(he.anchor);
                nh.entries.push_back(HeightEntry{ anchor, he.position });
            }
            out.push_back(Entry{ map_parent(e.parent), std::move(nh) });
        }
        return out;
    }

    static Column copy_column(const Column& col, int root, int offset) {
        return remap_column(col, [&](int n) { return n >= root ? n + offset : n; });
    }

    static Column shift_column(const Column& col, int offset) {
        return remap_column(col, [&](int n) { return n + offset; });
    }

    static Expr slice_component(const Expr& expr, int start, int end) {
        Expr out;
        for (int i = start; i < end; ++i) out.push_back(shift_column(expr[static_cast<std::size_t>(i)], -start));
        return out;
    }

    static Expr concatenate(const std::vector<Expr>& parts) {
        Expr result;
        for (const Expr& part : parts) {
            const int offset = static_cast<int>(result.size());
            materialized_count(1, static_cast<int>(part.size()), offset);
            for (const Column& col : part) result.push_back(shift_column(col, offset));
        }
        return result;
    }

    static bool pure_low(const Column& col) {
        return col.size() == 1 && col[0].height.entries.size() == 1 &&
               col[0].height.entries[0].anchor == 0 && col[0].height.entries[0].position.empty();
    }

    static bool low_column(const Column& col, int parent) {
        return pure_low(col) && col[0].parent == parent;
    }

    static Expr tail_template(const Expr& expr, int cut) {
        const std::function<int(int)> map = [&](int n) {
            if (n != 0 && n < cut) throw std::runtime_error("Invalid TSωDBMS product reference");
            return n == 0 ? 0 : n - cut + 1;
        };
        Expr out;
        out.emplace_back();
        for (std::size_t i = static_cast<std::size_t>(cut); i < expr.size(); ++i)
            out.push_back(remap_column(expr[i], map));
        return out;
    }

    static Expr root_substitute(const Expr& head, const Expr& tmpl) {
        Expr result;
        std::vector<int> mapping(tmpl.size(), 0);
        for (std::size_t i = 0; i < tmpl.size(); ++i) {
            mapping[i] = static_cast<int>(result.size());
            if (tmpl[i].empty()) {
                const int offset = static_cast<int>(result.size());
                materialized_count(1, static_cast<int>(head.size()), offset);
                for (const Column& col : head) result.push_back(shift_column(col, offset));
            } else {
                const std::function<int(int)> mp = [&](int n) { return mapping[static_cast<std::size_t>(n)]; };
                result.push_back(remap_column(tmpl[i], mp));
            }
        }
        return result;
    }

    static Expr clean_matrix_padding(const Expr& expr) {
        Expr result;
        result.emplace_back();
        result.push_back(Column{ Entry{ 0, Height{ { HeightEntry{ 0, {} } } } } });
        for (std::size_t i = 1; i < expr.size(); ++i) result.push_back(shift_column(expr[i], 1));
        if (pure_low(expr.back())) result.back()[0].height.entries[0].anchor = 0;
        return result;
    }

    static Expr convert_matrix_one(const Expr& expr, bool to_relative) {
        if (expr.size() == 1) return expr;
        if (to_relative) {
            if (low_column(expr[1], 0)) return expr;
            std::size_t cut = 0;
            for (std::size_t i = 1; i < expr.size(); ++i)
                if (low_column(expr[i], 0)) { cut = i; break; }
            if (cut == 0) return clean_matrix_padding(expr);
            Expr head = clean_matrix_padding(slice_component(expr, 0, static_cast<int>(cut)));
            Expr tail = tail_template(expr, static_cast<int>(cut));
            Expr threshold = head;
            threshold.push_back(Column{ Entry{ 1, Height{ { HeightEntry{ 0, {} } } } } });
            return compare(tail, threshold) < 0 ? root_substitute(head, tail) : tail;
        }
        if (!low_column(expr[1], 0)) throw std::runtime_error("Invalid TSωDBMS relative prefix");
        const Expr bound = [] {
            Expr b;
            b.emplace_back();
            b.push_back(Column{ Entry{ 0, Height{ { HeightEntry{ 0, {} } } } } });
            b.push_back(Column{ Entry{ 1, Height{ { HeightEntry{ 1, Expr{ Column{} } } } } } });
            return b;
        }();
        if (compare(expr, bound) < 0) return expr;
        std::size_t cut = 0;
        for (std::size_t i = 2; i < expr.size(); ++i)
            if (low_column(expr[i], 0)) { cut = i; break; }
        if (cut == 0) cut = expr.size();
        Expr prefix(expr.begin(), expr.begin() + static_cast<std::ptrdiff_t>(cut));
        Expr restored;
        restored.emplace_back();
        bool gap = false;
        for (std::size_t i = 2; i < prefix.size(); ++i) {
            const Column& col = prefix[i];
            const bool last = (i == prefix.size() - 1);
            bool all_ge = true;
            for (const Entry& e : col)
                if (!(e.parent >= 1 && e.height.entries[0].anchor >= 1)) { all_ge = false; break; }
            if (all_ge) {
                Column candidate = shift_column(col, -1);
                const bool corrected_last = last && pure_low(candidate);
                if (!low_column(candidate, 0) && !corrected_last) {
                    restored.push_back(std::move(candidate));
                    continue;
                }
            }
            if (last && pure_low(col) && col[0].parent > 1) {
                restored.push_back(Column{ Entry{ col[0].parent - 1, Height{ { HeightEntry{ 0, {} } } } } });
                continue;
            }
            const int parent = col[0].parent - 1;
            if (parent > 0) restored.push_back(Column{ Entry{ parent, Height{ { HeightEntry{ 0, {} } } } } });
            gap = true;
            break;
        }
        if (gap) return root_substitute(restored, expr);
        if (cut == expr.size()) return restored;
        return root_substitute(restored, tail_template(expr, static_cast<int>(cut)));
    }

    static Expr convert_matrix(const Expr& expr, bool to_relative) {
        std::vector<int> roots;
        for (std::size_t i = 0; i < expr.size(); ++i)
            if (expr[i].empty()) roots.push_back(static_cast<int>(i));
        std::vector<Expr> parts;
        for (std::size_t i = 0; i < roots.size(); ++i) {
            const int end = i + 1 < roots.size() ? roots[i + 1] : static_cast<int>(expr.size());
            parts.push_back(convert_matrix_one(slice_component(expr, roots[i], end), to_relative));
        }
        std::sort(parts.begin(), parts.end(), [&](const Expr& a, const Expr& b) { return compare(a, b) > 0; });
        return concatenate(parts);
    }

    static Expr suspend(const Expr& head, const Expr& tmpl) {
        const int offset = static_cast<int>(head.size()) - 1;
        const std::function<int(int)> mp = [&](int p) { return p == 0 ? 1 : p + offset; };
        const std::function<int(int, const Expr&)> ma = [&](int a, const Expr& position) {
            return (a == 0 && position.empty()) ? 0 : a + offset;
        };
        Expr out = head;
        for (std::size_t i = 1; i < tmpl.size(); ++i) out.push_back(remap_column(tmpl[i], mp, ma));
        return out;
    }

    static Expr unsuspend(const Expr& expr, int cut) {
        const std::function<int(int)> mp = [&](int p) {
            if (p != 1 && p < cut) throw std::runtime_error("Invalid TSωDBMS suspension parent");
            return p == 1 ? 0 : p - cut + 1;
        };
        const std::function<int(int, const Expr&)> ma = [&](int a, const Expr& position) {
            if (a == 0 && position.empty()) return 0;
            if (a < cut) throw std::runtime_error("Invalid TSωDBMS suspension anchor");
            return a - cut + 1;
        };
        Expr out;
        out.emplace_back();
        for (std::size_t i = static_cast<std::size_t>(cut); i < expr.size(); ++i)
            out.push_back(remap_column(expr[i], mp, ma));
        return out;
    }

    // -----------------------------------------------------------------
    // CNF 转换
    // -----------------------------------------------------------------
    static Ordinal relative_to_cnf(const Expr& expr) {
        if (expr.empty()) return ord_zero();
        if (expr.size() == 1) return ord_one();
        std::vector<int> roots;
        for (std::size_t i = 0; i < expr.size(); ++i)
            if (expr[i].empty()) roots.push_back(static_cast<int>(i));
        if (roots.size() > 1) {
            Ordinal result = ord_zero();
            for (std::size_t j = 0; j < roots.size(); ++j) {
                const int end = j + 1 < roots.size() ? roots[j + 1] : static_cast<int>(expr.size());
                result = ord_add(result, relative_to_cnf(slice_component(expr, roots[j], end)));
            }
            return result;
        }
        if (!low_column(expr[1], 0)) throw std::runtime_error("Invalid TSωDBMS relative prefix");
        std::size_t two = 0;
        for (std::size_t i = 2; i < expr.size(); ++i)
            if (low_column(expr[i], 0)) { two = i; break; }
        if (two > 0) {
            Expr a(expr.begin(), expr.begin() + static_cast<std::ptrdiff_t>(two));
            return ord_mul(relative_to_cnf(a), relative_to_cnf(tail_template(expr, static_cast<int>(two))));
        }
        if (expr.size() == 2) return ord_omega();
        std::size_t three = 0;
        for (std::size_t i = 2; i < expr.size(); ++i)
            if (low_column(expr[i], 1)) { three = i; break; }
        if (three == 0) {
            Ordinal e;
            e.epsilon = true;
            e.epsilon_expr = expr;
            return e;
        }
        Ordinal head;
        if (three == 2) {
            head = ord_one();
        } else {
            head.epsilon = true;
            head.epsilon_expr = Expr(expr.begin(), expr.begin() + static_cast<std::ptrdiff_t>(three));
        }
        return ord_pow_omega(ord_mul(head, relative_to_cnf(unsuspend(expr, static_cast<int>(three)))));
    }

    static Expr relative_power(const Ordinal& exponent) {
        if (exponent.epsilon) return exponent.epsilon_expr;
        if (exponent.terms.empty()) return Expr{ Column{} };
        if (exponent.terms.size() > 1 || exponent.terms[0].coefficient > 1) {
            Expr result;
            result.emplace_back();
            for (const CnfTerm& term : exponent.terms) {
                const Expr factor = relative_power(ord_pow_omega(term.exponent));
                const int count = materialized_count(term.coefficient,
                                                     static_cast<int>(factor.size()) - 1,
                                                     static_cast<int>(result.size()));
                for (int j = 0; j < count; ++j) result = root_substitute(result, factor);
            }
            return result;
        }
        std::optional<Ordinal> maximum = max_epsilon_atom(exponent);
        if (!maximum) return suspend(low(), relative_from_cnf(exponent));
        const Ordinal quotient = ord_pow_omega(ord_tail_after(*maximum, exponent.terms[0].exponent));
        return suspend(maximum->epsilon_expr, relative_from_cnf(quotient));
    }

    static Expr relative_from_cnf(const Ordinal& value) {
        if (value.epsilon) return value.epsilon_expr;
        std::vector<Expr> blocks;
        int length = 0;
        for (const CnfTerm& term : value.terms) {
            const Expr block = relative_power(term.exponent);
            const int count = materialized_count(term.coefficient, static_cast<int>(block.size()), length);
            for (int j = 0; j < count; ++j) blocks.push_back(block);
            length += static_cast<int>(block.size()) * count;
        }
        return concatenate(blocks);
    }

    static Ordinal tsomega_to_cnf(const Expr& expr) {
        if (!expr.empty() && !expr[0].empty())
            throw std::runtime_error("TSωDBMS ordinal must start with an empty column");
        return relative_to_cnf(convert_matrix(expr, true));
    }

    static Expr cnf_to_tsomega(const Ordinal& value) {
        return convert_matrix(relative_from_cnf(value), false);
    }

    // -----------------------------------------------------------------
    // 展开
    // -----------------------------------------------------------------
    static std::vector<Expr> root_components(const Expr& expr) {
        if (expr.empty() || !expr[0].empty())
            throw std::runtime_error("Invalid TSωDBMS root components");
        std::vector<int> roots;
        for (std::size_t i = 0; i < expr.size(); ++i)
            if (expr[i].empty()) roots.push_back(static_cast<int>(i));
        std::vector<Expr> result;
        for (std::size_t j = 0; j < roots.size(); ++j) {
            const int end = j + 1 < roots.size() ? roots[j + 1] : static_cast<int>(expr.size());
            result.push_back(slice_component(expr, roots[j], end));
        }
        return result;
    }

    static Expr matrix_add(const Expr& a, const Expr& b) {
        std::vector<Expr> left = root_components(a);
        std::vector<Expr> right = root_components(b);
        if (right.empty()) return a;
        while (!left.empty() && compare(left.back(), right[0]) < 0) left.pop_back();
        left.insert(left.end(), right.begin(), right.end());
        return concatenate(left);
    }

    static Expr matrix_tail(const Expr& a, const Expr& b) {
        if (compare(a, b) > 0) throw std::runtime_error("Negative matrix tail");
        std::vector<Expr> left = root_components(a);
        std::vector<Expr> right = root_components(b);
        std::size_t k = 0;
        while (k < left.size() && k < right.size() && compare(left[k], right[k]) == 0) ++k;
        std::vector<Expr> out(right.begin() + static_cast<std::ptrdiff_t>(k), right.end());
        return concatenate(out);
    }

    static Expr height_fs(const Expr& value, int index, bool alter) {
        return fundamental_sequence(value, index, alter);
    }

    static Height limit_approximation(const Height& h, const std::optional<Height>& lower, int index, bool alter) {
        const HeightEntry& last = h.entries.back();
        const int anchor = last.anchor;
        const Expr& alpha = last.position;
        Height prefix;
        prefix.entries.assign(h.entries.begin(), h.entries.end() - 1);
        Height zero_cap = prefix;
        zero_cap.entries.push_back(HeightEntry{ anchor, Expr{} });
        Expr beta;
        if (lower && compare_height(zero_cap, *lower) <= 0) {
            if (lower->entries.size() < h.entries.size() ||
                lower->entries[prefix.entries.size()].anchor != anchor)
                throw std::runtime_error("Invalid TSωDBMS limit prefix");
            for (std::size_t i = 0; i < prefix.entries.size(); ++i) {
                if (lower->entries[i].anchor != prefix.entries[i].anchor ||
                    compare(lower->entries[i].position, prefix.entries[i].position) != 0)
                    throw std::runtime_error("Invalid TSωDBMS limit prefix");
            }
            beta = lower->entries[prefix.entries.size()].position;
            if (compare(beta, alpha) >= 0) throw std::runtime_error("Invalid TSωDBMS limit interval");
        }
        const Expr difference = beta.empty() ? alpha : matrix_tail(beta, alpha);
        const Expr approximation = height_fs(difference, index, alter);
        if (approximation.empty()) throw std::runtime_error("Zero TSωDBMS limit approximation");
        Height result = prefix;
        result.entries.push_back(HeightEntry{ anchor,
                                              beta.empty() ? approximation : matrix_add(beta, approximation) });
        return result;
    }

    static Expr expand(const Expr& expr, int index, bool shorter, bool alter) {
        if (index < 0) throw std::runtime_error("Invalid fundamental sequence index");
        if (expr.empty()) return expr;
        const std::size_t right = expr.size() - 1;
        const Column& col = expr[right];
        if (col.empty()) return Expr(expr.begin(), expr.end() - 1);
        const Entry& top = col.back();
        const int root = top.parent;
        const Height& h = top.height;
        Column replacement(col.begin(), col.end() - 1);
        if (is_limit(h.entries.back().position)) {
            replacement.push_back(Entry{ root, limit_approximation(h, height(replacement), index, alter) });
            Expr result(expr.begin(), expr.end() - 1);
            result.push_back(std::move(replacement));
            return result;
        }
        std::optional<Height> next = compute_new_height(h, expr, root);
        if (next) replacement = merge_column(replacement, Column{ Entry{ root, *next } });
        replacement = merge_column(replacement, expr[static_cast<std::size_t>(root)]);
        materialized_count(static_cast<long long>(index),
                           static_cast<int>(right) - root,
                           static_cast<int>(expr.size()));
        Expr result(expr.begin(), expr.end() - 1);
        result.push_back(std::move(replacement));
        for (int w = 1; w <= index; ++w) {
            const int offset = (static_cast<int>(right) - root) * w;
            const std::function<int(int)> mp = [&, offset](int n) { return n >= root ? n + offset : n; };
            for (int i = root + 1; i <= static_cast<int>(right); ++i)
                result.push_back(remap_column(result[static_cast<std::size_t>(i)], mp));
        }
        if (shorter) result.pop_back();
        return result;
    }

    static Expr fundamental_sequence(const Expr& expr, int index, bool alter) {
        if (index < 0) throw std::runtime_error("Invalid fundamental sequence index");
        if (!is_limit(expr)) {
            if (expr.empty()) return expr;
            return Expr(expr.begin(), expr.end() - 1);
        }
        const Column& col = expr.back();
        const Entry& top = col.back();
        if (is_limit(top.height.entries.back().position))
            return expand(expr, index, /*shorter=*/true, alter);
        return expand(expr, index, /*shorter=*/!alter, alter);
    }

    // -----------------------------------------------------------------
    // 显示
    // -----------------------------------------------------------------
    static bool is_simple_exponent(const std::string& e) {
        if (e == "w") return true;
        if (e.empty()) return false;
        for (char c : e)
            if (c < '0' || c > '9') return false;
        return true;
    }

    static std::string superscript(const std::string& base, const std::string& exponent, bool pair) {
        if (pair || is_simple_exponent(exponent)) return base + "^" + exponent;
        return base + "^(" + exponent + ")";
    }

    static std::string display_ordinal(const Ordinal& value) {
        if (max_epsilon_atom(value)) return display(cnf_to_tsomega(value));
        const std::vector<CnfTerm> terms = ord_terms(value);
        if (terms.empty()) return "0";
        std::string out;
        bool first = true;
        for (const CnfTerm& t : terms) {
            if (!first) out += '+';
            first = false;
            if (ord_is_zero(t.exponent)) {
                out += std::to_string(t.coefficient);
                continue;
            }
            std::string base = "w";
            if (ord_compare(t.exponent, ord_one()) != 0)
                base = superscript(base, display_ordinal(t.exponent), false);
            if (t.coefficient != 1) base += "*" + std::to_string(t.coefficient);
            out += base;
        }
        return out;
    }

    static std::string height_display(const Height& h) {
        std::string inner;
        for (std::size_t i = 0; i < h.entries.size(); ++i) {
            if (i) inner += ',';
            const HeightEntry& he = h.entries[i];
            inner += std::to_string(he.anchor + 1) + "@" + display_ordinal(tsomega_to_cnf(he.position));
        }
        return "(" + inner + ")";
    }

    static std::string entry_display(const Entry& e) {
        return superscript(std::to_string(e.parent + 1), height_display(e.height), true);
    }

    static std::string display(const Expr& expr) {
        std::string out;
        for (const Column& col : expr) {
            if (col.empty()) {
                out += "(0)";
                continue;
            }
            out += '(';
            for (std::size_t k = 0; k < col.size(); ++k) {
                if (k) out += ',';
                out += entry_display(col[k]);
            }
            out += ')';
        }
        return out;
    }

    // -----------------------------------------------------------------
    // 解析
    // -----------------------------------------------------------------
    static std::vector<std::string> split_top_level(std::string_view text, char sep) {
        std::vector<std::string> result;
        std::size_t start = 0;
        std::vector<char> closing;
        for (std::size_t i = 0; i < text.size(); ++i) {
            const char ch = text[i];
            if (ch == '(' || ch == '{') closing.push_back(ch == '(' ? ')' : '}');
            else if (ch == ')' || ch == '}') {
                if (closing.empty() || closing.back() != ch) throw std::runtime_error("Mismatched TSωDBMS brackets");
                closing.pop_back();
            } else if (ch == sep && closing.empty()) {
                result.push_back(std::string(text.substr(start, i - start)));
                start = i + 1;
            }
        }
        if (!closing.empty()) throw std::runtime_error("Unclosed TSωDBMS brackets");
        result.push_back(std::string(text.substr(start)));
        return result;
    }

    static std::size_t closing_index(std::string_view text, std::size_t start) {
        std::vector<char> closing;
        for (std::size_t i = start; i < text.size(); ++i) {
            const char ch = text[i];
            if (ch == '(' || ch == '{') closing.push_back(ch == '(' ? ')' : '}');
            else if (ch == ')' || ch == '}') {
                if (closing.empty() || closing.back() != ch) throw std::runtime_error("Mismatched TSωDBMS brackets");
                closing.pop_back();
                if (closing.empty()) return i;
            }
        }
        throw std::runtime_error("Unclosed TSωDBMS brackets");
    }

    static int parse_index(const std::string& text) {
        if (!all_digits(text)) throw std::runtime_error("Invalid TSωDBMS index");
        long long value = 0;
        for (char c : text) {
            value = value * 10 + (c - '0');
            if (value > 100000000LL) throw std::runtime_error("Invalid TSωDBMS index");
        }
        if (value < 1) throw std::runtime_error("Invalid TSωDBMS index");
        return static_cast<int>(value - 1);
    }

    static bool all_digits(const std::string& s) {
        if (s.empty()) return false;
        for (char c : s)
            if (c < '0' || c > '9') return false;
        return true;
    }

    static std::string strip_ws_normalize(std::string_view text) {
        std::string out;
        out.reserve(text.size());
        for (std::size_t i = 0; i < text.size();) {
            const unsigned char c = static_cast<unsigned char>(text[i]);
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { ++i; continue; }
            if (c == 0xCF && i + 1 < text.size() &&
                static_cast<unsigned char>(text[i + 1]) == 0x89) {
                out.push_back('w');
                i += 2;
                continue;
            }
            out.push_back(static_cast<char>(c));
            ++i;
        }
        return out;
    }

    static std::string replace_u8(std::string s, const char* from, char to) {
        const std::size_t flen = std::string(from).size();
        if (flen == 0) return s;
        std::string out;
        out.reserve(s.size());
        for (std::size_t i = 0; i < s.size();) {
            if (i + flen <= s.size() && s.compare(i, flen, from) == 0) {
                out.push_back(to);
                i += flen;
            } else {
                out.push_back(s[i]);
                ++i;
            }
        }
        return out;
    }

    static Ordinal parse_ordinal(const std::string& raw) {
        const std::string source = strip_ws_normalize(raw);
        if (source.empty() || source.find("Limit") != std::string::npos)
            throw std::runtime_error("Invalid TSωDBMS ordinal");
        const std::vector<std::string> sum = split_top_level(source, '+');
        if (sum.size() > 1) {
            Ordinal value = ord_zero();
            for (const std::string& term : sum) value = ord_add(value, parse_ordinal(term));
            return value;
        }
        const std::string prod_src = replace_u8(source, "\xC2\xB7", '*');
        const std::vector<std::string> product = split_top_level(prod_src, '*');
        if (product.size() > 1) {
            if (product.size() != 2 || !all_digits(product[1]))
                throw std::runtime_error("Invalid CNF coefficient");
            return ord_mul(parse_ordinal(product[0]), ord_natural(std::stoll(product[1])));
        }
        if (all_digits(source)) return ord_natural(std::stoll(source));
        if (source == "w") return ord_omega();
        if (source.size() >= 2 && source[0] == 'w' && source[1] == '^')
            return ord_pow_omega(parse_ordinal(source.substr(2)));
        if (source[0] == '(' || source[0] == '{') {
            const std::size_t end = closing_index(source, 0);
            if (source == "()" || (source[0] == '(' && end + 1 < source.size() && source[end + 1] == '('))
                return tsomega_to_cnf(from_display(source));
            if (end == source.size() - 1) return parse_ordinal(source.substr(1, end - 1));
        }
        throw std::runtime_error("Invalid TSωDBMS ordinal: " + source);
    }

    // /^(\d+)\^?(\(.*\))$/ → (parent, body)
    static bool match_entry(const std::string& entry, int& parent, std::string& body) {
        std::size_t i = 0;
        if (i >= entry.size() || entry[i] < '0' || entry[i] > '9') return false;
        while (i < entry.size() && entry[i] >= '0' && entry[i] <= '9') ++i;
        parent = parse_index(entry.substr(0, i));
        if (i < entry.size() && entry[i] == '^') ++i;
        if (i >= entry.size() || entry[i] != '(') return false;
        const std::size_t end = closing_index(entry, i);
        if (end != entry.size() - 1) return false;
        body = entry.substr(i);
        return true;
    }

    static Expr from_display(const std::string& text) {
        const std::string source = strip_ws_normalize(text);
        Expr result;
        std::size_t start = 0;
        while (start < source.size()) {
            if (source[start] != '(') throw std::runtime_error("Invalid TSωDBMS column");
            const std::size_t end = closing_index(source, start);
            const std::vector<std::string> marked = split_top_level(source.substr(start + 1, end - start - 1), ':');
            if (marked.size() > 2) throw std::runtime_error("Invalid TSωDBMS column index");
            if (marked.size() == 2) parse_index(marked[1]);
            const std::string content = marked[0];
            Column col;
            if (content != "" && content != "0") {
                for (const std::string& entry : split_top_level(content, ',')) {
                    int parent = 0;
                    std::string body;
                    if (!match_entry(entry, parent, body)) throw std::runtime_error("Invalid TSωDBMS entry");
                    if (closing_index(body, 0) != body.size() - 1) throw std::runtime_error("Invalid TSωDBMS height");
                    std::string inner = body.substr(1, body.size() - 2);
                    while (inner.size() > 0 && inner[0] == '(' &&
                           closing_index(inner, 0) == inner.size() - 1)
                        inner = inner.substr(1, inner.size() - 2);
                    const std::vector<std::string> slots = split_top_level(inner, ',');
                    Height h;
                    for (const std::string& slot : slots) {
                        const std::vector<std::string> parts = split_top_level(slot, '@');
                        if (parts.size() != 2) throw std::runtime_error("Invalid TSωDBMS height slot");
                        HeightEntry he;
                        he.anchor = parse_index(parts[0]);
                        he.position = cnf_to_tsomega(parse_ordinal(parts[1]));
                        if (he.anchor > parent) throw std::runtime_error("TSωDBMS anchors must be <= parent");
                        h.entries.push_back(std::move(he));
                    }
                    for (std::size_t k = 1; k < h.entries.size(); ++k) {
                        const HeightEntry& prev = h.entries[k - 1];
                        const HeightEntry& cur = h.entries[k];
                        if (prev.anchor >= cur.anchor ||
                            compare(prev.position, cur.position) <= 0 ||
                            is_limit(prev.position))
                            throw std::runtime_error("TSωDBMS height entries must strictly increase");
                    }
                    if (col.size() &&
                        (col.back().parent <= parent || compare_height(col.back().height, h) >= 0))
                        throw std::runtime_error("TSωDBMS entries must have decreasing parents and increasing heights");
                    col.push_back(Entry{ parent, std::move(h) });
                }
            }
            result.push_back(std::move(col));
            start = end + 1;
        }
        return result;
    }

    static std::string expand_text_impl(std::string_view text, int term, bool alter) {
        if (term < 1) term = 1;
        std::optional<TsOmegaDbmsParsed> parsed;
        try {
            parsed = parse(text);
        } catch (...) {
            return kParseFail;
        }
        if (!parsed) return kParseFail;
        const Expr& e = parsed->expr;
        if (e.empty()) return {};
        try {
            const Expr r = fundamental_sequence(e, term, alter);
            return display(r);
        } catch (const std::exception&) {
            return kResourceFail;
        }
    }
};

} // namespace omegay::notation

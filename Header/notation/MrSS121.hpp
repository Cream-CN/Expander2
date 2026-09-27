#pragma once
#include "D:\expander\Expander\Header\common\mrssshare.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

    struct Mrss121Notation {
        static constexpr const char* kName = "MrSS1.2.1";

        // 标准 notation 接口：仅支持顶层纯整数序列。
        // 若展开结果含嵌套结构，则原样返回 seq。
        [[nodiscard]] static std::vector<int> expand(
            const std::vector<int>& seq, int term) {
            if (term < 1) term = 1;
            auto expr_seq = mrss::from_int_sequence(seq);
            auto result = expand_expr(expr_seq, term);
            auto opt = mrss::to_int_sequence(result);
            return opt ? *opt : seq;
        }

        [[nodiscard]] static std::string suffix() { return {}; }
        [[nodiscard]] static std::string expand_string(
            std::string_view seq, int term) {
            if (term < 1) term = 1;
            auto parsed = mrss::Parser::parse(seq);
            if (!parsed) return "[]";
            auto result = expand_expr(*parsed, term);
            return mrss::Parser::to_string(result);
        }
        [[nodiscard]] static std::optional<std::vector<mrss::Expr>>
            parse(std::string_view seq) {
            return mrss::Parser::parse(seq);
        }
        [[nodiscard]] static std::string
            to_string(const std::vector<mrss::Expr>& seq) {
            return mrss::Parser::to_string(seq);
        }

    private:
        using Expr = mrss::Expr;
        using Seq = std::vector<Expr>;
        [[nodiscard]] static bool starts_with(const Expr& a, const Expr& b) {
            if (a.is_atom() && b.is_atom()) {
                return a.value == b.value;
            }
            if (!a.is_atom() && b.is_atom()) {
                if (a.children.empty()) return false;
                return starts_with(a.children.front(), b);
            }
            if (a.is_atom() && !b.is_atom()) {
                return false;
            }
            if (a.children.size() < b.children.size()) return false;
            for (std::size_t i = 0; i < b.children.size(); ++i) {
                if (!starts_with(a.children[i], b.children[i])) {
                    return false;
                }
            }
            return true;
        }
        [[nodiscard]] static std::optional<std::size_t>
            find_bad_root(const Seq& seq) {
            if (seq.size() < 2) return std::nullopt;
            const Expr& last = seq.back();
            for (std::size_t i = seq.size() - 1; i-- > 0;) {
                const Expr& cand = seq[i];
                if (cand.is_atom() && last.is_atom()) {
                    if (cand.value < last.value) return i;
                }
                else if (starts_with(last, cand)) {
                    return i;
                }
            }
            return std::nullopt;
        }
        [[nodiscard]] static int difference(
            const Expr& last, const Expr& root) {
            return mrss::last_value(last) - mrss::last_value(root) - 1;
        }
        [[nodiscard]] static Seq bad_part(const Seq& seq, std::size_t root) {
            return Seq(
                seq.begin() + static_cast<std::ptrdiff_t>(root),
                seq.end() - 1);
        }
        [[nodiscard]] static Seq replace_last(
            const Seq& seq, const Seq& bad, int diff, int term) {
            Seq result(seq.begin(), seq.end() - 1);

            for (int i = 1; i <= term; ++i) {
                for (const auto& e : bad) {
                    result.push_back(mrss::add_value(e, diff * i));
                }
            }
            if (!bad.empty() && !result.empty()) {
                const int root_val = mrss::last_value(bad.front());
                int extra = 0;
                while (mrss::last_value(result.back()) <= root_val &&
                    extra < term) {
                    for (const auto& e : bad) {
                        result.push_back(mrss::add_value(e, diff * (term + extra + 1)));
                    }
                    ++extra;
                }
            }

            return result;
        }
        [[nodiscard]] static Seq expand_y_1(const Seq& seq, int term) {
            if (seq.empty()) return {};
            if (mrss::is_one(seq.back())) {
                Seq r = seq;
                r.pop_back();
                return r;
            }

            auto root_opt = find_bad_root(seq);
            if (!root_opt) {
                Seq r = seq;
                r.pop_back();
                return r;
            }

            const std::size_t root = *root_opt;
            const int diff = difference(seq.back(), seq[root]);
            const Seq bad = bad_part(seq, root);
            return replace_last(seq, bad, diff, term);
        }
        [[nodiscard]] static Seq expand_high(const Seq& seq, int term) {
            if (seq.empty()) return {};

            if (mrss::is_one(seq.back())) {
                Seq r = seq;
                r.pop_back();
                return r;
            }

            auto root_opt = find_bad_root(seq);
            if (!root_opt) {
                Seq r = seq;
                r.pop_back();
                return r;
            }

            const std::size_t root = *root_opt;
            const int diff = difference(seq.back(), seq[root]);
            const Seq bad = bad_part(seq, root);
            return replace_last(seq, bad, diff, term);
        }
        [[nodiscard]] static Seq expand_expr(const Seq& seq, int term) {
            if (seq.empty()) return {};

            if (mrss::is_one(seq.back())) {
                Seq r = seq;
                r.pop_back();
                return r;
            }

            const int order = mrss::expression_order(seq);
            if (order <= 1) {
                return expand_y_1(seq, term);
            }
            return expand_high(seq, term);
        }
    };

} // namespace omegay::notation
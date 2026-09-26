#pragma once

// Header/notation/mrss121.hpp
// MrSS1.2.1 记号实现。
// 依赖：mrssshare.hpp
// 命名空间：omegay::notation
// 标准：C++20
//
// 本实现按 CONTRIBUTING.txt 中 notation 模块的标准形态提供：
//   static constexpr const char* kName;
//   static std::vector<int> expand(const std::vector<int>&, int);
//   static std::string suffix();
//
// 由于 MrSS1.2.1 的原生表达式是递归嵌套的，std::vector<int> 只能承载
// 顶层纯整数序列。嵌套表达式请使用 expand_string(std::string_view, int)。
//
// 本实现覆盖：
//   - 后继表达式（末项为 1）的处理
//   - 一阶表达式的 1-Y 展开
//   - 高阶表达式按文档 3.3 的坏部复制式展开（近似实现）
// 不覆盖（超出本版范围）：
//   - 山脉图阶数 >= 3 时的 PrPS 展开
//   - 副根分支 3.3.1.1 / 3.3.1.2 的完整细节
//   - 差项 3.3.4 的完整实现

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

        // 扩展接口：直接处理 MrSS1.2.1 文本表达式，支持嵌套括号。
        // 示例："1,(1,2),(1,2,3)"
        [[nodiscard]] static std::string expand_string(
            std::string_view seq, int term) {
            if (term < 1) term = 1;
            auto parsed = mrss::Parser::parse(seq);
            if (!parsed) return "[]";
            auto result = expand_expr(*parsed, term);
            return mrss::Parser::to_string(result);
        }

        // 便捷转发：解析为内部表达式。
        [[nodiscard]] static std::optional<std::vector<mrss::Expr>>
            parse(std::string_view seq) {
            return mrss::Parser::parse(seq);
        }

        // 便捷转发：格式化为文本。
        [[nodiscard]] static std::string
            to_string(const std::vector<mrss::Expr>& seq) {
            return mrss::Parser::to_string(seq);
        }

    private:
        using Expr = mrss::Expr;
        using Seq = std::vector<Expr>;

        // 判断 a 是否以 b 作为前缀开头。
        //   - 原子与原子：值相等即为前缀
        //   - 表达式与原子：递归到第一个子元素
        //   - 表达式与表达式：逐项匹配 b 的子元素
        //   - 原子与表达式：不可能
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

        // 坏根搜索：从右向左找最后一个满足“末项以此元素开头”的元素。
        // 对纯原子序列退化为“最后一个小于末项的元素”。
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

        // 差值：末项差 - 1
        [[nodiscard]] static int difference(
            const Expr& last, const Expr& root) {
            return mrss::last_value(last) - mrss::last_value(root) - 1;
        }

        // 坏部：末项(不含)到坏根(含)的部分
        [[nodiscard]] static Seq bad_part(const Seq& seq, std::size_t root) {
            return Seq(
                seq.begin() + static_cast<std::ptrdiff_t>(root),
                seq.end() - 1);
        }

        // 3.3.2 / 3.3.3：用坏部每项 + 差值 × 复制次数替代末项。
        // 若替换后末项仍不大于坏根，则继续追加坏部副本（3.3.3）。
        [[nodiscard]] static Seq replace_last(
            const Seq& seq, const Seq& bad, int diff, int term) {
            Seq result(seq.begin(), seq.end() - 1);

            for (int i = 1; i <= term; ++i) {
                for (const auto& e : bad) {
                    result.push_back(mrss::add_value(e, diff * i));
                }
            }

            // 3.3.3：若坏部非空且末项未超过坏根末项，则继续追加。
            // 这里使用末项值比较，追加次数与 term 相同。
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

        // 一阶 1-Y 展开。
        [[nodiscard]] static Seq expand_y_1(const Seq& seq, int term) {
            if (seq.empty()) return {};

            // 后继表达式：末项为 1
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

        // 高阶展开：按文档 3.3 的复制式规则。
        // 说明：本版不实现副根分支 3.3.1 与差项 3.3.4，
        // 仅实现 3.3.2 / 3.3.3 的核心复制逻辑。
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

        // 展开主入口。
        // 先处理后继，再按表达式阶数选择一阶或高阶展开。
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
#pragma once

// Header/notation/mrssshare.hpp
// MrSS 系列共享的递归表达式、解析器与基础工具。
// 供 MrSS1.2.1 及其他 MrSS 记号复用。
//
// 命名空间：omegay::notation::mrss
// 依赖：标准库，无 Windows API，纯头文件，C++20。

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace omegay::notation::mrss {

    // ---------------------------------------------------------------------------
    // 递归表达式
    // ---------------------------------------------------------------------------

    struct Expr {
        bool atom = true;
        int value = 0;
        std::vector<Expr> children;

        Expr() = default;

        explicit Expr(int v)
            : atom(true), value(v) {}

        explicit Expr(std::vector<Expr> c)
            : atom(false), value(0), children(std::move(c)) {}

        [[nodiscard]] bool is_atom() const noexcept { return atom; }
        [[nodiscard]] bool is_expr() const noexcept { return !atom; }
    };

    // ---------------------------------------------------------------------------
    // 解析器 / 格式化器
    // ---------------------------------------------------------------------------

    class Parser {
    public:
        // 解析整个字符串为表达式序列。
        // 成功返回序列，失败返回 std::nullopt。
        [[nodiscard]] static std::optional<std::vector<Expr>>
            parse(std::string_view str) {
            Parser p(str);
            auto seq = p.parse_sequence_until('\0');
            if (!seq) return std::nullopt;
            p.skip_ws();
            if (!p.eof()) return std::nullopt;
            return seq;
        }

        // 序列格式化。空序列输出 "[]"。
        [[nodiscard]] static std::string
            to_string(const std::vector<Expr>& seq) {
            if (seq.empty()) return "[]";
            std::string r;
            for (std::size_t i = 0; i < seq.size(); ++i) {
                if (i != 0) r += ',';
                r += to_string(seq[i]);
            }
            return r;
        }

        // 单个表达式格式化。
        [[nodiscard]] static std::string
            to_string(const Expr& e) {
            if (e.is_atom()) {
                return std::to_string(e.value);
            }
            std::string r = "(";
            for (std::size_t i = 0; i < e.children.size(); ++i) {
                if (i != 0) r += ',';
                r += to_string(e.children[i]);
            }
            r += ")";
            return r;
        }

    private:
        explicit Parser(std::string_view s) : s_(s) {}

        void skip_ws() {
            while (pos_ < s_.size() &&
                std::isspace(static_cast<unsigned char>(s_[pos_]))) {
                ++pos_;
            }
        }

        [[nodiscard]] bool eof() const noexcept { return pos_ >= s_.size(); }

        [[nodiscard]] char peek() const noexcept {
            return eof() ? '\0' : s_[pos_];
        }

        char get() noexcept {
            return eof() ? '\0' : s_[pos_++];
        }

        [[nodiscard]] std::optional<int> parse_int() {
            skip_ws();
            if (eof() || !std::isdigit(static_cast<unsigned char>(peek()))) {
                return std::nullopt;
            }
            int val = 0;
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
                val = val * 10 + (get() - '0');
            }
            return val;
        }

        [[nodiscard]] std::optional<Expr> parse_element() {
            skip_ws();
            if (eof()) return std::nullopt;

            if (peek() == '(') {
                get(); // '('
                auto seq = parse_sequence_until(')');
                if (!seq) return std::nullopt;
                skip_ws();
                if (eof() || get() != ')') return std::nullopt;
                return Expr(std::move(*seq));
            }

            auto v = parse_int();
            if (!v) return std::nullopt;
            return Expr(*v);
        }

        [[nodiscard]] std::optional<std::vector<Expr>>
            parse_sequence_until(char end) {
            std::vector<Expr> result;

            skip_ws();
            if (!eof() && peek() == end) return result;

            while (true) {
                auto e = parse_element();
                if (!e) return std::nullopt;
                result.push_back(std::move(*e));

                skip_ws();
                if (eof()) break;
                if (peek() == end) break;

                if (get() != ',') return std::nullopt;
                skip_ws();

                // 允许尾随逗号
                if (!eof() && peek() == end) break;
            }

            return result;
        }

        std::string_view s_;
        std::size_t pos_ = 0;
    };

    // ---------------------------------------------------------------------------
    // 基础工具
    // ---------------------------------------------------------------------------

    [[nodiscard]] inline bool is_one(const Expr& e) {
        return e.is_atom() && e.value == 1;
    }

    // n 阶元素定义：
    //   单个 1 为 0 阶元素；
    //   单个数值项或由 1 组成的项为 1 阶元素；
    //   由 0~n 阶元素组成的元素为 n+1 阶元素。
    [[nodiscard]] inline int element_order(const Expr& e) {
        if (e.is_atom()) {
            return e.value == 1 ? 0 : 1;
        }

        int max_child = -1;
        for (const auto& c : e.children) {
            max_child = std::max(max_child, element_order(c));
        }
        return max_child + 1;
    }

    [[nodiscard]] inline int expression_order(const std::vector<Expr>& seq) {
        int max_order = -1;
        for (const auto& e : seq) {
            max_order = std::max(max_order, element_order(e));
        }
        return max_order;
    }

    [[nodiscard]] inline int first_value(const Expr& e) {
        if (e.is_atom()) return e.value;
        if (e.children.empty()) return 0;
        return first_value(e.children.front());
    }

    [[nodiscard]] inline int last_value(const Expr& e) {
        if (e.is_atom()) return e.value;
        if (e.children.empty()) return 0;
        return last_value(e.children.back());
    }

    [[nodiscard]] inline Expr add_value(const Expr& e, int delta) {
        if (e.is_atom()) {
            return Expr(e.value + delta);
        }

        Expr r = e;
        if (!r.children.empty()) {
            r.children.back() = add_value(r.children.back(), delta);
        }
        return r;
    }

    // 从纯整数序列构造表达式序列。
    [[nodiscard]] inline std::vector<Expr> from_int_sequence(
        const std::vector<int>& seq) {
        std::vector<Expr> out;
        out.reserve(seq.size());
        for (int v : seq) {
            out.emplace_back(v);
        }
        return out;
    }

    // 尝试转回纯整数序列；若含嵌套表达式则返回 std::nullopt。
    [[nodiscard]] inline std::optional<std::vector<int>> to_int_sequence(
        const std::vector<Expr>& seq) {
        std::vector<int> out;
        out.reserve(seq.size());
        for (const auto& e : seq) {
            if (!e.is_atom()) return std::nullopt;
            out.push_back(e.value);
        }
        return out;
    }

} // namespace omegay::notation::mrss
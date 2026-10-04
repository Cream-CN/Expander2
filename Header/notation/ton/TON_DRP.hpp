#pragma once
// ============================================================
// TON_DRP.hpp —— 对应 TON_DRP.ts
// 记号：Degrees of Reflection with Passthrough
//
// 相对 TON_DoR 增加「穿透」（passthrough）：BuiltQ 携带 ai（自身）、a0（上一层的
// a[1]）与游标 d。d 初始为 -1，一旦本层判定「低于 d」就把 d 重置回 -1（即换层），
// 从而让负值子项可以穿透到更深的层继续判定。
// ============================================================

#include "ton_Common.hpp"
#include "ton_helpers.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation::ton {

    namespace ton_drp_detail {

        /// 对应 TON_DRP.ts 的 BuiltQ(a, ai, b, a0, d)。
        /// d 既可能是叶子 -1，也可能是一个项（回溯时传入的 a 本身），故按项表达；
        /// TS 的 `d === -1` 为严格相等，对应 jsExact(d, -1)。
        [[nodiscard]] inline bool BuiltQ(const Term& a, const Term& ai, const Term& b,
                                         const Term& a0, const Term& d) {
            if (jsExact(a, 0) || noraiseLt(a, b)) return true;
            if (jsExact(d, -1) && noraiseLt(a, L(0)) && noraiseGt(a, ai)) return false;
            if (noraiseLt(a, d)) return BuiltQ(a, ai, b, a0, L(-1));
            if (jsExact(d, -1) && noraiseLt(a.at(0), L(0)) && noraiseLt(a.at(1), a0)) {
                return BuiltQ(a, ai, b, a0, a);
            }
            return BuiltQ(a.at(0), ai, b, a0, d) && BuiltQ(a.at(1), ai, b, a0, d);
        }

    } // namespace ton_drp_detail

    // ------------------------------------------------------------
    // TonDRPNotation
    // ------------------------------------------------------------

    class TonDRPNotation : public NoRaiseNotationBase {
    public:
        static constexpr std::string_view kName = "TON_DRP";
        static constexpr std::string_view kDescription =
            "Degrees of Reflection with Passthrough";

        /// 定义文本：源包 Docs 为空、未提供正式定义，按约定以 \n 作换行占位。
        static constexpr std::string_view kDefinitionText =
            "TON_DRP —— Degrees of Reflection with Passthrough\n"
            "Term ::= Limit | 0 | \\CE\\A9 | [Term, Term]\n"
            "在 DoR 的穿透候选判定上加入游标 d：负值子项可穿透至更深层继续判定。\n"
            "[PLACEHOLDER] 正式定义文本待补录\n";

        /// 进程级实例，对应 TS 的模块级单例及其模块级 data / DRPStd。
        [[nodiscard]] static const TonDRPNotation& notation() {
            static const TonDRPNotation instance{};
            return instance;
        }

        // ---------- NotationDefinition ----------

        [[nodiscard]] std::string_view id() const override { return "ton-drp"; }
        [[nodiscard]] std::string_view name() const override { return kDescription; }
        [[nodiscard]] std::string_view simple_name() const override { return "TON_DRP"; }
        [[nodiscard]] std::string_view definition_text() const override { return kDefinitionText; }

        /// 对应 init: () => [Infinity, -1]
        [[nodiscard]] std::vector<Term> init() const override { return init_pair_neg1(); }

        /// 对应 StandardQ(a)，带模块级缓存 DRPStd（只缓存真）。
        [[nodiscard]] bool StandardQ(const Term& a) const {
            StandardCache& cache = standardCache();
            if (cache.contains(a)) return true;

            if (a.isLeaf()) {
                cache.insert(a);
                return true;
            }
            const Term a1 = a.at(1);
            const Term a0 = a.at(0);
            const bool ok = StandardQ(a1)
                && StandardQ(a0)
                && (a0.isLeaf() || noraiseLe(a1, a0.at(1)))
                && [&] {
                       for (const Term& x : smallpart(a1)) {
                           if (!ton_drp_detail::BuiltQ(x, x, a, a1, L(-1))) return false;
                       }
                       return true;
                   }();
            if (ok) cache.insert(a);
            return ok;
        }

        /// 序列约定：含终止符 -2 的扁平序列；[ASSUMPTION] int 不含 Infinity。
        [[nodiscard]] static std::vector<int> expand(const std::vector<int>& seq, int term) {
            if (seq.empty()) return {};
            if (term < 0) return {};
            const Term t = unflatten(seq);
            return flatten_to_ints(notation().FS(t, static_cast<std::size_t>(term)));
        }

        [[nodiscard]] static std::string suffix() { return " [TON_DRP]"; }

    protected:
        /// 对应 FS 开头：`if ('' + term === 'Infinity') term = [-1, 0, -2]`
        [[nodiscard]] Term infinityReplacement() const override { return infinity_pair_neg1_0(); }

        [[nodiscard]] std::shared_ptr<TonGeneratorBody> makeGenerator(const Term& term) const override {
            return std::make_shared<Generator>(term, *this);
        }

    private:
        /// 对应 TON_gen(term)：系统数恒为 0，产出 Copy(beta)。
        class Generator : public ZeroSystemGenerator {
        public:
            Generator(const Term& term, const TonDRPNotation& owner)
                : ZeroSystemGenerator(term), owner_(owner) {}

        protected:
            [[nodiscard]] bool StandardQ(const Term& beta) override {
                return owner_.StandardQ(beta);
            }

        private:
            const TonDRPNotation& owner_;
        };
    };

} // namespace omegay::notation::ton

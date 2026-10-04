#pragma once
// ============================================================
// TON_DoR.hpp —— 对应 TON_DoR.ts
// 记号：Degrees of Reflection
//
// DoR 家族的基准形态：BuiltQ 以「x 是否小于 0」分流 ——
//   x < 0 时要求 x 低于界 b，或在不超过 a 的前提下对两支递归；
//   x >= 0 时只看 x 是否为 0 或两支递归。
// smallpart(a[1]) 给出 a[1] 中所有「小于 0」的子项，作为穿透候选。
// ============================================================

#include "ton_Common.hpp"
#include "ton_helpers.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation::ton {

    namespace ton_dor_detail {

        /// 对应 TON_DoR.ts 的 BuiltQ(a, b, x)。
        [[nodiscard]] inline bool BuiltQ(const Term& a, const Term& b, const Term& x) {
            if (noraiseLt(x, L(0))) {
                return noraiseLt(x, b)
                    || (noraiseLe(x, a) && BuiltQ(a, b, x.at(0)) && BuiltQ(a, b, x.at(1)));
            }
            return jsExact(x, 0) || (BuiltQ(a, b, x.at(0)) && BuiltQ(a, b, x.at(1)));
        }

    } // namespace ton_dor_detail

    // ------------------------------------------------------------
    // TonDoRNotation
    // ------------------------------------------------------------

    class TonDoRNotation : public NoRaiseNotationBase {
    public:
        static constexpr std::string_view kName = "TON_DoR";
        static constexpr std::string_view kDescription = "Degrees of Reflection";

        /// 定义文本：源包 Docs 为空、未提供正式定义，按约定以 \n 作换行占位。
        static constexpr std::string_view kDefinitionText =
            "TON_DoR —— Degrees of Reflection\n"
            "Term ::= Limit | 0 | \\CE\\A9 | [Term, Term]\n"
            "以 smallpart(a[1]) 取出的负值为穿透候选，逐一支判定 n-built。\n"
            "[PLACEHOLDER] 正式定义文本待补录\n";

        /// 进程级实例，对应 TS 的模块级单例及其模块级 data / DRStd。
        [[nodiscard]] static const TonDoRNotation& notation() {
            static const TonDoRNotation instance{};
            return instance;
        }

        // ---------- NotationDefinition ----------

        [[nodiscard]] std::string_view id() const override { return "ton-dr"; }
        [[nodiscard]] std::string_view name() const override { return kDescription; }
        [[nodiscard]] std::string_view simple_name() const override { return "TON_DoR"; }
        [[nodiscard]] std::string_view definition_text() const override { return kDefinitionText; }

        /// 对应 init: () => [Infinity, -1]
        [[nodiscard]] std::vector<Term> init() const override { return init_pair_neg1(); }

        /// 对应 StandardQ(a)，带模块级缓存 DRStd（只缓存真）。
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
                           if (!ton_dor_detail::BuiltQ(x, a, x)) return false;   // BuiltQ(x, a, x)
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

        [[nodiscard]] static std::string suffix() { return " [TON_DoR]"; }

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
            Generator(const Term& term, const TonDoRNotation& owner)
                : ZeroSystemGenerator(term), owner_(owner) {}

        protected:
            [[nodiscard]] bool StandardQ(const Term& beta) override {
                return owner_.StandardQ(beta);
            }

        private:
            const TonDoRNotation& owner_;
        };
    };

} // namespace omegay::notation::ton

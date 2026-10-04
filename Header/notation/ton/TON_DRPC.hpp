#pragma once
// ============================================================
// TON_DRPC.hpp —— 对应 TON_DRPC.ts
// 记号：Degrees of Reflection with Passthrough (reflection configuration)
//
// 本模块是 DRP 与 DRC 的组合形态：BuiltQ 同时携带穿透游标 d 与反射配置源 (ap, xp)，
// 共六个参数 (a, ap, ai, b, a0, d)。相对 DRP 的两处变化：
//   ① 界判定改用截断映射：compare(r(a, ap), r(ai, b)) 取代 compare(a, ai)；
//   ② 递归下潜时按 x 是否为负值重算新的穿透源 x2 = (x < 0 ? x : ap)。
// 注意本模块没有独立入口项，判定从 d === -1 开始。
// ============================================================

#include "ton_Common.hpp"
#include "ton_helpers.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation::ton {

    namespace ton_drpc_detail {

        /// 对应 TON_DRPC.ts 的 BuiltQ(a, ap, ai, b, a0, d)。
        /// TS 中 `a === 0`、`d === -1` 均为严格相等，对应 jsExact。
        [[nodiscard]] inline bool BuiltQ(const Term& a, const Term& ap, const Term& ai,
                                         const Term& b, const Term& a0, const Term& d) {
            if (jsExact(a, 0) || noraiseLt(a, b)) return true;
            if (jsExact(d, -1) && noraiseLt(a, L(0)) && noraiseGt(r(a, ap), r(ai, b))) return false;
            if (noraiseLt(a, d)) return BuiltQ(a, ap, ai, b, a0, L(-1));

            // var x2 = compare(a, 0) < 0 ? a : ap;
            const Term x2 = noraiseLt(a, L(0)) ? a : ap;

            if (jsExact(d, -1) && noraiseLt(a.at(0), L(0)) && noraiseLt(r(a.at(1), x2), r(a0, b))) {
                return BuiltQ(a, ap, ai, b, a0, a);
            }
            return BuiltQ(a.at(0), x2, ai, b, a0, d)
                && BuiltQ(a.at(1), x2, ai, b, a0, d);
        }

    } // namespace ton_drpc_detail

    // ------------------------------------------------------------
    // TonDRPCNotation
    // ------------------------------------------------------------

    class TonDRPCNotation : public NoRaiseNotationBase {
    public:
        static constexpr std::string_view kName = "TON_DRPC";
        static constexpr std::string_view kDescription =
            "Degrees of Reflection with Passthrough (reflection configuration)";

        /// 定义文本：源包 Docs 为空、未提供正式定义，按约定以 \n 作换行占位。
        static constexpr std::string_view kDefinitionText =
            "TON_DRPC —— Degrees of Reflection with Passthrough (reflection configuration)\n"
            "Term ::= Limit | 0 | \\CE\\A9 | [Term, Term]\n"
            "组合 DRP 的穿透游标 d 与 DRC 的反射配置：\n"
            "界判定改用截断映射 r，下潜时按当前项正负重算穿透源 x2。\n"
            "[PLACEHOLDER] 正式定义文本待补录\n";

        /// 进程级实例，对应 TS 的模块级单例及其模块级 data / DRPCStd。
        [[nodiscard]] static const TonDRPCNotation& notation() {
            static const TonDRPCNotation instance{};
            return instance;
        }

        // ---------- NotationDefinition ----------

        [[nodiscard]] std::string_view id() const override { return "ton-drpc"; }
        [[nodiscard]] std::string_view name() const override { return kDescription; }
        [[nodiscard]] std::string_view simple_name() const override { return "TON_DRPC"; }
        [[nodiscard]] std::string_view definition_text() const override { return kDefinitionText; }

        /// 对应 init: () => [Infinity, -1]
        [[nodiscard]] std::vector<Term> init() const override { return init_pair_neg1(); }

        /// 对应 StandardQ(a)，带模块级缓存 DRPCStd（只缓存真）。
        /// TS 调用为 BuiltQ(x, a, x, a, a[1], -1)。
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
                           if (!ton_drpc_detail::BuiltQ(x, a, x, a, a1, L(-1))) return false;
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

        [[nodiscard]] static std::string suffix() { return " [TON_DRPC]"; }

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
            Generator(const Term& term, const TonDRPCNotation& owner)
                : ZeroSystemGenerator(term), owner_(owner) {}

        protected:
            [[nodiscard]] bool StandardQ(const Term& beta) override {
                return owner_.StandardQ(beta);
            }

        private:
            const TonDRPCNotation& owner_;
        };
    };

} // namespace omegay::notation::ton

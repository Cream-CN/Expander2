#pragma once
// ============================================================
// TON_DRC.hpp —— 对应 TON_DRC.ts
// 记号：Degrees of Reflection (reflection configuration)
//
// 相对 TON_DoR 引入反射配置（reflection configuration）：BuiltQ 除 (a, b, x) 外
// 还携带父项 ap 与父项的穿透源 xp，判定改用截断映射 r 比较 ——
// compare(r(x, xp), r(a, ap)) <= 0 取代 DoR 的 compare(x, a) <= 0。
// 递归时 x < 0 一支把 xp 更新为 x 本身；x >= 0 一支沿用外层 xp。
// ============================================================

#include "ton_Common.hpp"
#include "ton_helpers.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation::ton {

    namespace ton_drc_detail {

        /// 对应 TON_DRC.ts 的 BuiltQ(a, ap, b, x, xp)。
        [[nodiscard]] inline bool BuiltQ(const Term& a, const Term& ap, const Term& b,
                                         const Term& x, const Term& xp) {
            if (noraiseLt(x, L(0))) {
                return noraiseLt(x, b)
                    || (!compareGt(r(x, xp), r(a, ap))
                        && BuiltQ(a, ap, b, x.at(0), x)
                        && BuiltQ(a, ap, b, x.at(1), x));
            }
            return jsExact(x, 0)
                || (BuiltQ(a, ap, b, x.at(0), xp) && BuiltQ(a, ap, b, x.at(1), xp));
        }

    } // namespace ton_drc_detail

    // ------------------------------------------------------------
    // TonDRCNotation
    // ------------------------------------------------------------

    class TonDRCNotation : public NoRaiseNotationBase {
    public:
        static constexpr std::string_view kName = "TON_DRC";
        static constexpr std::string_view kDescription =
            "Degrees of Reflection (reflection configuration)";

        /// 定义文本：源包 Docs 为空、未提供正式定义，按约定以 \n 作换行占位。
        static constexpr std::string_view kDefinitionText =
            "TON_DRC —— Degrees of Reflection (reflection configuration)\n"
            "Term ::= Limit | 0 | \\CE\\A9 | [Term, Term]\n"
            "在 DoR 上引入反射配置：以截断映射 r 比较子项与父项，\n"
            "负值分支把穿透源更新为当前项，非负分支沿用外层穿透源。\n"
            "[PLACEHOLDER] 正式定义文本待补录\n";

        /// 进程级实例，对应 TS 的模块级单例及其模块级 data / DRCStd。
        [[nodiscard]] static const TonDRCNotation& notation() {
            static const TonDRCNotation instance{};
            return instance;
        }

        // ---------- NotationDefinition ----------

        [[nodiscard]] std::string_view id() const override { return "ton-drc"; }
        [[nodiscard]] std::string_view name() const override { return kDescription; }
        [[nodiscard]] std::string_view simple_name() const override { return "TON_DRC"; }
        [[nodiscard]] std::string_view definition_text() const override { return kDefinitionText; }

        /// 对应 init: () => [Infinity, -1]
        [[nodiscard]] std::vector<Term> init() const override { return init_pair_neg1(); }

        /// 对应 StandardQ(a)，带模块级缓存 DRCStd（只缓存真）。
        /// TS 调用为 BuiltQ(x, a, a, x, a)，即 (a=x, ap=a, b=a, x=x, xp=a)。
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
                           if (!ton_drc_detail::BuiltQ(x, a, a, x, a)) return false;
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

        [[nodiscard]] static std::string suffix() { return " [TON_DRC]"; }

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
            Generator(const Term& term, const TonDRCNotation& owner)
                : ZeroSystemGenerator(term), owner_(owner) {}

        protected:
            [[nodiscard]] bool StandardQ(const Term& beta) override {
                return owner_.StandardQ(beta);
            }

        private:
            const TonDRCNotation& owner_;
        };
    };

} // namespace omegay::notation::ton

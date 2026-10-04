#pragma once
// ============================================================
// TON_I.hpp —— 对应 TON_I.ts
// 记号：Iteration of n-built from below (no passthrough)
//
// 与 TON_IBP 的差别：本记号「no passthrough」，BuiltQ 直接以 0 为界递归判定，
// 不引入 reflection configuration（不需要 r，也不需要 IBP 的穿透参数 c）。
// 显示与比较走不提升（noraise）版本，生成器填入的系统数恒为字面量 0，
// 这些共同形态由公共层 NoRaiseNotationBase 承载。
//
// 结构调整（见 ton_Common.hpp 顶部说明）：末项展开终止符 -2 不作为节点字段存储，
// 而在 flatten / key / jsonKey 端按原语义补出，可观测行为不变。
// ============================================================

#include "ton_Common.hpp"
#include "ton_helpers.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation::ton {

    namespace ton_i_detail {

        /// 对应 TON_I.ts 的 BuiltQ(a, b, n, x)：
        ///   n ? (compare(x,0) < 0 && BuiltQ(x, b, n-1, x))
        ///       || ((compare(x,0) >= 0 || compare(x,a) <= 0)
        ///           && (x === 0 || (BuiltQ(a,b,n,x[0]) && BuiltQ(a,b,n,x[1]))))
        ///     : compare(a, b) < 0
        /// && / || 的短路顺序逐字保留：它决定递归展开次序，也决定 undefined 子树
        /// 是否被真正访问（访问即抛 TypeError），两者均属可观测行为。
        [[nodiscard]] inline bool BuiltQ(const Term& a, const Term& b,
                                         std::size_t n, const Term& x) {
            if (n == 0) return noraiseLt(a, b);
            if (noraiseLt(x, L(0)) && BuiltQ(x, b, n - 1, x)) return true;
            return (noraiseGe(x, L(0)) || noraiseLe(x, a))
                && (jsExact(x, 0)
                    || (BuiltQ(a, b, n, x.at(0)) && BuiltQ(a, b, n, x.at(1))));
        }

    } // namespace ton_i_detail

    // ------------------------------------------------------------
    // TonINotation
    // ------------------------------------------------------------

    class TonINotation : public NoRaiseNotationBase {
    public:
        static constexpr std::string_view kName = "TON_I";
        static constexpr std::string_view kDescription =
            "Iteration of n-built from below (no passthrough)";

        /// 定义文本：源包 Docs 为空、未提供正式定义，按约定以 \n 作换行占位。
        static constexpr std::string_view kDefinitionText =
            "TON_I —— Iteration of n-built from below (no passthrough)\n"
            "Term ::= Limit | 0 | \\CE\\A9 | [Term, Term]\n"
            "n-built 判定直接以 0 为界递归，不引入穿透参数。\n"
            "FS(term, n) 取标准项序列的第 n 项。\n"
            "[PLACEHOLDER] 正式定义文本待补录\n";

        /// 进程级实例，对应 TS 的模块级单例及其模块级 data / IStd。
        [[nodiscard]] static const TonINotation& notation() {
            static const TonINotation instance{};
            return instance;
        }

        // ---------- NotationDefinition ----------

        [[nodiscard]] std::string_view id() const override { return "ton-i"; }
        [[nodiscard]] std::string_view name() const override { return kDescription; }
        [[nodiscard]] std::string_view simple_name() const override { return "TON_I"; }
        [[nodiscard]] std::string_view definition_text() const override { return kDefinitionText; }

        /// 对应 init: [Infinity, [-1,[0,[0,-1,-2],-2],-2], [-1,[0,0,-2],-2], [-1,0,-2], -1]
        [[nodiscard]] std::vector<Term> init() const override {
            return init_iteration();
        }

        /// 对应 StandardQ(a)，带模块级缓存 IStd（TS 只写入 true，此处一致）。
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
                       for (const Index& index : smallindex(a1)) {
                           if (!ton_i_detail::BuiltQ(extract(a1, index), a,
                                                     get_n(a1, index),
                                                     extract(a1, index))) {
                               return false;
                           }
                       }
                       return true;
                   }();
            if (ok) cache.insert(a);
            return ok;
        }

        // ---------- 工程序列接口 ----------

        /// 序列约定：元素为含终止符 -2 的扁平序列，即 TS 的 `('' + term).split(',')`。
        /// [ASSUMPTION] int 无法表示 TS 用作展开起点的 Infinity，故序列入口不含 Infinity 项；
        /// 该情形请直接调用 Term 层的 FS(L(inf), n)。
        [[nodiscard]] static std::vector<int> expand(const std::vector<int>& seq, int term) {
            if (seq.empty()) return {};
            if (term < 0) return {};                   // [ASSUMPTION] 负下标在 TS 中无对应语义
            const Term t = unflatten(seq);
            return flatten_to_ints(notation().FS(t, static_cast<std::size_t>(term)));
        }

        [[nodiscard]] static std::string suffix() { return " [TON_I]"; }

    protected:
        [[nodiscard]] Term infinityReplacement() const override {
            return infinity_pair_iteration();
        }

        [[nodiscard]] std::shared_ptr<TonGeneratorBody> makeGenerator(const Term& term) const override {
            return std::make_shared<Generator>(term, *this);
        }

    private:
        /// 对应 TON_gen(term)：系统数恒为 0，产出 Copy(beta)（基类默认形态）。
        class Generator : public ZeroSystemGenerator {
        public:
            Generator(const Term& term, const TonINotation& owner)
                : ZeroSystemGenerator(term), owner_(owner) {}

        protected:
            [[nodiscard]] bool StandardQ(const Term& beta) override {
                return owner_.StandardQ(beta);
            }

        private:
            const TonINotation& owner_;
        };
    };

} // namespace omegay::notation::ton

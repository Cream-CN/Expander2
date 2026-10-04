#pragma once
// ============================================================
// TON_main.hpp —— 对应 TON_main.ts
// 记号：Taranovsky's ordinal notation（TON 主记号）
//
// 「带系统数 sys」一族（TON_main / TON_MC / TON_MPC）的起点：
//   * display / compare 走带提升的版本（TON_main_display / TON_compare）；
//   * FS 先算 sys；sys 为 Infinity 时短路给出结果，否则把项 raise 到 sys，
//     再判断是否命中 mark(sys) 走 mark_FS 快路径（见公共层 prepareSysFS）；
//   * 生成器填入 sys，产出 regress_repeated(beta)。
// 与 MC/MPC 的差别仅在标准性判定 BuiltQ 的形式与 mark 的构造。
// ============================================================

#include "ton_Common.hpp"
#include "ton_helpers.hpp"

#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation::ton {

    namespace ton_main_detail {

        /// JS 真值判定（仅 0 与 NaN 为假；本处 n 不会是 undefined）。
        /// TS 的 `n ? BuiltQ(n-1,...) : compare(a,b) < 0` 在 n 为 NaN 时走假支，
        /// 故 n 必须以 double 携带真值语义，不能提前转成整数。
        [[nodiscard]] inline bool jsTruthy(double n) { return n != 0.0 && !std::isnan(n); }

        /// 对应 TON_main.ts 的 BuiltQ(n, b, a, x)：
        ///   n ? BuiltQ(n-1, b, x, x)
        ///       || (compare(x,a) <= 0 && (typeof x === 'number' ? x >= 0
        ///                                     : (BuiltQ(n,b,a,x[1]) && BuiltQ(n,b,a,x[0]))))
        ///     : compare(a, b) < 0
        /// 形参顺序与 TS 逐字一致；递归子调用为 (n-1, b, x, x)。
        /// 短路次序保留：x[1] 先于 x[0] 求值，访问 undefined 子树时同样抛 TypeError。
        [[nodiscard]] inline bool BuiltQ(double n, const Term& b,
                                         const Term& a, const Term& x) {
            if (!jsTruthy(n)) return compareLt(a, b);
            if (BuiltQ(n - 1.0, b, x, x)) return true;
            if (!compareLe(x, a)) return false;
            if (x.isLeaf()) return x.value() >= 0;      // typeof x === 'number' ? x >= 0
            return BuiltQ(n, b, a, x.right()) && BuiltQ(n, b, a, x.left());
        }

    } // namespace ton_main_detail

    // ------------------------------------------------------------
    // TonMainNotation
    // ------------------------------------------------------------

    class TonMainNotation : public TonNotationCore {
    public:
        static constexpr std::string_view kName = "TON";
        static constexpr std::string_view kDescription =
            "Taranosvky's ordinal notation";   // TS 原文拼写如此，按「保留原标识语义」不改

        /// 定义文本：源包 Docs 为空、未提供正式定义，按约定以 \n 作换行占位。
        static constexpr std::string_view kDefinitionText =
            "TON —— Taranovsky's ordinal notation\n"
            "Term ::= Limit | 0 | \\CE\\A9<sub>n</sub> | [Term, Term]\n"
            "系统数 sys = max(0, 项中出现的最大数字)。\n"
            "FS(term, n)：先把项按 sys 提升（raise），命中 mark(sys) 时走 mark_FS；\n"
            "否则枚举标准项序列并取第 n 项，最后以 regress 反复归一后输出。\n"
            "[PLACEHOLDER] 正式定义文本待补录\n";

        /// 进程级实例，对应 TS 的模块级单例及其模块级 data / StdTrue。
        [[nodiscard]] static const TonMainNotation& notation() {
            static const TonMainNotation instance{};
            return instance;
        }

        // ---------- NotationDefinition ----------

        [[nodiscard]] std::string_view id() const override { return "ton-m"; }
        [[nodiscard]] std::string_view name() const override { return kDescription; }
        [[nodiscard]] std::string_view simple_name() const override { return "TON"; }
        [[nodiscard]] std::string_view category_id() const override { return "category-ton"; }
        [[nodiscard]] std::string_view definition_text() const override { return kDefinitionText; }

        [[nodiscard]] std::string display(const Term& term) const override {
            return TON_main_display(term);
        }
        [[nodiscard]] bool is_limit(const Term& term) const override { return TON_limit(term); }
        [[nodiscard]] int compare(const Term& x, const Term& y) const override {
            return TON_compare(x, y);
        }

        /// 对应 init: () => [Infinity, 0, -1]
        [[nodiscard]] std::vector<Term> init() const override {
            return {L(std::numeric_limits<double>::infinity()), L(0), L(-1)};
        }

        /// 对应 StandardQ(n, a)；TS 中 n 由 FS 传入的 sys 充当，故为 double。
        /// 带模块级缓存 StdTrue，只缓存判定为真者。
        [[nodiscard]] bool StandardQ(double n, const Term& a) const {
            StandardCache& cache = standardCache();
            if (cache.contains(a)) return true;

            if (a.isLeaf()) {
                cache.insert(a);
                return true;
            }
            const Term a1 = a.right();
            const Term a0 = a.left();
            const bool ok = StandardQ(n, a1)
                && StandardQ(n, a0)
                && (a0.isLeaf() || !compareGt(a1, a0.right()))   // compare(a[1], a[0][1]) <= 0
                && ton_main_detail::BuiltQ(n, a, a1, a1);
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

        [[nodiscard]] static std::string suffix() { return " [TON]"; }

    protected:
        /// 对应 FS 的 sys 分支：Infinity 短路、raise、mark/mark_FS 快路径。
        [[nodiscard]] FSPrepareResult prepareForFS(const Term& term, std::size_t n) const override {
            return prepareSysFS(term, n, &mark_main);
        }

        /// [ASSUMPTION] 此处对已 raise 过的项重算 sys。raise 只把叶子 k（0<=k<sys）
        /// 改写为 [-1, k+1, -2]，不引入大于 sys 的数字，故与 TS 中在 raise 之前算出的
        /// sys 取值相同。
        [[nodiscard]] std::shared_ptr<TonGeneratorBody> makeGenerator(const Term& term) const override {
            return std::make_shared<Generator>(term, sysOf(term), *this);
        }

    private:
        /// 对应 TON_gen(term, sys)：填入 sys，产出 regress_repeated(beta)。
        class Generator : public SysSystemGenerator {
        public:
            Generator(const Term& term, double sys, const TonMainNotation& owner)
                : SysSystemGenerator(term, sys), owner_(owner) {}

        protected:
            [[nodiscard]] bool StandardQ(const Term& beta) override {
                return owner_.StandardQ(sys(), beta);
            }
            [[nodiscard]] Term yieldValue(const Term& beta) const override {
                return regress_repeated(beta);
            }

        private:
            const TonMainNotation& owner_;
        };
    };

} // namespace omegay::notation::ton

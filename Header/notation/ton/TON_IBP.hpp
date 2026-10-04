#pragma once
// ============================================================
// TON_IBP.hpp —— 对应 TON_IBP.ts
// 记号：Iteration of n-built from below
//
// 与 TON_I 的差别（passthrough 版）：BuiltQ 额外接受参数 c —— 即沿 index 归约后
// 的公共前缀展开序列 —— 用于「反射穿透」的界判定；因此需要 get_a2 / get_n 两步。
// display / compare 仍走不提升（noraise）版本，生成器填入的系统数恒为字面量 0。
//
// 【JS 别名细节】TS 中 a2 是普通数字数组，被 get_n **就地**弹出后再作为 c 传入
// BuiltQ。C++ 侧按同一顺序串行执行（先 get_n 削减、再读 c），因此 c 观察到的
// 与 TS 一样是「弹出后」的序列。TS 里 `compare(x, c)` 的 c 是数组而非项，
// `'' + c` 与项的扁平串同为逗号连接，故以「项 vs 裸扁平序列」比较等价复刻。
// ============================================================

#include "ton_Common.hpp"
#include "ton_helpers.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation::ton {

    namespace ton_ibp_detail {

        using Flat = std::vector<double>;

        /// 对应 TON_IBP.ts 的 BuiltQ(a, b, c, n)。
        /// TS 用 `n ? subterm_index(a).every(...) : compare(a,b) < 0`，
        /// every 的回调内是四条 `||`，短路顺序逐字保留。
        [[nodiscard]] inline bool BuiltQ(const Term& a, const Term& b,
                                         const Flat& c, std::size_t n) {
            if (n == 0) return noraiseLt(a, b);
            for (const Index& x : subterm_index(a)) {
                const Term xa = extract(a, x);
                if (noraiseLe(xa, a)) continue;                              // 第 1 条 ||
                if (noraiseGe(xa, L(0))) continue;                           // 第 2 条 ||
                if (BuiltQ(xa, b, c, n - 1)) continue;                       // 第 3 条 ||
                bool any = false;                                            // 第 4 条 ||：some
                for (std::size_t yindex = 0; yindex < x.size() && !any; ++yindex) {
                    const Index y(x.begin(), x.begin() + yindex);
                    const Term xy = extract(a, y);
                    if (noraiseGe(xy, L(0))) continue;                       // false
                    if (BuiltQ(xy, b, c, n - 1)) { any = true; break; }      // true
                    if (xy.isLeaf()) continue;                               // typeof === 'number' → false
                    if (noraise_compare_flat(xy.at(1), c) >= 0) continue;    // compare(x[1], c) >= 0 → false
                    bool ok = true;
                    // 对应 `for (var zindex = x.length; zindex >= yindex; --zindex)`：
                    // TS 中 zindex 可减到 -1 自然终止；无符号自减会回绕，故改为先执行后判等。
                    for (std::size_t zindex = x.size();; --zindex) {
                        const Index z(x.begin(), x.begin() + zindex);
                        if (noraiseLt(extract(a, z), xy)) { ok = false; break; }
                        if (zindex == yindex) break;
                    }
                    if (ok) any = true;
                }
                if (!any) return false;                                      // every 失败
            }
            return true;
        }

    } // namespace ton_ibp_detail

    // ------------------------------------------------------------
    // TonIBPNotation
    // ------------------------------------------------------------

    class TonIBPNotation : public NoRaiseNotationBase {
    public:
        static constexpr std::string_view kName = "TON_IBP";
        static constexpr std::string_view kDescription =
            "Iteration of n-built from below";

        /// 定义文本：源包 Docs 为空，未提供正式定义，按约定以 \n 作换行占位。
        static constexpr std::string_view kDefinitionText =
            "TON_IBP —— Iteration of n-built from below\n"
            "Term ::= \\CE\\A9^n | [Term, Term]\n"
            "n-built 判定带穿透参数 c（沿 index 归约后的公共前缀序列）\n"
            "[PLACEHOLDER] 正式定义文本待补录\n";

        /// 进程级实例，对应 TS 的模块级单例及其模块级 data / IBPStd。
        [[nodiscard]] static const TonIBPNotation& notation() {
            static const TonIBPNotation instance{};
            return instance;
        }

        // ---------- NotationDefinition ----------

        [[nodiscard]] std::string_view id() const override { return "ton-ibp"; }
        [[nodiscard]] std::string_view name() const override { return kDescription; }
        [[nodiscard]] std::string_view simple_name() const override { return "TON_IBP"; }
        [[nodiscard]] std::string_view definition_text() const override { return kDefinitionText; }

        /// 对应 init: 与 TON_I 完全相同的五个初值项。
        [[nodiscard]] std::vector<Term> init() const override { return init_iteration(); }

        /// 对应 StandardQ(a)：带模块级缓存 IBPStd。
        /// TS 的写法是 `return result ? (IBPStd[str] = result) : result;`
        /// —— 只缓存判定为真者，此处一致。
        [[nodiscard]] bool StandardQ(const Term& a) const {
            StandardCache& cache = standardCache();
            if (cache.contains(a)) return true;

            bool result = false;
            if (a.isLeaf()) {
                result = true;
            } else {
                const Term a1 = a.at(1);
                const Term a0 = a.at(0);
                result = StandardQ(a1)
                    && StandardQ(a0)
                    && (a0.isLeaf() || noraiseLe(a1, a0.at(1)))
                    && [&] {
                           for (const Index& a1index : smallindex(a1)) {
                               std::vector<double> a2 = get_a2(a1, a1index);
                               const std::size_t n = get_n(a2);   // 就地削减 a2，c 随后读到削减结果
                               if (!ton_ibp_detail::BuiltQ(extract(a1, a1index), a,
                                                           a2, n)) {
                                   return false;
                               }
                           }
                           return true;
                       }();
            }
            if (result) cache.insert(a);
            return result;
        }

        // ---------- 工程序列接口 ----------

        /// [ASSUMPTION] int 无法表示 Infinity，序列入口不含 Infinity；
        /// 极限展开请直接调用 Term 层的 FS(L(inf), n)。
        [[nodiscard]] static std::vector<int> expand(const std::vector<int>& seq, int term) {
            if (seq.empty()) return {};
            if (term < 0) return {};
            const Term t = unflatten(seq);
            return flatten_to_ints(notation().FS(t, static_cast<std::size_t>(term)));
        }

        [[nodiscard]] static std::string suffix() { return " [TON_IBP]"; }

    protected:
        /// 对应 FS 开头的 Infinity 替换（与 TON_I 同形）。
        [[nodiscard]] Term infinityReplacement() const override {
            return infinity_pair_iteration();
        }

        [[nodiscard]] std::shared_ptr<TonGeneratorBody> makeGenerator(const Term& term) const override {
            return std::make_shared<Generator>(term, *this);
        }

    private:
        /// 对应 TON_gen(term)：系统数恒为 0，产出 Copy(beta)。
        class Generator : public ZeroSystemGenerator {
        public:
            Generator(const Term& term, const TonIBPNotation& owner)
                : ZeroSystemGenerator(term), owner_(owner) {}

        protected:
            [[nodiscard]] bool StandardQ(const Term& beta) override {
                return owner_.StandardQ(beta);
            }

        private:
            const TonIBPNotation& owner_;
        };
    };

} // namespace omegay::notation::ton

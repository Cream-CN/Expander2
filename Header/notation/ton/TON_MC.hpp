#pragma once
// ============================================================
// TON_MC.hpp —— 对应 TON_MC.ts
// 记号：TON (reflection configuration) without passthrough
//
// 与 TON_main 同属「带系统数 sys」一族，FS 前置流程一致（mark 的构造不同：
// 使用 mark_reflection）。差别在标准性判定：本记号的 BuiltQ(a, b, n) 以
// reflection configuration 为界 —— 通过公共层的 r 把子项与父项截断后比较，
// 并用 totest 累加器收集「不低于父项」的子树，逐层检验。
// 「without passthrough」指：截断值不向更深层传递（对比 TON_MPC 的 rc 参数）。
// ============================================================

#include "ton_Common.hpp"
#include "ton_helpers.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation::ton {

    namespace ton_mc_detail {

        /// 对应 BuiltQ 内部的闭包组：extractparent、refresh_totest 与共享的 totest。
        /// TS 里 totest 在外层函数创建、在 every 回调中每轮重置为空数组，
        /// refresh_totest 向其 push —— 此处以同一引用贯穿，行为一致。
        class Checker {
        public:
            Checker(const Term& a, const Term& b, double n) : a_(a), b_(b), n_(n) {}

            /// 对应 BuiltQ(a, b, n)。
            [[nodiscard]] bool run() {
                if (!(n_ > 0)) return compareLt(a_, b_);
                for (const Index& x : subterm_index(a_)) {
                    if (!checkOne(x)) return false;     // Array.prototype.every 短路
                }
                return true;
            }

        private:
            /// 对应 extractparent(x)。
            [[nodiscard]] Term extractparent(const Index& x) const {
                if (x.empty()) return b_;
                return extract(a_, Index(x.begin(), x.end() - 1));
            }

            /// 对应 refresh_totest(d, e)。
            void refresh_totest(const Index& d, const Index& e) {
                const Term ed = extract(a_, d);
                if (ed.isLeaf()) return;                       // typeof extract(a,d) === 'number'
                if (compareLt(ed, extractparent(e))) return;
                totest_.push_back(d);
                Index d0 = d; d0.push_back(0);
                refresh_totest(d0, e);
                Index d1 = d; d1.push_back(1);
                refresh_totest(d1, e);
            }

            /// 对应 every 回调体。
            [[nodiscard]] bool checkOne(const Index& x) {
                // ① compare(r(extract(a,x), extractparent(x)), r(a,b)) <= 0 → 视为通过
                if (!compareGt(r(extract(a_, x), extractparent(x)), r(a_, b_))) return true;

                // ② x.some((t, zindex) => compare(extract(a, x.slice(0,zindex)), b) < 0) → 通过
                for (std::size_t zindex = 0; zindex < x.size(); ++zindex) {
                    if (compareLt(extract(a_, Index(x.begin(), x.begin() + zindex)), b_)) return true;
                }

                // ③ 重置 totest，收集并逐前缀检验
                totest_.clear();
                refresh_totest(x, x);

                // for (var y = x.slice(); y.length > 0; y.pop())
                for (Index y = x; !y.empty(); y.pop_back()) {
                    bool all_prefix = true;
                    for (std::size_t dz = 0; dz + y.size() < x.size(); ++dz) {
                        const std::size_t stop = y.size() + dz;
                        if (compareLt(extract(a_, Index(x.begin(), x.begin() + stop)),
                                      extractparent(y))) { all_prefix = false; break; }
                    }
                    if (!all_prefix) continue;

                    bool all_totest = true;
                    for (const Index& z : totest_) {
                        if (compareLt(extract(a_, z), extractparent(y))) { all_totest = false; break; }
                    }
                    if (!all_totest) continue;

                    if (run_sub(extract(a_, y), extractparent(y), n_ - 1.0)) return true;
                }
                return false;
            }

            /// 递归入口（对应 BuiltQ(extract(a,y), extractparent(y), n-1)）。
            [[nodiscard]] bool run_sub(const Term& a, const Term& b, double n) {
                Checker sub(a, b, n);
                return sub.run();
            }

            Term a_;
            Term b_;
            double n_;
            std::vector<Index> totest_;
        };

    } // namespace ton_mc_detail

    // ------------------------------------------------------------
    // TonMCNotation
    // ------------------------------------------------------------

    class TonMCNotation : public TonNotationCore {
    public:
        static constexpr std::string_view kName = "TON_MC";
        static constexpr std::string_view kDescription =
            "TON (reflection configuration) without passthrough";

        /// 定义文本：源包 Docs 为空、未提供正式定义，按约定以 \n 作换行占位。
        static constexpr std::string_view kDefinitionText =
            "TON_MC —— TON (reflection configuration) without passthrough\n"
            "在 TON 的基础上引入 reflection configuration：\n"
            "子项经截断映射 r 后与父项的截断值比较，不通过者进入 totest 逐层检验；\n"
            "截断值不向更深层传递（无 passthrough）。\n"
            "[PLACEHOLDER] 正式定义文本待补录\n";

        /// 进程级实例，对应 TS 的模块级单例及其模块级 data / MCStd。
        [[nodiscard]] static const TonMCNotation& notation() {
            static const TonMCNotation instance{};
            return instance;
        }

        // ---------- NotationDefinition ----------

        [[nodiscard]] std::string_view id() const override { return "ton-mc"; }
        [[nodiscard]] std::string_view name() const override { return kDescription; }
        [[nodiscard]] std::string_view simple_name() const override { return "TON_MC"; }
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

        /// 对应 StandardQ(n, a)，带模块级缓存 MCStd（只缓存真）。
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
                && (a0.isLeaf() || !compareGt(a1, a0.right()))
                && ton_mc_detail::Checker(a1, a, n).run();   // BuiltQ(a[1], a, n)
            if (ok) cache.insert(a);
            return ok;
        }

        /// 序列约定同 TON_main：含终止符 -2 的扁平序列。
        [[nodiscard]] static std::vector<int> expand(const std::vector<int>& seq, int term) {
            if (seq.empty()) return {};
            if (term < 0) return {};
            const Term t = unflatten(seq);
            return flatten_to_ints(notation().FS(t, static_cast<std::size_t>(term)));
        }

        [[nodiscard]] static std::string suffix() { return " [TON_MC]"; }

    protected:
        [[nodiscard]] FSPrepareResult prepareForFS(const Term& term, std::size_t n) const override {
            return prepareSysFS(term, n, &mark_reflection);
        }

        [[nodiscard]] std::shared_ptr<TonGeneratorBody> makeGenerator(const Term& term) const override {
            return std::make_shared<Generator>(term, sysOf(term), *this);
        }

    private:
        /// 对应 TON_gen(term, sys)：填入 sys，产出 regress_repeated(beta)。
        class Generator : public SysSystemGenerator {
        public:
            Generator(const Term& term, double sys, const TonMCNotation& owner)
                : SysSystemGenerator(term, sys), owner_(owner) {}

        protected:
            [[nodiscard]] bool StandardQ(const Term& beta) override {
                return owner_.StandardQ(sys(), beta);
            }
            [[nodiscard]] Term yieldValue(const Term& beta) const override {
                return regress_repeated(beta);
            }

        private:
            const TonMCNotation& owner_;
        };
    };

} // namespace omegay::notation::ton

#pragma once
// ============================================================
// TON_MPC.hpp —— 对应 TON_MPC.ts
// 记号：TON with passthrough (reflection configuration)
//
// 与 TON_MC 的唯一实质差别（逐行对照 TS 原文可验证）：
//   BuiltQ 多出一个参数 rc（= r(a[1], a)，即父层的截断值），
//   并在最内层判定中插入一条「穿透」短路：
//       compare(r(extract(a,y), extractparent(y)), rc) < 0 || BuiltQ(..., rc, n-1)
//   其余结构（extractparent / refresh_totest / totest / 三层检验）与 MC 逐字相同。
// FS 前置流程、mark_reflection、regress_repeated 产出等均与 MC 一致。
// ============================================================

#include "ton_Common.hpp"
#include "ton_helpers.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation::ton {

    namespace ton_mpc_detail {

        /// 对应 BuiltQ(a, b, rc, n) 的闭包组；结构同 MC 的 Checker，仅多出 rc 与穿透短路。
        class Checker {
        public:
            Checker(const Term& a, const Term& b, const Term& rc, double n)
                : a_(a), b_(b), rc_(rc), n_(n) {}

            [[nodiscard]] bool run() {
                if (!(n_ > 0)) return compareLt(a_, b_);       // JS 真值判定：n 为 NaN 时亦走此支
                for (const Index& x : subterm_index(a_)) {
                    if (!checkOne(x)) return false;            // every 短路
                }
                return true;
            }

        private:
            [[nodiscard]] Term extractparent(const Index& x) const {
                if (x.empty()) return b_;
                return extract(a_, Index(x.begin(), x.end() - 1));
            }

            void refresh_totest(const Index& d, const Index& e) {
                const Term ed = extract(a_, d);
                if (ed.isLeaf()) return;
                if (compareLt(ed, extractparent(e))) return;
                totest_.push_back(d);
                Index d0 = d; d0.push_back(0);
                refresh_totest(d0, e);
                Index d1 = d; d1.push_back(1);
                refresh_totest(d1, e);
            }

            [[nodiscard]] bool checkOne(const Index& x) {
                // ① compare(r(extract(a,x), extractparent(x)), r(a,b)) <= 0
                if (!compareGt(r(extract(a_, x), extractparent(x)), r(a_, b_))) return true;

                // ② x.some((t, zindex) => compare(extract(a, x.slice(0,zindex)), b) < 0)
                for (std::size_t zindex = 0; zindex < x.size(); ++zindex) {
                    if (compareLt(extract(a_, Index(x.begin(), x.begin() + zindex)), b_)) return true;
                }

                // ③ 重置 totest 并收集
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

                    // 穿透短路（MPC 相对 MC 新增的一支）：截断值已低于 rc 即通过，
                    // 否则继续向下递归并携带同一个 rc。
                    const Term sub = extract(a_, y);
                    const Term parent = extractparent(y);
                    if (compareLt(r(sub, parent), rc_)) return true;
                    if (Checker(sub, parent, rc_, n_ - 1.0).run()) return true;
                }
                return false;
            }

            Term a_;
            Term b_;
            Term rc_;
            double n_;
            std::vector<Index> totest_;
        };

    } // namespace ton_mpc_detail

    // ------------------------------------------------------------
    // TonMPCNotation
    // ------------------------------------------------------------

    class TonMPCNotation : public TonNotationCore {
    public:
        static constexpr std::string_view kName = "TON_MPC";
        static constexpr std::string_view kDescription =
            "TON with passthrough (reflection configuration)";

        /// 定义文本：源包 Docs 为空、未提供正式定义，按约定以 \n 作换行占位。
        static constexpr std::string_view kDefinitionText =
            "TON_MPC —— TON with passthrough (reflection configuration)\n"
            "在 TON_MC 的反射配置上加入穿透：\n"
            "判定携带父层截断值 rc = r(a[1], a)，子项截断值低于 rc 时直接通过，\n"
            "否则继续向下递归并传递同一个 rc。\n"
            "[PLACEHOLDER] 正式定义文本待补录\n";

        /// 进程级实例，对应 TS 的模块级单例及其模块级 data / MPCStd。
        [[nodiscard]] static const TonMPCNotation& notation() {
            static const TonMPCNotation instance{};
            return instance;
        }

        // ---------- NotationDefinition ----------

        [[nodiscard]] std::string_view id() const override { return "ton-mpc"; }
        [[nodiscard]] std::string_view name() const override { return kDescription; }
        [[nodiscard]] std::string_view simple_name() const override { return "TON_MPC"; }
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

        /// 对应 StandardQ(n, a)，带模块级缓存 MPCStd（只缓存真）。
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
                && ton_mpc_detail::Checker(a1, a, r(a1, a), n).run();   // BuiltQ(a[1], a, r(a[1],a), n)
            if (ok) cache.insert(a);
            return ok;
        }

        /// 序列约定同 TON_main / TON_MC。
        [[nodiscard]] static std::vector<int> expand(const std::vector<int>& seq, int term) {
            if (seq.empty()) return {};
            if (term < 0) return {};
            const Term t = unflatten(seq);
            return flatten_to_ints(notation().FS(t, static_cast<std::size_t>(term)));
        }

        [[nodiscard]] static std::string suffix() { return " [TON_MPC]"; }

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
            Generator(const Term& term, double sys, const TonMPCNotation& owner)
                : SysSystemGenerator(term, sys), owner_(owner) {}

        protected:
            [[nodiscard]] bool StandardQ(const Term& beta) override {
                return owner_.StandardQ(sys(), beta);
            }
            [[nodiscard]] Term yieldValue(const Term& beta) const override {
                return regress_repeated(beta);
            }

        private:
            const TonMPCNotation& owner_;
        };
    };

} // namespace omegay::notation::ton

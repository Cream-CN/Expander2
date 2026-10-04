#pragma once
// ============================================================
// ton_helpers.hpp —— 对应 ton_helpers.ts
//
// 该文件是 TON 记号族的共享运算层，逐条对应 TS 的 7 个导出：
//   TON_noraise_compare / TON_noraise_display / raise /
//   TON_compare / TON_main_display / TON_limit / r
//
// 其中 TON_noraise_compare 的实现下沉到 ton_Common.hpp（smallindex、smallpart
// 等共享遍历工具同样依赖它），此处按 TS 的导出面重导出，保持接口一一对应。
//
// 语义要点：
//   * TS 里 term 为 number 时是叶子、为数组时取 [0]/[1]；C++ 侧用 Term::isLeaf()
//     与 left()/right() 表达同一判别，非叶子被当作数字使用时按 JS 强制转换得 NaN，
//     故一切比较均为 false —— 与源程序可观测行为一致。
//   * 比较一律走 flatten 后的字典序，与 TS 的 `('' + x).split(',').map(e => +e)` 等价。
//   * r 的分支顺序与 && 短路次序逐字保留，因为它决定递归展开顺序与异常传播。
// ============================================================

#include "ton_Common.hpp"

#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

namespace omegay::notation::ton {

    // ------------------------------------------------------------
    // 1. TON_noraise_compare —— 实现位于 ton_Common.hpp
    // ------------------------------------------------------------
    // TS 在本文件导出该函数；C++ 侧因 smallindex / smallpart 等共享遍历也需要它，
    // 故实现上收到 ton_Common.hpp（同命名空间），此处不再重复声明。

    // ------------------------------------------------------------
    // 2. TON_noraise_display
    // ------------------------------------------------------------
    // TS:
    //   typeof term === 'number'
    //       ? term === Infinity ? 'Limit' : term < 0 ? '0' : 'Ω'
    //       : display(term[0]) + display(term[1]) + 'C'

    [[nodiscard]] inline std::string TON_noraise_display(const Term& term) {
        if (term.isLeaf()) {
            const double v = term.value();
            if (v == std::numeric_limits<double>::infinity()) return "Limit";
            return v < 0 ? "0" : "\xCE\xA9";              // Ω (U+03A9, UTF-8)
        }
        return TON_noraise_display(term.left()) + TON_noraise_display(term.right()) + "C";
    }

    // ------------------------------------------------------------
    // 3. raise
    // ------------------------------------------------------------
    // TS:
    //   typeof term === 'number'
    //       ? term >= 0 && term < sys ? [-1, raise(term + 1, sys), -2] : term
    //       : [raise(term[0], sys), raise(term[1], sys), -2]
    //
    // 终止符 -2 由 Term::node 隐含，序列化端补出（见 ton_Common.hpp 顶部说明）。

    [[nodiscard]] inline Term raise(const Term& term, double sys) {
        if (term.isLeaf()) {
            const double v = term.value();
            if (v >= 0 && v < sys) return N(L(-1), raise(L(v + 1), sys));
            return term;
        }
        return N(raise(term.left(), sys), raise(term.right(), sys));
    }

    // ------------------------------------------------------------
    // 4. TON_compare
    // ------------------------------------------------------------
    // TS 先由 `('' + x).split(',').map(Number)` 取 sysx / sysy（即 flatten 后的
    // Math.max(0, ...)），仅在两者均有限且至少一个为正时，把 x、y 同时提升到
    // max(sysx, sysy) 再做字典序比较；否则直接比较原展开序列。
    //
    // [ASSUMPTION] TS 的 `sysx < Infinity` 在 sysx 为 NaN 时同为 true，此时
    // raise 的 `v >= 0 && v < sys` 亦恒假，故 NaN 路径下两侧行为一致，无需特判。

    [[nodiscard]] inline int TON_compare(const Term& x, const Term& y) {
        const std::vector<double> fx = flatten(x);
        const std::vector<double> fy = flatten(y);
        const double sysx = jsMax0(fx);
        const double sysy = jsMax0(fy);
        const double inf = std::numeric_limits<double>::infinity();

        if (sysx < inf && sysy < inf && (sysx > 0 || sysy > 0)) {
            const double sys = std::max(sysx, sysy);      // 对应 Math.max(sysx, sysy)
            return compareFlat(flatten(raise(x, sys)), flatten(raise(y, sys)));
        }
        return compareFlat(fx, fy);
    }

    [[nodiscard]] inline bool compareLt(const Term& x, const Term& y) { return TON_compare(x, y) < 0; }
    [[nodiscard]] inline bool compareLe(const Term& x, const Term& y) { return TON_compare(x, y) <= 0; }
    [[nodiscard]] inline bool compareGt(const Term& x, const Term& y) { return TON_compare(x, y) > 0; }

    // ------------------------------------------------------------
    // 5. TON_main_display
    // ------------------------------------------------------------
    // TS 与 TON_noraise_display 的唯一差别：正有限叶子输出 'Ω<sub>' + term + '</sub>'。

    [[nodiscard]] inline std::string TON_main_display(const Term& term) {
        if (term.isLeaf()) {
            const double v = term.value();
            if (v == std::numeric_limits<double>::infinity()) return "Limit";
            if (v < 0) return "0";
            return "\xCE\xA9<sub>" + numberToString(v) + "</sub>";  // Ω<sub>n</sub>
        }
        return TON_main_display(term.left()) + TON_main_display(term.right()) + "C";
    }

    // ------------------------------------------------------------
    // 6. TON_limit
    // ------------------------------------------------------------
    // TS: typeof term === 'number' ? term >= 0 : typeof term[1] !== 'number' || term[1] >= 0
    // 注意 `typeof term[1] !== 'number'` 对 undefined（叶子取子项的结果）同样为真。

    [[nodiscard]] inline bool TON_limit(const Term& term) {
        if (term.isLeaf()) return term.value() >= 0;
        const Term r1 = term.at(1);
        return !r1.isLeaf() || r1.value() >= 0;
    }

    // ------------------------------------------------------------
    // 7. r —— reflection configuration 下的截断映射
    // ------------------------------------------------------------
    // TS:
    //   if (typeof a === 'number') return a;
    //   if (TON_compare(a, b) > 0) {
    //       if (TON_compare(a[0], b) > 0) return [r(a[0], b), r(a[1], b), -2];
    //       else                          return [-0.5, r(a[1], b), -2];
    //   } else {
    //       if (TON_compare(a, b) < 0) return a;
    //       else                       return -0.5;
    //   }

    [[nodiscard]] inline Term r(const Term& a, const Term& b) {
        if (a.isLeaf()) return a;
        if (compareGt(a, b)) {
            if (compareGt(a.left(), b)) return N(r(a.left(), b), r(a.right(), b));
            return N(L(-0.5), r(a.right(), b));
        }
        if (compareLt(a, b)) return a;
        return L(-0.5);
    }

    // ------------------------------------------------------------
    // 8. prepareSysFS —— sys 家族共同的 FS 前置流程（依赖 raise，故置于辅助层）
    // ------------------------------------------------------------
    // TS 中 TON_main / TON_MC / TON_MPC 的 FS 开头逐字相同（mark 的构造除外）：
    //   ① sys = 项的展开序列最大值（叶子时即其本身）；
    //   ② sys 为 Infinity：直接给出 [n,[n,...,[n, n]...]] 形态并短路返回；
    //   ③ term = raise(term, sys)；
    //   ④ sys >= 1 且提升后的项与 mark(sys) 的展开串相等：返回 mark_FS(sys, n)。
    // mark 在两族中构造不同（TON_main 用 mark_main，MC/MPC 用 mark_reflection），以参数注入。
    [[nodiscard]] inline FSPrepareResult prepareSysFS(const Term& term, std::size_t n,
                                                     Term (*mark)(double)) {
        const double sys = sysOf(term);
        if (sys == std::numeric_limits<double>::infinity()) {
            Term res = N(L(static_cast<double>(n)), L(static_cast<double>(n)));
            for (std::size_t i = 0; i < n; ++i) res = N(L(-1), res);
            return FSPrepareResult{true, res};
        }
        const Term raised = raise(term, sys);
        if (sys >= 1 && keyOf(raised) == keyOf(mark(sys))) {
            return FSPrepareResult{true, mark_FS(sys, n)};
        }
        return FSPrepareResult{false, raised};
    }

    // ------------------------------------------------------------
    // 9. NoRaiseNotationBase —— 6 个不提升（noraise）记号的共同形态
    // ------------------------------------------------------------
    // TON_I / TON_IBP / TON_DoR / TON_DRP / TON_DRC / TON_DRPC 共用：
    //   display = TON_noraise_display, is_limit = TON_limit, compare = TON_noraise_compare，
    //   生成器填入字面量 0 且产出 Copy(beta)，FS 前置只做 Infinity 替换。
    // 各模块只需提供 Infinity 替换后的项（两族形态不同）与自己的 StandardQ / 生成器。
    // 定义于辅助层是因为其成员直接调用本文件的三个导出。

    class NoRaiseNotationBase : public TonNotationCore {
    public:
        [[nodiscard]] std::string display(const Term& term) const override {
            return TON_noraise_display(term);
        }
        [[nodiscard]] bool is_limit(const Term& term) const override { return TON_limit(term); }
        [[nodiscard]] int compare(const Term& x, const Term& y) const override {
            return TON_noraise_compare(x, y);
        }
        [[nodiscard]] std::string_view category_id() const override { return "category-ton"; }

    protected:
        using TonNotationCore::TonNotationCore;

        /// 对应 FS 开头 `if ('' + term === 'Infinity') term = <本函数返回值>`。
        [[nodiscard]] virtual Term infinityReplacement() const = 0;

        [[nodiscard]] FSPrepareResult prepareForFS(const Term& term, std::size_t) const override {
            if (keyOf(term) == "Infinity") {
                return FSPrepareResult{false, infinityReplacement()};
            }
            return FSPrepareResult{false, term};
        }
    };

} // namespace omegay::notation::ton

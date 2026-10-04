#pragma once
// ============================================================
// ton_Common.hpp —— TON 记号族的公共基础设施
//
// 来源：TON 的 TypeScript 库（ton_helpers.ts 与 9 个记号模块共用的实现）。
// 本文件只做一件事：定义「记号项 Term」及其必需的运算，并把 9 个模块中
// 逐字相同的骨架（TON_gen、序列缓存、结构与遍历工具）上收到这里。
//
// 【TON 的特殊性与本次结构调整】
// TS 中每个项统一写作 number 或 [左子项, 右子项, -2]，末项 -2 是「展开终止符」。
// 按用户规范：终止符不作为节点字段存储（去除末项的展开选项），而在序列化端
// 按原语义直接补出（保留末项）。这是无损改写 —— 在全部 TS 源文件中，-2 只参与
// 「扁平化字符串 / 字典序比较 / JSON 缓存键」，从未被当作可寻址子项读回（t[2] 无出现）。
// 因此比较、显示、缓存键、长度判定等所有可观测行为均保持不变。
//
// 【语义保真的三条硬约束】
// 1. 项是引用语义（shared_ptr 节点），复刻 JS 数组的别名与就地修改。生成器主体、
//    get_n/get_a2 的 scan、MC/MPC 的 totest 都依赖就地改写，值语义会改变结果。
// 2. 保留 undefined 传播：对叶子取子项得 undefined（JS 数字取属性不报错），对
//    undefined 再取子项抛 js_type_error，等价 TS 在 ESM 严格模式下的 TypeError 传播。
// 3. && / || 与 Array.every / Array.some 的短路顺序逐字保留，因为它决定递归展开顺序
//    与是否触发异常，属于可观测行为。
// ============================================================

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace omegay::notation::ton {

    // ------------------------------------------------------------
    // 0. 异常：对应 JS 运行时在严格模式下抛出的 TypeError
    // ------------------------------------------------------------

    class ton_error : public std::runtime_error {
    public:
        explicit ton_error(const std::string& message)
            : std::runtime_error(message) {}
    };

    /// 对应 JS：对 undefined 取下标、对原始值赋属性时抛出的 TypeError。
    class js_type_error : public ton_error {
    public:
        explicit js_type_error(std::string_view what)
            : ton_error("TypeError: " + std::string(what)) {}
    };

    // ------------------------------------------------------------
    // 1. Term —— 记号项（number 叶 | 二分子项节点 | undefined）
    // ------------------------------------------------------------

    struct TermNode;

    enum class TermKind {
        leaf,       ///< 对应 TS `typeof x === 'number'`
        node,       ///< 对应 TS 的 [左, 右, -2]；终止符不入字段，由序列化端补出
        undefined,  ///< 对应 JS 的 undefined
    };

    class Term {
    public:
        static Term leaf(double v) {
            Term t;
            t.kind_ = TermKind::leaf;
            t.value_ = v;
            return t;
        }

        /// 对应 TS 的 [A, B, -2]。定义在 TermNode 完整声明之后。
        static Term node(const Term& left, const Term& right);

        static Term jsUndefined() {
            Term t;
            t.kind_ = TermKind::undefined;
            return t;
        }

        [[nodiscard]] TermKind kind() const noexcept { return kind_; }
        [[nodiscard]] bool isLeaf() const noexcept { return kind_ == TermKind::leaf; }
        [[nodiscard]] bool isNode() const noexcept { return kind_ == TermKind::node; }
        [[nodiscard]] bool isUndefined() const noexcept { return kind_ == TermKind::undefined; }

        /// 叶子取值。非叶子按 JS 强制转换语义得 NaN，使一切比较为 false。
        [[nodiscard]] double value() const {
            return kind_ == TermKind::leaf ? value_
                                           : std::numeric_limits<double>::quiet_NaN();
        }

        /// 只读子项，对应 TS 的 t[0] / t[1]。定义在 TermNode 完整声明之后。
        [[nodiscard]] Term at(std::size_t i) const;

        /// 可写子项，对应 TS 严格模式下的 t[0] = v / t[1] = v（仅节点可写）。
        [[nodiscard]] Term& atRef(std::size_t i);

        [[nodiscard]] Term left() const { return at(0); }
        [[nodiscard]] Term right() const { return at(1); }

        /// 引用相等，对应 JS 的 ===（数组按对象标识比较）。
        [[nodiscard]] bool operator==(const Term& o) const noexcept {
            if (kind_ != o.kind_) return false;
            if (kind_ == TermKind::leaf) return value_ == o.value_;
            if (kind_ == TermKind::node) return node_ == o.node_;
            return true;
        }
        [[nodiscard]] bool operator!=(const Term& o) const noexcept { return !(*this == o); }

    private:
        TermKind kind_ = TermKind::undefined;
        double value_ = std::numeric_limits<double>::quiet_NaN();
        std::shared_ptr<TermNode> node_;
    };

    struct TermNode {
        Term left_;
        Term right_;
    };

    // -------- Term 中依赖 TermNode 完整定义的部分 --------

    inline Term Term::node(const Term& left, const Term& right) {
        auto n = std::make_shared<TermNode>();
        n->left_ = left;
        n->right_ = right;
        Term t;
        t.kind_ = TermKind::node;
        t.node_ = std::move(n);
        return t;
    }

    inline Term Term::at(std::size_t i) const {
        switch (kind_) {
        case TermKind::node:  return i == 0 ? node_->left_ : node_->right_;
        case TermKind::leaf:  return Term::jsUndefined();
        default:              throw js_type_error("cannot read subterm of undefined");
        }
    }

    inline Term& Term::atRef(std::size_t i) {
        if (kind_ != TermKind::node) {
            throw js_type_error("cannot assign to property of a non-object term");
        }
        return i == 0 ? node_->left_ : node_->right_;
    }

    /// 字面量书写辅助：L(n) 为叶子，N(a, b) 为节点（隐含末项 -2）。
    [[nodiscard]] inline Term L(double v) { return Term::leaf(v); }
    [[nodiscard]] inline Term N(const Term& a, const Term& b) { return Term::node(a, b); }

    // ------------------------------------------------------------
    // 2. JS 语义辅助
    // ------------------------------------------------------------

    /// JS `t === v`（v 为数字）。
    [[nodiscard]] inline bool jsExact(const Term& t, double v) {
        return t.isLeaf() && t.value() == v;
    }

    /// 以下四个对应 TS 中「把项当数字用」的隐式转换比较：非叶子恒为 false。
    [[nodiscard]] inline bool jsLt0(const Term& t) { return t.isLeaf() && t.value() < 0.0; }
    [[nodiscard]] inline bool jsGt0(const Term& t) { return t.isLeaf() && t.value() > 0.0; }
    [[nodiscard]] inline bool jsGte0(const Term& t) { return t.isLeaf() && t.value() >= 0.0; }

    /// 对应 JS `Math.max(0, ...)`：任一 NaN 使结果为 NaN，任一 +∞ 使结果为 +∞。
    [[nodiscard]] inline double jsMax0(const std::vector<double>& xs) {
        double acc = 0.0;
        bool has_nan = false;
        for (double x : xs) {
            if (std::isnan(x)) has_nan = true;
            if (x > acc) acc = x;
        }
        return has_nan ? std::numeric_limits<double>::quiet_NaN() : acc;
    }

    // ------------------------------------------------------------
    // 3. 字符串化：复刻 `'' + x`、`JSON.stringify(x)` 与数字转字符串
    // ------------------------------------------------------------

    /// JS Number → String：整数不带小数点，Infinity → "Infinity"，NaN → "NaN"，
    /// 其余取最短可往返表示（如 "-0.5"）。
    [[nodiscard]] inline std::string numberToString(double v) {
        constexpr double kInfinity = std::numeric_limits<double>::infinity();
        if (std::isnan(v)) return "NaN";
        if (v == kInfinity) return "Infinity";
        if (v == -kInfinity) return "-Infinity";
        // JS: String(-0) === "0"，Array.prototype.join 同样把 -0 输出为 "0"。
        if (v == 0.0) return "0";
        if (v == std::floor(v) && std::fabs(v) < 1e21) {
            return std::to_string(static_cast<long long>(v));
        }
        char buf[64];
        for (int precision = 15; precision <= 17; ++precision) {
            std::snprintf(buf, sizeof(buf), "%.*g", precision, v);
            if (std::strtod(buf, nullptr) == v) return buf;
        }
        std::snprintf(buf, sizeof(buf), "%.17g", v);
        return buf;
    }

    /// 前序展开（叶子序列），每个节点在子项之后补出终止符 -2。
    /// 对应 `('' + term).split(',').map(e => +e)`。
    inline void appendFlat(const Term& t, std::vector<double>& out) {
        switch (t.kind()) {
        case TermKind::leaf:
            out.push_back(t.value());
            return;
        case TermKind::node:
            appendFlat(t.left(), out);
            appendFlat(t.right(), out);
            out.push_back(-2.0);          // 保留末项：终止符在此显式补出
            return;
        default:
            // JS: [undefined, 0, -2] → ",0,-2" → Number("") === 0
            out.push_back(0.0);
            return;
        }
    }

    [[nodiscard]] inline std::vector<double> flatten(const Term& t) {
        std::vector<double> out;
        if (t.isUndefined()) {
            out.push_back(std::numeric_limits<double>::quiet_NaN()); // Number("undefined")
            return out;
        }
        appendFlat(t, out);
        return out;
    }

    /// 对应 `('' + term).split(',').length`（不复制序列）。
    [[nodiscard]] inline std::size_t flattenCount(const Term& t) {
        switch (t.kind()) {
        case TermKind::leaf:
        case TermKind::undefined: return 1;
        default: return flattenCount(t.left()) + flattenCount(t.right()) + 1; // +1 为补出的 -2
        }
    }

    /// 序列内字符串化：undefined 在数组中贡献空串（JS Array.prototype.join 语义）。
    [[nodiscard]] inline std::string joinKey(const Term& t) {
        switch (t.kind()) {
        case TermKind::leaf: return numberToString(t.value());
        case TermKind::node: return joinKey(t.left()) + "," + joinKey(t.right()) + ",-2";
        default:             return std::string();
        }
    }

    /// 顶层 `'' + term`：TS 用作缓存键（data[datakey]）与结构相等判据。
    [[nodiscard]] inline std::string keyOf(const Term& t) {
        return t.isUndefined() ? "undefined" : joinKey(t);
    }

    /// JSON.stringify 的等价键，对应各模块 StandardQ 的标准性缓存下标。
    /// [ASSUMPTION] TS 直接以该字符串作对象下标；undefined / Infinity / NaN 按 JS
    /// 的 JSON 序列化规则归一为 "null"，与源文件行为一致。
    [[nodiscard]] inline std::string jsonKey(const Term& t) {
        std::string out;
        struct Walk {
            static void go(const Term& x, std::string& s) {
                switch (x.kind()) {
                case TermKind::leaf:
                    s += std::isfinite(x.value()) ? numberToString(x.value()) : "null";
                    break;
                case TermKind::node:
                    s += "[";
                    go(x.left(), s);
                    s += ",";
                    go(x.right(), s);
                    s += ",-2]";          // 末项在序列化端保留
                    break;
                default:
                    s += "null";
                    break;
                }
            }
        };
        if (t.isUndefined()) return "undefined";
        Walk::go(t, out);
        return out;
    }

    // ------------------------------------------------------------
    // 4. 字典序比较（TS 侧 `comp` 闭包的迭代版）
    // ------------------------------------------------------------

    /// 逐元素比较；较短者先耗尽则其更小。NaN 既不 > 也不 <，与 TS 一致地跳过继续比较。
    [[nodiscard]] inline int compareFlat(const std::vector<double>& a,
                                         const std::vector<double>& b) {
        const std::size_t n = std::min(a.size(), b.size());
        for (std::size_t i = 0; i < n; ++i) {
            if (a[i] > b[i]) return 1;
            if (a[i] < b[i]) return -1;
        }
        if (a.size() > b.size()) return 1;
        if (a.size() < b.size()) return -1;
        return 0;
    }

    /// 对应 ton_helpers.ts 的 TON_noraise_compare（不做 raise 提升）。
    /// 放在公共层是因为 smallindex / smallpart 等共享遍历也需要它。
    [[nodiscard]] inline int TON_noraise_compare(const Term& x, const Term& y) {
        return compareFlat(flatten(x), flatten(y));
    }

    [[nodiscard]] inline bool noraiseLt(const Term& x, const Term& y) {
        return TON_noraise_compare(x, y) < 0;
    }
    [[nodiscard]] inline bool noraiseLe(const Term& x, const Term& y) {
        return TON_noraise_compare(x, y) <= 0;
    }
    [[nodiscard]] inline bool noraiseGt(const Term& x, const Term& y) {
        return TON_noraise_compare(x, y) > 0;
    }
    [[nodiscard]] inline bool noraiseGe(const Term& x, const Term& y) {
        return TON_noraise_compare(x, y) >= 0;
    }

    /// 项与「裸扁平序列」的比较。对应 TS 中把 a2（普通数字数组）直接当作比较右操作数。
    /// 注意 TS 走的是 `('' + c).split(',').map(e => +e)`：空数组的 `'' + []` 为 ""，
    /// split 得 [""]，Number("") === 0，故空数组等价于单元素序列 {0}，而非长度 0。
    /// （a2 被 get_n 削空后仍会作为 c 传入 BuiltQ，该分支可达。）
    [[nodiscard]] inline int noraise_compare_flat(const Term& x, const std::vector<double>& flat) {
        if (flat.empty()) return compareFlat(flatten(x), std::vector<double>{0.0});
        return compareFlat(flatten(x), flat);
    }

    // ------------------------------------------------------------
    // 5. 结构与遍历工具（各模块中实现逐字相同的部分）
    // ------------------------------------------------------------

    /// 对应 Copy：深拷贝；undefined 上调用像 TS 一样抛 TypeError。
    [[nodiscard]] inline Term Copy(const Term& x) {
        switch (x.kind()) {
        case TermKind::leaf: return x;
        case TermKind::node: return Term::node(Copy(x.left()), Copy(x.right()));
        default: throw js_type_error("cannot read property of undefined");
        }
    }

    using Index = std::vector<std::size_t>;   ///< 索引路径：0 = 左子，1 = 右子

    /// 对应 extract：按索引路径取子项，空路径返回自身。
    [[nodiscard]] inline Term extract(const Term& term, const Index& index) {
        if (index.empty()) return term;
        Index rest(index.begin() + 1, index.end());
        return extract(term.at(index[0]), rest);
    }

    /// 可写版本，对应 get_n / get_a2 中 `subterm[k] = ...` 的就地改写。
    [[nodiscard]] inline Term& extract_mut(Term& term, const Index& index) {
        if (index.empty()) return term;
        Index rest(index.begin() + 1, index.end());
        return extract_mut(term.atRef(index[0]), rest);
    }

    /// 对应 subterm_index：所有子项的索引路径，前序（含空路径表示自身）。
    [[nodiscard]] inline std::vector<Index> subterm_index(const Term& a) {
        std::vector<Index> result;
        struct Walk {
            static void go(const Term& x, Index begin, std::vector<Index>& out) {
                out.push_back(begin);
                if (!x.isNode()) return;   // 叶子 / undefined 不再下潜
                Index l = begin; l.push_back(0);
                go(x.left(), std::move(l), out);
                Index r = begin; r.push_back(1);
                go(x.right(), std::move(r), out);
            }
        };
        Walk::go(a, Index{}, result);
        return result;
    }

    /// 对应 smallindex：值「小于 0」的子项所在索引路径；遇叶子 0 停止下潜。
    [[nodiscard]] inline std::vector<Index> smallindex(const Term& a) {
        if (jsExact(a, 0)) return {};
        std::vector<Index> result;
        struct Walk {
            static void go(const Term& x, Index begin, std::vector<Index>& out) {
                if (jsExact(x, 0)) return;
                if (noraiseLt(x, L(0))) {
                    out.push_back(begin);
                } else {
                    Index l = begin; l.push_back(0);
                    go(x.at(0), std::move(l), out);
                    Index r = begin; r.push_back(1);
                    go(x.at(1), std::move(r), out);
                }
            }
        };
        Walk::go(a, Index{}, result);
        return result;
    }

    /// 对应 smallpart：值「小于 0」的子项集合（保留引用语义）；遇叶子 0 停止下潜。
    [[nodiscard]] inline std::vector<Term> smallpart(const Term& term) {
        std::vector<Term> result;
        struct Walk {
            static void go(const Term& x, std::vector<Term>& out) {
                if (jsExact(x, 0)) return;
                if (noraiseLt(x, L(0))) {
                    result_push(out, x);
                } else {
                    go(x.at(0), out);
                    go(x.at(1), out);
                }
            }
            static void result_push(std::vector<Term>& out, const Term& x) { out.push_back(x); }
        };
        Walk::go(term, result);
        return result;
    }

    // ------------------------------------------------------------
    // 6. 系统数、标记项、回退与提升
    // ------------------------------------------------------------

    /// 对应 `typeof term === 'number' ? term : Math.max(0, ...('' + term).split(',').map(Number))`。
    [[nodiscard]] inline double sysOf(const Term& term) {
        return term.isLeaf() ? term.value() : jsMax0(flatten(term));
    }

    /// TON_main 的 mark。
    [[nodiscard]] inline Term mark_main(double sys) {
        Term res = N(N(L(-1), L(sys)), L(sys));
        for (double i = sys - 1; i > 0; i -= 1) res = N(L(-1), res);
        return res;
    }

    /// TON_MC / TON_MPC 的 mark（两模块实现逐字相同）。
    [[nodiscard]] inline Term mark_reflection(double sys) {
        Term res = L(sys);
        for (double i = sys; i > 0; i -= 1) res = N(N(L(-1), L(sys)), res);
        for (double i = sys - 1; i > 0; i -= 1) res = N(L(-1), res);
        return res;
    }

    /// mark_FS：三模块实现逐字相同。
    [[nodiscard]] inline Term mark_FS(double sys, std::size_t n) {
        Term res = L(sys - 1);
        for (std::size_t i = 0; i < n; ++i) res = N(L(sys - 1), res);
        for (double i = sys - 1; i > 0; i -= 1) res = N(L(-1), res);
        return res;
    }

    /// 对应 regress：形如 [-1, k, -2]（k > 0）的节点折叠为叶子 k-1。
    [[nodiscard]] inline Term regress(const Term& x) {
        if (!x.isNode()) return x;
        if (jsExact(x.left(), -1) && jsGt0(x.right())) return L(x.right().value() - 1);
        return N(regress(x.left()), regress(x.right()));
    }

    /// 对应 regress_repeated：反复 regress 直到扁平键不再变化。
    [[nodiscard]] inline Term regress_repeated(const Term& x) {
        Term cur = Copy(x);
        while (true) {
            Term next = regress(cur);
            if (keyOf(next) == keyOf(cur)) return next;
            cur = std::move(next);
        }
    }

    // ------------------------------------------------------------
    // 7. n-built 判定共享工具（TON_I 与 TON_IBP 的实现逐字相同）
    // ------------------------------------------------------------
    // JS 端 `get_n(term, index)` 会先算出公共前缀 a2，再就地弹出 a2 求 n；
    // TON_IBP 把这两步拆成 get_a2 / get_n，并把**同一个 a2 数组**既作为参数 c
    // 传给 BuiltQ、又在实参求值时被 get_n 就地修改。因此 c 观察到的是弹出后的
    // 序列。C++ 侧按同一顺序串行执行（先 get_n 再调用 BuiltQ），保持别名可见性一致。

    /// 对应 `a2[a2.length - 1]`；空数组时为 undefined，用 nullopt 表示。
    [[nodiscard]] inline std::optional<double> lastOf(const std::vector<double>& v) {
        return v.empty() ? std::optional<double>{} : std::optional<double>{v.back()};
    }

    /// 对应 `a2[a2.length - 2]`；长度不足 2 时为 undefined。
    [[nodiscard]] inline std::optional<double> secondLastOf(const std::vector<double>& v) {
        return v.size() < 2 ? std::optional<double>{} : std::optional<double>{v[v.size() - 2]};
    }

    /// 对应 TON_IBP 的 get_a2（亦即 TON_I 的 get_n 前半段）。
    /// 返回原项与「沿 index 归约后的项」展开序列的公共前缀。
    [[nodiscard]] inline std::vector<double> get_a2(const Term& term, const Index& index) {
        Term a = Copy(term);
        Index a1index = index;

        for (std::size_t i = 0; i < a1index.size();) {
            if (a1index[i] == 0) {
                if (i == 0) {
                    a = a.at(0);                       // a = a[0]（叶子时为 undefined）
                } else {
                    const Index pre(a1index.begin(), a1index.begin() + i - 1);
                    Term& subterm = extract_mut(a, pre);
                    const std::size_t k = a1index[i - 1];
                    subterm.atRef(k) = subterm.at(k).at(0);   // subterm[k] = subterm[k][0]
                }
                a1index.erase(a1index.begin() + i);    // splice(i, 1)，不递增 i
            } else {
                ++i;
            }
        }

        if (a1index.empty()) {
            a = L(0);
        } else {
            const Index pre(a1index.begin(), a1index.end() - 1);
            Term& subterm = extract_mut(a, pre);
            subterm.atRef(a1index.back()) = L(0);
        }

        /// 对应 scan：把「右端过大」的左子收缩一层。
        struct Scan {
            static void go(Term& x) {
                if (x.isLeaf()) return;                // typeof x === 'number'
                const Term c0 = x.at(0);               // x 为 undefined 时抛 TypeError
                if (c0.isLeaf()) return;               // typeof x[0] === 'number'
                if (noraiseGt(x.at(1), c0.at(1))) x.atRef(0) = c0.at(0);  // c0 为 undefined 时抛 TypeError
                go(x.atRef(0));
                go(x.atRef(1));
            }
        };
        Scan::go(a);

        const Term alim = a;
        const std::vector<double> str1 = flatten(Copy(term));
        const std::vector<double> str2 = flatten(alim);

        std::vector<double> a2;
        for (std::size_t i = 0; i < str1.size() && i < str2.size() && str1[i] == str2[i]; ++i) {
            a2.push_back(str1[i]);                     // 对应 shift 双序列、逐个同值入栈
        }
        return a2;
    }

    /// 对应 TON_IBP 的 get_n：**就地**削减 a2 并返回层数 n。
    [[nodiscard]] inline std::size_t get_n(std::vector<double>& a2) {
        std::size_t n = 0;
        while (lastOf(a2) == -2.0) a2.pop_back();
        if (lastOf(a2) == -1.0) {
            ++n;
            a2.pop_back();
        } else {
            return n;                                  // 空数组时 lastOf 为 nullopt，与 undefined !== -1 一致
        }
        while (lastOf(a2) == -2.0 && secondLastOf(a2) == -1.0) {
            ++n;
            a2.erase(a2.end() - 2, a2.end());          // 对应 splice(a2.length - 2, 2)
        }
        return n;
    }

    /// 对应 TON_I 的 get_n(term, index)：算前缀后只求层数（a2 不外露，弹出不影响可观测结果）。
    [[nodiscard]] inline std::size_t get_n(const Term& term, const Index& index) {
        std::vector<double> a2 = get_a2(term, index);
        return get_n(a2);
    }

    // ------------------------------------------------------------
    // 8. 生成器骨架（TS 的 TON_gen function*）
    // ------------------------------------------------------------
    // 9 个模块的 TON_gen 主体逐字相同，只有三处随模块变化：
    //   * 填入的系统数：main/MC/MPC 用 sys，其余 6 个模块用字面量 0；
    //   * 标准性谓词 StandardQ 的签名与实现；
    //   * 产出形态：noraise 系列产出 Copy(beta)，main/MC/MPC 产出 regress_repeated(beta)。
    // TS 用 function* + yield；此处以显式可续跑状态机等价实现：主体只有唯一悬挂点
    // `n = yield ...`，故只需区分「首次进入」与「从 yield 之后恢复」。

    class TonGeneratorBody {
    public:
        explicit TonGeneratorBody(const Term& term)
            : beta_(Copy(term)), len_(flattenCount(term)) {}
        virtual ~TonGeneratorBody() = default;

        TonGeneratorBody(const TonGeneratorBody&) = delete;
        TonGeneratorBody& operator=(const TonGeneratorBody&) = delete;

        /// 对应 gen.next(value)。首次调用忽略 value（TS 中传 undefined，
        /// 但框架首次调用后总以整数恢复）；恢复调用先把 value 赋给内部 n 并置 flag = false。
        ///
        /// 【JS 生成器的关闭语义】一旦主体抛出异常，JS 生成器即永久关闭，
        /// 其后每次 next() 都返回 { value: undefined, done: true } 而不再执行主体。
        /// 本实现在首次抛出时记录该状态，之后的调用一律返回 undefined。
        Term next(std::optional<double> sent = std::nullopt) {
            if (closed_) return Term::jsUndefined();
            try {
                return step(sent);
            } catch (...) {
                closed_ = true;              // 对应 JS: 生成器抛出后进入 completed
                throw;                       // 本次调用仍向上传播异常
            }
        }

    protected:
        /// 填入的系统数（TS 中的 sys 或字面量 0）。
        [[nodiscard]] virtual Term systemValue() const = 0;
        /// 模块自身的标准性谓词。
        [[nodiscard]] virtual bool StandardQ(const Term& beta) = 0;
        /// 产出形态，默认 Copy(beta)。
        [[nodiscard]] virtual Term yieldValue(const Term& beta) const { return Copy(beta); }

    private:
        /// 生成器主体的单次推进（原 next 的实现）。
        Term step(std::optional<double> sent) {
            if (started_) {
                n_ = sent.value_or(std::numeric_limits<double>::quiet_NaN());
                flag_ = false;               // `n = yield ...` 恢复后的下一条语句
            }
            started_ = true;

            while (true) {
                if (flag_ && advanceFlagBranch()) continue;   // continue mainloop
                flag_ = true;

                // 上限在 double 域比较：TS 的 len + n * 2 在 n 为 NaN 时使条件恒假。
                const double limit = static_cast<double>(len_) + n_ * 2.0;
                bool restart_mainloop = false;
                while (static_cast<double>(flattenCount(beta_)) < limit) {
                    if (!StandardQ(beta_)) { restart_mainloop = true; break; }  // continue mainloop
                    growOnce();
                }
                if (restart_mainloop) continue;

                if (StandardQ(beta_)) {
                    Term out = yieldValue(beta_);
                    return out;              // 对应 yield；下次 next() 走恢复分支
                }
            }
        }

        /// 对应 `if (flag) { ... }`；返回 true 表示 TS 里执行了 continue mainloop。
        bool advanceFlagBranch() {
            const Term sys = systemValue();
            if (beta_.isLeaf() && beta_.value() >= 0) {
                beta_ = L(-1);
            } else if (jsExact(beta_.at(1), -1)) {
                beta_ = beta_.at(0);
                return true;                 // continue mainloop
            } else if (jsGte0(beta_.at(1))) {
                beta_.atRef(1) = L(-1);
            } else if (jsExact(beta_.at(1).at(1), -1)) {
                beta_ = N(N(beta_.at(0), beta_.at(1).at(0)), sys);
            } else if (jsGte0(beta_.at(1).at(1))) {
                beta_.atRef(1).atRef(1) = L(-1);
            } else {
                Term c3 = beta_;
                Term c1 = beta_.at(1).at(1);
                while (!c1.at(1).isLeaf()) {
                    c3 = c3.at(1);
                    c1 = c1.at(1);
                }
                if (jsExact(c1.at(1), -1)) {
                    c3.atRef(1) = N(N(c3.at(1).at(0), c1.at(0)), sys);
                } else {
                    c1.atRef(1) = L(-1);
                }
            }
            return false;
        }

        /// 对应内层 while 的单次「补一项」。
        void growOnce() {
            const Term sys = systemValue();
            if (beta_.isNode()) {
                Term c1 = beta_;
                while (!c1.at(1).isLeaf()) c1 = c1.at(1);
                c1.atRef(1) = N(c1.at(1), sys);
            } else {
                beta_ = N(beta_, sys);
            }
        }

        Term beta_;
        std::size_t len_ = 0;
        double n_ = 0;
        bool flag_ = true;
        bool started_ = false;
        bool closed_ = false;      ///< 生成器因异常永久关闭（对应 JS 的 completed 状态）
    };

    // ------------------------------------------------------------
    // 9. 生成器骨架的两类公共派生（决定 TS 中填入的系统数）
    // ------------------------------------------------------------

    /// 对应 6 个 noraise 模块的 `TON_gen(term)`：恒填入字面量 0。
    class ZeroSystemGenerator : public TonGeneratorBody {
    public:
        explicit ZeroSystemGenerator(const Term& term) : TonGeneratorBody(term) {}

    protected:
        [[nodiscard]] Term systemValue() const override { return L(0); }
    };

    /// 对应 TON_main / TON_MC / TON_MPC 的 `TON_gen(term, sys)`：
    /// sys 由调用点（FS）算好后传入，与 TS 一致，不在生成器内部重算。
    class SysSystemGenerator : public TonGeneratorBody {
    public:
        SysSystemGenerator(const Term& term, double sys)
            : TonGeneratorBody(term), sys_(L(sys)), sys_value_(sys) {}

    protected:
        [[nodiscard]] Term systemValue() const override { return sys_; }
        [[nodiscard]] double sys() const noexcept { return sys_value_; }

    private:
        Term sys_;
        double sys_value_ = 0.0;
    };

    // ------------------------------------------------------------
    // 10. 序列缓存（对应各模块的 `var data = {}` 与挂在条目上的 gen）
    // ------------------------------------------------------------

    class SequenceCache {
    public:
        using Factory = std::function<std::shared_ptr<TonGeneratorBody>(const Term&)>;

        /// 对应 TS 中 FS 的缓存主体。
        ///
        /// 【登记时机】TS 的语句顺序是
        ///     dataterm = data[datakey] = [];   // ① 条目先入表
        ///     dataterm.gen = TON_gen(term);    // ② 挂生成器
        ///     dataterm[0] = dataterm.gen.next().value;   // ③ 首次推进，可能抛出
        /// 若 ③ 抛出，条目仍留在表中（且其 gen 已永久关闭），后续同一 term 的
        /// 调用会跳过 ①②③，直接读到 dataterm[n] === undefined 并返回 undefined。
        /// 此处按同一顺序实现，异常只在首次调用时向上传播。
        Term fs(const Term& term, std::size_t n, const Factory& factory) {
            const std::string datakey = keyOf(term);
            auto it = map_.find(datakey);
            if (it == map_.end()) {
                Entry entry;
                entry.gen = factory(term);
                entry.memo.resize(1);
                it = map_.emplace(datakey, std::move(entry)).first;
                Entry& fresh = it->second;
                fresh.memo[0] = fresh.gen->next();  // ③ 可能抛出；条目已登记
            }
            Entry& dataterm = it->second;
            if (n < dataterm.memo.size() && !dataterm.memo[n].isUndefined()) {
                return dataterm.memo[n];                 // dataterm[n] !== undefined
            }
            if (n >= dataterm.memo.size()) dataterm.memo.resize(n + 1);
            Term value = dataterm.gen->next(static_cast<double>(n));
            dataterm.memo[n] = value;
            return value;
        }

    private:
        struct Entry {
            std::shared_ptr<TonGeneratorBody> gen;
            std::vector<Term> memo;
        };
        std::map<std::string, Entry> map_;
    };

    /// 标准性缓存（对应 IStd / StdTrue / MCStd / MPCStd / IBPStd / DRStd / DRPStd /
    /// DRCStd / DRPCStd）。各模块一致地只缓存「已判定为标准」的项。
    class StandardCache {
    public:
        [[nodiscard]] bool contains(const Term& a) const {
            return set_.find(jsonKey(a)) != set_.end();
        }
        void insert(const Term& a) { set_.insert(jsonKey(a)); }

    private:
        std::unordered_set<std::string> set_;
    };

    // ------------------------------------------------------------
    // 11. 扁平序列 ↔ Term（与工程 expand(std::vector<int>, int) 接口对接）
    // ------------------------------------------------------------

    /// 把带终止符 -2 的扁平序列还原为 Term。序列形如 TS 的 `('' + term).split(',')`。
    /// [ASSUMPTION] TS 库对外不暴露扁平入口，本函数为使头文件可被工程的序列接口调用而补；
    /// 序列非法（-2 处栈深不足、结束时栈不为 1）时抛 ton_error，不回退为静默错误结果。
    [[nodiscard]] inline Term unflatten(const std::vector<int>& seq) {
        std::vector<Term> stack;
        for (int v : seq) {
            if (v == -2) {
                if (stack.size() < 2) throw ton_error("unflatten: dangling terminator -2");
                Term right = std::move(stack.back()); stack.pop_back();
                Term left = std::move(stack.back()); stack.pop_back();
                stack.push_back(Term::node(left, right));
            } else {
                stack.push_back(Term::leaf(static_cast<double>(v)));
            }
        }
        if (stack.size() != 1) throw ton_error("unflatten: malformed flat sequence");
        return std::move(stack.front());
    }

    /// 整数版 flatten，供序列接口回传。
    /// [ASSUMPTION] 工程侧序列元素为 int，无法表示 TS 用作「展开起点」占位的 Infinity，
    /// 故含非有限叶子的项不可经此路径回传，遇到即抛 ton_error 而非截断成脏值。
    [[nodiscard]] inline std::vector<int> flatten_to_ints(const Term& t) {
        std::vector<int> out;
        for (const double d : flatten(t)) {
            if (!std::isfinite(d)) throw ton_error("flatten_to_ints: non-finite leaf cannot be represented as int");
            out.push_back(static_cast<int>(d));
        }
        return out;
    }

    // ------------------------------------------------------------
    // 12. NotationDefinition —— 对应 TS 的 @/notation-definition.ts
    // ------------------------------------------------------------
    // [ASSUMPTION] 原 TS 接口文件未随包提供，此处按 10 个模块实际使用到的字段还原：
    // id / name / simple_name / category_id / display / is_limit / compare / FS /
    // credit_text_id / init，并新增「定义文本」槽位（换行以 \n 占位）。

    class NotationDefinition {
    public:
        virtual ~NotationDefinition() = default;

        [[nodiscard]] virtual std::string_view id() const = 0;              ///< TS id
        [[nodiscard]] virtual std::string_view name() const = 0;            ///< TS name
        [[nodiscard]] virtual std::string_view simple_name() const = 0;     ///< TS simple_name
        [[nodiscard]] virtual std::string_view category_id() const = 0;     ///< TS category_id
        [[nodiscard]] virtual std::string_view credit_text_id() const = 0;  ///< TS credit_text_id
        [[nodiscard]] virtual std::string_view definition_text() const = 0; ///< 定义文本，换行用 \n 占位

        [[nodiscard]] virtual std::string display(const Term& term) const = 0;   ///< TS display
        [[nodiscard]] virtual bool is_limit(const Term& term) const = 0;         ///< TS is_limit
        [[nodiscard]] virtual int compare(const Term& x, const Term& y) const = 0; ///< TS compare
        [[nodiscard]] virtual Term FS(const Term& term, std::size_t n) const = 0;  ///< TS FS
        [[nodiscard]] virtual std::vector<Term> init() const = 0;                 ///< TS init()
    };

    /// C++20 concept：约束「可直接当记号使用」的 struct 形态，与工程既有
    /// XxxNotation（kName / expand / suffix）约定一致。
    template <class T>
    concept NotationLike = requires(const Term& t, std::size_t n) {
        { T::kName } -> std::convertible_to<std::string_view>;
        { T::suffix() } -> std::convertible_to<std::string>;
        { T::notation().FS(t, n) } -> std::same_as<Term>;
        { T::notation().display(t) } -> std::convertible_to<std::string>;
        { T::notation().compare(t, t) } -> std::same_as<int>;
        { T::notation().is_limit(t) } -> std::same_as<bool>;
        { T::notation().init() } -> std::same_as<std::vector<Term>>;
    };

    // ------------------------------------------------------------
    // 13. TonNotationCore —— 9 个模块共同的 FS 流程骨架
    // ------------------------------------------------------------
    // TS 中每个模块的 FS 都是同一形状：
    //   ① 对 term 做前置处理（改写，或直接给出结果并返回）；
    //   ② 以 keyOf(term) 为键查模块级 data；缺项则建条目、预取下标 0；
    //   ③ 命中 dataterm[n] 直接返回，否则 gen.next(n) 推进并记录。
    // ②③ 已由 SequenceCache 实现，此处只把 ① 抽成一个钩子，使 9 个模块各自
    // 只需描述自己特有的那部分。
    //
    // [ASSUMPTION] 各模块的 data / Std 缓存在 TS 中是模块级全局对象，进程内共享
    // 且跨调用累积；C++ 侧对应「每个记号一个进程级实例」（见各模块的 notation()）。
    // 与 TS 同为单线程语义，未加锁；多线程共享同一记号需外部同步。

    struct FSPrepareResult {
        bool direct = false;   ///< true：value 即最终结果，不进入缓存与生成器
        Term value;            ///< direct 时为结果；否则为改写后继续处理的项
    };

    class TonNotationCore : public NotationDefinition {
    public:
        [[nodiscard]] Term FS(const Term& term, std::size_t n) const override {
            const FSPrepareResult prepared = prepareForFS(term, n);
            if (prepared.direct) return prepared.value;
            return cache_.fs(prepared.value, n,
                             [this](const Term& t) { return makeGenerator(t); });
        }

        /// 供 StandardQ 使用的模块级标准性缓存（对应 TS 的 IStd / StdTrue / …）。
        [[nodiscard]] StandardCache& standardCache() const noexcept { return std_; }

        /// 对应 credit_text_id 的实际文案槽位；TS 侧文案在本地化文件中，未随包提供。
        /// [ASSUMPTION] 原包 Docs 目录为空，故此处返回与 TS 一致的键名本身。
        [[nodiscard]] std::string_view credit_text_id() const override { return "credit.ton"; }

    protected:
        TonNotationCore() = default;

        /// 钩子 ①：默认不做前置处理，直接进入缓存与生成器。
        [[nodiscard]] virtual FSPrepareResult prepareForFS(const Term& term, std::size_t) const {
            return FSPrepareResult{false, term};
        }

        /// 派生类据此构造自己的 TON_gen 实例（对应 TS 的 TON_gen(term) / TON_gen(term, sys)）。
        [[nodiscard]] virtual std::shared_ptr<TonGeneratorBody> makeGenerator(const Term& term) const = 0;

    private:
        mutable SequenceCache cache_;
        mutable StandardCache std_;
    };

    // ------------------------------------------------------------
    // 14. noraise 家族共同形态的承载位置说明
    // ------------------------------------------------------------
    // NoRaiseNotationBase（6 个不提升记号共用的 display / is_limit / compare 与
    // Infinity 替换前置）依赖 ton_helpers.ts 的导出，故定义在 ton_helpers.hpp 中，
    // 以免公共层反向依赖辅助层。以下 Infinity 替换项与 init 表只用到 Term，留在公共层。

    /// Infinity 替换项：[-1, 0, -2]（DoR / DRP / DRC / DRPC 四者一致）。
    [[nodiscard]] inline Term infinity_pair_neg1_0() { return N(L(-1), L(0)); }

    /// Infinity 替换项：[-1, [[0, [0,-1,-2],-2], 0, -2], -2]（TON_I / TON_IBP 的 FS 前置）。
    /// 注意与 init_iteration()[1] 不同：后者少一层，是 [-1, [0,[0,-1,-2],-2], -2]。
    [[nodiscard]] inline Term infinity_pair_iteration() {
        return N(L(-1), N(N(L(0), N(L(0), L(-1))), L(0)));
    }

    /// init[1]：[-1, [0, [0,-1,-2], -2], -2]（展开为 -1,0,0,-1,-2,-2,-2，共 7 项）。
    [[nodiscard]] inline Term init_iteration_second() {
        return N(L(-1), N(L(0), N(L(0), L(-1))));
    }

    /// init: [Infinity, -1]（DoR / DRP / DRC / DRPC 四者一致）。
    [[nodiscard]] inline std::vector<Term> init_pair_neg1() {
        return {L(std::numeric_limits<double>::infinity()), L(-1)};
    }

    /// init: [Infinity, [-1,[0,[0,-1,-2],-2],-2], [-1,[0,0,-2],-2], [-1,0,-2], -1]
    ///（TON_I / TON_IBP 一致）。
    [[nodiscard]] inline std::vector<Term> init_iteration() {
        return {
            L(std::numeric_limits<double>::infinity()),
            init_iteration_second(),
            N(L(-1), N(L(0), L(0))),
            N(L(-1), L(0)),
            L(-1),
        };
    }

} // namespace omegay::notation::ton

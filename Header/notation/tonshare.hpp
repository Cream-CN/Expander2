
#pragma once

#include <cmath>
#include <cstdint>
#include <deque>
#include <functional>
#include <limits>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace omegay::notation::tonshare {

using TonValue = double;
constexpr TonValue kInf = std::numeric_limits<TonValue>::infinity();

// ---------------------------------------------------------------------
// 术语表示与分配
// ---------------------------------------------------------------------
// TS 用 JS 数组 [l, r, -2]（第三分量恒为 -2）与裸数字；这里以显式树表示。
// 节点右孩子与左孩子均为术语（不会是 -2）；-2 隐含在节点自身，不占存储。
struct TonNode {
    TonValue v = 0;      // 叶子数值（或节点时未使用）
    TonNode* l = nullptr;
    TonNode* r = nullptr;
    bool is_node = false;
};

class TonArena {
public:
    [[nodiscard]] TonNode* leaf(TonValue v) {
        nodes_.emplace_back();
        TonNode* p = &nodes_.back();
        p->v = v;
        return p;
    }
    // l/r 可为 const 指针（如共享叶子/其他 arena 的节点）；所有权在发出方，
    // 这里仅存储。所有被存入的节点要么属于本 arena、要么是永不修改的共享叶子。
    [[nodiscard]] TonNode* node(const TonNode* l, const TonNode* r) {
        nodes_.emplace_back();
        TonNode* p = &nodes_.back();
        p->l = const_cast<TonNode*>(l);
        p->r = const_cast<TonNode*>(r);
        p->is_node = true;
        return p;
    }

private:
    // deque：元素地址在扩容后保持稳定，已发出的指针不会失效。
    std::deque<TonNode> nodes_;
};

[[nodiscard]] inline bool is_leaf(const TonNode* t) { return t == nullptr || !t->is_node; }
[[nodiscard]] inline bool is_inf(const TonNode* t) { return is_leaf(t) && t->v == kInf; }
// 共享只读叶子：0 与 -1（TS 以裸数字出现；此处用指针表示，永不修改）。
[[nodiscard]] inline const TonNode* zero_leaf() {
    static TonNode z{0, nullptr, nullptr, false};
    return &z;
}
[[nodiscard]] inline const TonNode* neg_one_leaf() {
    static TonNode n{-1, nullptr, nullptr, false};
    return &n;
}

// ---------------------------------------------------------------------
// 扁平化（对齐 JSON.stringify(...).split(',') 的前序序列）
// ---------------------------------------------------------------------

inline void flatten(const TonNode* t, std::vector<TonValue>& out) {
    if (t == nullptr) { out.push_back(0); return; }
    if (!t->is_node) { out.push_back(t->v); return; }
    flatten(t->l, out);
    flatten(t->r, out);
    out.push_back(-2);
}

[[nodiscard]] inline std::vector<TonValue> flatten_vec(const TonNode* t) {
    std::vector<TonValue> out;
    flatten(t, out);
    return out;
}

[[nodiscard]] inline std::size_t flatten_size(const TonNode* t) { return flatten_vec(t).size(); }

[[nodiscard]] inline std::string fmt_val(TonValue v) {
    if (v == kInf) return "Infinity";
    if (std::isfinite(v) && v == std::floor(v)) return std::to_string(static_cast<long long>(v));
    return std::to_string(v);
}

// 用作缓存键的扁平文本（如 "0,-1,-2" / "Infinity"）。
[[nodiscard]] inline std::string flatten_key(const TonNode* t) {
    const std::vector<TonValue> f = flatten_vec(t);
    std::string out;
    for (std::size_t i = 0; i < f.size(); ++i) {
        if (i) out += ',';
        out += fmt_val(f[i]);
    }
    return out;
}

// Math.max(0, ...flatten)：记号系统的“系统高度”（raise 家族用）。
[[nodiscard]] inline TonValue max_value(const TonNode* t) {
    const std::vector<TonValue> f = flatten_vec(t);
    TonValue m = 0;
    for (TonValue v : f)
        if (v > m) m = v;
    return m;
}

// ---------------------------------------------------------------------
// 结构等价 / 深拷贝 / 尺寸
// ---------------------------------------------------------------------

[[nodiscard]] inline bool structural_equal(const TonNode* a, const TonNode* b) {
    return flatten_key(a) == flatten_key(b);
}

[[nodiscard]] inline TonNode* copy(TonArena& arena, const TonNode* t) {
    if (t == nullptr) return arena.leaf(0);
    if (!t->is_node) return arena.leaf(t->v);
    return arena.node(copy(arena, t->l), copy(arena, t->r));
}

// ---------------------------------------------------------------------
// 比较
// ---------------------------------------------------------------------

[[nodiscard]] inline int lex_compare(const std::vector<TonValue>& a, const std::vector<TonValue>& b) {
    std::size_t i = 0;
    while (i < a.size() && i < b.size()) {
        if (a[i] > b[i]) return 1;
        if (a[i] < b[i]) return -1;
        ++i;
    }
    return a.size() > b.size() ? 1 : (a.size() < b.size() ? -1 : 0);
}

// TS: TON_noraise_compare —— 扁平序列字典序。
[[nodiscard]] inline int compare_noraise(const TonNode* x, const TonNode* y) {
    return lex_compare(flatten_vec(x), flatten_vec(y));
}

// TS: raise —— 把 < sys 的非负叶子逐层提升。
[[nodiscard]] inline TonNode* raise(TonArena& arena, const TonNode* t, TonValue sys) {
    if (t == nullptr) return arena.leaf(0);
    if (!t->is_node) {
        const TonValue v = t->v;
        if (v >= 0 && v < sys) return arena.node(arena.leaf(-1), raise(arena, arena.leaf(v + 1), sys));
        return arena.leaf(v);
    }
    return arena.node(raise(arena, t->l, sys), raise(arena, t->r, sys));
}

// TS: TON_compare —— 先按双方系统高度 raise，再字典序。
[[nodiscard]] inline int compare_raise(const TonNode* x, const TonNode* y) {
    std::vector<TonValue> fx = flatten_vec(x);
    std::vector<TonValue> fy = flatten_vec(y);
    TonValue sysx = 0, sysy = 0;
    for (TonValue v : fx)
        if (v > sysx) sysx = v;
    for (TonValue v : fy)
        if (v > sysy) sysy = v;
    if (sysx < kInf && sysy < kInf && (sysx > 0 || sysy > 0)) {
        TonArena arena;
        const TonValue s = sysx > sysy ? sysx : sysy;
        x = raise(arena, x, s);
        y = raise(arena, y, s);
        fx = flatten_vec(x);
        fy = flatten_vec(y);
    }
    return lex_compare(fx, fy);
}

// TS: r —— 反射构型辅助函数；结果只参与比较，不存入术语。
[[nodiscard]] inline const TonNode* r(TonArena& arena, const TonNode* a, const TonNode* b) {
    if (a == nullptr) return zero_leaf();
    if (!a->is_node) return a;
    const int c = compare_raise(a, b);
    if (c > 0) {
        if (compare_raise(a->l, b) > 0) return arena.node(r(arena, a->l, b), r(arena, a->r, b));
        return arena.node(arena.leaf(-0.5), r(arena, a->r, b));
    }
    return c < 0 ? a : arena.leaf(-0.5);
}

// ---------------------------------------------------------------------
// 极限判定（TS: TON_limit）
// ---------------------------------------------------------------------

[[nodiscard]] inline bool is_limit(const TonNode* t) {
    if (t == nullptr) return false;
    if (!t->is_node) return t->v >= 0;
    if (t->r == nullptr || !t->r->is_node) return t->r != nullptr && t->r->v >= 0;
    return true;
}

// ---------------------------------------------------------------------
// 显示文本
// ---------------------------------------------------------------------

[[nodiscard]] inline std::string display_noraise(const TonNode* t) {
    if (t == nullptr) return "0";
    if (!t->is_node) {
        if (t->v == kInf) return "Limit";
        return t->v < 0 ? "0" : "Ω";
    }
    return display_noraise(t->l) + display_noraise(t->r) + "C";
}

[[nodiscard]] inline std::string display_main(const TonNode* t) {
    if (t == nullptr) return "0";
    if (!t->is_node) {
        if (t->v == kInf) return "Limit";
        if (t->v < 0) return "0";
        // 用 Ω<n>（角括号定界）消除“Ω0”+“0”叶子与多位数合并的歧义。
        return "Ω<" + fmt_val(t->v) + ">";
    }
    return display_main(t->l) + display_main(t->r) + "C";
}

// ---------------------------------------------------------------------
// 解析（后缀栈；term term C 文法线性可判）
// ---------------------------------------------------------------------

// main_style=true 时 "Ω" 可带十进制数字（"Ω3"）；false 时 "Ω" 后不得跟数字。
[[nodiscard]] inline const TonNode* parse_impl(TonArena& arena, std::string_view s, bool main_style) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n'))
        s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
        s.remove_suffix(1);
    if (s.empty()) return nullptr;
    std::vector<const TonNode*> stack;
    std::size_t i = 0;
    while (i < s.size()) {
        if (s.compare(i, 5, "Limit") == 0) {
            stack.push_back(arena.leaf(kInf));
            i += 5;
            continue;
        }
        if (s[i] == '0') {
            stack.push_back(arena.leaf(-1));
            ++i;
            continue;
        }
        // Ω (U+03A9, UTF-8 编码 CE A9)：用字节序列比较，避免多字节字符字面量
        // 在不同执行字符集下的实现定义值。
        // main 家族：Ω<数字>（角括号定界，无歧义）；noraise 家族：裸 Ω（不吞数字，
        // 后面的 '0' 会作为下一个叶子记号单独消费，其他数字字符则解析失败）。
        if (i + 2 <= s.size() && static_cast<unsigned char>(s[i]) == 0xCE &&
            static_cast<unsigned char>(s[i + 1]) == 0xA9) {
            i += 2;
            if (main_style) {
                if (i >= s.size() || s[i] != '<') return nullptr;
                ++i;
                long long v = 0;
                bool has_digits = false;
                while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
                    v = v * 10 + (s[i] - '0');
                    has_digits = true;
                    ++i;
                }
                if (!has_digits || i >= s.size() || s[i] != '>') return nullptr;
                ++i;
                stack.push_back(arena.leaf(static_cast<TonValue>(v)));
                continue;
            }
            stack.push_back(arena.leaf(0.0));
            continue;
        }
        if (s[i] == 'C') {
            if (stack.size() < 2) return nullptr;
            const TonNode* b = stack.back();
            stack.pop_back();
            const TonNode* a = stack.back();
            stack.pop_back();
            stack.push_back(arena.node(const_cast<TonNode*>(a), const_cast<TonNode*>(b)));
            ++i;
            continue;
        }
        return nullptr;
    }
    if (stack.size() != 1) return nullptr;
    return stack[0];
}

// ---------------------------------------------------------------------
// regress / regress_repeated（raise 家族 yield 前的化简）
// ---------------------------------------------------------------------

[[nodiscard]] inline TonNode* regress(TonArena& arena, const TonNode* x) {
    if (x == nullptr) return arena.leaf(0);
    if (!x->is_node) return arena.leaf(x->v);
    if (is_leaf(x->l) && x->l->v == -1 && is_leaf(x->r) && x->r->v > 0) return arena.leaf(x->r->v - 1);
    return arena.node(regress(arena, x->l), regress(arena, x->r));
}

[[nodiscard]] inline const TonNode* regress_repeated(TonArena& arena, const TonNode* x) {
    const TonNode* prev = x;
    const TonNode* cur = regress(arena, prev);
    while (!structural_equal(prev, cur)) {
        prev = cur;
        cur = regress(arena, prev);
    }
    return cur;
}

// ---------------------------------------------------------------------
// mark(sys) / mark_FS(sys, n) —— 各 raise 家族的系统极限项与它的基本列。
// 注意：TON 与 TON_MC/TON_MPC 的 mark 构造不同（前者单层循环、后者双层），
// 分别以 mark_main / mark_mc 提供；mark_FS 三者一致。
// ---------------------------------------------------------------------

[[nodiscard]] inline TonNode* mark_main(TonArena& arena, TonValue sys) {
    TonNode* res = arena.node(arena.node(arena.leaf(-1), arena.leaf(sys)), arena.leaf(sys));
    for (TonValue i = sys - 1; i > 0; --i) res = arena.node(arena.leaf(-1), res);
    return res;
}

[[nodiscard]] inline TonNode* mark_mc(TonArena& arena, TonValue sys) {
    TonNode* res = arena.leaf(sys);
    for (TonValue i = sys; i > 0; --i) res = arena.node(arena.node(arena.leaf(-1), arena.leaf(sys)), res);
    for (TonValue i = sys - 1; i > 0; --i) res = arena.node(arena.leaf(-1), res);
    return res;
}

[[nodiscard]] inline TonNode* mark_fs(TonArena& arena, TonValue sys, int n) {
    TonNode* res = arena.leaf(sys - 1);
    for (int i = 0; i < n; ++i) res = arena.node(arena.leaf(sys - 1), res);
    for (TonValue i = sys - 1; i > 0; --i) res = arena.node(arena.leaf(-1), res);
    return res;
}

// ---------------------------------------------------------------------
// 索引机器（TON_I / TON_IBP / TON_MC / TON_MPC 的标准判定需要）
// ---------------------------------------------------------------------
// 路径：vector<int>，0=左孩子，1=右孩子。extract 沿路径取子项。

[[nodiscard]] inline const TonNode* extract(const TonNode* term, const std::vector<int>& index) {
    const TonNode* t = term;
    for (int c : index) {
        if (t == nullptr || !t->is_node) return nullptr;
        t = c == 0 ? t->l : t->r;
    }
    return t;
}

[[nodiscard]] inline TonNode* extract_mut(TonNode* term, const std::vector<int>& index, std::size_t n) {
    TonNode* t = term;
    for (std::size_t k = 0; k < n; ++k) {
        if (t == nullptr || !t->is_node) return nullptr;
        t = index[k] == 0 ? t->l : t->r;
    }
    return t;
}

// TS: subterm_index —— 所有前缀路径（含根与叶子）。
inline void collect_subterm_index(const TonNode* a, std::vector<int> begin, std::vector<std::vector<int>>& out) {
    out.push_back(begin);
    if (a == nullptr || !a->is_node) return;
    std::vector<int> b0 = begin;
    b0.push_back(0);
    std::vector<int> b1 = begin;
    b1.push_back(1);
    collect_subterm_index(a->l, b0, out);
    collect_subterm_index(a->r, b1, out);
}

// TS: smallindex —— 从根递归：若当前子项的扁平序列 < 0 则记录其路径并停止向下，
// 否则进入左右子项。叶子 0 与 >=1 不产出（>=1 在 TS 中会崩溃，此处防御性返回）。
inline void collect_smallindex(const TonNode* a, std::vector<int> begin, std::vector<std::vector<int>>& out) {
    if (a == nullptr) return;
    if (!a->is_node) {
        if (a->v == 0) return;
        if (a->v < 0) out.push_back(begin);
        return;
    }
    if (compare_noraise(a, zero_leaf()) < 0) {
        out.push_back(begin);
        return;
    }
    std::vector<int> b0 = begin;
    b0.push_back(0);
    std::vector<int> b1 = begin;
    b1.push_back(1);
    collect_smallindex(a->l, b0, out);
    collect_smallindex(a->r, b1, out);
}

// TS: smallpart —— 扁平序列 < 0 的子项本身（DoR 家族用）；语义同上。
inline void collect_smallpart(const TonNode* a, std::vector<const TonNode*>& out) {
    if (a == nullptr) return;
    if (!a->is_node) {
        if (a->v == 0) return;
        if (a->v < 0) out.push_back(a);
        return;
    }
    if (compare_noraise(a, zero_leaf()) < 0) {
        out.push_back(a);
        return;
    }
    collect_smallpart(a->l, out);
    collect_smallpart(a->r, out);
}

// TS: scan —— 若 x[1] > x[0][1] 则 x[0] = x[0][0]，随后递归。
// 注：TS 用 JS 数组 > 比较（字符串序）；noraise 家族术语只含 0/-1/-2，
// 字符串序与数值序一致，此处用数值字典序等价。
inline void scan(TonNode* x) {
    if (x == nullptr || !x->is_node) return;
    if (x->l == nullptr || !x->l->is_node) return;
    if (x->r != nullptr && x->l->r != nullptr && compare_noraise(x->r, x->l->r) > 0) x->l = x->l->l;
    scan(x->l);
    scan(x->r);
}

// TS: get_a2 —— 构造可变副本、按 index 替换，返回 a 与 alim 扁平序列的公共前缀。
[[nodiscard]] inline std::vector<TonValue> get_a2(TonArena& arena, const TonNode* term, const std::vector<int>& index) {
    TonNode* a = copy(arena, term);
    std::vector<int> a1index = index;
    for (std::size_t i = 0; i < a1index.size();) {
        if (a1index[i] == 0) {
            if (i == 0) {
                a = a->l;
            } else {
                TonNode* sub = extract_mut(a, a1index, i - 1);
                if (sub == nullptr) return {};
                TonNode* child = a1index[i - 1] == 0 ? sub->l : sub->r;
                if (child == nullptr) return {};
                if (a1index[i - 1] == 0) sub->l = child->l;
                else sub->r = child->l;
            }
            a1index.erase(a1index.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            ++i;
        }
    }
    if (a1index.empty()) {
        a = arena.leaf(0);
    } else {
        TonNode* sub = extract_mut(a, a1index, a1index.size() - 1);
        if (sub == nullptr) return {};
        if (a1index.back() == 0) sub->l = arena.leaf(0);
        else sub->r = arena.leaf(0);
    }
    scan(a);
    std::vector<TonValue> f1 = flatten_vec(term);
    std::vector<TonValue> f2 = flatten_vec(a);
    std::vector<TonValue> a2;
    std::size_t k = 0;
    while (k < f1.size() && k < f2.size() && f1[k] == f2[k]) {
        a2.push_back(f1[k]);
        ++k;
    }
    return a2;
}

// TS: get_n —— 数 a2 尾部 "-1 与 -2 交替" 的组数；**就地缩减 a2**（对齐 TS 传引用弹栈）。
[[nodiscard]] inline int get_n(std::vector<TonValue>& a2) {
    int n = 0;
    while (!a2.empty() && a2.back() == -2) a2.pop_back();
    if (a2.empty()) return n;
    if (a2.back() == -1) {
        ++n;
        a2.pop_back();
    } else {
        return n;
    }
    while (a2.size() >= 2 && a2.back() == -2 && a2[a2.size() - 2] == -1) {
        ++n;
        a2.pop_back();
        a2.pop_back();
    }
    return n;
}

// TON_I 用的一步到位版本：get_a2 + get_n 合并在一个调用里。
[[nodiscard]] inline int get_n(TonArena& arena, const TonNode* term, const std::vector<int>& index) {
    std::vector<TonValue> a2 = get_a2(arena, term, index);
    return get_n(a2);
}

// TON_IBP 用：术语的扁平序列与既有向量比较（TS 的 comp 接收两个数组）。
[[nodiscard]] inline int compare_noraise_vec(const TonNode* t, const std::vector<TonValue>& v) {
    return lex_compare(flatten_vec(t), v);
}

// ---------------------------------------------------------------------
// 基本列(FS)生成器（TS: TON_gen）
// ---------------------------------------------------------------------
// 状态 = (work arena 中的 beta, flag, len)。每次调用前进到下一个 yield 点。
// 注意 branch2（beta[1] === -1 -> beta = beta[0]）要立即重进 step 块，与 TS 的
// continue mainloop 一致；这里用 while(true) 内层循环表达。

using StdFn = std::function<bool(const TonNode*, TonValue sys)>;

struct TonGenState {
    TonArena work;   // beta 及其衍生节点所在 arena（跨 .next() 调用存活）
    const TonNode* beta = nullptr;
    bool flag = true;
    int len = 0;
};

// 应用一次 flag 块；返回 false 表示 beta 是裸负数叶子（TS 崩溃，本移植停住返回原项）。
[[nodiscard]] inline bool ton_step(TonGenState& st, TonValue sys) {
    while (true) {
        const TonNode* beta = st.beta;
        if (!beta->is_node) {
            if (beta->v >= 0) {
                st.beta = st.work.leaf(-1);
                return true;
            }
            return false;   // 裸负数叶子：后继项，TS 在此崩溃
        }
        const TonNode* r1 = beta->r;
        if (is_leaf(r1) && r1->v == -1) {
            st.beta = beta->l;
            continue;       // branch2：重进 step 块
        }
        if (is_leaf(r1) && r1->v >= 0) {
            const_cast<TonNode*>(beta)->r = st.work.leaf(-1);
            return true;
        }
        const TonNode* r2 = r1->r;
        if (is_leaf(r2) && r2->v == -1) {
            st.beta = st.work.node(st.work.node(beta->l, r1->l), st.work.leaf(sys));
            return true;
        }
        if (is_leaf(r2) && r2->v >= 0) {
            const_cast<TonNode*>(r1)->r = st.work.leaf(-1);
            return true;
        }
        // else：沿 c1 = beta[1][1] 的右链下行
        const TonNode* c3 = beta;
        const TonNode* c1 = r2;
        while (c1->r != nullptr && c1->r->is_node) {
            c3 = c3->r;
            c1 = c1->r;
        }
        if (c1->r == nullptr || !(is_leaf(c1->r) && c1->r->v == -1)) {
            const_cast<TonNode*>(c1)->r = st.work.leaf(-1);
        } else {
            const_cast<TonNode*>(c3)->r =
                st.work.node(st.work.node(c3->r->l, c1->l), st.work.leaf(sys));
        }
        return true;
    }
}

// 前进到下一个 yield 点并返回产出值。
//   main_style=true : 产出 regress_repeated(beta)（TS: yield regress_repeated(beta)）
//   main_style=false: 产出 Copy(beta) 到 out_arena（TS: yield Copy(beta)）
[[nodiscard]] inline const TonNode* ton_gen_next(TonGenState& st, TonValue sys, int n,
                                                 const StdFn& std_fn, bool main_style,
                                                 TonArena& out_arena) {
    while (true) {
        if (st.flag) {
            if (!ton_step(st, sys)) {
                st.flag = false;
                return copy(out_arena, st.beta);
            }
        }
        st.flag = true;
        bool nonstandard = false;
        while (flatten_size(st.beta) < static_cast<std::size_t>(st.len + n * 2)) {
            if (!std_fn(st.beta, sys)) {
                nonstandard = true;
                break;
            }
            if (st.beta->is_node) {
                const TonNode* c1 = st.beta;
                while (c1->r != nullptr && c1->r->is_node) c1 = c1->r;
                const TonNode* leafv = c1->r;
                const_cast<TonNode*>(c1)->r = st.work.node(const_cast<TonNode*>(leafv), st.work.leaf(sys));
            } else {
                st.beta = st.work.node(st.beta, st.work.leaf(sys));
            }
        }
        if (nonstandard) continue;   // continue mainloop：flag 保持 true，重进 step
        if (std_fn(st.beta, sys)) {
            st.flag = false;
            if (main_style) return regress_repeated(st.work, st.beta);
            return copy(out_arena, st.beta);
        }
    }
}

// ---------------------------------------------------------------------
// 惰性 FS 缓存（对齐 TS 的 data[datakey]：逐项计算、按需缓存）
// ---------------------------------------------------------------------

struct TonFsCache {
    struct Entry {
        TonArena values;    // 产出值（noraise 家族深拷贝存放；raise 家族在 state.work 中）
        TonGenState state;
        std::vector<const TonNode*> vals;
    };
    std::map<std::string, Entry> entries;

    [[nodiscard]] const TonNode* fs(const TonNode* term, TonValue sys, int n, bool main_style,
                                    const StdFn& std_fn) {
        const std::string key = flatten_key(term);
        Entry& e = entries[key];
        if (e.vals.empty()) {
            e.state.beta = copy(e.state.work, term);
            e.state.flag = true;
            e.state.len = static_cast<int>(flatten_size(term));
            e.vals.push_back(ton_gen_next(e.state, sys, 0, std_fn, main_style, e.values));
        }
        while (static_cast<int>(e.vals.size()) <= n) {
            const int i = static_cast<int>(e.vals.size());
            e.vals.push_back(ton_gen_next(e.state, sys, i, std_fn, main_style, e.values));
        }
        return e.vals[static_cast<std::size_t>(n)];
    }
};

// ---------------------------------------------------------------------
// 记号极限项（noraise 家族：输入 "Limit" 时替换成的展开起点）
// ---------------------------------------------------------------------

// ibp_style=true  -> [-1, [[0, [0, -1, -2], -2], 0, -2], -2]（TON_I / TON_IBP）
// ibp_style=false -> [-1, 0, -2]（DoR 家族）
[[nodiscard]] inline TonNode* make_limit(TonArena& arena, bool ibp_style) {
    if (ibp_style) {
        TonNode* inner2 = arena.node(arena.leaf(0), arena.leaf(-1));                    // [0,-1,-2]
        TonNode* inner1 = arena.node(arena.node(arena.leaf(0), inner2), arena.leaf(0)); // [[0,[0,-1,-2],-2],0,-2]
        return arena.node(arena.leaf(-1), inner1);
    }
    return arena.node(arena.leaf(-1), arena.leaf(0));
}

// ---------------------------------------------------------------------
// 标准判定的真值缓存（对齐 TS StdTrue/DRStd/... 只缓存 true）
// ---------------------------------------------------------------------

template <typename Fn>
[[nodiscard]] inline bool std_memoized(std::map<std::string, bool>& memo, const TonNode* a, Fn&& compute) {
    if (a == nullptr) return false;
    if (!a->is_node) return true;
    const std::string key = flatten_key(a);
    if (memo.find(key) != memo.end()) return true;
    if (compute()) {
        memo[key] = true;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------
// 文本接口骨架（expand_text / expand_text_alter 共用）
// ---------------------------------------------------------------------

// raise 家族（TON / TON_MC / TON_MPC）：
//   sys = max(扁平值)；sys == Infinity -> [n,n,-2] 包 n 层 [-1,...]；
//   否则 raise 后若等于 mark(sys) 走 mark_FS；否则进入惰性生成器。
[[nodiscard]] inline std::string ton_expand_main(TonFsCache& cache, const TonNode* t, int n,
                                                 const StdFn& std_fn,
                                                 TonNode* (*mark_fn)(TonArena&, TonValue)) {
    // TS: sys = typeof term === 'number' ? term : Math.max(0, ...flatten(term))
    const TonValue sys = (!t->is_node) ? t->v : max_value(t);
    if (sys == kInf) {
        TonArena a;
        TonNode* res = a.node(a.leaf(static_cast<TonValue>(n)), a.leaf(static_cast<TonValue>(n)));
        for (int i = 0; i < n; ++i) res = a.node(a.leaf(-1), res);
        return display_main(res);
    }
    TonArena arena;
    const TonNode* raised = raise(arena, t, sys);
    if (sys >= 1 && flatten_key(raised) == flatten_key(mark_fn(arena, sys))) {
        return display_main(mark_fs(arena, sys, n));
    }
    return display_main(cache.fs(raised, sys, n, /*main_style=*/true, std_fn));
}

// noraise 家族：输入 "Limit" 时替换为记号极限项；sys 恒为 0。
[[nodiscard]] inline std::string ton_expand_noraise(TonFsCache& cache, const TonNode* t, int n,
                                                    const StdFn& std_fn, bool ibp_style_limit) {
    TonArena arena;
    if (is_inf(t)) t = make_limit(arena, ibp_style_limit);
    return display_noraise(cache.fs(t, 0, n, /*main_style=*/false, std_fn));
}

// 解析 + 展开的公共入口（各记号头文件的 expand_text 骨架）。
[[nodiscard]] inline std::string ton_expand_text(std::string_view text, int term, bool main_style,
                                                 bool ibp_style_limit, const StdFn& std_fn,
                                                 TonFsCache& cache,
                                                 TonNode* (*mark_fn)(TonArena&, TonValue)) {
    if (term < 0) term = 0;
    TonArena arena;
    const TonNode* t = parse_impl(arena, text, main_style);
    if (t == nullptr) {
        return "解析失败，请确认输入是 TON 文本（Limit / 0 / Ω / Ω<n> / C 后缀），且不是全角字符";
    }
    if (main_style) return ton_expand_main(cache, t, term, std_fn, mark_fn);
    return ton_expand_noraise(cache, t, term, std_fn, ibp_style_limit);
}

} // namespace omegay::notation::tonshare

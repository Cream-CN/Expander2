// Header/notation/xy-v1.hpp
// X-Y V1 记号（维度参数 D_XY = [1,0,1]）的 C++20 头文件实现。
// 逻辑忠实移植自 X-Y1.js（桌面上的参考实现），记号显示名 "X-Y V1"。
//
// 规范：omegay::notation::XYV1Notation
//  - kName    UTF-8 显示名
//  - expand   对整数序列执行 term 项展开，纯函数、不抛异常
//  - suffix   结果后缀（空）
// 依赖：仅标准库。不包含 <windows.h>，无 UI 依赖。
//
// 移植说明：
//  - 参考 JS 是 n-Y 通用展开器，维度由用户输入；X-Y V1 固定取 X-Y 的
//    维度序列 [1,0,1]（与 xy-v3.hpp 同一 D_XY）。
//  - UI/显示逻辑（displayMt、expandmultilimited、DOM 等）不属于记号接口，
//    按 CONTRIBUTING 约定一并去除；其中仅控制显示的参数 f 也被省略。
//  - 参考 JS 存在若干“vibe coding”遗留，本文件按可观察行为等价的方式
//    整理为直观的 C++20 实现，并在相应位置加注：
//      1) toSequence 中“无更小左邻的元素”在 JS 里被当作数组空洞丢弃
//         （如 [2,1] 展开得 []，丢失前导 2；[2,3] 甚至直接崩溃）；本移植
//         保留全部元素，以合成根为父，数学上更一致（[2,1] 得 [2]）。
//      2) drawMountain 增加“脚值必须严格下降”护栏，避免退化输入（前导
//         元素 >1 且无更小左邻）因根父节点 value=0 而无法终止。
//      3) 展开主循环与 fetchDimensionSequence/calcMaxCopyrow/copyItem 中
//         对空指针（head/parent 缺失）与越界列索引增加护栏，保证任意
//         输入都不崩溃；对正常输入（各元素均有更小左邻）这些护栏均不生效，
//         结果与参考 JS 完全一致。
//  - 与原 ε-Y 移植相同，使用 detail 命名空间 + deque Arena 管理 Entry
//    生命周期，函数间相互递归（expand / expandDimensionSequence）。
#pragma once
#include <cstddef>
#include <deque>
#include <string>
#include <utility>
#include <vector>

namespace omegay::notation {
    namespace xy_v1_detail {
        // ------------------------------------------------------------------
        // 数据节点：对应 JS 中“山”的每一个元素
        // ------------------------------------------------------------------
        struct Entry {
            int value = 0;
            std::vector<int> row;
            int cloumn = -1;   // 所在列索引
            int no = 0;        // 坏根列的列内编号（preprocess）
            Entry* parent = nullptr;
            Entry* head = nullptr;   // 正下方元素
            Entry* foot = nullptr;   // 正上方元素（'foot' in it 语义）
        };

        // ------------------------------------------------------------------
        // Arena：统一持有 Entry，返回裸指针。deque 在尾部追加不会使既有
        // 指针失效，因此展开过程中返回的 Entry* 始终有效。
        // ------------------------------------------------------------------
        class Arena {
        public:
            Entry* make(int value,
                std::vector<int> row,
                int cloumn,
                int no = 0,
                Entry* parent = nullptr,
                Entry* head = nullptr,
                Entry* foot = nullptr) {
                data_.emplace_back();
                Entry* e = &data_.back();
                e->value = value;
                e->row = std::move(row);
                e->cloumn = cloumn;
                e->no = no;
                e->parent = parent;
                e->head = head;
                e->foot = foot;
                return e;
            }
            // 合成根节点：row=[1], cloumn=-1, value=0（对应 JS 的合成根；
            // 对“无更小左邻”的退化元素用作父节点）
            Entry* make_root() {
                return make(0, { 1 }, -1);
            }

        private:
            std::deque<Entry> data_;
        };

        // ------------------------------------------------------------------
        // 前向声明（相互递归）
        // ------------------------------------------------------------------
        inline std::vector<int> expand_impl(const std::vector<Entry*>& s,
            int n,
            const std::vector<int>& d,
            Arena& arena);
        inline std::vector<Entry*> toSequence(const std::vector<int>& s,
            Arena& arena);
        inline std::vector<int> getFootRow(const Entry& it,
            const std::vector<int>& d);
        inline std::vector<int> rowDifference(const std::vector<int>& r1,
            const std::vector<int>& r2);
        inline std::vector<int> expandDimensionSequence(std::vector<int> s,
            int n,
            const std::vector<int>& d,
            Arena& arena);

        // ------------------------------------------------------------------
        // 基础工具
        // ------------------------------------------------------------------
        // compareRow：字典序比较两行；r1<r2 返回 -1，相等 0，r1>r2 返回 1
        // 注意：与源 JS 一致，先比较长度（短 < 长），长度相同再逐元素比较。
        inline int compareRow(const std::vector<int>& r1,
            const std::vector<int>& r2) {
            if (r1.size() < r2.size()) return -1;
            if (r1.size() > r2.size()) return 1;
            for (std::size_t k = 0; k < r1.size(); ++k) {
                if (r1[k] < r2[k]) return -1;
                if (r1[k] > r2[k]) return 1;
            }
            return 0;
        }
        // rowAddition：行加法（忠实移植 X-Y1.js）
        inline std::vector<int> rowAddition(const std::vector<int>& r1,
            const std::vector<int>& r2) {
            if (r1.size() < r2.size()) return r2;
            if (r2.empty()) return r1;
            const std::size_t k = r1.size() - r2.size();
            std::vector<int> b = r2;
            b[0] += r1[k];
            std::vector<int> out(r1.begin(),
                r1.begin() + static_cast<std::ptrdiff_t>(k));
            out.insert(out.end(), b.begin(), b.end());
            return out;
        }
        // rowDifference：行减法（忠实移植 X-Y1.js）
        inline std::vector<int> rowDifference(const std::vector<int>& r1,
            const std::vector<int>& r2) {
            if (r1.size() > r2.size()) return r1;
            if (compareRow(r1, r2) <= 0) return {};
            std::size_t i = 0;
            while (i < r1.size() && i < r2.size() && r1[i] == r2[i]) ++i;
            std::vector<int> row;
            row.reserve(r1.size() - i);
            row.push_back(r1[i] - r2[i]);
            for (std::size_t k = i + 1; k < r1.size(); ++k) row.push_back(r1[k]);
            return row;
        }
        // getFootRow：计算给定元素“脚”所在的行
        inline std::vector<int> getFootRow(const Entry& it,
            const std::vector<int>& d) {
            std::vector<int> row = { 1 };
            int diff = 0;
            if (it.parent != nullptr) {
                diff = static_cast<int>(rowDifference(it.row, it.parent->row).size());
            }
            if (compareRow(d, { 0 }) > 0) {
                while (diff-- > 0) row.push_back(0);
            }
            return rowAddition(it.row, row);
        }
        // isDimensionLimited：n-Y 的一维限制（d 长度为 1 时判定；X-Y 用 d=[1,0,1]
        // 恒为 false，保留以忠实移植）
        inline bool isDimensionLimited(const Entry& it,
            const std::vector<int>& d) {
            return d.size() == 1 &&
                it.row.size() == static_cast<std::size_t>(d[0]) &&
                (it.parent == nullptr ||
                    it.parent->row.size() < static_cast<std::size_t>(d[0]) ||
                    it.row[0] > it.parent->row[0]);
        }

        // ------------------------------------------------------------------
        // 图结构处理
        // ------------------------------------------------------------------
        // 建“山”：由 Entry 序列生成山形结构，返回各列的顶元素（o）
        inline std::vector<Entry*> drawMountain(std::vector<std::vector<Entry*>>& m,
            const std::vector<int>& d,
            Arena& arena) {
            std::vector<Entry*> o;
            o.reserve(m.size());
            for (std::size_t i = 0; i < m.size(); ++i) {
                Entry* it = m[i][0];
                while (it->value > 1) {
                    if (isDimensionLimited(*it, d)) break;
                    // 父节点值（合成根按 0 计）
                    const int pv = (it->parent != nullptr) ? it->parent->value : 0;
                    const int foot_val = it->value - pv;
                    // 防死循环护栏：脚值必须严格下降（foot_val < it->value），
                    // 否则停止建山。参考 JS 对“无更小左邻的退化输入”会因根父
                    // 节点 value 缺失（NaN / 0）导致脚值不降而无法终止；此护栏
                    // 保证 expand 对任意输入都终止。对正常输入（父值 >=1）无影响。
                    if (foot_val >= it->value) break;
                    std::vector<int> fr = getFootRow(*it, d);
                    Entry* foot = arena.make(foot_val, std::move(fr), it->cloumn,
                        0, nullptr, it);
                    it->foot = foot;   // 回写正上方脚（JS: it.foot={...}）
                    m[i].push_back(foot);
                    Entry* p = it->parent;
                    if (p != nullptr && p->foot != nullptr &&
                        compareRow(p->foot->row, foot->row) <= 0) {
                        p = p->foot;
                    }
                    while (p != nullptr && p->value >= foot->value) {
                        p = p->parent;
                    }
                    foot->parent = p;
                    it = foot;
                }
                o.push_back(it);
            }
            return o;
        }
        // preprocess：设置坏根列及其后各列的 no
        inline void preprocess(std::vector<std::vector<Entry*>>& m, Entry* b) {
            // 坏根列：no 置为 1..N
            const std::size_t bcol = static_cast<std::size_t>(b->cloumn);
            for (std::size_t i = 0; i < m[bcol].size(); ++i) {
                m[bcol][i]->no = static_cast<int>(i) + 1;
            }
            // 坏根之后各列：父列在坏根左边则 no=0，否则沿用父的 no
            for (std::size_t i = bcol + 1; i < m.size(); ++i) {
                for (std::size_t j = 0; j < m[i].size(); ++j) {
                    Entry* e = m[i][j];
                    if (e->parent == nullptr || e->parent->cloumn < b->cloumn) {
                        e->no = 0;
                    }
                    else {
                        e->no = e->parent->no;
                    }
                }
            }
        }
        // fetchDimensionSequence：由山取得维度序列（各相关元素的行长度）
        inline std::vector<int> fetchDimensionSequence(Entry* it) {
            std::vector<int> seq;
            seq.reserve(4);
            seq.push_back((it->parent != nullptr)
                ? static_cast<int>(it->parent->row.size()) : 0);
            seq.push_back(static_cast<int>(it->row.size()) + 1);
            if (it->parent == nullptr) return seq;
            it = it->parent;
            while (it->row.size() > 1) {
                const Entry* hp = (it->head != nullptr) ? it->head->parent : nullptr;
                if (hp != nullptr &&
                    static_cast<int>(hp->row.size()) < seq[0]) {
                    seq.insert(seq.begin(), static_cast<int>(hp->row.size()));
                }
                it = (it->head != nullptr) ? it->head->parent : nullptr;
                if (it == nullptr) break;
            }
            return seq;
        }
        // expandDimensionSequence：维度序列展开。对 X-Y（d=[1,0,1]）在维度
        // 序列上递归调用 expand，这是 X-Y 超越 ε-Y 的关键环节。
        inline std::vector<int> expandDimensionSequence(std::vector<int> s,
            int n,
            const std::vector<int>& d,
            Arena& arena) {
            const std::size_t len = s.size();
            // 末两项差 1：压入 (n-1) 个减一后的末项
            if (len >= 2 && s[len - 1] - s[len - 2] == 1) {
                int p = --s[len - 1];
                while (n-- > 0) s.push_back(p);
                return s;
            }
            // [0,a]：ω^a-Y / ω^ω-Y（X-Y 用 d=[1,0,1] 不会命中，保留以忠实移植）
            if (d.size() == 2 && d[0] == 0) {
                if (len >= 2) {
                    int p = --s[len - 1];
                    int b = p - s[len - 2];
                    if (d[1]) b = std::min(b, d[1] - 1);
                    while (n-- > 0) { p += b; s.push_back(p); }
                }
                return s;
            }
            // X-Y：在维度序列（转为序列）上递归展开
            if (compareRow(d, { 1, 0, 1 }) == 0) {
                return expand_impl(toSequence(s, arena), n, d, arena);
            }
            if (compareRow(d, { 1, 0, 2 }) == 0) {
                return expand_impl(toSequence(s, arena), n, { 1 }, arena);
            }
            if (d.size() > 2 && d[0] == 0 && d[1] == 0) {
                std::vector<int> sub(d.begin() + 2, d.end());
                return expand_impl(toSequence(s, arena), n, sub, arena);
            }
            // 默认 ω-Y
            if (!s.empty()) {
                int p = --s.back();
                while (n-- > 0) s.push_back(p);
            }
            return s;
        }
        // toSequence：整数序列 → Entry 序列（parent 指向左侧最近的较小元素；
        // 无较小元素时指向合成根，避免出现空洞——参考 JS 会在此丢元素甚至崩溃）
        inline std::vector<Entry*> toSequence(const std::vector<int>& s,
            Arena& arena) {
            std::vector<Entry*> seq;
            seq.reserve(s.size());
            for (std::size_t i = 0; i < s.size(); ++i) {
                if (s[i] <= 1) {
                    Entry* root = arena.make_root();
                    seq.push_back(arena.make(s[i], { 0 }, static_cast<int>(i), 0, root));
                    continue;
                }
                bool found = false;
                for (int j = static_cast<int>(i) - 1; j >= 0; --j) {
                    if (s[static_cast<std::size_t>(j)] < s[i]) {
                        seq.push_back(arena.make(s[i], { 0 }, static_cast<int>(i), 0,
                            seq[static_cast<std::size_t>(j)]));
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    Entry* root = arena.make_root();
                    seq.push_back(arena.make(s[i], { 0 }, static_cast<int>(i), 0, root));
                }
            }
            return seq;
        }

        // ------------------------------------------------------------------
        // 复制与展开
        // ------------------------------------------------------------------
        // calcMaxCopyrow：计算复制列时某元素的上限行
        inline std::vector<int> calcMaxCopyrow(
            const std::vector<std::vector<Entry*>>& m,
            Entry* b,
            Entry* t,
            Entry* it,
            const std::vector<int>& dim_seq,
            int index,
            int i,
            const std::vector<int>& d) {
            if (compareRow(d, { 0 }) == 0 || it->no == 0) return it->row;
            std::vector<int> diff;
            if (b->cloumn >= 0 && static_cast<std::size_t>(b->cloumn) < m.size() &&
                it->no >= 1 &&
                static_cast<std::size_t>(it->no - 1) <
                    m[static_cast<std::size_t>(b->cloumn)].size()) {
                diff = rowDifference(it->row,
                    m[static_cast<std::size_t>(b->cloumn)]
                        [static_cast<std::size_t>(it->no - 1)]->row);
            }
            else {
                diff = it->row;
            }
            if (it->no == b->no && diff.size() >= t->row.size() &&
                index + i + 1 >= 0 &&
                static_cast<std::size_t>(index + i + 1) < dim_seq.size()) {
                // 在位置 1 插入 (dim_seq[index+i+1] - t.row.length) 个 0
                int l = dim_seq[static_cast<std::size_t>(index + i + 1)] -
                    static_cast<int>(t->row.size());
                for (int z = 0; z < l; ++z) {
                    diff.insert(diff.begin() + 1, 0);
                }
            }
            // 取已复制出的目标列的末元素作为基
            const int pc = t->cloumn + (t->cloumn - b->cloumn) * i;
            Entry* p = nullptr;
            if (pc >= 0 && static_cast<std::size_t>(pc) < m.size() &&
                !m[static_cast<std::size_t>(pc)].empty()) {
                p = m[static_cast<std::size_t>(pc)].back();
            }
            while (p != nullptr && p->no > it->no) p = p->head;
            return rowAddition((p != nullptr) ? p->row : it->row, diff);
        }
        // copyItem：把一个源元素 it 复制进目标列 op（生成向上延伸的脚）
        inline void copyItem(std::vector<Entry*>& op,
            Entry* head,
            const std::vector<int>& max_row,
            const std::vector<int>& d,
            Arena& arena) {
            if (head->parent == nullptr || head->parent->cloumn < 0) return;
            while (true) {
                std::vector<int> row = getFootRow(*head, d);
                if (compareRow(row, max_row) > 0) return;
                Entry* foot = arena.make(head->value, std::move(row), head->cloumn,
                    head->no, head->parent, head);
                head->foot = foot;   // 回写正上方脚（JS: head.foot={...}）
                if (head->parent != nullptr && head->parent->foot != nullptr &&
                    compareRow(head->parent->foot->row, foot->row) <= 0) {
                    if (foot->parent != nullptr && foot->parent->foot != nullptr) {
                        foot->parent = foot->parent->foot;
                    }
                }
                op.push_back(foot);
                head = foot;
            }
        }
        // 核心展开
        inline std::vector<int> expand_impl(const std::vector<Entry*>& s,
            int n,
            const std::vector<int>& d,
            Arena& arena) {
            if (s.empty()) return {};
            if (s.back()->value <= 1) {
                // 删去末项
                std::vector<int> result;
                result.reserve(s.size() - 1);
                for (std::size_t i = 0; i + 1 < s.size(); ++i) {
                    result.push_back(s[i]->value);
                }
                return result;
            }
            std::vector<std::vector<Entry*>> m;
            m.reserve(s.size());
            for (Entry* e : s) m.push_back({ e });
            std::vector<Entry*> o = drawMountain(m, d, arena);
            // 末列顶元素无 head（退化输入无法建山）→ 删去末项
            Entry* t = o.back()->head;
            if (t == nullptr) {
                std::vector<int> result;
                result.reserve(s.size() - 1);
                for (std::size_t i = 0; i + 1 < s.size(); ++i) {
                    result.push_back(s[i]->value);
                }
                return result;
            }
            Entry* b = t->parent;
            int len = t->cloumn - ((b != nullptr) ? b->cloumn : -1);
            // 维度序列：取出、展开（X-Y 下会在维度序列上递归）
            std::vector<int> dim_seq = fetchDimensionSequence(t);
            const int index = static_cast<int>(dim_seq.size()) - 1;
            dim_seq = expandDimensionSequence(std::move(dim_seq), n, d, arena);

            std::vector<int> ex;
            if (o.back()->value > 1) {
                // 外层序列 o 递归展开（跨维度情形）
                std::vector<Entry*> outer;
                outer.reserve(o.size());
                for (Entry* e : o) {
                    Entry* p = nullptr;
                    if (e->parent != nullptr && e->parent->cloumn >= 0 &&
                        static_cast<std::size_t>(e->parent->cloumn) < outer.size()) {
                        p = outer[static_cast<std::size_t>(e->parent->cloumn)];
                    }
                    else {
                        p = arena.make_root();
                    }
                    outer.push_back(arena.make(e->value, { 0 }, e->cloumn, 0, p));
                }
                ex = expand_impl(outer, n, d, arena);
                if (n > 0) {
                    len = static_cast<int>((ex.size() - o.size()) / n);
                    const int bcol = t->cloumn - len;
                    if (bcol >= 0 && static_cast<std::size_t>(bcol) < o.size()) {
                        b = o[static_cast<std::size_t>(bcol)];
                    }
                }
            }
            else {
                // 去掉末列顶部的 foot，末列整体减一
                if (t->foot != nullptr) t->foot = nullptr;
                m[static_cast<std::size_t>(t->cloumn)].pop_back();
            }
            // 坏根为空（t 无 parent，且跨维度分支未重新定位到外层坏根）→
            // 退化输入，删去末项（与上方 t==nullptr 及 xy-v2 的 b==nullptr 一致）
            if (b == nullptr) {
                std::vector<int> result;
                result.reserve(s.size() - 1);
                for (std::size_t i = 0; i + 1 < s.size(); ++i) {
                    result.push_back(s[i]->value);
                }
                return result;
            }
            preprocess(m, b);
            for (Entry* e : m[static_cast<std::size_t>(t->cloumn)]) {
                e->value--;
            }
            // 复制坏根之后到末列的每一列 n 次
            for (int i = 0; i < n; ++i) {
                for (int j = b->cloumn + 1; j <= t->cloumn; ++j) {
                    if (j < 0 || static_cast<std::size_t>(j) >= m.size()) continue;
                    const std::size_t l = m.size();
                    std::vector<Entry*> op;
                    m.push_back(op);
                    std::vector<Entry*>& new_col = m.back();
                    for (std::size_t k = 0; k < m[static_cast<std::size_t>(j)].size(); ++k) {
                        Entry* src = m[static_cast<std::size_t>(j)][k];
                        const std::vector<int> max_row =
                            calcMaxCopyrow(m, b, t, src, dim_seq, index, i, d);
                        Entry* head = nullptr;
                        if (new_col.empty()) {
                            Entry* par = (src->parent != nullptr)
                                ? src->parent : arena.make_root();
                            head = arena.make(0, { 0 }, static_cast<int>(l),
                                m[static_cast<std::size_t>(j)][0]->no, par);
                        }
                        else {
                            head = arena.make(0, getFootRow(*new_col.back(), d),
                                static_cast<int>(l), src->no, src->parent,
                                new_col.back());
                            new_col.back()->foot = head;
                        }
                        new_col.push_back(head);
                        // 修正父列
                        if (head->parent != nullptr && head->parent->cloumn >= b->cloumn) {
                            const int pc = head->parent->cloumn + len * i + len;
                            if (pc >= 0 && static_cast<std::size_t>(pc) < m.size() &&
                                !m[static_cast<std::size_t>(pc)].empty()) {
                                head->parent = m[static_cast<std::size_t>(pc)].back();
                            }
                        }
                        while (head->parent != nullptr && head->parent->head != nullptr &&
                            compareRow(head->parent->row, head->row) > 0) {
                            head->parent = head->parent->head;
                        }
                        copyItem(new_col, new_col.back(), max_row, d, arena);
                    }
                    // 新列末值：跨维度时取外层展开值，否则沿用源列末值
                    int last_val;
                    if (!ex.empty()) {
                        const int eidx = j + len * i + len;
                        last_val = (eidx >= 0 && static_cast<std::size_t>(eidx) < ex.size())
                            ? ex[static_cast<std::size_t>(eidx)] : 0;
                    }
                    else {
                        last_val = m[static_cast<std::size_t>(j)].back()->value;
                    }
                    new_col.back()->value = last_val;
                    // 自底向上重算列值（山的不变量：下 = 上 + 下.parent）
                    for (std::size_t k = new_col.size() - 1; k > 0; --k) {
                        if (new_col[k - 1]->parent != nullptr) {
                            new_col[k - 1]->value =
                                new_col[k]->value + new_col[k - 1]->parent->value;
                        }
                    }
                }
            }
            // 返回各列底部元素的值
            std::vector<int> result;
            result.reserve(m.size());
            for (const std::vector<Entry*>& col : m) {
                result.push_back(col.empty() ? 0 : col[0]->value);
            }
            return result;
        }
        // 公开入口：X-Y V1（维度序列 [1,0,1]），term 上限 10（对应 JS expandmultilimited）
        inline std::vector<int> expand_entry(const std::vector<int>& seq,
            int term) {
            if (term < 1 || seq.empty()) return seq;
            if (term > 10) term = 10;
            Arena arena;
            const std::vector<int> d = { 1, 0, 1 };  // D_XY
            const std::vector<Entry*> s = toSequence(seq, arena);
            return expand_impl(s, term, d, arena);
        }
    }  // namespace xy_v1_detail

    struct XYV1Notation {
        static constexpr const char* kName = "X-Y V1";  // UTF-8
        [[nodiscard]] static std::string suffix() {
            return {};
        }
        [[nodiscard]] static std::vector<int> expand(const std::vector<int>& seq,
            int term) {
            return xy_v1_detail::expand_entry(seq, term);
        }
    };
}  // namespace omegay::notation

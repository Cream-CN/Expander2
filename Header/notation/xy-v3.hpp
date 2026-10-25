
#pragma once
#include <cstddef>
#include <deque>
#include <string>
#include <utility>
#include <vector>

namespace omegay::notation {
    namespace xy_v3_detail {
        // ------------------------------------------------------------------
        // 数据节点：对应 JS 中“山”的每一个元素
        // ------------------------------------------------------------------
        struct Entry {
            int value = 0;
            std::vector<int> row;
            int cloumn = -1;            // 所在列索引
            int idx = 0;                // 列内自底向上的序号
            int no = 0;                 // 坏根列的列内编号（setElementNo）
            int id = 0;                 // 复制用的 id
            Entry* parent = nullptr;
            Entry* head = nullptr;      // 正下方元素
            Entry* foot = nullptr;      // 正上方元素（'foot' in it 语义）
            Entry* ref = nullptr;       // 参考元素
            // 维度序列展开产生的偏移；非空表示该元素已设置 offset
            // （对应 JS 中 'offset' in it 的动态属性）
            std::vector<std::vector<int>> offset;
        };

        // ------------------------------------------------------------------
        // Arena：统一持有 Entry，返回裸指针。deque 在尾部追加不会使既有
        // 指针失效，因此展开过程中返回的 Entry* 始终有效。
        // ------------------------------------------------------------------
        class Arena {
        public:
            Entry* make() {
                data_.emplace_back();
                return &data_.back();
            }
            Entry* make(int value,
                std::vector<int> row,
                int cloumn,
                int idx,
                int no,
                int id,
                Entry* parent = nullptr,
                Entry* head = nullptr,
                Entry* foot = nullptr,
                Entry* ref = nullptr) {
                data_.emplace_back();
                Entry* e = &data_.back();
                e->value = value;
                e->row = std::move(row);
                e->cloumn = cloumn;
                e->idx = idx;
                e->no = no;
                e->id = id;
                e->parent = parent;
                e->head = head;
                e->foot = foot;
                e->ref = ref;
                return e;
            }
            // 合成根节点：row=[1], cloumn=-1（对应 JS 的 {row:[1], cloumn:-1}）
            Entry* make_root() {
                return make(0, { 1 }, -1, 0, 0, 0);
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
            bool f,
            Arena& arena);
        inline std::vector<Entry*> toSequence(const std::vector<int>& s,
            Arena& arena);
        inline std::vector<int> getFootRow(const Entry& it,
            const std::vector<int>& d);
        inline std::vector<int> rowDifference(const std::vector<int>& r1,
            const std::vector<int>& r2);
        inline std::vector<std::vector<int>> divide(const std::vector<int>& d);
        inline std::vector<int> merge(const std::vector<std::vector<int>>& d);
        inline std::vector<int> proc(const std::vector<int>& d);
        inline std::pair<int, int> getBootIndex(const std::vector<Entry*>& s,
            const std::vector<int>& d,
            Arena& arena);
        inline std::vector<Entry*> getMds(const std::vector<std::vector<Entry*>>& m,
            Arena& arena);
        inline void expandDimensionSequnece(std::vector<std::vector<Entry*>>& m,
            Entry* b,
            Entry* t,
            int n,
            const std::vector<int>& d,
            Arena& arena);

        // ------------------------------------------------------------------
        // 基础工具
        // ------------------------------------------------------------------
        // rowGenerator(n)：[1] 后接 (n-1) 个 0；n<=1 时为 [1]
        inline std::vector<int> rowGenerator(int n) {
            std::vector<int> row;
            row.reserve(n > 0 ? static_cast<std::size_t>(n) : 1u);
            row.push_back(1);
            for (int i = 1; i < n; ++i) row.push_back(0);
            return row;
        }
        // compareRow：字典序比较两行；r1>r2 返回 1，相等 0，r1<r2 返回 -1
        inline int compareRow(const std::vector<int>& r1,
            const std::vector<int>& r2) {
            std::size_t i = 0;
            for (; i < r1.size() && i < r2.size(); ++i) {
                if (r1[i] > r2[i]) return 1;
                if (r1[i] < r2[i]) return -1;
            }
            if (r1.size() > i) return 1;
            if (r2.size() > i) return -1;
            return 0;
        }
        // compareDimension：以第二元素比较“维度”（长度<=1 视为 1）
        inline int compareDimension(const std::vector<int>& r1,
            const std::vector<int>& r2) {
            const int a = (r1.size() <= 1) ? 1 : r1[1];
            const int b = (r2.size() <= 1) ? 1 : r2[1];
            return a - b;
        }
        // rowAddition：行加法（忠实移植 X-Y.js）
        inline std::vector<int> rowAddition(const std::vector<int>& r1,
            const std::vector<int>& r2) {
            if (r2.size() <= 1 || r2[1] == 1) {
                std::vector<int> out(r1);
                out.insert(out.end(), r2.begin(), r2.end());
                return out;
            }
            if (r1.size() <= 1 || r1[1] < r2[1]) return r2;
            int i = 1;
            while (++i < static_cast<int>(r1.size())) {
                if (r1[static_cast<std::size_t>(i)] < r2[1]) break;
            }
            std::vector<int> out(r1.begin(), r1.begin() + i);
            if (r2.size() > 1) {
                out.insert(out.end(), r2.begin() + 1, r2.end());
            }
            return out;
        }
        // rowDifference：行减法（忠实移植 X-Y.js；负长度切片在防御上返回空）
        inline std::vector<int> rowDifference(const std::vector<int>& r1,
            const std::vector<int>& r2) {
            if (compareRow(r1, r2) <= 0) return {};
            if (r1.size() > 1 && r1[1] == 1) {
                // JS: r1.slice(0, r1.length - r2.length)
                if (r1.size() >= r2.size()) {
                    return std::vector<int>(r1.begin(),
                        r1.begin() + static_cast<std::ptrdiff_t>(r1.size() - r2.size()));
                }
                return {};  // 防御：长度差为负时不切片（JS 负索引在此不可达）
            }
            std::size_t i = 0;
            for (; i < r1.size() && i < r2.size(); ++i) {
                if (r1[i] > r2[i]) break;
            }
            if (i < r1.size() && r1[i] == 1) {
                return std::vector<int>(r1.begin() + static_cast<std::ptrdiff_t>(i),
                    r1.end());
            }
            std::vector<int> out;
            out.reserve(r1.size() - i + 1);
            out.push_back(1);
            if (i < r1.size()) {
                out.insert(out.end(), r1.begin() + static_cast<std::ptrdiff_t>(i),
                    r1.end());
            }
            return out;
        }
        // proc(d)：X-Y V3 中为恒等变换
        inline std::vector<int> proc(const std::vector<int>& d) {
            return d;
        }
        // divide：维度序列细分
        inline std::vector<std::vector<int>> divide(const std::vector<int>& d) {
            std::vector<int> dd;
            if (d.size() > 1) dd.assign(d.begin() + 1, d.end());
            std::vector<std::vector<int>> ret;
            for (std::size_t i = 0; i + 1 < dd.size(); ++i) {
                int v = dd[i];
                while (v-- > 0) {
                    ret.push_back(rowGenerator(static_cast<int>(dd.size() - i)));
                }
            }
            if (!dd.empty() && dd.back() != 0) ret.push_back({ dd.back() });
            return ret;
        }
        // merge：维度序列合并
        inline std::vector<int> merge(const std::vector<std::vector<int>>& d) {
            if (d.empty()) return {};
            std::vector<int> ret = rowGenerator(static_cast<int>(d[0].size()));
            ret[0] -= 1;
            for (std::size_t i = 0; i + 1 < d.size(); ++i) {
                ret[ret.size() - d[i].size()]++;
            }
            ret[ret.size() - d.back().size()] += d.back()[0];
            ret.insert(ret.begin(), 0);
            return ret;
        }
        // isDimensionLimited：X-Y V3 中恒为 false
        inline bool isDimensionLimited(const Entry&, const std::vector<int>&) {
            return false;
        }
        // getFootRow：计算给定元素“脚”所在的行
        inline std::vector<int> getFootRow(const Entry& it,
            const std::vector<int>& d) {
            std::vector<int> row;
            if (it.parent != nullptr) {
                row = rowDifference(it.row, it.parent->row);
            }
            if (row.empty() || d.empty() ||
                (d.size() == 1 && d[0] == 0) ||
                (d.size() == 2 && d[0] == 0) ||
                (d.size() == 3 && d[0] == 1 && d[1] == 0 && d[2] == 1)) {
                return rowAddition(it.row, { 1 });
            }
            if (row.size() == 1) return rowAddition(it.row, { 1, 2 });
            return rowAddition(it.row, { 1, row[1] + 1 });
        }

        // ------------------------------------------------------------------
        // 图结构处理
        // ------------------------------------------------------------------
        // 设置每个元素的参考元素 ref
        inline void setElementRefrence(std::vector<std::vector<Entry*>>& m) {
            for (std::vector<Entry*>& col : m) {
                for (Entry* el : col) {
                    if (el->row.size() <= 1 && el->value <= 1) continue;
                    if (el->parent != nullptr &&
                        compareRow(el->row, el->parent->row) == 0) {
                        el->ref = el->parent;
                    }
                    else if (el->head != nullptr && el->head->parent != nullptr &&
                        el->head->parent->foot != nullptr &&
                        compareRow(el->head->parent->foot->row, el->row) <= 0) {
                        el->ref = el->head->parent->foot;
                        while (el->ref != nullptr &&
                            compareRow(el->ref->row, el->row) == 0) {
                            el->ref = el->ref->ref;
                        }
                    }
                    else if (el->head != nullptr && el->head->parent != nullptr) {
                        el->ref = el->head->parent;
                    }
                }
            }
        }
        // 设置每个元素的 no / id（b 为空时按 b_col=-1 处理）
        inline void setElementNo(std::vector<std::vector<Entry*>>& m, Entry* b) {
            int id = 0;
            const int b_col = (b != nullptr) ? b->cloumn : -1;
            for (std::size_t i = 0; i < m.size(); ++i) {
                for (std::size_t j = 0; j < m[i].size(); ++j) {
                    Entry* it = m[i][j];
                    if (static_cast<int>(i) == b_col) {
                        it->no = static_cast<int>(j) + 1;
                    }
                    else if (static_cast<int>(i) < b_col ||
                        (it->value <= 1 && j == 0)) {
                        it->no = 0;
                    }
                    else {
                        it->no = (it->ref != nullptr) ? it->ref->no : 0;
                    }
                    if (static_cast<int>(i) > b_col) it->id = id++;
                    if (static_cast<int>(i) == b_col || i == m.size() - 1) {
                        it->id = it->no;
                    }
                }
            }
        }
        // 从某个元素沿 ref 链回溯到根，返回根到该元素的链
        inline std::vector<Entry*> getReferenceChain(Entry* it) {
            std::vector<Entry*> c;
            while (it != nullptr) {
                c.insert(c.begin(), it);
                if (it->value <= 1 && it->row.size() == 1) break;
                it = it->ref;
            }
            return c;
        }
        // 整数序列 → Entry 序列（parent 指向左侧最近的较小元素；
        // 无较小元素时指向合成根，避免出现空洞）
        inline std::vector<Entry*> toSequence(const std::vector<int>& s,
            Arena& arena) {
            std::vector<Entry*> seq;
            seq.reserve(s.size());
            for (std::size_t i = 0; i < s.size(); ++i) {
                if (s[i] <= 1) {
                    Entry* root = arena.make_root();
                    seq.push_back(arena.make(s[i], {}, static_cast<int>(i),
                        0, 0, 0, root));
                    continue;
                }
                bool found = false;
                for (int j = static_cast<int>(i) - 1; j >= 0; --j) {
                    if (s[static_cast<std::size_t>(j)] < s[i]) {
                        seq.push_back(arena.make(s[i], {}, static_cast<int>(i),
                            0, 0, 0,
                            seq[static_cast<std::size_t>(j)]));
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    Entry* root = arena.make_root();
                    seq.push_back(arena.make(s[i], {}, static_cast<int>(i),
                        0, 0, 0, root));
                }
            }
            return seq;
        }
        // 建“山”：由 Entry 序列生成山形结构（列 × 堆叠元素）
        inline std::vector<std::vector<Entry*>> drawMountain(
            const std::vector<Entry*>& s,
            const std::vector<int>& d,
            Arena& arena) {
            std::vector<std::vector<Entry*>> m;
            m.reserve(s.size());
            for (Entry* e : s) {
                Entry* parent;
                if (e->parent != nullptr && e->parent->cloumn >= 0) {
                    parent = m[static_cast<std::size_t>(e->parent->cloumn)][0];
                }
                else {
                    parent = arena.make_root();
                }
                Entry* first = arena.make(e->value, { 1 }, e->cloumn,
                    0, 0, 0, parent);
                m.push_back({ first });
            }
            for (std::size_t i = 0; i < m.size(); ++i) {
                Entry* it = m[i][0];
                while (it->value > 1) {
                    if (isDimensionLimited(*it, d)) break;
                    // 父节点值（合成根按 0 计）
                    const int pv = (it->parent != nullptr) ? it->parent->value : 0;
                    const int foot_val = it->value - pv;
                    // 防死循环护栏：脚值必须严格下降（foot_val < it->value），否则
                    // 停止建山。参考 JS 对“无更小左邻的退化输入”会因根父节点
                    // value 缺失（NaN / 0）导致脚值不降而无法终止；此护栏保证
                    // expand 对任意输入都终止。对正常输入（父值 >=1）无影响。
                    if (foot_val >= it->value) break;
                    const std::vector<int> fr = getFootRow(*it, d);
                    Entry* foot = arena.make(foot_val,
                        fr,
                        static_cast<int>(i),
                        it->idx + 1,
                        0, 0,
                        nullptr,
                        it);
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
            }
            return m;
        }
        // getOds：由山取得“外层维度序列”
        inline std::vector<Entry*> getOds(const std::vector<std::vector<Entry*>>& m,
            Arena& arena) {
            std::vector<Entry*> o;
            o.reserve(m.size());
            for (const std::vector<Entry*>& col : m) {
                Entry* last = col.back();
                Entry* parent;
                if (last->value <= 1) {
                    parent = arena.make_root();
                }
                else if (last->parent != nullptr && last->parent->cloumn >= 0) {
                    parent = o[static_cast<std::size_t>(last->parent->cloumn)];
                }
                else {
                    parent = arena.make_root();
                }
                o.push_back(arena.make(last->value, { 1 }, col[0]->cloumn,
                    0, 0, 0, parent));
            }
            return o;
        }
        // getMds：由山取得“主维度序列”
        inline std::vector<Entry*> getMds(const std::vector<std::vector<Entry*>>& m,
            Arena& arena) {
            Entry* last = m.back().back();
            const std::vector<Entry*> chain = getReferenceChain(last);
            std::vector<int> seq;
            seq.reserve(chain.size());
            for (Entry* e : chain) {
                seq.push_back(e->row.size() <= 1 ? 1 : e->row[1]);
            }
            return toSequence(seq, arena);
        }
        // getBootIndex：定位坏根（返回 [列, 列内行序号]）
        inline std::pair<int, int> getBootIndex(const std::vector<Entry*>& s,
            const std::vector<int>& d,
            Arena& arena) {
            std::vector<std::vector<Entry*>> m = drawMountain(s, d, arena);
            setElementRefrence(m);
            Entry* t = m.back().back();
            if (t->value == 1 && t->head != nullptr) t = t->head;
            Entry* b = t->parent;

            // 分支一：跨维度且 proc(d) 为单元素 → 递归求外层序列的坏根
            if (b != nullptr && t->value - b->value > 1 && proc(d).size() == 1) {
                const std::vector<Entry*> o = getOds(m, arena);
                std::vector<int> subD;
                if (d.size() == 1 || divide(d).size() == 1) {
                    subD = d;
                }
                else {
                    const std::vector<std::vector<int>> div = divide(d);
                    std::vector<std::vector<int>> sub(div.begin() + 1, div.end());
                    subD = merge(sub);
                }
                const std::pair<int, int> boot = getBootIndex(o, subD, arena);
                return { boot.first,
                    static_cast<int>(m[static_cast<std::size_t>(boot.first)].size()) - 1 };
            }
            // 分支二：坏根维度低于 t → 沿参考链在主维度序列上递归
            if (b != nullptr && compareDimension(b->row, t->row) < 0) {
                const std::vector<Entry*> ch = getReferenceChain(t);
                std::vector<int> dd = { 0, 1 };
                if (d.size() == 3 && d[0] == 1 && d[1] == 0 && d[2] == 1) {
                    dd = d;
                }
                const std::vector<Entry*> mds = getMds(m, arena);
                const std::pair<int, int> boot = getBootIndex(mds, dd, arena);
                const int c = ch[static_cast<std::size_t>(boot.first)]->cloumn;
                return { c, m[static_cast<std::size_t>(c)].back()->idx };
            }
            return { (b != nullptr) ? b->cloumn : -1,
                (b != nullptr) ? b->idx : 0 };
        }

        // ------------------------------------------------------------------
        // 复制与展开
        // ------------------------------------------------------------------
        // copyElement：把一个源元素 it 复制进目标列 op
        inline void copyElement(std::vector<std::vector<Entry*>>& m,
            Entry* b,
            Entry* t,
            Entry* it,
            std::vector<Entry*>& op,
            int i,
            const std::vector<int>& d,
            Arena& arena) {
            const int b_col = (b != nullptr) ? b->cloumn : -1;

            if (it->value <= 1 && it->row.size() > 1) {
                it->parent = it->ref;
                while (it->parent != nullptr && it->parent->value > 1) {
                    it->parent = it->parent->ref;
                }
            }
            // 起始行；若源行长度<=1 则从 [1] 起（op 为空时也取 [1]）
            std::vector<int> min_row =
                (it->row.size() > 1 && !op.empty())
                    ? getFootRow(*op.back(), d)
                    : std::vector<int>{ 1 };
            // 参考实现中 max_row 最终恒等于 it->row：
            //   X-Y.js 在 `if (it.no > 0) { ... }` 块内计算 max_row，随后被
            //   无条件 `max_row = it.row` 覆盖，故该块对结果无影响（死代码），
            //   此处按可观察行为等价地省略，仅保留结果。
            const std::vector<int> max_row = it->row;

            std::vector<int> row = min_row;
            while (compareRow(row, max_row) <= 0) {
                Entry* e = arena.make(it->value,
                    row,
                    static_cast<int>(m.size()) - 1,
                    static_cast<int>(op.size()),
                    it->no,
                    it->id);
                op.push_back(e);
                if (op.size() > 1) {
                    op.back()->head = op[op.size() - 2];
                    op[op.size() - 2]->foot = op.back();
                }
                // 计算父列索引
                const int pcol = (it->parent != nullptr) ? it->parent->cloumn : -1;
                const int pc = (pcol >= b_col)
                    ? static_cast<int>(m.size()) - 1 + pcol - it->cloumn
                    : pcol;
                Entry* p = nullptr;
                if (pc >= 0 && static_cast<std::size_t>(pc) < m.size() &&
                    !m[static_cast<std::size_t>(pc)].empty()) {
                    p = m[static_cast<std::size_t>(pc)].back();
                }
                else {
                    p = arena.make_root();
                }
                while (p != nullptr && compareRow(p->row, row) > 0) {
                    p = p->head;
                }
                op.back()->parent = p;
                row = getFootRow(*op.back(), d);
            }
        }
        // copyCloumn：把源列 c 复制为一个新列，并根据展开序列 ex 修正末值
        inline void copyCloumn(std::vector<std::vector<Entry*>>& m,
            Entry* b,
            Entry* t,
            int c,
            int i,
            const std::vector<int>& ex,
            const std::vector<int>& d,
            Arena& arena) {
            Entry* it = m[static_cast<std::size_t>(c)][0];
            m.emplace_back();
            std::vector<Entry*>& new_col = m.back();
            while (true) {
                copyElement(m, b, t, it, new_col, i, d, arena);
                if (it->foot != nullptr) {
                    it = it->foot;
                }
                else {
                    break;
                }
            }
            const std::size_t new_idx = m.size() - 1;
            new_col.back()->value =
                (!ex.empty() && new_idx < ex.size())
                    ? ex[new_idx]
                    : it->value;
            it = new_col.back();
            while (it->head != nullptr) {
                if (it->head->parent != nullptr) {
                    it->head->value = it->value + it->head->parent->value;
                }
                it = it->head;
            }
        }
        // expandDimensionSequnece：维度序列展开，为相关元素设置 offset，
        // 递归调用 expand_impl（X-Y 超越 ε-Y 的关键环节）
        inline void expandDimensionSequnece(std::vector<std::vector<Entry*>>& m,
            Entry* b,
            Entry* t,
            int n,
            const std::vector<int>& d,
            Arena& arena) {
            if (compareDimension(b->row, t->row) >= 0) return;

            std::vector<int> dd = { 0, 1 };
            if (d.size() == 3 && d[0] == 1 && d[1] == 0 && d[2] == 1) dd = d;
            const std::vector<Entry*> s = getMds(m, arena);
            const int c = getBootIndex(s, dd, arena).first;

            for (int i = b->cloumn + 1; i <= t->cloumn; ++i) {
                if (i < 0 || static_cast<std::size_t>(i) >= m.size()) continue;
                for (std::size_t j = 0; j < m[static_cast<std::size_t>(i)].size(); ++j) {
                    if (i == t->cloumn &&
                        j == m[static_cast<std::size_t>(t->cloumn)].size() - 1) {
                        return;
                    }
                    Entry* it = m[static_cast<std::size_t>(i)][j];
                    if (it->no != b->no || it->ref == nullptr) continue;
                    if (compareDimension(rowDifference(it->row, it->ref->row),
                        b->row) <= 0) {
                        continue;
                    }

                    // 复制主维度序列并在 c+1 处插入新元素
                    std::vector<Entry*> ds = s;
                    int pos = 1;
                    const std::vector<int> diffRow = rowDifference(it->row, it->ref->row);
                    {
                        const int val = diffRow.size() < 2 ? 1 : diffRow[1];
                        Entry* ne = arena.make(val, {}, c + 1, 0, 0, 0,
                            ds[static_cast<std::size_t>(c)]);
                        ds.insert(ds.begin() + (c + 1), ne);
                    }
                    // 沿参考链上溯并继续插入
                    while (it->ref != nullptr && it->ref->cloumn > b->cloumn) {
                        it = it->ref;
                        const std::vector<int> dr = rowDifference(it->row, it->ref->row);
                        if (compareDimension(dr, b->row) <= 0) continue;
                        {
                            const int val = dr.size() < 2 ? 1 : dr[1];
                            Entry* ne = arena.make(val, {}, c + 1, 0, 0, 0,
                                ds[static_cast<std::size_t>(c)]);
                            ds.insert(ds.begin() + (c + 1), ne);
                        }
                        ds[static_cast<std::size_t>(c) + 2]->parent =
                            ds[static_cast<std::size_t>(c) + 1];
                        ++pos;
                    }
                    // 重排 cloumn 并修正 parent（沿祖链上溯至大于自身的父）
                    for (std::size_t k = 0; k < ds.size(); ++k) {
                        ds[k]->cloumn = static_cast<int>(k);
                        while (ds[k]->parent != nullptr &&
                            ds[k]->value <= ds[k]->parent->value) {
                            ds[k]->parent = ds[k]->parent->parent;
                        }
                    }

                    it = m[static_cast<std::size_t>(i)][j];
                    it->offset.clear();
                    const int len = static_cast<int>(ds.size()) - 1 - c;
                    const std::vector<int> ex = expand_impl(ds, n, dd, false, arena);
                    for (int ii = 0; ii < n; ++ii) {
                        const int ex_idx = c + len * (ii + 1) + pos;
                        it->offset.push_back({
                            1,
                            (ex_idx >= 0 && static_cast<std::size_t>(ex_idx) < ex.size())
                                ? ex[static_cast<std::size_t>(ex_idx)]
                                : 0 });
                    }
                }
            }
        }
        // 核心展开（f 在 X-Y V3 中恒为 false，保留参数以忠实移植 JS 签名）
        inline std::vector<int> expand_impl(const std::vector<Entry*>& s,
            int n,
            const std::vector<int>& d,
            bool f,
            Arena& arena) {
            (void)f;
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
            const std::pair<int, int> idx = getBootIndex(s, d, arena);
            std::vector<std::vector<Entry*>> m = drawMountain(s, d, arena);
            Entry* b = m[static_cast<std::size_t>(idx.first)]
                [static_cast<std::size_t>(idx.second)];
            Entry* t = m.back().back();
            std::vector<int> ex;
            if (t->value == 1 && t->head != nullptr) t = t->head;

            setElementRefrence(m);
            setElementNo(m, b);
            expandDimensionSequnece(m, b, t, n, d, arena);

            if (t->value - b->value > 1 && proc(d).size() == 1) {
                // 跨维度：在外层维度序列上递归展开
                const std::vector<Entry*> o = getOds(m, arena);
                std::vector<int> subD;
                if (d.size() == 1 || divide(d).size() == 1) {
                    subD = d;
                }
                else {
                    const std::vector<std::vector<int>> div = divide(d);
                    std::vector<std::vector<int>> sub(div.begin() + 1, div.end());
                    subD = merge(sub);
                }
                ex = expand_impl(o, n, subD, false, arena);
            }
            else if (t->foot != nullptr) {
                // 去掉末列顶部的 foot 元素
                m.back().pop_back();
                t->foot = nullptr;
                if (t->parent != nullptr) {
                    t->parent = t->parent->parent;
                }
            }
            // 末列整体减一
            for (Entry* e : m.back()) e->value--;

            // 把坏根列 b 中 no>=b.no 的元素复制追加到末列
            const int b_col = b->cloumn;
            const int t_col = t->cloumn;
            if (b_col >= 0 && static_cast<std::size_t>(b_col) < m.size() &&
                t_col >= 0 && static_cast<std::size_t>(t_col) < m.size()) {
                const std::size_t b_col_sz = m[static_cast<std::size_t>(b_col)].size();
                for (int i = b->no; i < static_cast<int>(b_col_sz); ++i) {
                    // 参考实现中 `const idxVal = t.idx; idx: idxVal++` 的
                    // const 自增属 vibe 遗留；按 ε-Y 移植惯例取 t.idx+1。
                    const int new_idx = t->idx + 1;
                    Entry* src = m[static_cast<std::size_t>(b_col)]
                        [static_cast<std::size_t>(i)];
                    Entry* head = m[static_cast<std::size_t>(t_col)].back();
                    Entry* e = arena.make(src->value, src->row, t_col,
                        new_idx, src->no, src->no,
                        src->parent, head, nullptr, src);
                    m[static_cast<std::size_t>(t_col)].push_back(e);
                    if (m[static_cast<std::size_t>(t_col)].size() >= 2) {
                        m[static_cast<std::size_t>(t_col)]
                            [m[static_cast<std::size_t>(t_col)].size() - 2]
                            ->foot = e;
                    }
                }
            }
            // 复制坏根之后到末列的每一列 n 次
            for (int i = 0; i < n; ++i) {
                for (int j = b_col + 1; j <= t_col; ++j) {
                    if (j >= 0 && static_cast<std::size_t>(j) < m.size()) {
                        copyCloumn(m, b, t, j, i, ex, d, arena);
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
        // 公开入口：X-Y V3（维度序列 [1,0,1]），term 上限 10（对应 JS expandXY）
        inline std::vector<int> expand_entry(const std::vector<int>& seq,
            int term) {
            if (term < 1 || seq.empty()) return seq;
            if (term > 10) term = 10;
            Arena arena;
            const std::vector<Entry*> s = toSequence(seq, arena);
            const std::vector<int> d = { 1, 0, 1 };  // D_XY
            return expand_impl(s, term, d, false, arena);
        }
    }  // namespace xy_v3_detail

    struct XYV3Notation {
        static constexpr const char* kName = "X-Y V3";  // UTF-8
        [[nodiscard]] static std::string suffix() {
            return {};
        }
        [[nodiscard]] static std::vector<int> expand(const std::vector<int>& seq,
            int term) {
            return xy_v3_detail::expand_entry(seq, term);
        }
    };
}  // namespace omegay::notation


#pragma once
#include <cstddef>
#include <deque>
#include <string>
#include <utility>
#include <vector>

namespace omegay::notation {
    namespace xy_v2_detail {
        // ------------------------------------------------------------------
        // 数据节点：对应 JS 中“山”的每一个元素
        // ------------------------------------------------------------------
        struct Entry {
            int value = 0;
            std::vector<int> row;
            int cloumn = -1;   // 所在列索引
            int no = 0;        // 坏根列的列内编号（setElementNo）
            Entry* parent = nullptr;
            Entry* head = nullptr;   // 正下方元素
            Entry* foot = nullptr;   // 正上方元素（'foot' in it 语义）
            // 维度序列展开相关（JS 的动态属性 chain / dim_seq）
            std::vector<Entry*> chain;    // 参考链（仅坏根 b 与 t.parent 设置）
            std::vector<int> dim_seq;     // 维度展开值（非空表示已处理）
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
            // 合成根节点：row=[1], cloumn=-1, value=0
            Entry* make_root() {
                return make(0, { 1 }, -1);
            }

        private:
            std::deque<Entry> data_;
        };

        // memo 表：对应 JS 的全局 mm（维度序列展开缓存）。随递归传递以保证纯函数。
        struct Memo {
            std::vector<std::vector<Entry*>> seqs;
            std::vector<int> ns;
            std::vector<std::vector<int>> ds;
            std::vector<std::vector<int>> exs;
        };

        // ------------------------------------------------------------------
        // 前向声明（相互递归）
        // ------------------------------------------------------------------
        inline std::vector<int> expand_impl(const std::vector<Entry*>& s,
            int n,
            const std::vector<int>& d,
            Arena& arena,
            Memo& memo);
        inline std::vector<Entry*> toSequence(const std::vector<int>& s,
            Arena& arena);
        inline std::vector<int> getFootRow(const Entry& it,
            const std::vector<int>& d);
        inline std::vector<int> rowDifference(const std::vector<int>& r1,
            const std::vector<int>& r2);
        inline void drawMountain(std::vector<std::vector<Entry*>>& m,
            const std::vector<int>& d,
            Arena& arena);
        inline std::vector<Entry*> getReferenceChain(Entry* it);
        inline std::vector<Entry*> getMainDimensionSequence(
            const std::vector<Entry*>& c,
            Entry* t,
            Arena& arena);
        inline int getBootCloumn(const std::vector<Entry*>& s,
            const std::vector<int>& d,
            Arena& arena);
        inline void expandElementDimensionSequence(
            std::vector<std::vector<Entry*>>& m,
            Entry* b,
            Entry* t,
            int n,
            const std::vector<int>& d,
            Arena& arena,
            Memo& memo);
        inline void copyElement(std::vector<Entry*>& op,
            Entry* head,
            const std::vector<int>& max_row,
            const std::vector<int>& d,
            Arena& arena);
        inline bool isEqualSequnce(const std::vector<Entry*>& s1,
            const std::vector<Entry*>& s2);

        // ------------------------------------------------------------------
        // 基础工具
        // ------------------------------------------------------------------
        // compareRow：与源 JS 一致，先比较长度（短 < 长），长度相同再逐元素。
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
        inline bool isDimensionLimited(const Entry& it,
            const std::vector<int>& d) {
            return d.size() == 1 &&
                it.row.size() == static_cast<std::size_t>(d[0]) &&
                (it.parent == nullptr ||
                    it.parent->row.size() < static_cast<std::size_t>(d[0]) ||
                    it.row[0] > it.parent->row[0]);
        }
        // isEqualSequnce：比较两个 Entry 序列（值 + 父列）
        inline bool isEqualSequnce(const std::vector<Entry*>& s1,
            const std::vector<Entry*>& s2) {
            if (s1.size() != s2.size()) return false;
            for (std::size_t i = 0; i < s1.size(); ++i) {
                if (s1[i]->value != s2[i]->value) return false;
                const int c1 = (s1[i]->parent != nullptr) ? s1[i]->parent->cloumn : -1;
                const int c2 = (s2[i]->parent != nullptr) ? s2[i]->parent->cloumn : -1;
                if (c1 != c2) return false;
            }
            return true;
        }

        // ------------------------------------------------------------------
        // 图结构处理
        // ------------------------------------------------------------------
        // drawMountain：建“山”（无返回值，直接修改 m）
        inline void drawMountain(std::vector<std::vector<Entry*>>& m,
            const std::vector<int>& d,
            Arena& arena) {
            for (std::size_t i = 0; i < m.size(); ++i) {
                Entry* it = m[i][0];
                while (it->value > 1) {
                    if (isDimensionLimited(*it, d)) break;
                    // 父节点值（合成根按 0 计）；脚值必须严格下降，否则停止建山
                    const int pv = (it->parent != nullptr) ? it->parent->value : 0;
                    const int foot_val = it->value - pv;
                    if (foot_val >= it->value) break;   // 护栏
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
            }
        }
        // setElementNo：设置每个元素的 no
        inline void setElementNo(std::vector<std::vector<Entry*>>& m, Entry* b) {
            for (std::size_t i = 0; i < m.size(); ++i) {
                for (std::size_t j = 0; j < m[i].size(); ++j) {
                    Entry* e = m[i][j];
                    if (static_cast<int>(i) == b->cloumn) {
                        e->no = static_cast<int>(j) + 1;
                    }
                    else if (static_cast<int>(i) < b->cloumn ||
                        (e->value <= 1 && j == 0)) {
                        e->no = 0;
                    }
                    else if (e->value > 1 && e->parent != nullptr &&
                        compareRow(e->row, e->parent->row) == 0) {
                        e->no = e->parent->no;
                    }
                    else if (e->head != nullptr && e->head->parent != nullptr) {
                        e->no = e->head->parent->no;
                    }
                    else {
                        e->no = 0;   // 护栏：head 缺失时置 0
                    }
                }
            }
        }
        // getReferenceChain：沿参考链回溯到根，返回根到该元素的链
        inline std::vector<Entry*> getReferenceChain(Entry* it) {
            std::vector<Entry*> c;
            if (it == nullptr) return c;
            while (true) {
                c.insert(c.begin(), it);
                if (it->value <= 1 && compareRow(it->row, { 0 }) == 0) break;
                if (it->value > 1 && it->parent != nullptr &&
                    compareRow(it->row, it->parent->row) == 0) {
                    it = it->parent;
                }
                else if (it->head != nullptr && it->head->parent != nullptr &&
                    it->head->parent->foot != nullptr &&
                    compareRow(it->row, it->head->parent->foot->row) >= 0) {
                    it = it->head->parent->foot;
                }
                else if (it->head != nullptr && it->head->parent != nullptr) {
                    it = it->head->parent;
                }
                else {
                    break;   // 护栏：无法继续上溯
                }
            }
            return c;
        }
        // getMainDimensionSequence：由参考链与 t 生成主维度序列
        inline std::vector<Entry*> getMainDimensionSequence(
            const std::vector<Entry*>& c,
            Entry* t,
            Arena& arena) {
            std::vector<Entry*> s;
            s.reserve(c.size() + 1);
            for (Entry* e : c) {
                s.push_back(arena.make(static_cast<int>(e->row.size()), { 0 }, e->cloumn));
            }
            s.push_back(arena.make(static_cast<int>(t->row.size()) + 1, { 0 }, t->cloumn));
            for (std::size_t i = 0; i < s.size(); ++i) {
                if (s[i]->value <= 1) {
                    s[i]->parent = arena.make_root();
                    continue;
                }
                bool found = false;
                for (int j = static_cast<int>(i) - 1; j >= 0; --j) {
                    if (s[static_cast<std::size_t>(j)]->value < s[i]->value) {
                        s[i]->parent = s[static_cast<std::size_t>(j)];
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    s[i]->parent = arena.make_root();   // 护栏：无更小左邻时指向根
                }
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
        // getBootCloumn：定位坏根列（返回列索引，根为 -1）
        inline int getBootCloumn(const std::vector<Entry*>& s,
            const std::vector<int>& d,
            Arena& arena) {
            std::vector<std::vector<Entry*>> m;
            m.reserve(s.size());
            for (Entry* e : s) m.push_back({ e });
            drawMountain(m, d, arena);
            // 末列需至少两个元素（t 为倒数第二个）；退化输入返回 -1（根）
            if (m.empty() || m.back().size() < 2) return -1;
            Entry* t = m.back()[m.back().size() - 2];
            Entry* b = t->parent;
            // d.length==1 分支（X-Y 用 d=[1,0,1]，d.length==3 不会命中；保留以忠实移植）
            if (d.size() == 1 && t->foot != nullptr && t->foot->value > 1) {
                std::vector<Entry*> o;
                o.reserve(m.size());
                for (const std::vector<Entry*>& col : m) {
                    Entry* last = col.back();
                    Entry* p = nullptr;
                    if (last->value <= 1) {
                        p = arena.make_root();
                    }
                    else if (last->parent != nullptr && last->parent->cloumn >= 0 &&
                        static_cast<std::size_t>(last->parent->cloumn) < o.size()) {
                        p = o[static_cast<std::size_t>(last->parent->cloumn)];
                    }
                    else {
                        p = arena.make_root();
                    }
                    o.push_back(arena.make(last->value, { 0 }, col[0]->cloumn, 0, p));
                }
                return getBootCloumn(o, d, arena);
            }
            if (b == nullptr) return -1;   // 护栏
            if (t->row.size() == b->row.size() || d.size() < 3) return b->cloumn;
            const std::vector<Entry*> chain = getReferenceChain(b);
            const std::vector<Entry*> mds = getMainDimensionSequence(chain, t, arena);
            return getBootCloumn(mds, d, arena);
        }

        // ------------------------------------------------------------------
        // 复制与展开
        // ------------------------------------------------------------------
        // copyElement：把一个源元素 head 复制进目标列 op（生成向上延伸的脚）
        inline void copyElement(std::vector<Entry*>& op,
            Entry* head,
            const std::vector<int>& max_row,
            const std::vector<int>& d,
            Arena& arena) {
            if (head->parent != nullptr && head->parent->cloumn < 0 &&
                head->head != nullptr && head->head->parent != nullptr &&
                compareRow(head->head->parent->row, max_row) < 0) {
                head->parent = head->head->parent;
            }
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
        // copyCloumn：把源列 j 复制为一个新列，并据 ex 修正末值
        inline void copyCloumn(std::vector<std::vector<Entry*>>& m,
            Entry* b,
            int j,
            int i,
            int len,
            const std::vector<int>& ex,
            const std::vector<int>& d,
            Arena& arena) {
            const std::size_t l = m.size();
            m.emplace_back();
            std::vector<Entry*>& new_col = m.back();
            const std::vector<Entry*>& src_col = m[static_cast<std::size_t>(j)];
            for (std::size_t k = 0; k < src_col.size(); ++k) {
                Entry* src = src_col[k];
                std::vector<int> max_row;
                if (!src->dim_seq.empty()) {
                    // 维度序列分支：由 dim_seq 与行差构造上限行
                    std::vector<int> beg = { 1 };
                    const int kk = (static_cast<std::size_t>(i) < src->dim_seq.size())
                        ? src->dim_seq[static_cast<std::size_t>(i)] : 0;
                    std::vector<int> row_dif;
                    if (src->row.empty()) {
                        row_dif.clear();
                    }
                    else if (src->row[0] - 1 == 0) {
                        row_dif.clear();
                    }
                    else {
                        row_dif.push_back(src->row[0] - 1);
                    }
                    for (std::size_t jj = 1; jj + 1 < src->row.size(); ++jj) {
                        row_dif.push_back(src->row[jj]);
                    }
                    for (int z = kk - 1; z > 0; --z) beg.push_back(0);
                    max_row = rowAddition(beg, row_dif);
                }
                else if (src->no > 0) {
                    // 常规分支：沿目标列（b.cloumn + len*(i+1)）找基
                    const int bc = b->cloumn + len * (i + 1);
                    max_row = src->row;
                    if (bc >= 0 && static_cast<std::size_t>(bc) < m.size() &&
                        b->cloumn >= 0 && static_cast<std::size_t>(b->cloumn) < m.size() &&
                        src->no >= 1 &&
                        static_cast<std::size_t>(src->no - 1) <
                            m[static_cast<std::size_t>(b->cloumn)].size()) {
                        const std::vector<Entry*>& base_col =
                            m[static_cast<std::size_t>(bc)];
                        const Entry* bsrc =
                            m[static_cast<std::size_t>(b->cloumn)]
                                [static_cast<std::size_t>(src->no - 1)];
                        for (std::size_t kk = base_col.size(); kk-- > 0;) {
                            if (base_col[kk]->no > src->no) continue;
                            max_row = rowAddition(base_col[kk]->row,
                                rowDifference(src->row, bsrc->row));
                            break;
                        }
                    }
                }
                else {
                    max_row = src->row;
                }
                if (compareRow(d, { 0 }) == 0) max_row = src->row;
                Entry* head = nullptr;
                if (new_col.empty()) {
                    Entry* par = (src->parent != nullptr) ? src->parent : arena.make_root();
                    head = arena.make(0, { 0 }, static_cast<int>(l),
                        src_col[0]->no, par);
                }
                else {
                    head = arena.make(0, getFootRow(*new_col.back(), d),
                        static_cast<int>(l), src->no, src->parent, new_col.back());
                    new_col.back()->foot = head;
                }
                new_col.push_back(head);
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
                copyElement(new_col, new_col.back(), max_row, d, arena);
            }
            // 新列末值：跨维度时取外层展开值，否则沿用源列末值
            int last_val;
            if (!ex.empty()) {
                const int eidx = j + len * i + len;
                last_val = (eidx >= 0 && static_cast<std::size_t>(eidx) < ex.size())
                    ? ex[static_cast<std::size_t>(eidx)] : 0;
            }
            else {
                last_val = src_col.back()->value;
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
        // expandElementDimensionSequence：维度序列展开，为相关元素设置 dim_seq
        inline void expandElementDimensionSequence(
            std::vector<std::vector<Entry*>>& m,
            Entry* b,
            Entry* t,
            int n,
            const std::vector<int>& d,
            Arena& arena,
            Memo& memo) {
            if (b->row.size() == t->row.size() || d.size() < 3) return;
            // 护栏：依赖 b.chain / t.parent.chain，缺失则跳过
            if (b->chain.empty() || t->parent == nullptr || t->parent->chain.empty()) return;
            for (std::size_t i = static_cast<std::size_t>(b->cloumn) + 1; i < m.size(); ++i) {
                for (std::size_t j = 0; j < m[i].size(); ++j) {
                    Entry* it = m[i][j];
                    if (it->no < b->no || it->row.size() <= b->row.size()) continue;
                    // 维度链：m[i][j] 的参考链 + t.parent.chain 的尾部
                    const std::vector<Entry*> refchain = getReferenceChain(it);
                    const std::size_t index = refchain.size();
                    std::vector<Entry*> c;
                    c.reserve(refchain.size() + b->chain.size() + 1);
                    for (Entry* e : refchain) {
                        c.push_back(arena.make(static_cast<int>(e->row.size()), { 0 }, 0));
                    }
                    const std::size_t tail_start = b->chain.size();
                    const std::vector<Entry*>& tpchain = t->parent->chain;
                    if (tail_start < tpchain.size()) {
                        for (std::size_t k = tail_start; k < tpchain.size(); ++k) {
                            c.push_back(arena.make(
                                static_cast<int>(tpchain[k]->row.size()), { 0 }, 0));
                        }
                    }
                    it->dim_seq.clear();
                    // 设置 row / cloumn / parent
                    for (std::size_t k = 0; k < c.size(); ++k) {
                        c[k]->row = { 0 };
                        c[k]->cloumn = static_cast<int>(k);
                        if (c[k]->value <= 1) c[k]->parent = arena.make_root();
                        if (k == index) {
                            if (b->chain.size() - 1 < c.size()) {
                                c[k]->parent = c[b->chain.size() - 1];
                            }
                            continue;
                        }
                        for (int kk = static_cast<int>(k); kk >= 0; --kk) {
                            if (c[static_cast<std::size_t>(kk)]->value < c[k]->value) {
                                c[k]->parent = c[static_cast<std::size_t>(kk)];
                                break;
                            }
                        }
                    }
                    // 追加末元素
                    Entry* last = arena.make(static_cast<int>(t->row.size()) + 1,
                        { 0 }, static_cast<int>(c.size()));
                    if (t->parent->chain.size() - 1 < c.size()) {
                        last->parent = c[t->parent->chain.size() - 1];
                    }
                    c.push_back(last);
                    const int len = static_cast<int>(c.size()) -
                        static_cast<int>(b->chain.size());
                    std::vector<int> ex;
                    // memo 查找（精确键命中，复用仅当缓存项数 >= n）
                    for (std::size_t k = 0; k < memo.seqs.size(); ++k) {
                        if (memo.ns[k] >= n &&
                            compareRow(memo.ds[k], d) == 0 &&
                            isEqualSequnce(c, memo.seqs[k])) {
                            ex = memo.exs[k];
                            break;
                        }
                    }
                    if (ex.empty()) {
                        ex = expand_impl(c, n, d, arena, memo);
                        memo.seqs.push_back(c);
                        memo.ns.push_back(n);
                        memo.ds.push_back(d);
                        memo.exs.push_back(ex);
                    }
                    for (int k = 1; k <= n; ++k) {
                        const int eidx = static_cast<int>(index) - 1 + len * k;
                        it->dim_seq.push_back(
                            (eidx >= 0 && static_cast<std::size_t>(eidx) < ex.size())
                                ? ex[static_cast<std::size_t>(eidx)] : 0);
                    }
                }
            }
        }
        // 核心展开
        inline std::vector<int> expand_impl(const std::vector<Entry*>& s,
            int n,
            const std::vector<int>& d,
            Arena& arena,
            Memo& memo) {
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
            const int c = getBootCloumn(s, d, arena);
            // 建山（每个元素以父列底部为父）
            std::vector<std::vector<Entry*>> m;
            m.reserve(s.size());
            for (Entry* e : s) {
                Entry* p = nullptr;
                if (e->parent != nullptr && e->parent->cloumn >= 0 &&
                    static_cast<std::size_t>(e->parent->cloumn) < m.size()) {
                    p = m[static_cast<std::size_t>(e->parent->cloumn)][0];
                }
                else {
                    p = arena.make_root();
                }
                m.push_back({ arena.make(e->value, { 0 }, e->cloumn, 0, p) });
            }
            drawMountain(m, d, arena);
            // 末列需至少两个元素（t 为倒数第二个）；退化输入返回“删去末项”
            if (m.back().size() < 2) {
                std::vector<int> result;
                result.reserve(s.size() - 1);
                for (std::size_t i = 0; i + 1 < s.size(); ++i) {
                    result.push_back(s[i]->value);
                }
                return result;
            }
            Entry* t = m.back()[m.back().size() - 2];
            Entry* b = t->parent;
            if (b == nullptr) {
                std::vector<int> result;
                result.reserve(s.size() - 1);
                for (std::size_t i = 0; i + 1 < s.size(); ++i) {
                    result.push_back(s[i]->value);
                }
                return result;
            }
            if (c < b->cloumn && c >= 0 &&
                static_cast<std::size_t>(c) < m.size() && !m[static_cast<std::size_t>(c)].empty()) {
                b = m[static_cast<std::size_t>(c)].back();
            }
            const int len = t->cloumn - b->cloumn;
            setElementNo(m, b);
            if (d.size() > 2) {
                b->chain = getReferenceChain(b);
                if (t->parent != nullptr) {
                    t->parent->chain = getReferenceChain(t->parent);
                }
                expandElementDimensionSequence(m, b, t, n, d, arena, memo);
            }
            std::vector<int> ex;
            if (t->foot != nullptr && t->foot->value > 1) {
                // 跨维度：在外层序列 o 上递归展开
                std::vector<Entry*> o;
                o.reserve(m.size());
                for (const std::vector<Entry*>& col : m) {
                    Entry* last = col.back();
                    Entry* p = nullptr;
                    if (last->value <= 1) {
                        p = arena.make_root();
                    }
                    else if (last->parent != nullptr && last->parent->cloumn >= 0 &&
                        static_cast<std::size_t>(last->parent->cloumn) < o.size()) {
                        p = o[static_cast<std::size_t>(last->parent->cloumn)];
                    }
                    else {
                        p = arena.make_root();
                    }
                    o.push_back(arena.make(last->value, { 0 }, col[0]->cloumn, 0, p));
                }
                for (Entry* e : m.back()) e->value--;
                ex = expand_impl(o, n, d, arena, memo);
            }
            else {
                // 去掉末列顶部的 foot，末列整体减一，坏根上移
                m.back().pop_back();
                if (t->foot != nullptr) t->foot = nullptr;
                for (Entry* e : m.back()) e->value--;
                if (t->parent != nullptr && t->parent->parent != nullptr) {
                    t->parent = t->parent->parent;
                }
            }
            // 把坏根列 b 中 no>=b.no 的元素复制到末列
            if (b->cloumn >= 0 && static_cast<std::size_t>(b->cloumn) < m.size() &&
                t->cloumn >= 0 && static_cast<std::size_t>(t->cloumn) < m.size()) {
                const std::size_t bcol = static_cast<std::size_t>(b->cloumn);
                const std::size_t tcol = static_cast<std::size_t>(t->cloumn);
                for (int i = b->no; i < static_cast<int>(m[bcol].size()); ++i) {
                    Entry* src = m[bcol][static_cast<std::size_t>(i)];
                    Entry* ne = arena.make(src->value, src->row, t->cloumn,
                        src->no, src->parent, m[tcol].back(), nullptr);
                    m[tcol].push_back(ne);
                    if (m[tcol].size() >= 2) {
                        m[tcol][m[tcol].size() - 2]->foot = ne;
                    }
                }
            }
            // 复制坏根之后到末列的每一列 n 次
            for (int i = 0; i < n; ++i) {
                for (int j = b->cloumn + 1; j <= t->cloumn; ++j) {
                    if (j < 0 || static_cast<std::size_t>(j) >= m.size()) continue;
                    copyCloumn(m, b, j, i, len, ex, d, arena);
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
        // 公开入口：X-Y V2（维度序列 [1,0,1]），term 上限 10（对应 JS expandmultilimited）
        inline std::vector<int> expand_entry(const std::vector<int>& seq,
            int term) {
            if (term < 1 || seq.empty()) return seq;
            if (term > 10) term = 10;
            Arena arena;
            Memo memo;
            const std::vector<int> d = { 1, 0, 1 };  // D_XY
            const std::vector<Entry*> s = toSequence(seq, arena);
            return expand_impl(s, term, d, arena, memo);
        }
    }  // namespace xy_v2_detail

    struct XYV2Notation {
        static constexpr const char* kName = "X-Y V2";  // UTF-8
        [[nodiscard]] static std::string suffix() {
            return {};
        }
        [[nodiscard]] static std::vector<int> expand(const std::vector<int>& seq,
            int term) {
            return xy_v2_detail::expand_entry(seq, term);
        }
    };
}  // namespace omegay::notation

// Header/notation/epsilon-y.hpp
// ε-Y 记号（即 1-Y，维度序列 {1}）的 C++20 头文件实现。
// 规范：omegay::notation::EpsilonYNotation
// 依赖：仅标准库。纯函数，不抛异常。
#pragma once

#include <cstddef>
#include <deque>
#include <string>
#include <utility>
#include <vector>

namespace omegay::notation {

    namespace epsilon_y_detail {

        struct Entry {
            int value = 0;
            std::vector<int> row;
            int cloumn = -1;
            int idx = 0;
            int no = 0;
            int id = 0;
            Entry* parent = nullptr;
            Entry* head = nullptr;
            Entry* foot = nullptr;
            Entry* ref = nullptr;
        };

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

            Entry* make_root() {
                return make(0, { 1 }, -1, 0, 0, 0);
            }

        private:
            std::deque<Entry> data_;
        };

        // 前向声明（相互递归）
        inline std::vector<int> expand_impl(const std::vector<Entry*>& s,
            int n,
            const std::vector<int>& d,
            bool f,
            Arena& arena);
        inline std::vector<Entry*> toSequence(const std::vector<int>& s, Arena& arena);
        inline std::vector<int> rowStandardization(const std::vector<int>& r,
            const std::vector<int>& d,
            Arena& arena);
        inline std::vector<int> getFootRow(const Entry& it,
            const std::vector<int>& d,
            Arena& arena);
        inline bool isDimensionLimited(const Entry& it,
            const std::vector<int>& d,
            Arena& arena);
        inline std::vector<std::vector<int>> divide(const std::vector<int>& d);
        inline std::vector<int> merge(const std::vector<std::vector<int>>& d);
        inline std::vector<int> proc(const std::vector<int>& d);

        // ------------------------------------------------------------------
        // 基础工具
        // ------------------------------------------------------------------

        inline std::vector<int> rowGenerator(int n) {
            std::vector<int> row(static_cast<std::size_t>(n), 0);
            if (n > 0) row[0] = 1;
            return row;
        }

        inline int compareRow(const std::vector<int>& r1, const std::vector<int>& r2) {
            std::size_t i = 0;
            for (; i < r1.size() && i < r2.size(); ++i) {
                if (r1[i] > r2[i]) return 1;
                if (r1[i] < r2[i]) return -1;
            }
            if (r1.size() > i) return 1;
            if (r2.size() > i) return -1;
            return 0;
        }

        inline int compareDimension(const std::vector<int>& r1, const std::vector<int>& r2) {
            const int a = (r1.size() <= 1) ? 1 : r1[1];
            const int b = (r2.size() <= 1) ? 1 : r2[1];
            return a - b;
        }

        inline std::vector<int> rowAddition(const std::vector<int>& r1,
            const std::vector<int>& r2) {
            std::size_t i = 0, j = 0;
            while (true) {
                if (i >= r1.size()) {
                    std::vector<int> result;
                    result.reserve(i + (r2.size() - j));
                    result.insert(result.end(), r1.begin(), r1.begin() + static_cast<std::ptrdiff_t>(i));
                    result.insert(result.end(), r2.begin() + static_cast<std::ptrdiff_t>(j), r2.end());
                    return result;
                }
                if (j >= r2.size()) {
                    return r1;
                }
                if (r1[i] < r2[j]) {
                    std::vector<int> result;
                    result.reserve(i + (r2.size() - j));
                    result.insert(result.end(), r1.begin(), r1.begin() + static_cast<std::ptrdiff_t>(i));
                    result.insert(result.end(), r2.begin() + static_cast<std::ptrdiff_t>(j), r2.end());
                    return result;
                }
                if (r1[i] == r2[j] && j + 1 < r2.size() && r2[j + 1] > r2[j]) {
                    ++i;
                    ++j;
                    continue;
                }
                ++i;
            }
        }

        inline std::vector<int> rowDifference(const std::vector<int>& r1,
            const std::vector<int>& r2) {
            std::size_t i = 0, j = 0;
            while (true) {
                if (i >= r1.size()) return {};
                if (j >= r2.size() || r1[i] > r2[j]) {
                    std::vector<int> row(r1.begin() + static_cast<std::ptrdiff_t>(i), r1.end());
                    if (row.empty()) return row;
                    int k = static_cast<int>(i);
                    while (k-- >= 0) {
                        if (k >= 0 && static_cast<std::size_t>(k) < r1.size() &&
                            r1[static_cast<std::size_t>(k)] < row[0]) {
                            row.insert(row.begin(), r1[static_cast<std::size_t>(k)]);
                        }
                    }
                    return row;
                }
                ++i;
                ++j;
            }
        }

        inline std::vector<std::vector<int>> divide(const std::vector<int>& d) {
            std::vector<int> dd;
            if (d.size() > 1) {
                dd.assign(d.begin() + 1, d.end());
            }
            std::vector<std::vector<int>> ret;
            for (std::size_t i = 0; i + 1 < dd.size(); ++i) {
                while (dd[i]-- > 0) {
                    ret.push_back(rowGenerator(static_cast<int>(dd.size() - i)));
                }
            }
            if (!dd.empty() && dd.back() != 0) {
                ret.push_back({ dd.back() });
            }
            return ret;
        }

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

        inline std::vector<int> proc(const std::vector<int>& d) {
            if (d.size() > 2 && d[0] == 0 && d[1] > 0) {
                const std::vector<std::vector<int>> div = divide(d);
                if (div.size() > 1) {
                    return { static_cast<int>(div[0].size()) - 1 };
                }
                if (div.size() == 1 && div[0].size() > 2) {
                    return { static_cast<int>(div[0].size()) - 2 };
                }
            }
            return d;
        }

        inline bool isDimensionLimited(const Entry& it,
            const std::vector<int>& d,
            Arena& arena) {
            const std::vector<int> pd = proc(d);
            const std::vector<int> foot = getFootRow(it, d, arena);

            if (pd.size() == 1 && pd[0] != 0 && foot.size() > 1 && foot[1] > pd[0]) {
                return true;
            }
            if (d.size() == 2 && d[0] == 0 && d[1] > 0 &&
                foot.size() > static_cast<std::size_t>(d[1])) {
                return true;
            }
            return false;
        }

        inline std::vector<int> rowStandardization(const std::vector<int>& r,
            const std::vector<int>& d,
            Arena& arena) {
            if (r.size() <= 2 || r[r.size() - 1] <= r[r.size() - 2]) {
                return r;
            }

            std::vector<int> s(r.begin(), r.end() - 1);
            s[s.size() - 1]++;

            const std::vector<Entry*> seq = toSequence(s, arena);
            const std::vector<int> rr = expand_impl(seq, 2, d, false, arena);
            if (rr.size() > r.size() - 1 &&
                rr[r.size() - 1] <= r[r.size() - 1] - 1) {
                return rowStandardization(s, d, arena);
            }
            return r;
        }

        inline std::vector<int> getFootRow(const Entry& it,
            const std::vector<int>& d,
            Arena& arena) {
            if (it.parent == nullptr) return { 1 };

            if (compareRow(it.row, it.parent->row) == 0 ||
                d.empty() ||
                (d.size() == 1 && d[0] == 0) ||
                (d.size() == 2 && d[0] == 0) ||
                (d.size() == 3 && d[0] == 0 && d[1] == 1 && d[2] == 0)) {
                std::vector<int> row = it.row;
                row.push_back(1);
                return row;
            }

            int i = 0;
            std::vector<int> row = { 1 };
            while (true) {
                ++i;
                if (static_cast<std::size_t>(i) < it.row.size()) {
                    row.push_back(it.row[static_cast<std::size_t>(i)]);
                }
                else {
                    row.push_back(0);
                }

                if (it.parent->row.size() <= static_cast<std::size_t>(i) ||
                    it.parent->row[static_cast<std::size_t>(i)] <
                    it.row[static_cast<std::size_t>(i)]) {
                    break;
                }
            }
            row.back()++;
            return rowStandardization(row, { 0 }, arena);
        }

        // ------------------------------------------------------------------
        // 图结构处理
        // ------------------------------------------------------------------

        inline void setElementRefrence(std::vector<std::vector<Entry*>>& m) {
            for (std::vector<Entry*>& col : m) {
                for (Entry* e : col) {
                    if (e->row.size() <= 1 && e->value <= 1) continue;
                    if (e->parent == nullptr) continue;

                    if (compareRow(e->row, e->parent->row) == 0) {
                        e->ref = e->parent;
                    }
                    else if (e->head != nullptr && e->head->parent != nullptr &&
                        e->head->parent->foot != nullptr &&
                        compareRow(e->head->parent->foot->row, e->row) <= 0) {
                        e->ref = e->head->parent->foot;
                        while (e->ref != nullptr &&
                            compareRow(e->ref->row, e->row) == 0) {
                            e->ref = e->ref->ref;
                        }
                    }
                    else if (e->head != nullptr) {
                        e->ref = e->head->parent;
                    }
                }
            }
        }

        inline void setElementNo(std::vector<std::vector<Entry*>>& m, Entry* b) {
            int id = 0;
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
                    else if (e->ref != nullptr) {
                        e->no = e->ref->no;
                    }

                    if (static_cast<int>(i) > b->cloumn) {
                        e->id = id++;
                    }
                    if (static_cast<int>(i) == b->cloumn || i == m.size() - 1) {
                        e->id = e->no;
                    }
                }
            }
        }

        inline std::vector<Entry*> getReferenceChain(Entry* it) {
            std::vector<Entry*> c;
            while (true) {
                c.insert(c.begin(), it);
                if (it->value <= 1 && it->row.size() == 1) break;
                if (it->ref == nullptr) break;
                it = it->ref;
            }
            return c;
        }

        inline std::vector<Entry*> toSequence(const std::vector<int>& s, Arena& arena) {
            std::vector<Entry*> seq;
            seq.reserve(s.size());

            for (std::size_t i = 0; i < s.size(); ++i) {
                if (s[i] <= 1) {
                    Entry* root = arena.make_root();
                    seq.push_back(arena.make(s[i], {}, static_cast<int>(i), 0, 0, 0, root));
                    continue;
                }

                bool found = false;
                for (int j = static_cast<int>(i) - 1; j >= 0; --j) {
                    if (s[static_cast<std::size_t>(j)] < s[i]) {
                        seq.push_back(arena.make(s[i], {}, static_cast<int>(i), 0, 0, 0,
                            seq[static_cast<std::size_t>(j)]));
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    Entry* root = arena.make_root();
                    seq.push_back(arena.make(s[i], {}, static_cast<int>(i), 0, 0, 0, root));
                }
            }
            return seq;
        }

        inline std::vector<std::vector<Entry*>> drawMountain(
            const std::vector<Entry*>& s,
            const std::vector<int>& d,
            Arena& arena) {
            std::vector<std::vector<Entry*>> m;
            m.reserve(s.size());

            for (Entry* e : s) {
                Entry* parent = nullptr;
                if (e->parent == nullptr || e->parent->cloumn < 0) {
                    parent = arena.make_root();
                }
                else {
                    parent = m[static_cast<std::size_t>(e->parent->cloumn)][0];
                }

                Entry* first = arena.make(e->value, { 1 }, e->cloumn, 0, 0, 0, parent);
                m.push_back({ first });
            }

            for (std::size_t i = 0; i < m.size(); ++i) {
                Entry* it = m[i][0];
                while (it->value > 1) {
                    if (isDimensionLimited(*it, d, arena)) break;

                    std::vector<int> foot_row = getFootRow(*it, d, arena);
                    Entry* foot = arena.make(it->value - it->parent->value,
                        std::move(foot_row),
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

        inline std::vector<Entry*> getOds(const std::vector<std::vector<Entry*>>& m,
            Arena& arena) {
            std::vector<Entry*> o;
            o.reserve(m.size());

            for (const std::vector<Entry*>& col : m) {
                Entry* last = col.back();
                Entry* parent = nullptr;

                if (last->value <= 1) {
                    parent = arena.make_root();
                }
                else if (last->parent != nullptr && last->parent->cloumn >= 0) {
                    parent = o[static_cast<std::size_t>(last->parent->cloumn)];
                }
                else {
                    parent = arena.make_root();
                }

                o.push_back(arena.make(last->value, { 1 }, col[0]->cloumn, 0, 0, 0, parent));
            }
            return o;
        }

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

        inline std::pair<int, int> getBootIndex(const std::vector<Entry*>& s,
            const std::vector<int>& d,
            Arena& arena) {
            std::vector<std::vector<Entry*>> m = drawMountain(s, d, arena);
            setElementRefrence(m);

            Entry* t = m.back().back();
            if (t->value == 1 && t->head != nullptr) t = t->head;
            Entry* b = t->parent;

            if (b != nullptr && t->value - b->value > 1 && proc(d).size() == 1) {
                std::vector<Entry*> o = getOds(m, arena);

                std::vector<int> new_d;
                if (d.size() == 1 || divide(d).size() == 1) {
                    new_d = d;
                }
                else {
                    const std::vector<std::vector<int>> div = divide(d);
                    std::vector<std::vector<int>> sub(div.begin() + 1, div.end());
                    new_d = merge(sub);
                }

                const std::pair<int, int> boot = getBootIndex(o, new_d, arena);
                return { boot.first, static_cast<int>(m[static_cast<std::size_t>(boot.first)].size()) - 1 };
            }

            if (b != nullptr && compareDimension(b->row, t->row) < 0) {
                const std::vector<Entry*> ch = getReferenceChain(t);
                std::vector<int> dd = { 0, 1 };
                if (d.size() > 2 && d[0] == 0 && d[1] == 0) {
                    dd.assign(d.begin() + 2, d.end());
                }
                if (d.size() == 3 && d[0] == 1 && d[1] == 0 && d[2] == 1) {
                    dd = d;
                }

                const std::vector<Entry*> mds = getMds(m, arena);
                const std::pair<int, int> boot = getBootIndex(mds, dd, arena);
                const int c = ch[static_cast<std::size_t>(boot.first)]->cloumn;
                return { c, m[static_cast<std::size_t>(c)].back()->idx };
            }

            return { b != nullptr ? b->cloumn : -1, b != nullptr ? b->idx : 0 };
        }

        // ------------------------------------------------------------------
        // 复制与展开
        // ------------------------------------------------------------------

        inline void copyElement(const std::vector<std::vector<Entry*>>& m,
            Entry* b,
            Entry* t,
            Entry* it,
            std::vector<Entry*>& op,
            int i,
            const std::vector<int>& d,
            bool f,
            Arena& arena) {
            if (it->value <= 1 && it->row.size() > 1) {
                it->parent = it->ref;
                while (it->parent != nullptr && it->parent->value > 1) {
                    it->parent = it->parent->ref;
                }
            }

            std::vector<int> min_row =
                it->row.size() > 1 ? getFootRow(*op.back(), d, arena) : std::vector<int>{ 1 };
            std::vector<int> max_row = it->row;
            Entry* r = nullptr;

            {
                int c;
                if (f) {
                    c = it->ref->cloumn + (t->cloumn - b->cloumn) * (i + 1);
                }
                else {
                    c = t->cloumn + i;
                }
                r = m[static_cast<std::size_t>(c)][0];

                while (r->foot != nullptr && r->foot->id <= it->ref->id) {
                    r = r->foot;
                }

                std::vector<int> fr;
                if (it->foot == nullptr) {
                    fr = getFootRow(*it, d, arena);
                }
                else {
                    fr = it->foot->row;
                }

                if (compareRow(rowDifference(it->row, it->ref->row), { 1, 2 }) < 0) {
                    max_row = rowAddition(r->row, rowDifference(it->row, it->ref->row));
                }
                else {
                    const std::vector<Entry*> seq = toSequence(rowDifference(fr, it->ref->row), arena);
                    const std::vector<int> expanded = expand_impl(
                        seq,
                        static_cast<int>(it->row.size()) + i + 1,
                        { 0 },
                        false,
                        arena);
                    max_row = rowAddition(r->row, rowDifference(expanded, it->ref->row));
                }
            }

            if (it->cloumn == t->cloumn && it->idx == t->idx &&
                compareRow(rowDifference(it->row, it->ref->row), { 1, 2 }) >= 0) {
                Entry* p = t->parent;
                t->parent = b;

                const std::vector<Entry*> seq =
                    toSequence(rowDifference(getFootRow(*t, d, arena), t->ref->row), arena);
                const std::vector<int> expanded = expand_impl(
                    seq,
                    static_cast<int>(it->row.size()) + i + 1,
                    { 0 },
                    false,
                    arena);
                max_row = rowAddition(r->row, rowDifference(expanded, t->ref->row));

                t->parent = p;
            }

            if (d.empty() ||
                (d.size() == 1 && d[0] == 0) ||
                (d.size() == 2 && d[0] == 0) ||
                (d.size() == 3 && d[0] == 0 && d[1] == 1 && d[2] == 0)) {
                max_row = it->row;
            }

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

                int pc;
                if (f) {
                    pc = it->parent->cloumn >= b->cloumn
                        ? static_cast<int>(m.size()) - 1 + it->parent->cloumn - it->cloumn
                        : it->parent->cloumn;
                }
                else {
                    pc = it->parent->cloumn >= b->cloumn
                        ? static_cast<int>(m.size()) - 2
                        : it->parent->cloumn;
                }

                Entry* p = nullptr;
                if (pc >= 0) {
                    p = m[static_cast<std::size_t>(pc)].back();
                }
                else {
                    p = arena.make_root();
                }

                while (p != nullptr && compareRow(p->row, row) > 0) {
                    p = p->head;
                }
                op.back()->parent = p;

                row = getFootRow(*op.back(), d, arena);
            }
        }

        inline void copyCloumn(std::vector<std::vector<Entry*>>& m,
            Entry* b,
            Entry* t,
            int c,
            int i,
            const std::vector<int>& ex,
            const std::vector<int>& d,
            bool f,
            Arena& arena) {
            Entry* it = m[static_cast<std::size_t>(c)][0];
            m.push_back({});
            std::vector<Entry*>& new_col = m.back();

            while (true) {
                copyElement(m, b, t, it, new_col, i, d, f, arena);
                if (it->foot != nullptr) {
                    it = it->foot;
                }
                else {
                    break;
                }
            }

            const std::size_t new_idx = m.size() - 1;
            new_col.back()->value =
                (new_idx < ex.size()) ? ex[new_idx] : it->value;

            it = new_col.back();
            while (it->head != nullptr) {
                it->head->value = it->value + it->head->parent->value;
                it = it->head;
            }
        }

        inline std::vector<int> expand_impl(const std::vector<Entry*>& s,
            int n,
            const std::vector<int>& d,
            bool f,
            Arena& arena) {
            if (s.empty()) return {};
            if (s.back()->value <= 1) {
                std::vector<int> result;
                result.reserve(s.size() - 1);
                for (std::size_t i = 0; i + 1 < s.size(); ++i) {
                    result.push_back(s[i]->value);
                }
                return result;
            }

            std::vector<std::vector<Entry*>> m = drawMountain(s, d, arena);
            Entry* t = m.back().back();
            std::vector<int> ex;

            if (t->value == 1 && t->head != nullptr) t = t->head;
            Entry* b = t->parent;

            setElementRefrence(m);
            setElementNo(m, b);

            if (b != nullptr && t->value - b->value > 1 && proc(d).size() == 1) {
                std::vector<Entry*> o = getOds(m, arena);

                std::vector<int> new_d;
                if (d.size() == 1 || divide(d).size() == 1) {
                    new_d = d;
                }
                else {
                    const std::vector<std::vector<int>> div = divide(d);
                    std::vector<std::vector<int>> sub(div.begin() + 1, div.end());
                    new_d = merge(sub);
                }

                ex = expand_impl(o, n, new_d, f, arena);
            }
            else if (t->foot != nullptr) {
                m.back().pop_back();
                t->foot = nullptr;
                if (t->parent != nullptr) {
                    t->parent = t->parent->parent;
                }
            }

            for (Entry* e : m.back()) {
                e->value--;
            }

            for (int i = b->no; i < static_cast<int>(m[static_cast<std::size_t>(b->cloumn)].size()); ++i) {
                int idx = t->idx;
                Entry* src = m[static_cast<std::size_t>(b->cloumn)][static_cast<std::size_t>(i)];
                Entry* e = arena.make(src->value,
                    src->row,
                    t->cloumn,
                    ++idx,
                    src->no,
                    src->no,
                    src->parent,
                    m[static_cast<std::size_t>(t->cloumn)].back(),
                    nullptr,
                    src);
                m[static_cast<std::size_t>(t->cloumn)].push_back(e);
                m[static_cast<std::size_t>(t->cloumn)]
                    [m[static_cast<std::size_t>(t->cloumn)].size() - 2]
                    ->foot = e;
            }

            for (int i = 0; i < n; ++i) {
                if (f) {
                    for (int j = b->cloumn + 1; j <= t->cloumn; ++j) {
                        copyCloumn(m, b, t, j, i, ex, d, true, arena);
                    }
                }
                else {
                    copyCloumn(m, b, t, t->cloumn, i, ex, d, false, arena);
                }
            }

            std::vector<int> result;
            result.reserve(m.size());
            for (const std::vector<Entry*>& col : m) {
                result.push_back(col[0]->value);
            }
            return result;
        }

        inline std::vector<int> expand_entry(const std::vector<int>& seq, int term) {
            if (term < 1 || seq.empty()) return seq;

            Arena arena;
            const std::vector<Entry*> s = toSequence(seq, arena);

            // ε-Y 即 1-Y，维度序列为 {1}
            const std::vector<int> d = { 1 };
            return expand_impl(s, term, d, false, arena);
        }

    }  // namespace epsilon_y_detail

    struct EpsilonYNotation {
        static constexpr const char* kName = "\xCE\xB5-Y";  // UTF-8: ε-Y

        [[nodiscard]] static std::string suffix() {
            return {};
        }

        [[nodiscard]] static std::vector<int> expand(const std::vector<int>& seq,
            int term) {
            return epsilon_y_detail::expand_entry(seq, term);
        }
    };

}  // namespace omegay::notation
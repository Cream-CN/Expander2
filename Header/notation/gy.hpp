// Header/notation/gy.hpp
#pragma once

#include <vector>
#include <string>
#include <deque>
#include <cstddef>

namespace omegay::notation {

    struct GYNotation {
        static constexpr const char* kName = "G-Y";

        [[nodiscard]] static std::vector<int> expand(const std::vector<int>& seq, int term);

        [[nodiscard]] static std::string suffix() { return ""; }
    };

    namespace gy_detail {

        struct Element {
            int value = 0;
            int column = 0;          // 对应 JS 的 cloumn
            int idx = 0;
            std::vector<int> row;

            Element* parent = nullptr;
            Element* head = nullptr;
            Element* foot = nullptr;
            Element* ref = nullptr;

            int no = 0;
            int id = 0;
            int bs = 0;
            Element* br = nullptr;

            std::vector<int> offset;
            bool has_offset = false;

            std::vector<std::vector<int>> offsets;

            std::vector<int> max_row;
            bool has_max_row = false;

            std::vector<int> ex;
            bool has_ex = false;
            int n = 0;
        };

        class Arena {
            std::deque<Element> data_;
        public:
            Element* make() {
                data_.emplace_back();
                return &data_.back();
            }

            Element* make(int v, int col, std::vector<int> r, Element* p = nullptr) {
                data_.emplace_back();
                Element* e = &data_.back();
                e->value = v;
                e->column = col;
                e->row = std::move(r);
                e->parent = p;
                return e;
            }
        };

        inline int compareRow(const std::vector<int>& r1,
            const std::vector<int>& r2,
            int idx = -1) {
            std::size_t i = 0;
            for (; i < r1.size() && i < r2.size(); ++i) {
                if (static_cast<int>(i) == idx && r1[i] == r2[i]) return 0;
                if (r1[i] > r2[i]) return 1;
                if (r1[i] < r2[i]) return -1;
            }
            if (r1.size() > i) return 1;
            if (r2.size() > i) return -1;
            return 0;
        }

        inline std::vector<int> rowAddition(const std::vector<int>& r1,
            const std::vector<int>& r2) {
            if (r2.size() <= 1 || (r2.size() > 1 && r2[1] == 1)) {
                std::vector<int> res = r1;
                res.insert(res.end(), r2.begin(), r2.end());
                return res;
            }
            std::vector<int> row = r1;
            while (!row.empty() && row.back() == 1) row.pop_back();
            row.insert(row.end(), r2.begin(), r2.end());
            return row;
        }

        inline std::vector<int> rowDifference(const std::vector<int>& r1,
            const std::vector<int>& r2) {
            std::size_t i = 0;
            for (; i < r1.size(); ++i) {
                if (i >= r2.size() || r1[i] < r2[i]) break;
            }
            if (i < r2.size() && r2[i] > 1) {
                int j = static_cast<int>(i) - 1;
                while (j >= 0 && r2[j] > 1) --j;
                if (j > 0) i = static_cast<std::size_t>(j);
            }
            if (i >= r2.size()) return {};
            return std::vector<int>(r2.begin() + i, r2.end());
        }

        // 前向声明
        inline std::vector<Element*> toSequence(const std::vector<int>& s, Arena& arena);
        inline std::vector<std::vector<Element*>> drawMountain(
            const std::vector<Element*>& s,
            const std::vector<int>& d,
            Arena& arena);
        inline std::vector<int> expand_elements(
            const std::vector<Element*>& s,
            int n,
            const std::vector<int>& d,
            bool f,
            Arena& arena);
        inline std::vector<int> expand_impl(
            const std::vector<int>& seq,
            int n,
            const std::vector<int>& d,
            bool f);
        inline std::vector<int> getFootRow(
            Element* it,
            const std::vector<int>& d,
            Arena& arena);
        inline int getBootIndex(
            const std::vector<int>& s,
            const std::vector<int>& d,
            Arena& arena);
        inline void setElementRefrence(
            std::vector<std::vector<Element*>>& m,
            Arena& arena);
        inline void setElementNo(
            std::vector<std::vector<Element*>>& m,
            Element* b);
        inline Element* getTpElement(
            std::vector<std::vector<Element*>>& m,
            Element* t,
            int no);
        inline void expandDimensionSequnece(
            std::vector<std::vector<Element*>>& m,
            Element* b,
            Element* t,
            int n,
            const std::vector<int>& d,
            Arena& arena);
        inline void copyElement(
            std::vector<std::vector<Element*>>& m,
            Element* b,
            Element* t,
            Element* it,
            std::vector<Element*>& op,
            int i,
            const std::vector<int>& d,
            bool f,
            Arena& arena);
        inline void copyCloumn(
            std::vector<std::vector<Element*>>& m,
            Element* b,
            Element* t,
            int c,
            int i,
            std::vector<std::vector<int>>& ex,
            const std::vector<int>& d,
            bool f,
            Arena& arena);

        inline std::vector<Element*> toSequence(const std::vector<int>& s, Arena& arena) {
            std::vector<Element*> seq;
            for (std::size_t i = 0; i < s.size(); ++i) {
                if (s[i] <= 1) {
                    Element* e = arena.make();
                    e->value = s[i];
                    e->column = static_cast<int>(i);

                    Element* root = arena.make();
                    root->value = 0;
                    root->column = -1;
                    root->row = { 1 };
                    root->parent = nullptr;
                    e->parent = root;

                    seq.push_back(e);
                }
                else {
                    for (int j = static_cast<int>(i) - 1; j >= 0; --j) {
                        if (s[j] < s[i]) {
                            Element* e = arena.make();
                            e->value = s[i];
                            e->column = static_cast<int>(i);
                            e->parent = seq[j];
                            seq.push_back(e);
                            break;
                        }
                    }
                }
            }
            return seq;
        }

        inline std::vector<std::vector<Element*>> drawMountain(
            const std::vector<Element*>& s,
            const std::vector<int>& d,
            Arena& arena) {
            std::vector<std::vector<Element*>> m;
            for (Element* e : s) {
                Element* root = arena.make();
                root->value = e->value;
                root->row = { 1 };
                root->column = e->column;
                root->idx = 0;

                if (e->parent && e->parent->column >= 0) {
                    root->parent = m[e->parent->column][0];
                }
                else {
                    Element* fake = arena.make();
                    fake->value = 0;
                    fake->row = { 1 };
                    fake->column = -1;
                    fake->parent = nullptr;
                    root->parent = fake;
                }
                m.push_back({ root });
            }

            for (std::size_t i = 0; i < m.size(); ++i) {
                Element* it = m[i][0];
                while (it->value > 1) {
                    Element* foot = arena.make();
                    foot->value = it->value - it->parent->value;
                    foot->row = getFootRow(it, d, arena);
                    foot->column = static_cast<int>(i);
                    foot->idx = it->idx + 1;
                    foot->head = it;

                    it->foot = foot;
                    m[i].push_back(foot);

                    Element* p = it->parent;
                    if (p->foot && compareRow(p->foot->row, foot->row) <= 0) {
                        p = p->foot;
                    }
                    while (p->value >= foot->value) {
                        p = p->parent;
                    }
                    foot->parent = p;
                    it = foot;
                }
            }
            return m;
        }

        inline std::vector<int> getFootRow(
            Element* it,
            const std::vector<int>& d,
            Arena& arena) {
            if (compareRow(it->row, it->parent->row) == 0) {
                std::vector<int> res = it->row;
                res.push_back(1);
                return res;
            }

            std::vector<int> row = { 1, it->row[1] + 1 };
            int i = 1;

            while (true) {
                std::vector<int> ex = expand_impl(row, i, d, false);
                if (!ex.empty()) ex.pop_back();

                int c = compareRow(ex, it->row);
                if (c == 0 || (c < 0 && compareRow(it->parent->row, ex) < 0)) {
                    return row;
                }

                if (c < 0) {
                    ++i;
                }
                else {
                    if (compareRow(ex, it->row, static_cast<int>(it->row.size()) - 1) == 0) {
                        if (static_cast<int>(ex.size()) > static_cast<int>(it->row.size()) + 1) {
                            return std::vector<int>(ex.begin(), ex.begin() + it->row.size() + 1);
                        }
                        return ex;
                    }

                    std::size_t j = 1;
                    for (; j < ex.size() && j < it->row.size(); ++j) {
                        if (ex[j] > it->row[j]) break;
                    }

                    std::vector<int> newRow(ex.begin(), ex.begin() + j);
                    if (j < it->row.size()) {
                        newRow.push_back(it->row[j] + 1);
                    }
                    else {
                        newRow.push_back(1);
                    }
                    row = std::move(newRow);
                    i = 1;
                }
            }
        }

        inline void setElementRefrence(
            std::vector<std::vector<Element*>>& m,
            Arena& arena) {
            for (std::size_t i = 0; i < m.size(); ++i) {
                for (std::size_t j = 0; j < m[i].size(); ++j) {
                    Element* e = m[i][j];
                    if (e->row.size() <= 1 && e->value <= 1) {
                        Element* fake = arena.make();
                        fake->value = 0;
                        fake->row = { 1 };
                        fake->column = -1;
                        fake->parent = nullptr;
                        e->ref = fake;
                        continue;
                    }
                    if (compareRow(e->row, e->parent->row) == 0) {
                        e->ref = e->parent;
                    }
                    else {
                        e->ref = e->head->parent;
                    }
                }
            }
        }

        inline void setElementNo(
            std::vector<std::vector<Element*>>& m,
            Element* b) {
            int id = 0;
            for (std::size_t i = 0; i < m.size(); ++i) {
                for (std::size_t j = 0; j < m[i].size(); ++j) {
                    Element* e = m[i][j];

                    if (static_cast<int>(i) == b->column) {
                        e->no = static_cast<int>(j) + 1;
                    }
                    else if (static_cast<int>(i) < b->column ||
                        (e->value <= 1 && j == 0)) {
                        e->no = 0;
                    }
                    else {
                        e->no = e->ref ? e->ref->no : 0;
                    }

                    if (static_cast<int>(i) > b->column) {
                        e->id = id++;
                    }
                    if (static_cast<int>(i) == b->column ||
                        static_cast<int>(i) == static_cast<int>(m.size()) - 1) {
                        e->id = e->no;
                    }
                }
            }
        }

        inline Element* getTpElement(
            std::vector<std::vector<Element*>>& m,
            Element* t,
            int no) {
            Element* tp = m[t->column][0];
            while (tp->no <= no && tp->foot) {
                tp = tp->foot;
            }
            return tp;
        }

        inline int getBootIndex(
            const std::vector<int>& s,
            const std::vector<int>& d,
            Arena& arena) {
            auto seq = toSequence(s, arena);
            auto m = drawMountain(seq, d, arena);
            if (m.back().size() < 2) return -1;
            return m.back()[m.back().size() - 2]->parent->column;
        }

        inline void expandDimensionSequnece(
            std::vector<std::vector<Element*>>& m,
            Element* b,
            Element* t,
            int n,
            const std::vector<int>& d,
            Arena& arena) {
            if (n <= 0) return;

            for (int i = 0; i <= b->idx; ++i) {
                m[b->column][i]->bs = 0;
                m[b->column][i]->br = m[b->column][i];
            }

            for (int i = b->column + 1; i <= t->column; ++i) {
                for (std::size_t j = 0; j < m[i].size(); ++j) {
                    Element* it = m[i][j];
                    Element* tp = getTpElement(m, t, it->no);

                    if (it->no < 0) {
                        it->bs = 0;
                        it->br = it;
                        continue;
                    }

                    if (!tp->has_ex) {
                        tp->ex = expand_impl(tp->row, n, d, false);
                        tp->has_ex = true;
                        tp->n = n;
                    }

                    while (compareRow(tp->row, it->row) > 0 &&
                        !tp->row.empty() &&
                        tp->row.back() > 1 &&
                        compareRow(tp->ex, it->row) < 0) {
                        ++tp->n;
                        tp->ex = expand_impl(tp->row, tp->n, d, false);
                    }

                    int idx = getBootIndex(tp->row, d, arena);
                    if (idx >= 0 &&
                        compareRow(it->row, tp->ex, static_cast<int>(it->row.size()) - 1) == 0 &&
                        tp->row.size() > 1 &&
                        (tp->ex.size() - 1 - it->row.size()) % (tp->row.size() - 1 - idx) == 0) {
                        it->bs = (it->ref ? it->ref->bs : 0) + 1;
                        it->br = it;
                    }
                    else if (it->ref) {
                        it->bs = it->ref->bs;
                        it->br = it->ref->br;
                    }
                    else {
                        it->bs = 0;
                        it->br = it;
                    }
                }
            }

            for (int i = b->column + 1; i <= t->column; ++i) {
                for (std::size_t j = 0; j < m[i].size(); ++j) {
                    Element* it = m[i][j];

                    if (it->no <= 0) {
                        it->max_row = it->row;
                        it->has_max_row = true;
                        continue;
                    }

                    if (compareRow(rowDifference(it->ref->row, it->row), { 2 }) < 0) {
                        it->offset = rowDifference(it->ref->row, it->row);
                        it->has_offset = true;
                        continue;
                    }

                    Element* tp = getTpElement(m, t, it->no);
                    if (compareRow(it->row, tp->row) >= 0) {
                        it->max_row = it->row;
                        it->has_max_row = true;
                        continue;
                    }

                    if (tp->n < it->bs + tp->head->bs * n +
                        static_cast<int>(it->row.size()) -
                        static_cast<int>(tp->row.size())) {
                        tp->n = it->bs + tp->head->bs * n +
                            static_cast<int>(it->row.size()) -
                            static_cast<int>(tp->row.size());
                        tp->ex = expand_impl(tp->row, tp->n, d, false);
                    }

                    if (compareRow(it->row, tp->ex, static_cast<int>(it->row.size()) - 1) == 0 &&
                        compareRow(it->row, it->ref->row) > 0) {
                        it->max_row = tp->ex;
                        it->has_max_row = true;
                        continue;
                    }

                    std::vector<Element*> is = toSequence(it->row, arena);
                    std::vector<Element*> ts = toSequence(tp->row, arena);
                    int idx = getBootIndex(tp->row, d, arena);

                    if (idx < 0 || static_cast<int>(tp->row.size()) <= idx + 1) continue;

                    if (compareRow(it->row,
                        std::vector<int>(tp->row.begin(), tp->row.begin() + idx + 1)) <= 0) {
                        it->offset = rowDifference(it->ref->row, it->row);
                        it->has_offset = true;
                        continue;
                    }

                    if (static_cast<int>(is.size()) > idx &&
                        static_cast<int>(ts.size()) > idx + 1) {
                        ts[idx + 1]->parent = is[idx];
                        ts[idx + 1]->ref = is[idx];
                    }

                    std::vector<Element*> seq = is;
                    seq.insert(seq.end(), ts.begin() + idx + 1, ts.end());
                    for (std::size_t k = 0; k < seq.size(); ++k) {
                        seq[k]->column = static_cast<int>(k);
                    }

                    std::vector<int> ex = expand_elements(
                        seq,
                        n * tp->head->bs + it->bs,
                        d,
                        false,
                        arena);

                    it->offsets.clear();
                    int si = (it->bs > 0 ||
                        (tp->head->br && tp->head->br->column == t->column))
                        ? static_cast<int>(it->ref->row.size())
                        : static_cast<int>(b->row.size());

                    for (int k = 1; k <= n; ++k) {
                        int step = static_cast<int>(seq.size()) - 1 - idx;
                        int start = si + k * step * tp->head->bs;
                        int end = static_cast<int>(is.size()) + k * step * tp->head->bs;

                        if (start < 0) start = 0;
                        if (end > static_cast<int>(ex.size())) end = static_cast<int>(ex.size());

                        if (start < end) {
                            it->offsets.push_back(
                                std::vector<int>(ex.begin() + start, ex.begin() + end));
                        }
                        else {
                            it->offsets.push_back({});
                        }
                    }
                }
            }
        }

        inline void copyElement(
            std::vector<std::vector<Element*>>& m,
            Element* b,
            Element* t,
            Element* it,
            std::vector<Element*>& op,
            int i,
            const std::vector<int>& d,
            bool f,
            Arena& arena) {
            Element* pt = it->parent;

            if (it->row.size() > 1) {
                it->parent = it->head->parent;
            }
            if (it->parent->foot &&
                compareRow(it->parent->foot->row, it->row) <= 0) {
                it->parent = it->parent->foot;
            }
            while (it->parent->value > it->value) {
                it->parent = it->parent->parent;
            }

            std::vector<int> min_row =
                it->row.size() > 1 ? getFootRow(op.back(), d, arena)
                : std::vector<int>{ 1 };
            std::vector<int> max_row = it->row;

            Element* tp = getTpElement(m, t, it->no);
            if (tp->no > it->no) tp = tp->head;

            if (it->has_max_row) {
                max_row = it->max_row;
            }
            else if (it->has_offset ||
                (tp->br && tp->br->column == t->column) ||
                it->bs > 0) {
                int c = it->ref->column + (t->column - b->column) * (i + 1);
                if (c >= 0 && c < static_cast<int>(m.size())) {
                    Element* r = m[c][0];
                    while (r->foot && r->foot->id <= it->ref->id) {
                        r = r->foot;
                    }
                    if (it->has_offset) {
                        max_row = rowAddition(r->row, it->offset);
                    }
                    else if (i < static_cast<int>(it->offsets.size())) {
                        max_row = r->row;
                        max_row.insert(max_row.end(),
                            it->offsets[i].begin(),
                            it->offsets[i].end());
                    }
                }
            }
            else {
                if (!it->offsets.empty() && tp->br) {
                    if (compareRow(it->offsets[0],
                        rowDifference(tp->br->row, tp->row)) <= 0) {
                        int c = t->column + (t->column - b->column) * i;
                        int rid = tp->id;
                        if (c >= 0 && c < static_cast<int>(m.size())) {
                            Element* r = m[c][0];
                            while (r->foot && r->foot->id <= rid) {
                                r = r->foot;
                            }
                            max_row = r->row;
                            if (i < static_cast<int>(it->offsets.size())) {
                                max_row.insert(max_row.end(),
                                    it->offsets[i].begin(),
                                    it->offsets[i].end());
                            }
                        }
                    }
                    else {
                        int c = tp->br->column + (t->column - b->column) * (i + 1);
                        int rid = tp->br->id;
                        if (c >= 0 && c < static_cast<int>(m.size())) {
                            Element* r = m[c][0];
                            while (r->foot && r->foot->id <= rid) {
                                r = r->foot;
                            }
                            max_row = r->row;
                            if (i < static_cast<int>(it->offsets.size())) {
                                max_row.insert(max_row.end(),
                                    it->offsets[i].begin(),
                                    it->offsets[i].end());
                            }
                        }
                    }
                }
            }

            std::vector<int> row = min_row;
            while (compareRow(row, max_row) <= 0) {
                if (compareRow(row, max_row, static_cast<int>(row.size()) - 1) == 0) {
                    it->parent = pt;
                }

                Element* e = arena.make();
                e->value = it->value;
                e->row = row;
                e->column = static_cast<int>(m.size()) - 1;
                e->idx = static_cast<int>(op.size());
                e->no = it->no;
                e->id = it->id;

                op.push_back(e);
                if (op.size() > 1) {
                    op.back()->head = op[op.size() - 2];
                    op[op.size() - 2]->foot = op.back();
                }

                int pc = it->parent->column >= b->column
                    ? static_cast<int>(m.size()) - 1 + it->parent->column - it->column
                    : it->parent->column;

                Element* p = nullptr;
                if (pc >= 0 && pc < static_cast<int>(m.size())) {
                    p = m[pc].back();
                }
                else {
                    p = arena.make();
                    p->value = 0;
                    p->row = { 1 };
                    p->column = -1;
                    p->parent = nullptr;
                }

                while (compareRow(p->row, row) > 0 && p->head) {
                    p = p->head;
                }
                op.back()->parent = p;

                row = getFootRow(op.back(), d, arena);
            }
        }

        inline void copyCloumn(
            std::vector<std::vector<Element*>>& m,
            Element* b,
            Element* t,
            int c,
            int i,
            std::vector<std::vector<int>>& ex,
            const std::vector<int>& d,
            bool f,
            Arena& arena) {
            Element* it = m[c][0];
            m.push_back({});

            while (true) {
                copyElement(m, b, t, it, m.back(), i, d, f, arena);
                if (it->foot) {
                    it = it->foot;
                }
                else {
                    break;
                }
            }

            Element* last = m.back().back();
            last->value = it->value;

            Element* e = last;
            while (e->head) {
                e->head->value = e->value + e->head->parent->value;
                e = e->head;
            }
        }

        inline std::vector<int> expand_elements(
            const std::vector<Element*>& s,
            int n,
            const std::vector<int>& d,
            bool f,
            Arena& arena) {
            if (s.empty()) return {};

            if (s.back()->value <= 1) {
                std::vector<int> res;
                for (std::size_t i = 0; i + 1 < s.size(); ++i) {
                    res.push_back(s[i]->value);
                }
                return res;
            }

            auto m = drawMountain(s, d, arena);
            Element* t = m.back().back();
            if (t->value == 1) t = t->head;
            Element* b = t->parent;

            setElementRefrence(m, arena);
            setElementNo(m, b);
            expandDimensionSequnece(m, b, t, n, d, arena);

            if (t->foot) {
                m.back().pop_back();
                t->foot = nullptr;
                t->parent = t->parent->parent;
            }

            for (Element* e : m.back()) {
                --e->value;
            }

            for (int i = b->no; i < static_cast<int>(m[b->column].size()); ++i) {
                Element* src = m[b->column][i];
                Element* newElem = arena.make();
                newElem->value = src->value;
                newElem->row = src->row;
                newElem->column = t->column;
                newElem->max_row = src->row;
                newElem->idx = ++t->idx;
                newElem->no = src->no;
                newElem->id = src->no;
                newElem->parent = src->parent;
                newElem->head = m[t->column].back();
                newElem->ref = src;

                m[t->column].push_back(newElem);
                m[t->column][m[t->column].size() - 2]->foot = newElem;
            }

            std::vector<std::vector<int>> ex;
            for (int i = 0; i < n; ++i) {
                for (int j = b->column + 1; j <= t->column; ++j) {
                    copyCloumn(m, b, t, j, i, ex, d, f, arena);
                }
            }

            std::vector<int> result;
            result.reserve(m.size());
            for (auto& col : m) {
                result.push_back(col[0]->value);
            }
            return result;
        }

        inline std::vector<int> expand_impl(
            const std::vector<int>& seq,
            int n,
            const std::vector<int>& d,
            bool f) {
            Arena arena;
            auto s = toSequence(seq, arena);
            return expand_elements(s, n, d, f, arena);
        }

    } // namespace gy_detail

    [[nodiscard]] inline std::vector<int> GYNotation::expand(
        const std::vector<int>& seq,
        int term) {
        if (seq.empty() || term < 1) return seq;
        const std::vector<int> d = { 0 };
        return gy_detail::expand_impl(seq, term, d, true);
    }

} // namespace omegay::notation
#pragma once
#include "../core/arena.hpp"
#include "../core/entry.hpp"
#include <algorithm>
#include <climits>
#include <string>
#include <vector>

namespace omegay::notation {

    struct OmegaYNotation {
        static constexpr const char* kName = "ω-Y sequence";

        // ---------- 序列比较（与 JS 的 sequence_compare 一致）----------
        static int sequence_compare(const std::vector<int>& a,
            const std::vector<int>& b) {
            size_t n = std::min(a.size(), b.size());
            for (size_t i = 0; i < n; ++i) {
                if (a[i] < b[i]) return -1;
                if (a[i] > b[i]) return 1;
            }
            if (a.size() == b.size()) return 0;
            return a.size() < b.size() ? -1 : 1;
        }

        static bool Y_limit(const std::vector<int>& seq) {
            return !seq.empty() && seq.back() > 1;
        }

    private:
        using Mountain = std::vector<std::vector<core::Entry*>>;

        static int vertical_compare(const std::vector<int>& a,
            const std::vector<int>& b) {
            if (a.size() > b.size()) return 1;
            if (a.size() < b.size()) return -1;
            for (size_t i = a.size(); i-- > 0;) {
                if (a[i] > b[i]) return 1;
                if (a[i] < b[i]) return -1;
            }
            return 0;
        }

        static bool same_row(const core::Entry* a, const core::Entry* b) {
            if (!a || !b) return false;
            return vertical_compare(a->y, b->y) == 0;
        }

        static std::vector<int> vertical_increase(std::vector<int> y, int d) {
            if (d < 0) d = 0;
            if ((size_t)d >= y.size()) y.resize((size_t)d + 1, 0);
            if (y[d] == 0) y[d] = 1; else y[d] += 1;
            std::fill(y.begin(), y.begin() + d, 0);
            return y;
        }

        static int dimension_difference(const std::vector<int>& c1,
            const std::vector<int>& c2) {
            int d = (int)std::max(c1.size(), c2.size());
            while (d-- > 0) {
                int v1 = (size_t)d < c1.size() ? c1[d] : 0;
                int v2 = (size_t)d < c2.size() ? c2[d] : 0;
                if (v1 != v2) return d;
            }
            return -1;
        }

        static core::Entry* find_lower(std::vector<core::Entry*>& column,
            const std::vector<int>& y) {
            if (column.empty()) return nullptr;
            int i1 = 0, i2 = (int)column.size() - 1, i;
            while (i1 < i2) {
                i = (i1 + i2) / 2;
                if (vertical_compare(column[i]->y, y) < 0) i2 = i;
                else i1 = i + 1;
            }
            return column[i2];
        }

        static core::Entry* find_higherequal(std::vector<core::Entry*>& column,
            const std::vector<int>& y) {
            if (column.empty()) return nullptr;
            int i1 = 0, i2 = (int)column.size() - 1, i;
            while (i1 < i2) {
                i = (i1 + i2 + 1) / 2;
                if (vertical_compare(column[i]->y, y) >= 0) i1 = i;
                else i2 = i - 1;
            }
            return column[i1];
        }

        static std::vector<core::Entry*> yslice(std::vector<core::Entry*>& column,
            const std::vector<int>& lowequal,
            const std::vector<int>& high) {
            if (column.empty()) return {};
            int start = 0, end = (int)column.size();
            {
                int i1 = 0, i2 = (int)column.size() - 1, i;
                while (i1 < i2) {
                    i = (i1 + i2) / 2;
                    if (vertical_compare(column[i]->y, high) < 0) i2 = i;
                    else i1 = i + 1;
                }
                start = i2;
            }
            {
                int i1 = start, i2 = (int)column.size() - 1, i;
                while (i1 < i2) {
                    i = (i1 + i2) / 2;
                    if (vertical_compare(column[i]->y, lowequal) < 0) i2 = i;
                    else i1 = i + 1;
                }
                end = i2;
            }
            if (start >= end) return {};
            return std::vector<core::Entry*>(column.begin() + start,
                column.begin() + end);
        }

        static bool contains(const std::vector<core::Entry*>& v,
            core::Entry* e) {
            return std::find(v.begin(), v.end(), e) != v.end();
        }

        static std::vector<core::Entry*> collect_usual(
            core::Entry* working_entry,
            std::vector<core::Entry*> collection = {}) {
            if (!working_entry) return collection;
            for (auto* e : working_entry->leftleg_up) {
                if (!e) continue;
                auto* child = e->rightleg_down;
                if (!child) continue;
                if (contains(collection, child)) continue;
                if (same_row(working_entry, child)) {
                    collection.push_back(child);
                    collection = collect_usual(child, collection);
                }
            }
            return collection;
        }

        static std::vector<core::Entry*> collect1D(
            core::Entry* working_entry,
            std::vector<core::Entry*> collection = {}) {
            if (!working_entry || !working_entry->rightleg_down) return collection;
            for (auto* child : working_entry->rightleg_down->leftleg_up) {
                if (!child) continue;
                if (contains(collection, child)) continue;
                if (same_row(working_entry, child)) {
                    collection.push_back(child);
                    collection = collect1D(child, collection);
                }
            }
            return collection;
        }

        static std::vector<core::Entry*> collect(core::Entry* working_entry) {
            if (!working_entry) return {};
            if (vertical_compare(working_entry->y, { 1 }) > 0 &&
                working_entry->rightleg_down &&
                dimension_difference(working_entry->y,
                    working_entry->rightleg_down->y) == 0) {
                return collect1D(working_entry);
            }
            return collect_usual(working_entry);
        }

        // ---------- 建山 ----------
        static Mountain from_sequence(core::EntryArena& arena,
            const std::vector<int>& seq) {
            Mountain mountain;
            mountain.resize(seq.size());
            for (size_t i = 0; i < seq.size(); ++i) {
                auto* bottom = arena.make(seq[i], (int)i, { 1 });
                // phantom 用 INT_MAX 模拟 JS 的 undefined：
                // JS 中 undefined < x 恒 false，INT_MAX < x 也恒 false
                auto* phantom = arena.make(INT_MAX, (int)i, {});
                bottom->rightleg_down = phantom;
                phantom->rightleg_up = bottom;
                if (i > 0) {
                    bottom->leftleg_down = mountain[i - 1][1];
                    mountain[i - 1][1]->leftleg_up.push_back(bottom);
                }
                mountain[i] = { bottom, phantom };
            }
            return mountain;
        }

        static std::vector<int> to_sequence(const Mountain& mountain) {
            std::vector<int> out;
            out.reserve(mountain.size());
            for (auto& column : mountain) {
                if (column.size() < 2) {
                    // 理论不会发生；保险起见给个 0 占位，保持列数对齐
                    out.push_back(0);
                    continue;
                }
                out.push_back(column[column.size() - 2]->value);
            }
            return out;
        }

        static core::Entry* create_entry(core::EntryArena& arena,
            core::Entry* parent,
            core::Entry* entry) {
            int d = dimension_difference(parent->y, entry->y) + 1;
            auto* newentry = arena.make(entry->value - parent->value,
                entry->x,
                vertical_increase(entry->y, d));
            newentry->rightleg_down = entry;
            entry->rightleg_up = newentry;
            newentry->leftleg_down = parent;
            parent->leftleg_up.push_back(newentry);
            return newentry;
        }

        static Mountain draw_mountain(core::EntryArena& arena, Mountain mountain) {
            for (auto& column : mountain) {
                while (true) {
                    if (column.empty()) break;
                    auto* entry = column[0];
                    if (!entry) break;
                    if (entry->value == 1) break;
                    core::Entry* parent = entry;
                    while (true) {
                        auto* up = parent->leftleg_down;
                        if (!up) break;
                        while (up->rightleg_up &&
                            vertical_compare(up->rightleg_up->y, parent->y) <= 0)
                            up = up->rightleg_up;
                        parent = up;
                        if (parent->value < entry->value) break;
                    }
                    column.insert(column.begin(),
                        create_entry(arena, parent, entry));
                }
            }
            return mountain;
        }

        static void fill_magma_edge(core::EntryArena& arena, Mountain& mountain,
            core::Entry* source_entry,
            core::Entry* leftleg_entry) {
            if (!source_entry || !source_entry->leftleg_down) return;
            if (!leftleg_entry || !leftleg_entry->rightleg_up) return;
            int targetx = source_entry->x - source_entry->leftleg_down->x +
                leftleg_entry->x;
            if (targetx < 0) return;
            if (mountain.size() <= (size_t)targetx)
                mountain.resize((size_t)targetx + 1);

            int d = dimension_difference(leftleg_entry->y,
                leftleg_entry->rightleg_up->y);
            for (; d >= 0; --d) {
                auto* newentry = arena.make(0, targetx,
                    vertical_increase(leftleg_entry->y, d));
                newentry->leftleg_down = leftleg_entry;
                leftleg_entry->leftleg_up.push_back(newentry);
                mountain[targetx].push_back(newentry);
            }
        }

        static void copy_single_edge(core::EntryArena& arena, Mountain& mountain,
            core::Entry* source_entry, int x_offset,
            int BR_x,
            const std::vector<int>* targety = nullptr) {
            if (!source_entry) return;
            std::vector<int> ty = targety ? *targety : source_entry->y;
            int tx = source_entry->x + x_offset;
            if (tx < 0) return;
            if (mountain.size() <= (size_t)tx)
                mountain.resize((size_t)tx + 1);

            auto* newentry = arena.make(0, tx, ty);

            if (!source_entry->y.empty() && source_entry->leftleg_down) {
                int lx = source_entry->leftleg_down->x + x_offset;
                if (lx < 0) { mountain[tx].push_back(newentry); return; }
                if (mountain.size() <= (size_t)lx)
                    mountain.resize((size_t)lx + 1);

                core::Entry* leftleg_entry = nullptr;
                if (source_entry->leftleg_down->x >= BR_x) {
                    leftleg_entry = find_lower(mountain[lx], newentry->y);
                }
                else {
                    leftleg_entry = source_entry->leftleg_down;
                }
                if (leftleg_entry) {
                    newentry->leftleg_down = leftleg_entry;
                    leftleg_entry->leftleg_up.push_back(newentry);
                }
            }
            mountain[tx].push_back(newentry);
        }

        // ---------- 核心展开 ----------
        static std::vector<int> omega_Y_limit(core::EntryArena& arena,
            const std::vector<int>& seq,
            int FSterm) {
            Mountain mountain = draw_mountain(arena, from_sequence(arena, seq));
            if (mountain.empty()) return seq;

            auto& child = mountain.back();
            if (child.empty() || !child[0]) return seq;          // ★ child[0] 判空
            core::Entry* BR = child[0]->leftleg_down;
            if (!BR) return seq;                                  // ★ BR 判空
            if (BR->x < 0 || BR->x >= (int)mountain.size()) return seq;

            int width = (int)mountain.size() - 1 - BR->x;
            if (width <= 0) return seq;

            auto& brCol = mountain[BR->x];
            auto it = std::find(brCol.begin(), brCol.end(), BR);
            if (it == brCol.end() || it + 1 == brCol.end()) return seq;
            std::vector<core::Entry*> top(it, brCol.end() - 1);
            top.insert(top.begin(), child[0]);

            // 第二座山
            std::vector<int> s = seq;
            --s[s.size() - 1];
            mountain = draw_mountain(arena, from_sequence(arena, s));
            if (BR->x < 0 || BR->x >= (int)mountain.size()) return seq;

            auto& col2 = mountain[BR->x];
            auto it2 = std::find_if(col2.begin(), col2.end(),
                [&](core::Entry* e) { return same_row(e, BR); });
            if (it2 == col2.end()) return seq;
            BR = *it2;
            if (!BR) return seq;                                  // ★ 再判空

            std::vector<std::vector<core::Entry*>> magma_entries;
            for (core::Entry* BR1 = BR; BR1; BR1 = BR1->rightleg_down) {
                for (auto* entry : collect(BR1)) {
                    if (!entry) continue;
                    int dx = entry->x - BR->x;
                    if (dx <= 0) continue;
                    if ((size_t)dx >= magma_entries.size())
                        magma_entries.resize((size_t)dx + 1);
                    magma_entries[dx].push_back(entry);
                }
                if (BR1->y.empty()) break;
            }

            for (int n = 1; n <= FSterm; ++n) {
                std::vector<core::Entry*> ref;
                ref.reserve(top.size());
                for (auto* topentry : top) {
                    if (!topentry || mountain.empty()) continue;
                    auto* lo = find_lower(mountain.back(), topentry->y);
                    if (lo) ref.push_back(lo);
                }
                if (ref.empty()) break;

                for (int dx = 1; dx <= width; ++dx) {
                    int colx = BR->x + n * width + dx;
                    if (colx < 0) continue;
                    if (mountain.size() <= (size_t)colx)
                        mountain.resize((size_t)colx + 1);

                    mountain[colx].clear();
                    if ((size_t)dx >= magma_entries.size()) continue;

                    for (auto* magma_entry : magma_entries[dx]) {
                        if (!magma_entry) continue;
                        copy_single_edge(arena, mountain, magma_entry,
                            n * width, BR->x);

                        auto* source_entry = magma_entry;
                        auto* he = find_higherequal(ref, magma_entry->y);
                        if (!he) continue;
                        std::vector<int> targety = he->y;
                        std::vector<int> targety0 = targety;

                        while (source_entry &&
                            !(source_entry->value <= 1 ||
                                contains(magma_entries[dx],
                                    source_entry->rightleg_up))) {
                            if (!source_entry->rightleg_up) break;
                            targety = vertical_increase(
                                targety, dimension_difference(
                                    source_entry->y,
                                    source_entry->rightleg_up->y));
                            source_entry = source_entry->rightleg_up;
                            copy_single_edge(arena, mountain, source_entry,
                                n * width, BR->x, &targety);
                        }

                        if (magma_entry->y.empty() || !magma_entry->leftleg_down)
                            continue;

                        int leftlegx = magma_entry->leftleg_down->x + n * width;
                        if (leftlegx < 0 ||
                            (size_t)leftlegx >= mountain.size()) continue;

                        for (auto* leftleg_entry :
                            yslice(mountain[leftlegx], magma_entry->y, targety0)) {
                            fill_magma_edge(arena, mountain, magma_entry,
                                leftleg_entry);
                        }
                    }

                    auto& col = mountain[colx];
                    std::sort(col.begin(), col.end(),
                        [](core::Entry* a, core::Entry* b) {
                            return vertical_compare(a->y, b->y) > 0;
                        });
                    for (size_t i = 0; i + 1 < col.size(); ++i) {
                        col[i]->rightleg_down = col[i + 1];
                        col[i + 1]->rightleg_up = col[i];
                    }
                    if (col.empty()) continue;
                    col[0]->value = 1;
                    for (size_t i = 1; i + 1 < col.size(); ++i) {
                        auto* up = col[i]->rightleg_up;
                        if (!up || !up->leftleg_down) continue;
                        col[i]->value = up->value + up->leftleg_down->value;
                    }
                }
            }
            return to_sequence(mountain);
        }

    public:
        // ---------- 对外接口 ----------
        [[nodiscard]] static std::vector<int> expand(
            const std::vector<int>& seq, int term) {
            if (seq.empty()) return {};
            if (seq.size() == 1 && seq[0] == 0) return seq;
            if (seq.back() == 1) {
                return std::vector<int>(seq.begin(), seq.end() - 1);
            }
            // 单项且末项 > 1：参考实现未定义，原样返回
            if (seq.size() == 1) return seq;

            core::EntryArena arena;
            try {
                int eff = term + (int)seq.size();
                auto full = omega_Y_limit(arena, seq, eff);
                if (full.size() < 2) return full;
                return std::vector<int>(full.begin(), full.end() - 1);
            }
            catch (...) {
                return seq;
            }
        }

        // FSalter：保留末项
        [[nodiscard]] static std::vector<int> expand_alter(
            const std::vector<int>& seq, int term) {
            if (seq.empty()) return {};
            if (seq.back() == 1) {
                return std::vector<int>(seq.begin(), seq.end() - 1);
            }
            if (seq.size() == 1) return seq;

            core::EntryArena arena;
            try {
                int eff = term + (int)seq.size();
                return omega_Y_limit(arena, seq, eff);
            }
            catch (...) {
                return seq;
            }
        }

        [[nodiscard]] static std::string suffix() { return {}; }
    };

} // namespace omegay::notation
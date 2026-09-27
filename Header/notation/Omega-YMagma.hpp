#pragma once

#include "../core/entry.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace omegay::notation::omega_y_detail {

using omegay::core::Entry;

inline constexpr int kInfinity = (std::numeric_limits<int>::max)();
[[nodiscard]] inline bool is_limit_sentinel(const std::vector<int>& s) noexcept {
    return s.size() == 1 && s[0] == kInfinity;
}
[[nodiscard]] inline int vertical_compare(
    const std::vector<int>& a, const std::vector<int>& b) noexcept
{
    if (a.size() > b.size()) return 1;
    if (a.size() < b.size()) return -1;
    for (std::size_t i = a.size(); i-- > 0;) {
        if (a[i] > b[i]) return 1;
        if (a[i] < b[i]) return -1;
    }
    return 0;
}

[[nodiscard]] inline bool same_row(const Entry* e1, const Entry* e2) noexcept {
    return vertical_compare(e1->y, e2->y) == 0;
}
[[nodiscard]] inline std::vector<int> vertical_increase(
    const std::vector<int>& y, int d)
{
    std::vector<int> c = y;
    if (d < 0) return c;
    const std::size_t idx = static_cast<std::size_t>(d);
    if (idx >= c.size()) c.resize(idx + 1, 0);
    if (c[idx] == 0) c[idx] = 1; else c[idx] += 1;
    for (int i = 0; i < d && i < static_cast<int>(c.size()); ++i)
        c[static_cast<std::size_t>(i)] = 0;
    return c;
}
[[nodiscard]] inline int dimension_difference(
    const std::vector<int>& c1, const std::vector<int>& c2) noexcept
{
    int d = static_cast<int>((std::max)(c1.size(), c2.size()));
    while (d-- > 0) {
        const int v1 = (d < static_cast<int>(c1.size())) ? c1[static_cast<std::size_t>(d)] : 0;
        const int v2 = (d < static_cast<int>(c2.size())) ? c2[static_cast<std::size_t>(d)] : 0;
        const bool has1 = d < static_cast<int>(c1.size());
        const bool has2 = d < static_cast<int>(c2.size());
        if (has1 != has2) return d;
        if (has1 && has2 && v1 != v2) return d;
    }
    return -1;
}
struct Arena {
    std::vector<std::unique_ptr<Entry>> owned;

    Entry* make(int value, int x, std::vector<int> y) {
        owned.push_back(std::make_unique<Entry>(value, x, std::move(y)));
        return owned.back().get();
    }
};

using Mountain = std::vector<std::vector<Entry*>>;
[[nodiscard]] inline Mountain from_sequence(
    Arena& arena, const std::vector<int>& seq)
{
    Mountain mountain;
    mountain.reserve(seq.size());
    for (int i = 0; i < static_cast<int>(seq.size()); ++i) {
        Entry* bottom = arena.make(seq[static_cast<std::size_t>(i)], i, {1});
        Entry* phantom = arena.make(0, i, {});
        bottom->rightleg_down = phantom;
        phantom->rightleg_up  = bottom;
        if (i > 0) {
            bottom->leftleg_down = mountain[static_cast<std::size_t>(i - 1)][1];
            mountain[static_cast<std::size_t>(i - 1)][1]->leftleg_up.push_back(bottom);
        }
        mountain.push_back({ bottom, phantom });
    }
    return mountain;
}
[[nodiscard]] inline std::vector<int> to_sequence(const Mountain& mountain) {
    std::vector<int> out;
    out.reserve(mountain.size());
    for (const auto& column : mountain) {
        if (column.size() < 2) { out.push_back(0); continue; }
        out.push_back(column[column.size() - 2]->value);
    }
    return out;
}
[[nodiscard]] inline Entry* create_entry(Arena& arena, Entry* parent, Entry* entry) {
    const int dd = dimension_difference(parent->y, entry->y) + 1;
    Entry* newentry = arena.make(
        entry->value - parent->value,
        entry->x,
        vertical_increase(entry->y, dd));
    newentry->rightleg_down = entry;
    entry->rightleg_up      = newentry;
    newentry->leftleg_down  = parent;
    parent->leftleg_up.push_back(newentry);
    return newentry;
}
inline Mountain draw_mountain(Arena& arena, Mountain mountain) {
    for (auto& column : mountain) {
        while (true) {
            Entry* entry = column[0];
            if (entry->value == 1) break;
            Entry* parent = entry;
            while (true) {
                Entry* up = parent->leftleg_down;
                while (up->rightleg_up &&
                       vertical_compare(up->rightleg_up->y, parent->y) <= 0) {
                    up = up->rightleg_up;
                }
                parent = up;
                if (parent->value < entry->value) break;
            }
            column.insert(column.begin(), create_entry(arena, parent, entry));
        }
    }
    return mountain;
}
[[nodiscard]] inline Entry* find_lower(
    const std::vector<Entry*>& column, const std::vector<int>& y) noexcept
{
    int i1 = 0, i2 = static_cast<int>(column.size()) - 1;
    while (i1 < i2) {
        const int i = (i1 + i2) / 2;
        if (vertical_compare(column[static_cast<std::size_t>(i)]->y, y) < 0) i2 = i;
        else i1 = i + 1;
    }
    return column[static_cast<std::size_t>(i2)];
}

[[nodiscard]] inline Entry* find_higherequal(
    const std::vector<Entry*>& column, const std::vector<int>& y) noexcept
{
    int i1 = 0, i2 = static_cast<int>(column.size()) - 1;
    while (i1 < i2) {
        const int i = (i1 + i2 + 1) / 2; // ceil
        if (vertical_compare(column[static_cast<std::size_t>(i)]->y, y) >= 0) i1 = i;
        else i2 = i - 1;
    }
    return column[static_cast<std::size_t>(i1)];
}

[[nodiscard]] inline std::vector<Entry*> yslice(
    const std::vector<Entry*>& column,
    const std::vector<int>& lowequal,
    const std::vector<int>& high)
{
    int i1 = 0, i2 = static_cast<int>(column.size()) - 1;
    while (i1 < i2) {
        const int i = (i1 + i2) / 2;
        if (vertical_compare(column[static_cast<std::size_t>(i)]->y, high) < 0) i2 = i;
        else i1 = i + 1;
    }
    const int start = i2;

    i1 = start; i2 = static_cast<int>(column.size()) - 1;
    while (i1 < i2) {
        const int i = (i1 + i2) / 2;
        if (vertical_compare(column[static_cast<std::size_t>(i)]->y, lowequal) < 0) i2 = i;
        else i1 = i + 1;
    }
    return std::vector<Entry*>(column.begin() + start, column.begin() + i2);
}
inline void collect_weak(Entry* working, std::vector<Entry*>& collection) {
    for (Entry* e : working->leftleg_up) {
        Entry* child = e->rightleg_down;
        if (std::find(collection.begin(), collection.end(), child) != collection.end())
            continue;
        if (same_row(working, child)) {
            collection.push_back(child);
            collect_weak(child, collection);
        }
    }
}

inline void collect_strong(Entry* working, std::vector<Entry*>& collection) {
    if (!working->rightleg_down) return;
    for (Entry* child : working->rightleg_down->leftleg_up) {
        if (std::find(collection.begin(), collection.end(), child) != collection.end())
            continue;
        if (same_row(working, child)) {
            collection.push_back(child);
            collect_strong(child, collection);
        }
    }
}
inline void fill_magma_edge(
    Arena& arena, Mountain& mountain,
    Entry* source_entry, Entry* leftleg_entry)
{
    const int targetx =
        source_entry->x - source_entry->leftleg_down->x + leftleg_entry->x;
    for (int d = dimension_difference(leftleg_entry->y, leftleg_entry->rightleg_up->y);
         d >= 0; --d)
    {
        Entry* newentry = arena.make(0, targetx,
            vertical_increase(leftleg_entry->y, d));
        newentry->leftleg_down = leftleg_entry;
        leftleg_entry->leftleg_up.push_back(newentry);
        mountain[static_cast<std::size_t>(targetx)].push_back(newentry);
    }
}
inline void copy_single_edge(
    Arena& arena, Mountain& mountain,
    Entry* source_entry, int x_offset, int BR_x,
    const std::vector<int>* targety = nullptr)
{
    const std::vector<int> ty = targety ? *targety : source_entry->y;
    Entry* newentry = arena.make(0, source_entry->x + x_offset, ty);
    if (!source_entry->y.empty()) { // underground 没有这条
        Entry* leftleg_entry = nullptr;
        if (source_entry->leftleg_down->x >= BR_x) {
            leftleg_entry = find_lower(
                mountain[static_cast<std::size_t>(source_entry->leftleg_down->x + x_offset)],
                newentry->y);
        } else {
            leftleg_entry = source_entry->leftleg_down;
        }
        newentry->leftleg_down = leftleg_entry;
        leftleg_entry->leftleg_up.push_back(newentry);
    }
    mountain[static_cast<std::size_t>(source_entry->x + x_offset)].push_back(newentry);
}
enum class MagmaKind { Medium, Strong };

[[nodiscard]] inline std::vector<int> magma_expand(
    const std::vector<int>& seq, int FSterm, MagmaKind kind)
{
    if (seq.empty()) return {};
    if (seq.back() == 1) {
        return std::vector<int>(seq.begin(), seq.end() - 1);
    }

    Arena arena;
    Mountain mountain = draw_mountain(arena, from_sequence(arena, seq));

    std::vector<Entry*>& child = mountain.back();
    Entry* BR = child[0]->leftleg_down;
    const int width = static_cast<int>(mountain.size()) - 1 - BR->x;
    std::vector<Entry*> top;
    {
        auto& col = mountain[static_cast<std::size_t>(BR->x)];
        auto it = std::find(col.begin(), col.end(), BR);
        top.assign(it, col.end() - 1);
        top.insert(top.begin(), child[0]);
    }
    std::vector<int> s = seq;
    s.back() -= 1;
    mountain = draw_mountain(arena, from_sequence(arena, s));
    {
        auto& col = mountain[static_cast<std::size_t>(BR->x)];
        for (Entry* e : col) {
            if (same_row(e, BR)) { BR = e; break; }
        }
    }
    std::vector<std::vector<Entry*>> magma_entries(
        static_cast<std::size_t>(width) + 1);

    if (kind == MagmaKind::Medium) {
        for (Entry* BR1 = BR; ; BR1 = BR1->rightleg_down) {
            std::vector<Entry*> collection;
            collect_weak(BR1, collection);
            for (Entry* entry : collection) {
                const int dx = entry->x - BR->x;
                if (dx >= 0 && dx < static_cast<int>(magma_entries.size()))
                    magma_entries[static_cast<std::size_t>(dx)].push_back(entry);
            }
            if (BR1->y.empty()) break;
        }
    } else { // Strong
        for (Entry* BR1 = BR; ; BR1 = BR1->rightleg_down) {
            if (!BR1->y.empty()) {
                std::vector<Entry*> collection;
                collect_strong(BR1, collection);
                for (Entry* entry : collection) {
                    const int dx = entry->x - BR->x;
                    if (dx >= 0 && dx < static_cast<int>(magma_entries.size()))
                        magma_entries[static_cast<std::size_t>(dx)].push_back(entry);
                }
            } else {
                for (int dx1 = 0;
                     BR->x + 1 + dx1 < static_cast<int>(mountain.size()); ++dx1) {
                    auto& col = mountain[static_cast<std::size_t>(BR->x + 1 + dx1)];
                    const int dx = dx1 + 1;
                    if (dx < static_cast<int>(magma_entries.size()))
                        magma_entries[static_cast<std::size_t>(dx)].push_back(col.back());
                }
                break;
            }
        }
    }

    for (int n = 1; n <= FSterm; ++n) {
        std::vector<Entry*> ref;
        ref.reserve(top.size());
        for (Entry* t : top)
            ref.push_back(find_lower(mountain.back(), t->y));

        for (int dx = 1; dx <= width; ++dx) {
            std::vector<Entry*> column;
            mountain.resize(static_cast<std::size_t>(BR->x + n * width + dx) + 1);
            mountain[static_cast<std::size_t>(BR->x + n * width + dx)] = column;

            for (Entry* magma_entry : magma_entries[static_cast<std::size_t>(dx)]) {
                copy_single_edge(arena, mountain, magma_entry, n * width, BR->x);

                Entry* source_entry = magma_entry;
                std::vector<int> targety = find_higherequal(ref, magma_entry->y)->y;
                const std::vector<int> targety0 = targety;
                while (!(source_entry->value <= 1 ||
                         std::find(magma_entries[static_cast<std::size_t>(dx)].begin(),
                                   magma_entries[static_cast<std::size_t>(dx)].end(),
                                   source_entry->rightleg_up) !=
                             magma_entries[static_cast<std::size_t>(dx)].end()))
                {
                    targety = vertical_increase(
                        targety,
                        dimension_difference(source_entry->y,
                                             source_entry->rightleg_up->y));
                    source_entry = source_entry->rightleg_up;
                    copy_single_edge(arena, mountain, source_entry, n * width, BR->x,
                                     &targety);
                }
                if (magma_entry->y.empty()) continue;
                const int leftlegx = magma_entry->leftleg_down->x + n * width;
                for (Entry* leftleg_entry :
                     yslice(mountain[static_cast<std::size_t>(leftlegx)],
                            magma_entry->y, targety0))
                {
                    fill_magma_edge(arena, mountain, magma_entry, leftleg_entry);
                }
            }
            std::sort(mountain[static_cast<std::size_t>(BR->x + n * width + dx)].begin(),
                      mountain[static_cast<std::size_t>(BR->x + n * width + dx)].end(),
                      [](Entry* a, Entry* b) {
                          return vertical_compare(a->y, b->y) > 0;
                      });

            auto& col = mountain[static_cast<std::size_t>(BR->x + n * width + dx)];
            for (std::size_t i = 0; i + 1 < col.size(); ++i) {
                col[i]->rightleg_down = col[i + 1];
                col[i + 1]->rightleg_up = col[i];
            }
            if (!col.empty()) col[0]->value = 1;
            for (std::size_t i = 1; i + 1 < col.size(); ++i) {
                col[i]->value = col[i]->rightleg_up->value +
                                col[i]->rightleg_up->leftleg_down->value;
            }
        }
    }

    return to_sequence(mountain);
}

} // namespace omegay::notation::omega_y_detail
namespace omegay::notation {

struct OmegaYMediumNotation {
    static constexpr const char* kName = "ω-Y (medium magma)";

    [[nodiscard]] static std::vector<int> expand(
        const std::vector<int>& seq, int term)
    {
        using namespace omega_y_detail;
        try {
            if (seq.empty()) return {};
            if (is_limit_sentinel(seq)) return { 1, 1 + term };
            if (seq.back() == 1)
                return std::vector<int>(seq.begin(), seq.end() - 1);
            return magma_expand(seq, term, MagmaKind::Medium);
        } catch (...) {
            return {};
        }
    }
    [[nodiscard]] static std::string suffix() { return {}; }
};

struct OmegaYStrongNotation {
    static constexpr const char* kName = "ω-Y (strong magma)";

    [[nodiscard]] static std::vector<int> expand(
        const std::vector<int>& seq, int term)
    {
        using namespace omega_y_detail;
        try {
            if (seq.empty()) return {};
            if (is_limit_sentinel(seq)) return { 1, 1 + term };
            if (seq.back() == 1)
                return std::vector<int>(seq.begin(), seq.end() - 1);
            return magma_expand(seq, term, MagmaKind::Strong);
        } catch (...) {
            return {};
        }
    }
    [[nodiscard]] static std::string suffix() { return {}; }
};

} // namespace omegay::notation
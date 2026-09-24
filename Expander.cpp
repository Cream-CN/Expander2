#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <format>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
namespace {
    constexpr int IDM_ABOUT = 1001;
    constexpr int IDM_EXIT = 1002;
    constexpr int IDM_HELP = 1003;
    constexpr int IDM_LEGAL = 1004;
    constexpr int IDM_FS = 101;
    constexpr int IDM_FSALTER = 102;
    enum class MagmaType : int { Weak = 0, Medium = 1, Strong = 2 };
    [[nodiscard]] std::string wstring_to_utf8(std::wstring_view wstr) {
        if (wstr.empty()) return {};
        const int size_needed = ::WideCharToMultiByte(
            CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()),
            nullptr, 0, nullptr, nullptr);
        std::string out(static_cast<size_t>(size_needed), '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()),
            out.data(), size_needed, nullptr, nullptr);
        return out;
    }
    [[nodiscard]] std::wstring utf8_to_wstring(std::string_view str) {
        if (str.empty()) return {};
        const int size_needed = ::MultiByteToWideChar(
            CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
        std::wstring out(static_cast<size_t>(size_needed), L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
            out.data(), size_needed);
        return out;
    }
    struct Entry {
        int value = 0;
        int x = 0;
        std::vector<int> y;
        uint64_t ykey = 0;
        std::vector<Entry*> leftleg_up;
        Entry* rightleg_up = nullptr;
        Entry* rightleg_down = nullptr;
        Entry* leftleg_down = nullptr;
        Entry() = default;
        Entry(int v, int x_, std::vector<int> y_)
            : value(v), x(x_), y(std::move(y_)) {
            refresh_key();
        }
        void refresh_key() noexcept {
            uint64_t k = static_cast<uint64_t>(y.size() & 0xFF) << 56;
            const size_t n = std::min<size_t>(y.size(), 7);
            for (size_t i = 0; i < n; ++i)
                k |= (static_cast<uint64_t>(y[i] & 0xFFFF) << (i * 8));
            ykey = k;
        }
    };
    [[nodiscard]] inline uint64_t make_ykey(const std::vector<int>& y) noexcept {
        uint64_t k = static_cast<uint64_t>(y.size() & 0xFF) << 56;
        const size_t n = std::min<size_t>(y.size(), 7);
        for (size_t i = 0; i < n; ++i)
            k |= (static_cast<uint64_t>(y[i] & 0xFFFF) << (i * 8));
        return k;
    }
    class EntryArena {
    public:
        Entry* make(int v = 0, int x = 0, std::vector<int> y = {}) {
            nodes_.push_back(std::make_unique<Entry>(v, x, std::move(y)));
            return nodes_.back().get();
        }
        Entry* make_empty() {
            nodes_.push_back(std::make_unique<Entry>());
            return nodes_.back().get();
        }
        void clear() noexcept { nodes_.clear(); }
        [[nodiscard]] size_t size() const noexcept { return nodes_.size(); }
    private:
        std::vector<std::unique_ptr<Entry>> nodes_;
    };
    using Mountain = std::vector<std::vector<Entry*>>;
    [[nodiscard]] inline int vertical_compare(const Entry* a, const Entry* b) noexcept {
        if (a->ykey == b->ykey) return 0;
        return a->ykey > b->ykey ? 1 : -1;
    }
    [[nodiscard]] inline bool same_row(const Entry* a, const Entry* b) noexcept {
        return a && b && a->ykey == b->ykey;
    }
    [[nodiscard]] inline bool key_greater(const Entry* a, const Entry* b) noexcept {
        return a->ykey > b->ykey;
    }
    [[nodiscard]] inline int dimension_difference(const std::vector<int>& c1,
        const std::vector<int>& c2) noexcept {
        const int maxd = static_cast<int>(std::max(c1.size(), c2.size()));
        for (int d = maxd - 1; d >= 0; --d) {
            const int v1 = d < static_cast<int>(c1.size()) ? c1[d] : 0;
            const int v2 = d < static_cast<int>(c2.size()) ? c2[d] : 0;
            if (v1 != v2) return d;
        }
        return -1;
    }
    [[nodiscard]] inline std::vector<int> vertical_increase(const std::vector<int>& y, int d) {
        std::vector<int> c = y;
        if (d >= static_cast<int>(c.size())) {
            c.resize(static_cast<size_t>(d) + 1, 0);
            c[static_cast<size_t>(d)] = 1;
        }
        else {
            ++c[static_cast<size_t>(d)];
        }
        std::fill_n(c.begin(), d, 0);
        return c;
    }
    [[nodiscard]] Entry* create_entry(EntryArena& arena, Entry* parent, Entry* entry) {
        Entry* newentry = arena.make_empty();
        newentry->value = entry->value - parent->value;
        newentry->x = entry->x;
        newentry->y = vertical_increase(
            entry->y, dimension_difference(parent->y, entry->y) + 1);
        newentry->refresh_key();                              // ← 刷新
        newentry->rightleg_down = entry;
        entry->rightleg_up = newentry;
        newentry->leftleg_down = parent;
        parent->leftleg_up.push_back(newentry);
        return newentry;
    }
    [[nodiscard]] Mountain from_sequence(EntryArena& arena, const std::vector<int>& seq) {
        Mountain mountain;
        mountain.reserve(seq.size());
        for (int i = 0; i < static_cast<int>(seq.size()); ++i) {
            Entry* bottom = arena.make(seq[i], i, { 1 });
            Entry* phantom = arena.make(0, i, {});
            bottom->rightleg_down = phantom;
            phantom->rightleg_up = bottom;
            if (i > 0) {
                bottom->leftleg_down = mountain[static_cast<size_t>(i) - 1][1];
                mountain[static_cast<size_t>(i) - 1][1]->leftleg_up.push_back(bottom);
            }
            mountain.push_back({ bottom, phantom });
        }
        return mountain;
    }
    [[nodiscard]] std::vector<int> to_sequence(const Mountain& mountain) {
        std::vector<int> seq;
        seq.reserve(mountain.size());
        for (const auto& col : mountain) {
            if (col.size() >= 2) {
                seq.push_back(col[col.size() - 2]->value);
            }
        }
        return seq;
    }
    [[nodiscard]] Entry* find_lower(const std::vector<Entry*>& column,
        const std::vector<int>& y) {
        if (column.empty()) return nullptr;
        const uint64_t target = make_ykey(y);
        int i1 = 0, i2 = static_cast<int>(column.size()) - 1;
        while (i1 < i2) {
            int i = (i1 + i2) / 2;
            if (column[static_cast<size_t>(i)]->ykey < target)
                i2 = i;
            else
                i1 = i + 1;
        }
        return column[static_cast<size_t>(i2)];
    }
    [[nodiscard]] Entry* find_higherequal(const std::vector<Entry*>& column,
        const std::vector<int>& y) {
        if (column.empty()) return nullptr;
        const uint64_t target = make_ykey(y);
        int i1 = 0, i2 = static_cast<int>(column.size()) - 1;
        while (i1 < i2) {
            int i = (i1 + i2 + 1) / 2;
            if (column[static_cast<size_t>(i)]->ykey >= target)
                i1 = i;
            else
                i2 = i - 1;
        }
        return column[static_cast<size_t>(i1)];
    }
    [[nodiscard]] std::vector<Entry*> yslice(const std::vector<Entry*>& column,
        const std::vector<int>& lowequal,
        const std::vector<int>& high) {
        if (column.empty()) return {};
        const uint64_t low_key = make_ykey(lowequal);
        const uint64_t high_key = make_ykey(high);
        int i1 = 0, i2 = static_cast<int>(column.size()) - 1;
        while (i1 < i2) {
            int i = (i1 + i2) / 2;
            if (column[static_cast<size_t>(i)]->ykey < high_key)
                i2 = i;
            else
                i1 = i + 1;
        }
        const int start = i2;
        i1 = start;
        i2 = static_cast<int>(column.size()) - 1;
        while (i1 < i2) {
            int i = (i1 + i2) / 2;
            if (column[static_cast<size_t>(i)]->ykey < low_key)
                i2 = i;
            else
                i1 = i + 1;
        }
        std::vector<Entry*> result;
        for (int i = start; i < i2 && i < static_cast<int>(column.size()); ++i)
            result.push_back(column[static_cast<size_t>(i)]);
        return result;
    }
    [[nodiscard]] std::vector<Entry*> collect_usual(
        Entry* working_entry, std::vector<Entry*> collection = {}) {
        for (Entry* e : working_entry->leftleg_up) {
            Entry* child = e->rightleg_down;
            if (std::ranges::find(collection, child) != collection.end()) continue;
            if (same_row(working_entry, child)) {
                collection.push_back(child);
                collect_usual(child, collection);
            }
        }
        return collection;
    }
    [[nodiscard]] std::vector<Entry*> collect1D(
        Entry* working_entry, std::vector<Entry*> collection = {}) {
        for (Entry* child : working_entry->rightleg_down->leftleg_up) {
            if (std::ranges::find(collection, child) != collection.end()) continue;
            if (same_row(working_entry, child)) {
                collection.push_back(child);
                collect1D(child, collection);
            }
        }
        return collection;
    }
    inline void ensure_column(Mountain& m, int idx) {
        if (idx >= static_cast<int>(m.size()))
            m.resize(static_cast<size_t>(idx) + 1);
    }
    inline void insert_sorted_desc(std::vector<Entry*>& column, Entry* e) {
        if (column.empty()) { column.push_back(e); return; }
        if (column.back()->ykey >= e->ykey) {
            column.push_back(e);
            return;
        }
        auto it = std::lower_bound(column.begin(), column.end(), e,
            [](const Entry* a, const Entry* b) { return a->ykey > b->ykey; });
        column.insert(it, e);
    }
    inline void sort_column_desc_if_needed(std::vector<Entry*>& column) {
        if (std::ranges::is_sorted(column, key_greater)) return;
        std::ranges::sort(column, key_greater);
    }
    inline void link_vertical(std::vector<Entry*>& column) {
        for (size_t i = 0; i + 1 < column.size(); ++i) {
            if (!column[i] || !column[i + 1]) continue;
            column[i]->rightleg_down = column[i + 1];
            column[i + 1]->rightleg_up = column[i];
        }
        if (!column.empty() && column[0]) column[0]->value = 1;
        for (size_t i = 1; i + 1 < column.size(); ++i) {
            auto* e = column[i];
            if (!e || !e->rightleg_up || !e->rightleg_up->leftleg_down) continue;
            e->value = e->rightleg_up->value + e->rightleg_up->leftleg_down->value;
        }
    }
    void fill_magma_edge(EntryArena& arena, Mountain& mountain,
        Entry* source_entry, Entry* leftleg_entry) {
        const int targetx = source_entry->x - source_entry->leftleg_down->x + leftleg_entry->x;
        ensure_column(mountain, targetx);
        int d = dimension_difference(leftleg_entry->y, leftleg_entry->rightleg_up->y);
        for (; d >= 0; --d) {
            Entry* newentry = arena.make_empty();
            newentry->x = targetx;
            newentry->y = vertical_increase(leftleg_entry->y, d);
            newentry->refresh_key();
            newentry->leftleg_down = leftleg_entry;
            leftleg_entry->leftleg_up.push_back(newentry);
            insert_sorted_desc(mountain[static_cast<size_t>(targetx)], newentry);
        }
    }
    void copy_single_edge(EntryArena& arena, Mountain& mountain,
        Entry* source_entry, int x_offset, int BR_x,
        const std::vector<int>& targety = {}) {
        std::vector<int> ty = targety.empty() ? source_entry->y : targety;
        Entry* newentry = arena.make_empty();
        newentry->x = source_entry->x + x_offset;
        newentry->y = std::move(ty);
        newentry->refresh_key();
        if (!source_entry->y.empty()) {
            Entry* leftleg_entry;
            if (source_entry->leftleg_down->x >= BR_x) {
                const int col_idx = source_entry->leftleg_down->x + x_offset;
                ensure_column(mountain, col_idx);
                leftleg_entry = find_lower(mountain[static_cast<size_t>(col_idx)], newentry->y);
            }
            else {
                leftleg_entry = source_entry->leftleg_down;
            }
            if (leftleg_entry) {
                newentry->leftleg_down = leftleg_entry;
                leftleg_entry->leftleg_up.push_back(newentry);
            }
        }
        const int col_idx = source_entry->x + x_offset;
        ensure_column(mountain, col_idx);
        insert_sorted_desc(mountain[static_cast<size_t>(col_idx)], newentry);
    }
    [[nodiscard]] Mountain draw_mountain(EntryArena& arena, Mountain mountain) {
        for (auto& column : mountain) {
            while (true) {
                Entry* entry = column[0];
                if (entry->value == 1) break;
                Entry* parent = entry;
                while (true) {
                    Entry* up = parent->leftleg_down;
                    while (up->rightleg_up &&
                        vertical_compare(up->rightleg_up, parent) <= 0)
                        up = up->rightleg_up;
                    parent = up;
                    if (parent->value < entry->value) break;
                }
                Entry* newentry = create_entry(arena, parent, entry);
                column.insert(column.begin(), newentry);
            }
        }
        return mountain;
    }
    [[nodiscard]] Entry* find_entry_by_coords(const Mountain& mountain,
        int x, const std::vector<int>& y) {
        if (x < 0 || x >= static_cast<int>(mountain.size())) return nullptr;
        const uint64_t target = make_ykey(y);
        for (auto* e : mountain[static_cast<size_t>(x)]) {
            if (e && e->x == x && e->ykey == target) return e;
        }
        return nullptr;
    }
    struct WeakPolicy {
        static std::vector<Entry*> collect(Entry* br1) { return collect_usual(br1); }
        static constexpr bool kHasPhantomBranch = false;
    };
    struct MediumPolicy {
        static std::vector<Entry*> collect(Entry* br1) { return collect_usual(br1); }
        static constexpr bool kHasPhantomBranch = false;
    };
    struct StrongPolicy {
        static std::vector<Entry*> collect(Entry* br1) {
            return br1->y.empty() ? std::vector<Entry*>{} : collect1D(br1);
        }
        static constexpr bool kHasPhantomBranch = true;
    };
    template <class Policy>
    [[nodiscard]] std::vector<int> magma_impl(const std::vector<int>& seq, int FSterm) {
        if (seq.size() < 2) return seq;
        EntryArena arena;
        Mountain mountain = draw_mountain(arena, from_sequence(arena, seq));
        auto& child = mountain.back();
        Entry* BR = child[0]->leftleg_down;
        const int width = static_cast<int>(mountain.size()) - 1 - BR->x;
        const int BR_x = BR->x;
        const std::vector<int> BR_y = BR->y;
        auto& top_col = mountain[static_cast<size_t>(BR_x)];
        int idx = 0;
        while (idx < static_cast<int>(top_col.size()) && top_col[static_cast<size_t>(idx)] != BR) ++idx;
        std::vector<Entry*> top;
        for (int i = idx; i < static_cast<int>(top_col.size()) - 1; ++i)
            top.push_back(top_col[static_cast<size_t>(i)]);
        top.insert(top.begin(), child[0]);
        std::vector<int> s = seq;
        --s[s.size() - 1];
        Mountain newmountain = draw_mountain(arena, from_sequence(arena, s));
        Entry* BR_new = find_entry_by_coords(newmountain, BR_x, BR_y);
        if (!BR_new) return seq;
        std::vector<std::vector<Entry*>> magma_entries(static_cast<size_t>(width) + 1);
        for (Entry* BR1 = BR_new; BR1 != nullptr; BR1 = BR1->rightleg_down) {
            auto collected = Policy::collect(BR1);
            for (Entry* e : collected) {
                if (!e) continue;
                const int dx = e->x - BR_new->x;
                if (dx < 0 || dx > width) continue;
                auto& slot = magma_entries[static_cast<size_t>(dx)];
                if (std::ranges::find(slot, e) == slot.end())
                    slot.push_back(e);
            }
            if constexpr (Policy::kHasPhantomBranch) {
                if (BR1->y.empty()) {
                    for (int dx = 1; dx <= width; ++dx) {
                        const int col_idx = BR_new->x + dx;
                        if (col_idx < static_cast<int>(newmountain.size()) &&
                            !newmountain[static_cast<size_t>(col_idx)].empty()) {
                            Entry* last = newmountain[static_cast<size_t>(col_idx)].back();
                            auto& slot = magma_entries[static_cast<size_t>(dx)];
                            if (std::ranges::find(slot, last) == slot.end())
                                slot.push_back(last);
                        }
                    }
                    break;
                }
            }
            if (BR1->y.empty()) break;
        }
        for (int n = 1; n <= FSterm; ++n) {
            std::vector<Entry*> ref;
            if (!newmountain.empty() && !newmountain.back().empty()) {
                for (Entry* topentry : top) {
                    if (!topentry) { ref.push_back(nullptr); continue; }
                    ref.push_back(find_lower(newmountain.back(), topentry->y));
                }
            }
            for (int dx = 1; dx <= width; ++dx) {
                const int col_idx = BR_new->x + n * width + dx;
                ensure_column(newmountain, col_idx);
                auto& column = newmountain[static_cast<size_t>(col_idx)];
                column.clear();
                for (Entry* magma_entry : magma_entries[static_cast<size_t>(dx)]) {
                    if (!magma_entry) continue;
                    copy_single_edge(arena, newmountain, magma_entry, n * width, BR_new->x);
                    Entry* source_entry = magma_entry;
                    Entry* higher = find_higherequal(ref, magma_entry->y);
                    std::vector<int> targety = higher ? higher->y : magma_entry->y;
                    const std::vector<int> targety0 = targety;
                    while (!(source_entry->value <= 1 ||
                        std::ranges::find(magma_entries[static_cast<size_t>(dx)],
                            source_entry->rightleg_up) !=
                        magma_entries[static_cast<size_t>(dx)].end())) {
                        if (!source_entry->rightleg_up) break;
                        const int d = dimension_difference(source_entry->y,
                            source_entry->rightleg_up->y);
                        targety = vertical_increase(targety, d);
                        source_entry = source_entry->rightleg_up;
                        copy_single_edge(arena, newmountain, source_entry,
                            n * width, BR_new->x, targety);
                    }
                    if (!magma_entry->y.empty()) {
                        const int leftlegx = magma_entry->leftleg_down->x + n * width;
                        ensure_column(newmountain, leftlegx);
                        auto slice = yslice(newmountain[static_cast<size_t>(leftlegx)],
                            magma_entry->y, targety0);
                        for (Entry* leftleg_entry : slice) {
                            if (!leftleg_entry) continue;
                            fill_magma_edge(arena, newmountain, magma_entry, leftleg_entry);
                        }
                    }
                }
                sort_column_desc_if_needed(column);
                link_vertical(column);
            }
        }
        return to_sequence(newmountain);
    }
    [[nodiscard]] std::vector<int> weak_magma(const std::vector<int>& seq, int FSterm) {
        return magma_impl<WeakPolicy>(seq, FSterm);
    }
    [[nodiscard]] std::vector<int> medium_magma(const std::vector<int>& seq, int FSterm) {
        return magma_impl<MediumPolicy>(seq, FSterm);
    }
    [[nodiscard]] std::vector<int> strong_magma(const std::vector<int>& seq, int FSterm) {
        return magma_impl<StrongPolicy>(seq, FSterm);
    }
    struct SequenceHash {
        size_t operator()(const std::vector<int>& v) const noexcept {
            size_t h = 1469598103934665603ull;
            for (int x : v) {
                h ^= static_cast<size_t>(x);
                h *= 1099511628211ull;
            }
            return h;
        }
    };
    class ExpansionCache {
    public:
        [[nodiscard]] std::optional<std::vector<int>> get(
            const std::vector<int>& key, int term) const {
            auto it = data_.find(key);
            if (it == data_.end()) return std::nullopt;
            if (term < 0 || term >= static_cast<int>(it->second.size())) return std::nullopt;
            const auto& v = it->second[static_cast<size_t>(term)];
            if (v.empty()) return std::nullopt;
            return v;
        }
        void put(const std::vector<int>& key, int term, std::vector<int> result) {
            auto& slot = data_[key];
            if (static_cast<int>(slot.size()) <= term)
                slot.resize(static_cast<size_t>(term) + 1);
            slot[static_cast<size_t>(term)] = std::move(result);
        }
    private:
        std::unordered_map<std::vector<int>, std::vector<std::vector<int>>, SequenceHash> data_;
    };
    struct ExpanderContext {
        MagmaType type = MagmaType::Weak;
        ExpansionCache cache;
        std::vector<int> expand(const std::vector<int>& seq, int FSterm, bool removeLast) const {
            std::vector<int> result;
            switch (type) {
            case MagmaType::Weak:   result = weak_magma(seq, FSterm);   break;
            case MagmaType::Medium: result = medium_magma(seq, FSterm); break;
            case MagmaType::Strong: result = strong_magma(seq, FSterm); break;
            }
            if (removeLast && result.size() > 1) result.pop_back();
            return result;
        }
        std::vector<int> FS(const std::vector<int>& seq, int FSterm);
        std::vector<int> FSalter(const std::vector<int>& seq, int FSterm);
    };
    template <bool RemoveLast>
    [[nodiscard]] std::vector<int> fs_impl(ExpanderContext& ctx,
        const std::vector<int>& seq, int FSterm) {
        if (seq.empty()) return {};
        if (seq.back() == 1)
            return { seq.begin(), seq.end() - 1 };
        auto& cache = ctx.cache;
        if (auto cached = cache.get(seq, FSterm)) return *cached;
        auto result = ctx.expand(seq, FSterm, RemoveLast);
        cache.put(seq, FSterm, result);
        return result;
    }
    std::vector<int> ExpanderContext::FS(const std::vector<int>& seq, int FSterm) {
        return fs_impl<true>(*this, seq, FSterm);
    }
    std::vector<int> ExpanderContext::FSalter(const std::vector<int>& seq, int FSterm) {
        return fs_impl<false>(*this, seq, FSterm);
    }
    // ============================================================
    // 序列解析
    // ============================================================
    [[nodiscard]] std::vector<int> parse_sequence(std::string_view str) {
        std::vector<int> seq;
        for (auto part : str | std::views::split(',')) {
            std::string token{ part.begin(), part.end() };
            token.erase(std::remove_if(token.begin(), token.end(),
                [](unsigned char c) { return std::isspace(c); }),
                token.end());
            if (token.empty()) continue;
            try { seq.push_back(std::stoi(token)); }
            catch (...) { /* 忽略非法项 */ }
        }
        return seq;
    }
    [[nodiscard]] std::string seq_to_string(const std::vector<int>& seq) {
        if (seq.empty()) return "[]";
        std::string out;
        for (size_t i = 0; i < seq.size(); ++i) {
            if (i) out += ',';
            out += std::to_string(seq[i]);
        }
        return out;
    }
    // ============================================================
    // UI
    // ============================================================
    struct MenuDeleter {
        void operator()(HMENU m) const noexcept { if (m) ::DestroyMenu(m); }
    };
    using MenuPtr = std::unique_ptr<std::remove_pointer_t<HMENU>, MenuDeleter>;
    struct BrushDeleter {
        void operator()(HBRUSH b) const noexcept { if (b) ::DeleteObject(b); }
    };
    using BrushPtr = std::unique_ptr<std::remove_pointer_t<HBRUSH>, BrushDeleter>;
    struct UiState {
        HWND hEditSeq{};
        HWND hEditTerm{};
        HWND hComboMagma{};
        HWND hBtnFS{};
        HWND hBtnFSalter{};
        BrushPtr background;
        ExpanderContext ctx;   // ← 取代全局
    };
    [[nodiscard]] UiState* getUi(HWND hwnd) {
        return reinterpret_cast<UiState*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    [[nodiscard]] HMENU createMenuBar() {
        HMENU hMenu = ::CreateMenu();
        HMENU hFileMenu = ::CreatePopupMenu();
        ::AppendMenuW(hFileMenu, MF_STRING, IDM_EXIT, L"退出(&X)");
        ::AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hFileMenu), L"文件(&F)");
        HMENU hHelpMenu = ::CreatePopupMenu();
        ::AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP, L"帮助(&H)...");
        ::AppendMenuW(hHelpMenu, MF_STRING, IDM_ABOUT, L"关于(&A)...");
        ::AppendMenuW(hHelpMenu, MF_STRING, IDM_LEGAL, L"法律声明(&L)...");
        ::AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hHelpMenu), L"帮助(&H)");
        return hMenu;
    }
    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE: {
            auto* ui = new UiState{};
            ui->background.reset(::CreateSolidBrush(::GetSysColor(COLOR_WINDOW)));
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ui));
            MenuPtr menu(createMenuBar());
            ::SetMenu(hwnd, menu.release());
            constexpr int menuHeight = 25;
            ::CreateWindowExW(0, L"STATIC", L"序列 (用逗号分隔):",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, menuHeight + 10, 150, 20, hwnd, nullptr, nullptr, nullptr);
            ui->hEditSeq = ::CreateWindowExW(0, L"EDIT", L"1,2,3",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                10, menuHeight + 30, 200, 20, hwnd, nullptr, nullptr, nullptr);
            ::CreateWindowExW(0, L"STATIC", L"项数:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, menuHeight + 55, 50, 20, hwnd, nullptr, nullptr, nullptr);
            ui->hEditTerm = ::CreateWindowExW(0, L"EDIT", L"1",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                60, menuHeight + 55, 80, 20, hwnd, nullptr, nullptr, nullptr);
            ::CreateWindowExW(0, L"STATIC", L"记号:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, menuHeight + 85, 80, 20, hwnd, nullptr, nullptr, nullptr);
            ui->hComboMagma = ::CreateWindowExW(0, L"COMBOBOX", L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
                100, menuHeight + 82, 140, 100, hwnd, nullptr, nullptr, nullptr);
            ::SendMessageW(ui->hComboMagma, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Weak Omega-Y"));
            ::SendMessageW(ui->hComboMagma, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Medium Omega-Y"));
            ::SendMessageW(ui->hComboMagma, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Strong Omega-Y"));
            ::SendMessageW(ui->hComboMagma, CB_SETCURSEL, 0, 0);
            ui->hBtnFS = ::CreateWindowExW(0, L"BUTTON", L"移除末项",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, menuHeight + 115, 120, 25, hwnd,
                reinterpret_cast<HMENU>(IDM_FS), nullptr, nullptr);
            ui->hBtnFSalter = ::CreateWindowExW(0, L"BUTTON", L"保留末项",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                140, menuHeight + 115, 120, 25, hwnd,
                reinterpret_cast<HMENU>(IDM_FSALTER), nullptr, nullptr);
            break;
        }
        case WM_ERASEBKGND: {
            auto* ui = getUi(hwnd);
            if (!ui) break;
            HDC hdc = reinterpret_cast<HDC>(wParam);
            RECT rect;
            ::GetClientRect(hwnd, &rect);
            ::FillRect(hdc, &rect, ui->background.get());
            return 1;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = ::BeginPaint(hwnd, &ps);
            auto* ui = getUi(hwnd);
            if (ui) {
                RECT rect;
                ::GetClientRect(hwnd, &rect);
                ::FillRect(hdc, &rect, ui->background.get());
            }
            ::EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN: {
            auto* ui = getUi(hwnd);
            if (!ui) break;
            HDC hdcStatic = reinterpret_cast<HDC>(wParam);
            ::SetBkMode(hdcStatic, TRANSPARENT);
            ::SetTextColor(hdcStatic, ::GetSysColor(COLOR_WINDOWTEXT));
            return reinterpret_cast<LRESULT>(ui->background.get());
        }
        case WM_COMMAND: {
            auto* ui = getUi(hwnd);
            if (!ui) break;
            if (HIWORD(wParam) == CBN_SELCHANGE &&
                reinterpret_cast<HWND>(lParam) == ui->hComboMagma) {
                int sel = static_cast<int>(::SendMessageW(ui->hComboMagma, CB_GETCURSEL, 0, 0));
                switch (sel) {
                case 0: ui->ctx.type = MagmaType::Weak;   break;
                case 1: ui->ctx.type = MagmaType::Medium; break;
                case 2: ui->ctx.type = MagmaType::Strong; break;
                default: break;
                }
                break;
            }
            switch (LOWORD(wParam)) {
            case IDM_HELP: {
                ::MessageBoxW(hwnd,
                    L"鸣谢:Hyp Cos,naruyoko,test_alpha-0\n"
                    L"请注意 代码系利用人工智能技术生成",
                    L"帮助", MB_OK | MB_ICONINFORMATION);
                break;
            }
            case IDM_ABOUT: {
                ::MessageBoxW(hwnd,
                    L"ω-Y 展开器 v1.2\n\n"
                    L"基于 hypcos/notation-explorer 的 C++ 实现\n"
                    L"请注意 代码系利用人工智能技术生成\n"
                    L"本软件使用Unlicense授权\n但在中华人民共和国大陆地区法律下，它应当是需要署名的\n",
                    L"关于", MB_OK | MB_ICONINFORMATION);
                break;
            }
            case IDM_LEGAL: {
                ::MessageBoxW(hwnd,
                    L"本软件使用Unlicense授权:"
                    L"This is free and unencumbered software released into the public domain.\n"
                    L"Anyone is free to copy, modify, publish, use, compile, sell, or\n"
                    L"distribute this software, either in source code form or as a compiled\n"
                    L"binary, for any purpose, commercial or non‑commercial, and by any\n"
                    L"means.\n\n"
                    L"In jurisdictions that recognize copyright laws, the author or authors\n"
                    L"of this software dedicate any and all copyright interest in the\n"
                    L"software to the public domain. We make this dedication for the benefit\n"
                    L"of the public at large and to the detriment of our heirs and\n"
                    L"successors. We intend this dedication to be an overt act of\n"
                    L"relinquishment in perpetuity of all present and future rights to this\n"
                    L"software under copyright law.\n\n"
                    L"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n"
                    L"EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\n"
                    L"MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.\n"
                    L"IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR\n"
                    L"OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,\n"
                    L"ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
                    L"OTHER DEALINGS IN THE SOFTWARE.\n\n"
                    L"在中华人民共和国大陆地区法律下，使用本软件建议保留署名。",
                    L"法律声明", MB_OK | MB_ICONINFORMATION);
                break;
            }
            case IDM_EXIT:
                ::PostQuitMessage(0);
                break;
            case IDM_FS:
            case IDM_FSALTER: {
                wchar_t bufSeq[256]{}, bufTerm[64]{};
                ::GetWindowTextW(ui->hEditSeq, bufSeq, 256);
                ::GetWindowTextW(ui->hEditTerm, bufTerm, 64);
                auto seq = parse_sequence(wstring_to_utf8(bufSeq));
                int term = 0;
                try { term = std::stoi(wstring_to_utf8(bufTerm)); }
                catch (...) {
                    ::MessageBoxW(hwnd, L"项数必须是整数", L"错误", MB_OK | MB_ICONERROR);
                    break;
                }
                if (term < 1) {
                    ::MessageBoxW(hwnd, L"项数必须为正整数", L"错误", MB_OK | MB_ICONERROR);
                    break;
                }
                if (seq.empty()) {
                    ::MessageBoxW(hwnd, L"序列不能为空，为什么要输入 LHO 喵", L"错误", MB_OK | MB_ICONERROR);
                    break;
                }
                if (seq.size() < 2) {
                    ::MessageBoxW(hwnd, L"序列至少需要 2 个元素，你难道真的要展开 ω‑Y(1) 吗",
                        L"错误", MB_OK | MB_ICONERROR);
                    break;
                }
                std::vector<int> result = (LOWORD(wParam) == IDM_FS)
                    ? ui->ctx.FS(seq, term)
                    : ui->ctx.FSalter(seq, term);
                auto wresult = utf8_to_wstring(seq_to_string(result));
                ::MessageBoxW(hwnd, wresult.c_str(), L"展开结果", MB_OK | MB_ICONINFORMATION);
                break;
            }
            default: break;
            }
            break;
        }
        case WM_DESTROY: {
            auto* ui = getUi(hwnd);
            delete ui;
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            ::PostQuitMessage(0);
            break;
        }
        default:
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return 0;
    }
}
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OmegaY";
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!::RegisterClassW(&wc)) return 0;
    HWND hwnd = ::CreateWindowExW(
        0, L"OmegaY", L"ω‑Y 展开器 (Magma)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 320, 250,
        nullptr, nullptr, hInstance, nullptr);
    if (!hwnd) return 0;
    ::ShowWindow(hwnd, nCmdShow);
    ::UpdateWindow(hwnd);
    MSG msg;
    while (::GetMessageW(&msg, nullptr, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
    return 0;
}

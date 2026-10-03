#pragma once

#include <algorithm>
#include <climits>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Header/common/Matrix.hpp"

namespace omegay::notation {

    struct UPMSNotation {
        static constexpr const char* kName = "UPMS";

        // 对矩阵 m 执行 term 项展开。
        //   - 若 m 的最后一列全为 0（后继形）→ 返回去掉最后一列的矩阵。
        //   - 若无法定位坏根 → 返回空矩阵。
        //   - 否则返回 G + B + Bh_1 + ... + Bh_term。
        // 结果矩阵的行数与输入一致。
        [[nodiscard]] static omegay::common::Matrix
            expand(const omegay::common::Matrix& m, int term);

        [[nodiscard]] static std::string suffix() { return {}; }
    };

    // ---------------------------------------------------------------------
    // 内部实现细节；不对外暴露。
    // ---------------------------------------------------------------------
    namespace detail {

        // 内部统一用「列列表」表示矩阵：cols[c][r] 是第 c 列第 r 行的值。
        using Cols = std::vector<std::vector<int>>;

        inline int col_count(const Cols& c) noexcept {
            return static_cast<int>(c.size());
        }
        inline int row_count(const Cols& c) noexcept {
            return c.empty() ? 0 : static_cast<int>(c.front().size());
        }

        // (col, a) → a 层祖先列集合的缓存
        using AncestorCache = std::map<std::pair<int, int>, std::set<int>>;

        int  get_b_parent(const Cols& m, int col_index, int b, AncestorCache& cache);
        std::set<int> get_a_ancestors(const Cols& m, int col_index, int a,
            AncestorCache& cache);

        // ---- get_b_parent -----------------------------------------------------
        inline int get_b_parent(const Cols& m, int col_index, int b,
            AncestorCache& cache) {
            const int nr = row_count(m);
            const int row_idx = b - 1;
            if (row_idx >= nr) return -1;

            const int c = m[col_index][row_idx];
            const auto ancestors = get_a_ancestors(m, col_index, b - 1, cache);

            int best_col = -1;
            for (int anc : ancestors) {
                if (anc >= col_index) continue;
                if (m[anc][row_idx] < c && anc > best_col) best_col = anc;
            }
            return best_col;
        }

        // ---- get_a_ancestors --------------------------------------------------
        inline std::set<int> get_a_ancestors(const Cols& m, int col_index, int a,
            AncestorCache& cache) {
            const auto key = std::make_pair(col_index, a);
            if (auto it = cache.find(key); it != cache.end()) return it->second;

            std::set<int> ancestors;
            ancestors.insert(col_index);

            bool changed = true;
            const int max_iter = col_count(m) + 10;
            int iter = 0;
            while (changed && iter < max_iter) {
                changed = false;
                ++iter;
                const std::vector<int> current(ancestors.begin(), ancestors.end());
                for (int c : current) {
                    int parent = -1;
                    if (a == 0) {
                        if (c > 0) parent = c - 1;
                    }
                    else {
                        parent = get_b_parent(m, c, a, cache);
                    }
                    if (parent >= 0 && ancestors.find(parent) == ancestors.end()) {
                        ancestors.insert(parent);
                        changed = true;
                    }
                }
            }
            cache.emplace(key, ancestors);
            return ancestors;
        }

        // ---- find_bad_root ----------------------------------------------------
        struct BadRoot {
            int root_col;
            int t;
        };

        inline std::optional<BadRoot>
            find_bad_root(const Cols& m, AncestorCache& cache) {
            const int nc = col_count(m);
            const int nr = row_count(m);
            const auto& last = m[nc - 1];

            int t = -1;
            for (int r = nr - 1; r >= 0; --r) {
                if (last[r] != 0) { t = r + 1; break; }
            }
            if (t < 0) return std::nullopt;

            const int root = get_b_parent(m, nc - 1, t, cache);
            if (root < 0) return std::nullopt;
            return BadRoot{ root, t };
        }

        // ---- compute_delta ----------------------------------------------------
        inline std::vector<int>
            compute_delta(const Cols& m, int root_col, int t) {
            const int nr = row_count(m);
            const auto& last = m.back();
            const auto& root = m[root_col];

            std::vector<int> delta(static_cast<size_t>(nr), 0);
            for (int r = 0; r < nr; ++r) {
                if (r + 1 < t) delta[r] = last[r] - root[r];
            }
            return delta;
        }

        // ---- compute_upms_verification_roots ---------------------------------
        struct VrCache {
            std::map<std::pair<int, int>, int> map;
            void set(int c, int r, int v) { map[{c, r}] = v; }
            int  get(int c, int r) const {
                auto it = map.find({ c, r });
                return it == map.end() ? -1 : it->second;
            }
        };

        inline VrCache compute_upms_verification_roots(
            const Cols& m, int root_col, int t, AncestorCache& cache) {

            const int last_col = col_count(m) - 1;
            const int y = root_col;
            const int alpha = last_col;
            VrCache vr;

            auto is_in_bad_part = [&](int col, int row) {
                return col >= y && col < alpha && row < t - 1;
                };
            auto get_verified_root = [&](int col, int row) -> int {
                if (!is_in_bad_part(col, row)) return -1;
                return vr.get(col, row);
                };

            for (int row = 0; row < t - 1; ++row) {
                for (int col = y; col < alpha; ++col) {
                    const int k = row + 1;

                    if (col == y) { vr.set(col, row, 1); continue; }
                    if (row == 0) { vr.set(col, row, 1); continue; }

                    const auto k_ancestors = get_a_ancestors(m, col, k, cache);
                    const int Ayk = m[y][row];

                    bool contains_Ayk = k_ancestors.count(y) > 0;
                    bool ancestor_has_vr0 = false;
                    for (int anc : k_ancestors) {
                        if (get_verified_root(anc, row) == 0) {
                            ancestor_has_vr0 = true;
                            break;
                        }
                    }
                    const int b_parent = get_b_parent(m, col, k, cache);

                    if (!contains_Ayk || ancestor_has_vr0 || b_parent < 0) {
                        vr.set(col, row, 0); continue;
                    }
                    if (b_parent != y) { vr.set(col, row, 1); continue; }

                    bool exists_w_vr0 = false;
                    for (int w = 0; w < row; ++w) {
                        if (get_verified_root(col, w) == 0) {
                            exists_w_vr0 = true;
                            break;
                        }
                    }
                    if (exists_w_vr0) { vr.set(col, row, 0); continue; }

                    bool exists_v_cond = false;
                    for (int v_row = row + 1; v_row < t - 1; ++v_row) {
                        if (get_b_parent(m, col, v_row + 1, cache) != y) {
                            exists_v_cond = true;
                            break;
                        }
                    }
                    if (exists_v_cond) { vr.set(col, row, 0); continue; }

                    // ---- 构造 base_col ----
                    std::vector<int> base_col(static_cast<size_t>(row + 1));
                    for (int r = 0; r <= row; ++r) {
                        base_col[r] = (r < row) ? m[col][r] + 1 : m[col][r];
                    }

                    // ---- 寻找 u ----
                    int u = -1;
                    for (int c = col + 1; c <= alpha; ++c) {
                        bool less = false;
                        for (int r = 0; r <= row; ++r) {
                            if (m[c][r] < base_col[r]) { less = true; break; }
                            if (m[c][r] > base_col[r]) break;
                        }
                        if (less) { u = c + 1; break; }
                    }
                    if (u < 0) { vr.set(col, row, 1); continue; }

                    // ---- 切出 X ----
                    const int start_x = col;
                    const int end_x = u - 2;
                    Cols X(m.begin() + start_x, m.begin() + end_x + 1);

                    // ---- 切出 Y ----
                    const auto k_anc_alpha = get_a_ancestors(m, alpha, k, cache);
                    int j = -1;
                    for (int anc : k_anc_alpha) {
                        if (m[anc][row] == Ayk + 1) { j = anc + 1; break; }
                    }
                    if (j < 0) j = alpha + 1;
                    const int start_y = j - 1;
                    const int end_y = alpha;
                    Cols Y(m.begin() + start_y, m.begin() + end_y + 1);

                    // ---- 取全局最大值 * 2 作为移位基准 ----
                    int max_val = INT_MIN;
                    for (const auto& col_v : m) {
                        for (int v : col_v) if (v > max_val) max_val = v;
                    }
                    const int x_val = max_val * 2;

                    // ---- 变换 X → Xc ----
                    Cols Xc = X;
                    for (int s = 1; s <= k - 1; ++s) {
                        const int s_row = s - 1;
                        const int dx = x_val - m[col][s_row];
                        for (size_t xi = 0; xi < Xc.size(); ++xi) {
                            if (get_verified_root(start_x + static_cast<int>(xi),
                                s_row) == 1) {
                                Xc[xi][s_row] += dx;
                            }
                        }
                    }

                    // ---- 变换 Y → Yc ----
                    Cols Yc = Y;
                    for (int s = 1; s <= k - 1; ++s) {
                        const int s_row = s - 1;
                        const int dy = x_val - m[start_y][s_row];
                        for (size_t yi = 0; yi < Yc.size(); ++yi) {
                            const int orig_col = start_y + static_cast<int>(yi);
                            const auto s_anc =
                                get_a_ancestors(m, orig_col, s, cache);
                            if (orig_col == start_y || s_anc.count(start_y) > 0) {
                                Yc[yi][s_row] += dy;
                            }
                        }
                    }

                    // ---- 逐列逐行比较 Xc 与 Yc ----
                    int cmp = 0;
                    {
                        bool decided = false;
                        const size_t mc = std::min(Xc.size(), Yc.size());
                        for (size_t ci = 0; ci < mc && !decided; ++ci) {
                            const size_t mr =
                                std::min(Xc[ci].size(), Yc[ci].size());
                            for (size_t ri = 0; ri < mr; ++ri) {
                                if (Xc[ci][ri] < Yc[ci][ri]) { cmp = -1; decided = true; break; }
                                if (Xc[ci][ri] > Yc[ci][ri]) { cmp = 1; decided = true; break; }
                            }
                            if (!decided) {
                                if (Xc[ci].size() < Yc[ci].size()) { cmp = -1; decided = true; }
                                else if (Xc[ci].size() > Yc[ci].size()) { cmp = 1; decided = true; }
                            }
                        }
                        if (!decided) {
                            if (Xc.size() < Yc.size()) cmp = -1;
                            else if (Xc.size() > Yc.size()) cmp = 1;
                        }
                    }
                    vr.set(col, row, cmp < 0 ? 0 : 1);
                }
            }
            return vr;
        }

        // ---- generate_bh_upms -------------------------------------------------
        inline Cols generate_bh_upms(const Cols& B, const std::vector<int>& delta,
            int t, int h, int root_col,
            const VrCache& vr) {
            Cols result = B;
            const int nr = row_count(B);
            for (size_t ci = 0; ci < result.size(); ++ci) {
                const int orig_col = root_col + static_cast<int>(ci);
                for (int r = 0; r < nr; ++r) {
                    int k = 0;
                    if (r + 1 < t && vr.get(orig_col, r) == 1) k = 1;
                    result[ci][r] += h * delta[r] * k;
                }
            }
            return result;
        }

        // ---- 列列表 → Matrix --------------------------------------------------
        inline omegay::common::Matrix cols_to_matrix(const Cols& cols) {
            if (cols.empty()) return omegay::common::Matrix{};
            const int nr = row_count(cols);
            omegay::common::Matrix m(nr, static_cast<int>(cols.size()));
            for (int c = 0; c < static_cast<int>(cols.size()); ++c) {
                const int lim = std::min(nr, static_cast<int>(cols[c].size()));
                for (int r = 0; r < lim; ++r) m(r, c) = cols[c][r];
            }
            return m;
        }

    } // namespace detail

    // ---------------------------------------------------------------------
    // 公共接口实现
    // ---------------------------------------------------------------------
    inline omegay::common::Matrix
        UPMSNotation::expand(const omegay::common::Matrix& m, int term) {
        using namespace detail;

        if (m.empty() || term < 1) return omegay::common::Matrix{};

        // Matrix（列主序）→ Cols（列列表）
        Cols cols(static_cast<size_t>(m.cols()));
        for (int c = 0; c < m.cols(); ++c) {
            cols[c].resize(static_cast<size_t>(m.rows()));
            for (int r = 0; r < m.rows(); ++r) cols[c][r] = m(r, c);
        }

        // ---- 后继形：末列全 0 ⇒ 去掉末列 ----
        bool last_all_zero = true;
        for (int v : cols.back()) {
            if (v != 0) { last_all_zero = false; break; }
        }
        if (last_all_zero) {
            cols.pop_back();
            return cols_to_matrix(cols);
        }

        // ---- 定位坏根 ----
        AncestorCache cache;
        const auto bad = find_bad_root(cols, cache);
        if (!bad) return omegay::common::Matrix{};

        const int root = bad->root_col;
        const int t = bad->t;

        // ---- 切分 G | B | δ ----
        Cols G(cols.begin(), cols.begin() + root);
        Cols B(cols.begin() + root, cols.end() - 1);

        const auto delta = compute_delta(cols, root, t);
        const auto vr = compute_upms_verification_roots(cols, root, t, cache);

        // ---- 拼接结果 ----
        Cols result;
        result.reserve(G.size() + B.size() * static_cast<size_t>(term + 1));
        result.insert(result.end(), G.begin(), G.end());
        result.insert(result.end(), B.begin(), B.end());

        for (int h = 1; h <= term; ++h) {
            Cols Bh = generate_bh_upms(B, delta, t, h, root, vr);
            for (auto& col : Bh) result.push_back(std::move(col));
        }

        return cols_to_matrix(result);
    }

} // namespace omegay::notation
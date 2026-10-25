// cms.hpp
// CMS —— Crane Matrix System（Crane 矩阵系统）。
//
// 逻辑移植自 ne-rewritten/src/notations/BM-like/CMS.ts，仅保留核心展开
// （FS）与 BM 文本接口，舍弃记忆化缓存（UI 层职责）。
//
// 该记号原版只有 FS（无 FS_alter / shorter），且后继型（末列全零）在展开
// 内部以“删去末列”处理，因此 FS 与 FS_alter 菜单项都映射到同一展开。
// 内部逐列用变长列向量表示，以严格对应原版对动态增长矩阵 m2 的父项计算。
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Header/notation/bmlike_share.hpp"

namespace omegay::notation {

    struct CMSNotation {
        static constexpr const char* kName = "CMS";
        [[nodiscard]] static std::string expand_text(std::string_view, int);
        [[nodiscard]] static std::string suffix() { return {}; }
    };

    namespace cms_detail {

        using bmlike::Matrix;

        using Cols = std::vector<std::vector<int>>;

        [[nodiscard]] inline int col_at(const Cols& m, int x, int y) noexcept {
            if (x < 0 || x >= static_cast<int>(m.size())) return 0;
            if (y < 0 || y >= static_cast<int>(m[x].size())) return 0;
            return m[x][y];
        }

        // BM 父项（带缓存），对应 CMS.ts parent。
        class ParentCache {
        public:
            explicit ParentCache(const Cols& m) : m_(m) {}

            int parent(int x, int y) {
                const std::uint64_t key =
                    (static_cast<std::uint64_t>(static_cast<unsigned>(x)) << 32)
                    | static_cast<unsigned>(y);
                const auto it = cache_.find(key);
                if (it != cache_.end()) return it->second;

                int p = x;
                while (true) {
                    p = y > 0 ? parent(p, y - 1) : p - 1;
                    if (p < 0) break;
                    if (col_at(m_, p, y) < col_at(m_, x, y)) break;
                }
                cache_[key] = p;
                return p;
            }

        private:
            const Cols& m_;
            std::unordered_map<std::uint64_t, int> cache_;
        };

        // CMS.ts L：在 x1..x2 之间，自顶向下找第一个使 x2 经 y 行父链
        // 恰好落到 x1 的非零行 y。ymax 固定取自原始末列。
        [[nodiscard]] inline int L(ParentCache& pc, int ymax, int x1, int x2,
                                   const Cols& m) {
            for (int y = ymax; y >= 0; --y) {
                if (col_at(m, x2, y) == 0) continue;
                int x = x2;
                while (x1 < x) x = pc.parent(x, y);
                if (x == x1) return y;
            }
            return -1;
        }

        // 裁剪每列尾随 0（对应 normalize），并返回。
        inline void normalize_cols(Cols& m) {
            for (auto& col : m) {
                int h = static_cast<int>(col.size());
                while (h > 0 && col[h - 1] == 0) --h;
                col.resize(static_cast<std::size_t>(h));
            }
        }

        [[nodiscard]] inline std::string cols_to_string(const Cols& m) {
            std::string out;
            for (const auto& col : m) {
                int h = static_cast<int>(col.size());
                while (h > 0 && col[h - 1] == 0) --h;
                out += '(';
                if (h == 0) out += '0';
                for (int y = 0; y < h; ++y) {
                    if (y) out += ',';
                    out += std::to_string(col[y]);
                }
                out += ')';
            }
            return out;
        }

        [[nodiscard]] inline Cols expand(const Matrix& input, int FSterm) {
            // 标准化为变长列（每列 rows 项，对齐 TS standardize）。
            const int rows = input.rows();
            Cols m1(static_cast<std::size_t>(input.cols()));
            for (int c = 0; c < input.cols(); ++c) {
                m1[c].resize(static_cast<std::size_t>(rows));
                for (int y = 0; y < rows; ++y) m1[c][y] = input(y, c);
            }

            const int endcol = static_cast<int>(m1.size()) - 1;
            Cols m2(m1.begin(), m1.begin() + endcol);
            const std::vector<int>& child = m1[endcol];
            const int ymax = static_cast<int>(child.size()) - 1;

            int LNZ = ymax;
            for (; LNZ >= 0; --LNZ)
                if (child[LNZ] > 0) break;
            if (LNZ < 0 || FSterm <= 0) return m2;

            ParentCache m1pc(m1);
            const int BR = m1pc.parent(endcol, LNZ);
            const std::vector<int>& BRcolumn = m1[BR];

            std::vector<int> offset(static_cast<std::size_t>(LNZ), 0);
            for (int y = 0; y < LNZ; ++y)
                offset[y] = child[y] - col_at(m1, BR, y);

            // ascending(r,x,y) 始终基于原始矩阵 m1。
            ParentCache asc_pc(m1);
            std::unordered_map<std::uint64_t, char> asc_cache;
            auto ascending = [&](auto&& self, int r, int x, int y) -> bool {
                const std::uint64_t key =
                    (static_cast<std::uint64_t>(static_cast<unsigned>(r)) << 40)
                    ^ (static_cast<std::uint64_t>(static_cast<unsigned>(x)) << 8)
                    ^ static_cast<unsigned>(y);
                const auto it = asc_cache.find(key);
                if (it != asc_cache.end()) return it->second != 0;
                const bool v =
                    r <= x && (r == x ||
                               self(self, r, asc_pc.parent(x, y), y));
                asc_cache[key] = v ? 1 : 0;
                return v;
            };

            // offset_asc[x][y]：仅在相对 BR 上升的位置保留增量。
            std::vector<std::vector<int>> offset_asc(
                static_cast<std::size_t>(endcol));
            for (int x = BR; x < endcol; ++x) {
                offset_asc[x].assign(static_cast<std::size_t>(rows), 0);
                for (int y = 0; y < rows; ++y)
                    if (y < LNZ && ascending(ascending, BR, x, y))
                        offset_asc[x][y] = offset[y];
            }

            for (int n = 1; n <= FSterm; ++n) {
                for (int col = BR; col < endcol; ++col) {
                    std::vector<int> nc(static_cast<std::size_t>(rows), 0);
                    for (int y = 0; y < rows; ++y)
                        nc[y] = col_at(m1, col, y) + offset_asc[col][y] * n;
                    m2.push_back(std::move(nc));
                }
            }

            int col = endcol;
            while (BR < --col) {
                if (L(m1pc, ymax, BR, col, m1) > LNZ) break;
            }
            if (col == BR) {
                if (ymax > 0) {
                    bool all_zero = true;
                    for (const auto& c : m2) {
                        const int v = ymax < static_cast<int>(c.size())
                                          ? c[ymax] : 0;
                        if (v != 0) { all_zero = false; break; }
                    }
                    if (all_zero)
                        for (auto& c : m2)
                            if (static_cast<int>(c.size()) > ymax)
                                c.resize(static_cast<std::size_t>(ymax));
                }
                return m2;
            }

            // 追加提升后的末列（k<=LNZ 都施加阶差，含 LNZ 行）。
            {
                std::vector<int> nc(static_cast<std::size_t>(rows), 0);
                for (int y = 0; y < rows; ++y) {
                    const int value = child[y];
                    nc[y] = value + (y <= LNZ ? value - col_at(m1, BR, y) : 0)
                                     * FSterm;
                }
                m2.push_back(std::move(nc));
            }

            const int c = col;
            const int c_ = c + (endcol - BR);
            const int d = static_cast<int>(m2.size()) - 1;

            ParentCache m2pc(m2);
            Cols D;
            for (int cc = endcol; cc < d; ++cc) {
                std::vector<int> nc = m2[cc];
                for (int k = 0; k < static_cast<int>(nc.size()); ++k) {
                    if (k > LNZ) continue;
                    int u = 0;
                    int ss = cc;
                    while (true) {
                        const int nextss = m2pc.parent(ss, k);
                        if (nextss < endcol) break;
                        ++u;
                        ss = nextss;
                    }
                    if (L(m2pc, ymax, ss, d, m2) >= k - 1)
                        nc[k] = col_at(m2, c_, k) + u;
                }
                D.push_back(std::move(nc));
            }

            Cols result;
            for (int i = 0; i < c_; ++i)
                result.push_back(std::move(m2[i]));
            for (auto& cc : D) result.push_back(std::move(cc));

            if (ymax > 0) {
                bool all_zero = true;
                for (const auto& ccol : result) {
                    const int v = ymax < static_cast<int>(ccol.size())
                                      ? ccol[ymax] : 0;
                    if (v != 0) { all_zero = false; break; }
                }
                if (all_zero)
                    for (auto& ccol : result)
                        if (static_cast<int>(ccol.size()) > ymax)
                            ccol.resize(static_cast<std::size_t>(ymax));
            }
            normalize_cols(result);
            return result;
        }

    }  // namespace cms_detail

    [[nodiscard]] inline std::string
        CMSNotation::expand_text(std::string_view text, int term) {
        const auto e = bmlike::parse(text);
        if (!e) return bmlike::kParseError;
        if (e->infinity)
            return bmlike::matrix_to_bm_string(bmlike::infinity_fs(term));
        if (e->m.cols() == 0) return {};
        const auto cols = cms_detail::expand(e->m, term);
        return cms_detail::cols_to_string(cols);
    }

}  // namespace omegay::notation

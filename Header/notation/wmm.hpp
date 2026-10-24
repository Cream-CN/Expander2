// wmm.hpp
// wMM —— Weak Mutant Matrix（弱突变矩阵系统）。
//
// 逻辑移植自 ne-rewritten/src/notations/BM-like/wMM.ts，仅保留核心展开与
// BM 文本接口，舍弃 UI 记忆化与 console.assert。
//
// 关键概念：每个单元格除数值外还对应一个“vertical 序列”（整数数组，先比
// 长度再反向逐项比较）；父项判定依赖 vertical 比较。展开时按根与宽高复制
// 坏部，并通过 reference 行号映射逐列重建。原版用对象身份缓存 vertical，
// 这里改为每个 Grid 自带缓存，并在每轮复制后清空（对应 vertical_cache.delete）。
#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Header/notation/bmlike_share.hpp"

namespace omegay::notation {

    struct WMMNotation {
        static constexpr const char* kName = "wMM";
        [[nodiscard]] static std::string expand_text(std::string_view, int);
        [[nodiscard]] static std::string expand_text_alter(std::string_view, int);
        [[nodiscard]] static std::string suffix() { return {}; }
    };

    namespace wmm_detail {

        using bmlike::Matrix;
        using Vert = std::vector<int>;

        struct Cell { int x; int y; };

        // 稀疏列网格：列可在任意索引被创建（创建时为空列），越界读取为 0。
        class Grid {
        public:
            std::vector<std::vector<int>> col;
            std::vector<char> exists;

            void ensure(int x) {
                if (x >= static_cast<int>(col.size())) {
                    col.resize(static_cast<std::size_t>(x) + 1);
                    exists.resize(static_cast<std::size_t>(x) + 1, 0);
                }
            }
            void set_empty_column(int x) {
                ensure(x);
                col[x].clear();
                exists[x] = 1;
                invalidate_vertical();
            }
            void assign_column(int x, std::vector<int> v) {
                ensure(x);
                col[x] = std::move(v);
                exists[x] = 1;
                invalidate_vertical();
            }
            [[nodiscard]] bool has(int x) const noexcept {
                return x >= 0 && x < static_cast<int>(exists.size()) && exists[x];
            }
            [[nodiscard]] int at(int x, int y) const noexcept {
                if (!has(x)) return 0;
                if (y < 0 || y >= static_cast<int>(col[x].size())) return 0;
                return col[x][y];
            }
            [[nodiscard]] int col_length(int x) const noexcept {
                return has(x) ? static_cast<int>(col[x].size()) : 0;
            }
            // 最后一个正值所在行；全空为 -1（对应 findLastIndex(e=>e)）。
            [[nodiscard]] int last_nonzero(int x) const noexcept {
                if (!has(x)) return -1;
                for (int y = static_cast<int>(col[x].size()) - 1; y >= 0; --y)
                    if (col[x][y] != 0) return y;
                return -1;
            }
            void truncate_trailing_zeros(int x) {
                const int i = last_nonzero(x);
                ensure(x);
                col[x].resize(static_cast<std::size_t>(i + 1));  // i=-1 → 0
                exists[x] = 1;
            }

            // ---- vertical ----
            void invalidate_vertical() { vert_computed_ = false; }

            // 取单元格的 vertical 序列（含越界回退逻辑）。
            [[nodiscard]] Vert vertical(int x, int y) {
                if (!vert_computed_) build_vertical();
                if (x < static_cast<int>(vert_.size()) &&
                    y < static_cast<int>(vert_[x].size()))
                    return vert_[x][y];
                // 越界回退（TS get_vertical 尾部）。
                int ending;
                const Vert* base;
                Vert empty;
                if (x < static_cast<int>(vert_.size())) {
                    ending = static_cast<int>(vert_[x].size()) - 1;
                    base = ending >= 0 ? &vert_[x][ending] : &empty;
                } else {
                    ending = 0;
                    base = &empty;
                }
                return vertical_increase(*base, y - ending - 1);
            }

            [[nodiscard]] static int vertical_compare(const Vert& a,
                                                      const Vert& b) {
                if (a.size() != b.size())
                    return a.size() < b.size() ? -1 : 1;
                for (int i = static_cast<int>(a.size()) - 1; i >= 0; --i)
                    if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
                return 0;
            }

            [[nodiscard]] static Vert
                vertical_increase(const Vert& y, int d) {
                Vert c = y;
                if (static_cast<int>(c.size()) < d + 1)
                    c.resize(static_cast<std::size_t>(d) + 1, 0);
                for (int k = 0; k < d; ++k) c[k] = 0;  // fill(0,0,d)
                c[d] = d < static_cast<int>(y.size()) ? y[d] + 1 : 1;
                return c;
            }

            // 父项（对应 wMM.ts parent / parentCheck）。
            [[nodiscard]] Cell parent(const Cell& cur) {
                if (at(cur.x, cur.y) == 0) return Cell{ -1, cur.y };
                Cell p = cur;
                do { p = parent_check(p); }
                while (at(p.x, p.y) != at(cur.x, cur.y) - 1);
                return p;
            }

        private:
            // vert_[x][y]：已存在列、列内已有行的 vertical；可能含空列。
            std::vector<std::vector<Vert>> vert_;
            bool vert_computed_ = false;

            [[nodiscard]] Cell parent_check(const Cell& c) {
                if (c.y == 0) return Cell{ c.x - 1, 0 };
                const int p = parent(Cell{ c.x, c.y - 1 }).x;
                int i = std::max(c.y, col_length(p) - 1);
                const int target = at(c.x, c.y);
                while (at(p, i) < target - 1 ||
                       vertical_compare(vertical(p, i),
                                        vertical(c.x, c.y)) > 0) {
                    --i;
                }
                return Cell{ p, i };
            }

            void build_vertical() {
                vert_.assign(col.size(), {});
                for (int x = 0; x < static_cast<int>(col.size()); ++x) {
                    if (!exists[x]) continue;
                    const int len = static_cast<int>(col[x].size());
                    vert_[x].resize(static_cast<std::size_t>(len));
                    for (int y = 0; y < len; ++y) {
                        int i = y;
                        do {
                            --i;
                        } while (i >= 0 && at(x, y) == at(x, i));
                        const Vert base = i >= 0 ? vert_[x][i] : Vert{};
                        vert_[x][y] = vertical_increase(base, y - i - 1);
                    }
                }
                vert_computed_ = true;
            }
        };

        // ascendingAt 的坐标缓存（A 专用）。
        class AscendingCache {
        public:
            explicit AscendingCache(Cell root) : root_(root) {}
            int get(Grid& A, const Cell& cur) {
                const int key = cur.x * 1048576 + cur.y;
                const auto it = map_.find(key);
                if (it != map_.end()) return it->second;
                int v;
                if (cur.x < root_.x) v = -1;
                else if (cur.x == root_.x) v = cur.y;
                else v = get(A, A.parent(cur));
                map_[key] = v;
                return v;
            }

        private:
            Cell root_;
            std::unordered_map<int, int> map_;
        };

        inline void normalize_grid(Grid& g) {
            for (int x = 0; x < static_cast<int>(g.col.size()); ++x)
                if (g.has(x)) g.truncate_trailing_zeros(x);
        }

        [[nodiscard]] inline std::string grid_to_string(Grid& g) {
            normalize_grid(g);
            std::string out;
            for (int x = 0; x < static_cast<int>(g.col.size()); ++x) {
                if (!g.has(x)) continue;
                const auto& c = g.col[x];
                int h = static_cast<int>(c.size());
                while (h > 0 && c[h - 1] == 0) --h;
                out += '(';
                if (h == 0) out += '0';
                for (int y = 0; y < h; ++y) {
                    if (y) out += ',';
                    out += std::to_string(c[y]);
                }
                out += ')';
            }
            return out;
        }

        [[nodiscard]] inline Grid expand(const Matrix& input, int FSterm) {
            Grid M;
            for (int c = 0; c < input.cols(); ++c) {
                std::vector<int> col(static_cast<std::size_t>(input.rows()));
                for (int r = 0; r < input.rows(); ++r) col[r] = input(r, c);
                M.assign_column(c, std::move(col));
            }

            const int LNZx = static_cast<int>(M.col.size()) - 1;
            const int LNZy = M.last_nonzero(LNZx);
            const int LNZ = M.at(LNZx, LNZy);

            // 收集末列各项沿父链到达 LNZ-1 的位置。
            std::vector<std::vector<int>> collection(
                static_cast<std::size_t>(LNZx) + 1);
            std::vector<char> coll_exists(
                static_cast<std::size_t>(LNZx) + 1, 0);
            Cell working{ LNZx, LNZy };
            do {
                while (M.at(working.x, working.y) != LNZ - 1)
                    working = M.parent(working);
                collection[working.x].insert(
                    collection[working.x].begin(), working.y);
                coll_exists[working.x] = 1;
            } while (--working.y >= 0);

            std::vector<int> counts{ 1 };
            std::vector<int> columns;
            for (int x = 0; x <= LNZx; ++x)
                if (coll_exists[x]) {
                    counts.push_back(
                        static_cast<int>(collection[x].size()));
                    columns.push_back(x);
                }
            const int r = static_cast<int>(counts.size()) - 1;

            Cell root;
            if (counts[r] == 1) root = M.parent(Cell{ LNZx, LNZy });
            else root = Cell{ columns.back(),
                              collection[columns.back()][0] };

            const int width = LNZx - root.x;
            const int height = LNZy - root.y;

            Grid A;
            for (int x = 0; x < static_cast<int>(M.col.size()); ++x)
                if (M.has(x)) A.assign_column(x, M.col[x]);

            // --A[LNZx][LNZy]
            A.ensure(LNZx);
            A.col[LNZx][LNZy] -= 1;
            A.invalidate_vertical();
            // 把根列自 root.y 起的后缀拷到末列自 LNZy 起。
            for (int dy = 0; root.y + dy < M.col_length(root.x); ++dy) {
                const int dst = LNZy + dy;
                if (static_cast<int>(A.col[LNZx].size()) <= dst)
                    A.col[LNZx].resize(static_cast<std::size_t>(dst) + 1, 0);
                A.col[LNZx][dst] = M.at(root.x, root.y + dy);
            }
            A.invalidate_vertical();

            AscendingCache asc(root);

            for (int n = 1; n <= FSterm; ++n) {
                // reference 行号映射（按下标稀疏存储）。
                std::vector<int> reference;
                std::vector<char> ref_exists;
                auto ref_set = [&](int y1, int y2) {
                    if (static_cast<int>(reference.size()) <= y1) {
                        reference.resize(static_cast<std::size_t>(y1) + 1, 0);
                        ref_exists.resize(static_cast<std::size_t>(y1) + 1, 0);
                    }
                    reference[y1] = y2;
                    ref_exists[y1] = 1;
                };
                auto ref_get = [&](int idx) -> int {
                    if (idx >= 0 &&
                        idx < static_cast<int>(reference.size()) &&
                        ref_exists[idx])
                        return reference[idx];
                    return -1;
                };

                int y1 = 0, y2 = 0;
                while (y2 <= root.y + height * n) {
                    const int cmp = Grid::vertical_compare(
                        A.vertical(root.x, y1 + 1),
                        A.vertical(root.x + width * n, y2));
                    if (cmp > 0 || y1 >= root.y) {
                        ref_set(y1, y2);
                        ++y2;
                    } else {
                        ++y1;
                    }
                }

                for (int dx = 1; dx <= width; ++dx) {
                    const int x = root.x + dx;
                    const int target_x = x + width * n;
                    A.set_empty_column(target_x);
                    int lastmagma = -1;
                    const std::vector<int> src = A.col[x];  // 原始列快照
                    for (int y = 0; y < static_cast<int>(src.size()); ++y) {
                        const int val = src[y];
                        const int a = asc.get(A, Cell{ x, y });
                        if (a >= 0) {
                            if (a <= root.y &&
                                Grid::vertical_compare(
                                    A.vertical(root.x, a),
                                    A.vertical(x, y)) <= 0) {
                                const int jlo = ref_get(a - 1) + 1;
                                const int jhi = ref_get(a);
                                for (int j = jlo; j <= jhi; ++j) {
                                    A.col[target_x].push_back(
                                        val - A.at(root.x, a) +
                                        A.at(root.x + width * n, j));
                                }
                                lastmagma = a;
                            } else {
                                if (lastmagma >= 0) {
                                    A.col[target_x].push_back(
                                        val - A.at(root.x, lastmagma) +
                                        A.at(root.x + width * n,
                                             reference[lastmagma]));
                                } else {
                                    A.col[target_x].push_back(
                                        val - A.at(root.x, 0) +
                                        A.at(root.x + width * n, 0));
                                }
                            }
                        } else {
                            A.col[target_x].push_back(val);
                        }
                    }
                }

                A.invalidate_vertical();  // vertical_cache.delete(A)
            }

            return A;
        }

        [[nodiscard]] inline std::string
            run(std::string_view text, int term, bool shorter) {
            const auto e = bmlike::parse(text);
            if (!e) return bmlike::kParseError;
            if (e->infinity)
                return bmlike::matrix_to_bm_string(bmlike::infinity_fs(term));
            if (e->m.cols() == 0) return {};
            if (!bmlike::is_limit(e->m))
                return bmlike::matrix_to_bm_string(
                    bmlike::drop_last_column(e->m));
            Grid A = expand(e->m, term);
            if (shorter) {
                // 删去最后一列：把最大已存在列标记为不存在。
                int last = -1;
                for (int x = 0; x < static_cast<int>(A.col.size()); ++x)
                    if (A.has(x)) last = x;
                if (last >= 0) A.exists[last] = 0;
            }
            return grid_to_string(A);
        }

    }  // namespace wmm_detail

    [[nodiscard]] inline std::string
        WMMNotation::expand_text(std::string_view text, int term) {
        return wmm_detail::run(text, term, true);
    }

    [[nodiscard]] inline std::string
        WMMNotation::expand_text_alter(std::string_view text, int term) {
        return wmm_detail::run(text, term, false);
    }

}  // namespace omegay::notation

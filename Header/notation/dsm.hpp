// dsm.hpp
// DSM —— Diagonal Sudden Matrix（对角突变矩阵系统）。
//
// 逻辑移植自 ne-rewritten/src/notations/BM-like/DSM.ts，仅保留核心展开与
// BM 文本接口。该记号不使用 BM 的 parents，而是自行计算“严格父项矩阵”，
// 并在父项矩阵上完成前驱（predecessor）搜索、上升 / 基底传播与矩阵重建，
// 最后由父项矩阵还原数值（construct_matrix_values）。
//
// 外层为 Y_FS_variants：极限的第 n 个基本列为三角矩阵
// (0)(1)(2,1)...(n,...,1)；FS 在核心展开后再多删一列，FS_alter 不删。
#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Header/notation/bmlike_share.hpp"

namespace omegay::notation {

    struct DSMNotation {
        static constexpr const char* kName = "DSM";
        [[nodiscard]] static std::string expand_text(std::string_view, int);
        [[nodiscard]] static std::string expand_text_alter(std::string_view, int);
        [[nodiscard]] static std::string suffix() { return {}; }
    };

    namespace dsm_detail {

        using bmlike::Matrix;

        // 变长列矩阵（列优先，对应 TS number[][]）。
        using Cols = std::vector<std::vector<int>>;
        struct Position { int r; int c; };

        [[nodiscard]] inline int v_at(const Cols& m, int r, int c) noexcept {
            if (c < 0 || c >= static_cast<int>(m.size())) return 0;
            if (r < 0 || r >= static_cast<int>(m[c].size())) return 0;
            return m[c][r];
        }

        // 行优先父项矩阵（-1 表示无父项）。
        using PRM = std::vector<std::vector<int>>;

        [[nodiscard]] inline int p_at(const PRM& p, int r, int c) noexcept {
            if (r < 0 || r >= static_cast<int>(p.size())) return -1;
            if (c < 0 || c >= static_cast<int>(p[r].size())) return -1;
            return p[r][c];
        }

        // DSM.ts get_predecessor。
        [[nodiscard]] inline std::optional<Position>
            get_predecessor(const PRM& parentsRM, int r, int c) {
            if (p_at(parentsRM, r, c) != -1 || r == 0) return std::nullopt;
            const int up_row = r - 1;

            int curr = p_at(parentsRM, up_row, c);
            while (curr != -1) {
                if (p_at(parentsRM, up_row, curr) != -1 &&
                    p_at(parentsRM, r, curr) == -1)
                    return Position{ r, curr };
                const int next = p_at(parentsRM, up_row, curr);
                if (next == -1) return Position{ up_row, curr };
                curr = next;
            }
            return Position{ up_row, c };
        }

        // DSM.ts construct_matrix_values：由列优先父项矩阵还原数值矩阵。
        [[nodiscard]] inline Cols construct_matrix_values(
            const std::vector<std::vector<int>>& pcm) {
            const int cols = static_cast<int>(pcm.size());
            const int rows = cols > 0 ? static_cast<int>(pcm[0].size()) : 0;
            Cols m(static_cast<std::size_t>(cols),
                   std::vector<int>(static_cast<std::size_t>(rows), 0));
            for (int c = 0; c < cols; ++c)
                for (int r = 0; r < rows; ++r) {
                    const int p = pcm[c][r];
                    if (p != -1) m[c][r] = m[p][r] + 1;
                }
            return m;
        }

        // 列优先 → 行优先。
        [[nodiscard]] inline PRM to_row_major(
            const std::vector<std::vector<int>>& pcm) {
            const int cols = static_cast<int>(pcm.size());
            const int rows = cols > 0 ? static_cast<int>(pcm[0].size()) : 0;
            PRM rm(static_cast<std::size_t>(rows),
                   std::vector<int>(static_cast<std::size_t>(cols), -1));
            for (int r = 0; r < rows; ++r)
                for (int c = 0; c < cols; ++c) rm[r][c] = pcm[c][r];
            return rm;
        }

        // DSM.ts generate_expansion。pcm 为列优先父项矩阵。
        [[nodiscard]] inline std::vector<std::vector<int>> generate_expansion(
            const std::vector<std::vector<int>>& pcm, int bad_row,
            int bad_col, int times, bool strong) {
            const int rows = static_cast<int>(pcm[0].size());
            const int cols = static_cast<int>(pcm.size());
            const int last_col = cols - 1;

            int target_row = -1;
            for (int r = rows - 1; r >= 0; --r)
                if (pcm[last_col][r] != -1) { target_row = r; break; }

            const int S = bad_col;
            const int E = last_col;
            const int seg_dist = E - S;

            const PRM parents = to_row_major(pcm);

            std::vector<std::vector<int>> final_cm;

            if (target_row == bad_row) {
                // 小展开（Small Expansion）。
                PRM expanded(static_cast<std::size_t>(rows));
                for (int r = 0; r < rows; ++r) {
                    expanded[r].reserve(cols);
                    for (int c = 0; c < cols; ++c)
                        expanded[r].push_back(parents[r][c]);
                }
                for (int i = 1; i <= times; ++i) {
                    const int shift = i * seg_dist;
                    for (int c = S; c <= E; ++c) {
                        const int new_c = c + shift;
                        for (int r = 0; r < rows; ++r) {
                            const int orig = parents[r][c];
                            int np = orig;
                            if (c == S && r < target_row)
                                np = parents[r][E] + shift - seg_dist;
                            else if (orig >= bad_col)
                                np = orig + shift;
                            while (static_cast<int>(expanded[r].size()) <= new_c)
                                expanded[r].push_back(-1);
                            expanded[r][new_c] = np;
                        }
                    }
                }
                const int width = static_cast<int>(expanded[0].size());
                final_cm.assign(
                    static_cast<std::size_t>(width),
                    std::vector<int>(static_cast<std::size_t>(rows), -1));
                for (int r = 0; r < rows; ++r)
                    for (int c = 0; c < width; ++c)
                        final_cm[c][r] = expanded[r][c];
            } else {
                // 完整展开（Full Expansion）。
                PRM result(static_cast<std::size_t>(rows),
                           std::vector<int>(static_cast<std::size_t>(cols), -1));
                for (int r = 0; r < rows; ++r)
                    for (int c = 0; c < cols; ++c)
                        result[r][c] = parents[r][c];

                // 上部（r < target_row 的部分）复制。
                for (int i = 1; i <= times; ++i) {
                    const int shift = i * seg_dist;
                    for (int c = S + 1; c <= E; ++c)
                        for (int r = 0; r < rows; ++r) {
                            const int orig = parents[r][c];
                            int nv = -1;
                            if (!(r == target_row && c == E))
                                nv = orig >= bad_col ? orig + shift : orig;
                            result[r].push_back(nv);
                        }
                }
                const int current_cols = static_cast<int>(result[0].size());

                // 上升 / 基底标记（仅在原始 cols×rows 范围内传播）。
                std::vector<std::vector<char>> is_rising(
                    static_cast<std::size_t>(rows),
                    std::vector<char>(static_cast<std::size_t>(cols), 0));
                std::vector<std::vector<char>> is_base(
                    static_cast<std::size_t>(rows),
                    std::vector<char>(static_cast<std::size_t>(cols), 0));

                is_rising[bad_row][bad_col] = 1;
                bool changed = true;
                while (changed) {
                    changed = false;
                    for (int r = bad_row; r < rows; ++r)
                        for (int c = 0; c < cols; ++c) {
                            if (is_rising[r][c]) continue;
                            bool become = false;
                            if (r == bad_row && strong) {
                                const int lp = bad_row > 0
                                                   ? parents[bad_row - 1][c]
                                                   : c - 1;
                                if (lp != -1 && is_rising[r][lp]) become = true;
                            }
                            const int pp = parents[r][c];
                            if (pp != -1 && is_rising[r][pp]) become = true;
                            if (!become && r > bad_row) {
                                const int up = parents[r - 1][c];
                                if (up != -1 && is_rising[r - 1][up])
                                    become = true;
                            }
                            if (!become && r < rows - 1 && is_rising[r + 1][c])
                                become = true;
                            if (become) {
                                is_rising[r][c] = 1;
                                changed = true;
                            }
                        }
                }

                std::vector<int> queue_base;
                for (int c = 0; c < cols; ++c) {
                    if (strong) {
                        const int lp = bad_row > 0 ? parents[bad_row - 1][c]
                                                   : c - 1;
                        if (lp == bad_col) is_base[bad_row][c] = 1;
                    } else {
                        if (parents[bad_row][c] == bad_col)
                            is_base[bad_row][c] = 1;
                    }
                    if (is_base[bad_row][c]) queue_base.push_back(c);
                }
                for (std::size_t qi = 0; qi < queue_base.size(); ++qi) {
                    const int cur_parent = queue_base[qi];
                    for (int c = 0; c < cols; ++c) {
                        bool hit = false;
                        if (strong) {
                            const int lp = bad_row > 0
                                               ? parents[bad_row - 1][c]
                                               : c - 1;
                            if (lp == cur_parent) hit = true;
                        } else {
                            if (parents[bad_row][c] == cur_parent) hit = true;
                        }
                        if (hit && !is_base[bad_row][c]) {
                            is_base[bad_row][c] = 1;
                            queue_base.push_back(c);
                        }
                    }
                }

                const int R = target_row - bad_row;
                const int C = last_col - bad_col;
                const int final_rows = rows + R * times;
                for (int r = rows; r < final_rows; ++r)
                    result.push_back(
                        std::vector<int>(static_cast<std::size_t>(current_cols),
                                         -1));

                for (int i = 1; i <= times; ++i) {
                    const int row_shift = R * i;
                    const int col_shift = C * i;

                    for (int r = 0; r < rows; ++r)
                        for (int c = 0; c < cols; ++c)
                            if (is_rising[r][c]) {
                                const int nr = r + row_shift;
                                const int nc = c + col_shift;
                                int val = parents[r][c];
                                if (val != -1) val += col_shift;
                                result[nr][nc] = val;
                            }

                    for (int c = 0; c < cols; ++c)
                        if (is_base[bad_row][c]) {
                            const int nc = c + col_shift;
                            int base_parent;
                            if (strong)
                                base_parent = bad_row > 0
                                                  ? parents[bad_row - 1][c]
                                                  : c - 1;
                            else
                                base_parent = parents[bad_row][c];
                            const int nbp = base_parent != -1
                                                ? base_parent + col_shift : -1;
                            for (int k = 0; k < row_shift; ++k)
                                result[bad_row + k][nc] = nbp;
                        }
                }

                const int width = static_cast<int>(result[0].size());
                final_cm.assign(
                    static_cast<std::size_t>(width),
                    std::vector<int>(static_cast<std::size_t>(final_rows), -1));
                for (int r = 0; r < final_rows; ++r)
                    for (int c = 0; c < width; ++c)
                        final_cm[c][r] = result[r][c];
            }

            return final_cm;
        }

        // 变长列矩阵的 BM 字典序比较（定义见下；get_bad_item_info 需要用到）。
        [[nodiscard]] inline int cols_compare(const Cols& a, const Cols& b);

        struct BadInfo {
            int bad_row;
            int bad_col;
            std::vector<std::vector<int>> pcm;  // 列优先父项矩阵
        };

        // DSM.ts get_bad_item_info。
        [[nodiscard]] inline std::optional<BadInfo>
            get_bad_item_info(const Cols& matrix) {
            if (matrix.empty()) return std::nullopt;
            const int cols = static_cast<int>(matrix.size());
            int rows = 0;
            for (const auto& c : matrix)
                rows = std::max(rows, static_cast<int>(c.size()));

            PRM mrm(static_cast<std::size_t>(rows),
                    std::vector<int>(static_cast<std::size_t>(cols), 0));
            for (int r = 0; r < rows; ++r)
                for (int c = 0; c < cols; ++c)
                    mrm[r][c] = v_at(matrix, r, c);

            PRM prm(static_cast<std::size_t>(rows),
                    std::vector<int>(static_cast<std::size_t>(cols), -1));
            for (int c = 1; c < cols; ++c) {
                const int val = mrm[0][c];
                for (int k = c - 1; k >= 0; --k)
                    if (mrm[0][k] < val) { prm[0][c] = k; break; }
            }
            for (int r = 1; r < rows; ++r)
                for (int c = 0; c < cols; ++c) {
                    const int val = mrm[r][c];
                    int chain = c;
                    while (true) {
                        if (chain != c && mrm[r][chain] < val) {
                            prm[r][c] = chain;
                            break;
                        }
                        chain = chain != -1 ? prm[r - 1][chain] : -1;
                        if (chain == -1) break;
                    }
                }

            int target_row = -1;
            const int target_col = cols - 1;
            for (int r = rows - 1; r >= 0; --r)
                if (prm[r][target_col] != -1) { target_row = r; break; }
            if (target_row == -1) return std::nullopt;
            const int parent_col = prm[target_row][target_col];
            if (parent_col == -1) return std::nullopt;

            std::vector<Position> pool;
            std::vector<Position> options;
            Position curr{ target_row, parent_col };
            pool.push_back(curr);
            options.push_back(curr);
            auto pred = get_predecessor(prm, curr.r, curr.c);
            while (pred) {
                pool.push_back(*pred);
                curr = *pred;
                pred = get_predecessor(prm, curr.r, curr.c);
            }
            std::sort(pool.begin(), pool.end(),
                      [](const Position& a, const Position& b) {
                          return b.c < a.c;
                      });

            Position prev{ target_row, target_col };
            for (const auto& item : pool)
                if (item.r < prev.r) { options.push_back(item); prev = item; }

            std::vector<std::vector<int>> pcm(
                static_cast<std::size_t>(cols),
                std::vector<int>(static_cast<std::size_t>(rows), -1));
            for (int r = 0; r < rows; ++r)
                for (int c = 0; c < cols; ++c) pcm[c][r] = prm[r][c];

            const Cols standard_seg = construct_matrix_values(
                generate_expansion(pcm, target_row, parent_col, 1, true));

            Position bad_item{ 0, 0 };
            bool found = false;
            for (const auto& cand : pool) {
                const Cols cand_seg = construct_matrix_values(
                    generate_expansion(pcm, cand.r, cand.c, 1, true));
                if (cols_compare(cand_seg, standard_seg) < 0) {
                    Position chosen;
                    bool any = false;
                    int best_c = -1;
                    for (const auto& opt : options)
                        if (opt.c > cand.c && (!any || opt.c < best_c)) {
                            best_c = opt.c;
                            chosen = opt;
                            any = true;
                        }
                    bad_item = any ? chosen : options.back();
                    found = true;
                    break;
                }
            }
            if (!found) bad_item = options.back();

            return BadInfo{ bad_item.r, bad_item.c, std::move(pcm) };
        }

        // 变长列矩阵的 BM 字典序比较（同 bmlike::compare 语义）。
        [[nodiscard]] inline int
            cols_compare(const Cols& a, const Cols& b) {
            auto norm = [](const std::vector<int>& col) {
                int h = static_cast<int>(col.size());
                while (h > 0 && col[h - 1] == 0) --h;
                return h;
            };
            const int n = std::min(static_cast<int>(a.size()),
                                   static_cast<int>(b.size()));
            for (int c = 0; c < n; ++c) {
                const int ha = norm(a[c]), hb = norm(b[c]);
                const int k = std::min(ha, hb);
                for (int y = 0; y < k; ++y)
                    if (a[c][y] != b[c][y])
                        return a[c][y] < b[c][y] ? -1 : 1;
                if (ha != hb) return ha < hb ? -1 : 1;
            }
            if (a.size() != b.size())
                return a.size() < b.size() ? -1 : 1;
            return 0;
        }

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

        // DSM.ts expand_normal。
        [[nodiscard]] inline Cols expand_normal(const Cols& matrix, int times) {
            if (matrix.empty()) return {};
            const int cols = static_cast<int>(matrix.size());
            bool last_zero = true;
            for (int v : matrix[cols - 1])
                if (v != 0) { last_zero = false; break; }
            if (last_zero) return Cols(matrix.begin(), matrix.end() - 1);

            const auto info = get_bad_item_info(matrix);
            if (!info) return {};

            Cols full = construct_matrix_values(generate_expansion(
                info->pcm, info->bad_row, info->bad_col, times, false));
            if (!full.empty()) full.pop_back();
            normalize_cols(full);
            return full;
        }

        // 由标准化矩形 Matrix 取变长列。
        [[nodiscard]] inline Cols to_cols(const Matrix& m) {
            Cols cols(static_cast<std::size_t>(m.cols()));
            for (int c = 0; c < m.cols(); ++c) {
                cols[c].resize(static_cast<std::size_t>(m.rows()));
                for (int r = 0; r < m.rows(); ++r) cols[c][r] = m(r, c);
            }
            return cols;
        }

        [[nodiscard]] inline std::string
            run(std::string_view text, int term, bool shorter) {
            const auto e = bmlike::parse(text);
            if (!e) return bmlike::kParseError;
            if (e->infinity)
                return bmlike::matrix_to_bm_string(
                    bmlike::triangular_infinity_fs(term));
            if (e->m.cols() == 0) return {};
            if (!bmlike::is_limit(e->m))
                return bmlike::matrix_to_bm_string(
                    bmlike::drop_last_column(e->m));
            Cols longer = expand_normal(to_cols(e->m), term);
            if (shorter && !longer.empty()) longer.pop_back();
            return cols_to_string(longer);
        }

    }  // namespace dsm_detail

    [[nodiscard]] inline std::string
        DSMNotation::expand_text(std::string_view text, int term) {
        return dsm_detail::run(text, term, true);
    }

    [[nodiscard]] inline std::string
        DSMNotation::expand_text_alter(std::string_view text, int term) {
        return dsm_detail::run(text, term, false);
    }

}  // namespace omegay::notation

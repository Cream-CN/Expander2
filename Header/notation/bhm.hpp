// bhm.hpp
// BHM —— Bashicu Hyper Matrix（Bashicu 高阶矩阵系统），以及参数化系列
// nBM-BHM（BMS(n 行) + BHM：顶行小于 n 时退化为 BM4 展开）。
//
// 逻辑移植自 ne-rewritten/src/notations/BM-like/BHM.ts，仅保留核心展开与
// BM 文本接口；BM4 核心展开复用 bmlike_share.hpp::bm4_expand。
// FS / FS_alter 由 shorter 参数区分（对应 sequence_FS_variants）。
#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "Header/notation/bmlike_share.hpp"

namespace omegay::notation {

    struct BHMNotation {
        static constexpr const char* kName = "BHM";
        [[nodiscard]] static std::string expand_text(std::string_view, int);
        [[nodiscard]] static std::string expand_text_alter(std::string_view, int);
        [[nodiscard]] static std::string suffix() { return {}; }
    };

    namespace bhm_detail {

        using bmlike::Matrix;

        // 把若干（可能不等长的）列打包为规范化矩形。
        [[nodiscard]] inline Matrix
            pack_columns(const std::vector<std::vector<int>>& cols) {
            int rows = 0;
            for (const auto& c : cols)
                rows = std::max(rows, static_cast<int>(c.size()));
            Matrix m(rows, static_cast<int>(cols.size()));
            for (int c = 0; c < static_cast<int>(cols.size()); ++c)
                for (int y = 0; y < static_cast<int>(cols[c].size()); ++y)
                    m(y, c) = cols[c][y];
            return bmlike::normalize(m);
        }

        // BHM.ts ascension_thresholds。
        [[nodiscard]] inline std::vector<int> ascension_thresholds(
            const std::vector<std::vector<int>>& P, int r,
            const std::vector<int>& roots, int b) {
            const int nc = static_cast<int>(P.size());
            std::vector<int> result(nc, 0);
            result[r] = b;
            for (int i = r + 1; i < nc; ++i) {
                if (std::find(roots.begin(), roots.end(), i) != roots.end()) {
                    result[i] = b;
                } else {
                    int threshold = 0;
                    while (threshold < static_cast<int>(P[i].size()) &&
                           threshold < b) {
                        const int pp = bmlike::parent_at(P, i, threshold);
                        if (pp < 0 || threshold >= result[pp]) break;
                        ++threshold;
                    }
                    result[i] = threshold;
                }
            }
            return result;
        }

        // BHM.ts ascension_vector：末列相对根列 r、前 b 行的差。
        [[nodiscard]] inline std::vector<int>
            ascension_vector(const Matrix& m, int r, int b) {
            const int right = m.cols() - 1;
            std::vector<int> V(static_cast<std::size_t>(b), 0);
            for (int y = 0; y < b; ++y)
                V[y] = m(y, right) - bmlike::at(m, y, r);
            return V;
        }

        // BHM.ts ascend_vector：列 + w·V（仅前 A 行）。
        [[nodiscard]] inline std::vector<int> ascend_vector(
            const Matrix& m, int col, const std::vector<int>& V,
            int A, int w) {
            const int len = std::max(m.rows(), A);
            std::vector<int> out(static_cast<std::size_t>(len), 0);
            for (int y = 0; y < len; ++y) {
                const int v = bmlike::at(m, y, col);
                out[y] = y < A ? v + w * (y < static_cast<int>(V.size()) ? V[y] : 0)
                               : v;
            }
            return out;
        }

        // BHM.ts compute_expansion。A 为选定根对应的阈值向量（每列一个高度）。
        [[nodiscard]] inline Matrix compute_expansion(
            const Matrix& m, int r, const std::vector<int>& V,
            const std::vector<int>& A, int index, bool shorter) {
            const int right = m.cols() - 1;
            std::vector<std::vector<int>> cols;
            for (int c = 0; c < right; ++c) {
                std::vector<int> col;
                for (int y = 0; y < m.rows(); ++y) col.push_back(m(y, c));
                cols.push_back(std::move(col));
            }
            for (int w = 1; w <= index + 1; ++w) {
                if (shorter && w > index) break;
                for (int i = r; i < right; ++i) {
                    cols.push_back(ascend_vector(m, i, V, A[i], w));
                    if (w > index) break;
                }
            }
            return pack_columns(cols);
        }

        // BHM.ts extend：一次较短展开后再追加提升后的末列。
        [[nodiscard]] inline Matrix
            extend(const Matrix& m, int r, const std::vector<int>& V,
                   const std::vector<int>& A) {
            Matrix base = compute_expansion(m, r, V, A, 1, true);
            // 在 base（已规范化）之外补一列；直接用列向量重建以便等长处理。
            std::vector<std::vector<int>> cols;
            for (int c = 0; c < base.cols(); ++c)
                cols.push_back(bmlike::normalized_column(base, c));
            cols.push_back(ascend_vector(m, m.cols() - 1, V,
                                         A[m.cols() - 1], 1));
            return pack_columns(cols);
        }

        // BHM.ts BHM_expand。
        [[nodiscard]] inline Matrix
            bhm_expand(const Matrix& input, int index, bool shorter) {
            Matrix m = bmlike::standardize(input);
            const int nc = m.cols();
            if (nc == 0) return m;
            const int right = nc - 1;
            const int top = m.rows() - 1;
            if (top < 0) return bmlike::drop_last_column(m);  // 空末列（防御）

            const auto P = bmlike::parents(m);
            const int pr = bmlike::parent_at(P, right, top);
            const int special_root =
                pr < 0 ? -1 : bmlike::parent_at(P, pr, top);

            std::vector<int> roots;
            for (int i = right;;) {
                i = top > 0 ? bmlike::parent_at(P, i, top - 1) : i - 1;
                if (i <= special_root) break;
                if (bmlike::parent_at(P, i, top) == special_root)
                    roots.push_back(i);
            }
            if (roots.empty()) return bmlike::drop_last_column(m);  // 防御

            std::vector<std::vector<int>> A(nc), V(nc);
            for (int r : roots) {
                A[r] = ascension_thresholds(P, r, roots, top);
                V[r] = ascension_vector(m, r, top);
            }

            const Matrix threshold = extend(m, roots[0], V[roots[0]], A[roots[0]]);
            int ri = static_cast<int>(roots.size());
            for (int i = 0; i < static_cast<int>(roots.size()); ++i) {
                const int r = roots[i];
                if (bmlike::compare(extend(m, r, V[r], A[r]), threshold) < 0) {
                    ri = i;
                    break;
                }
            }
            const int r_actual = roots[ri - 1];
            return compute_expansion(m, r_actual, V[r_actual],
                                     A[r_actual], index, shorter);
        }

        // BHM.ts BM_BHM_expand：顶行小于 n 走 BM4，否则走 BHM。
        [[nodiscard]] inline Matrix
            bm_bhm_expand(const Matrix& input, int index, int n, bool shorter) {
            Matrix m = bmlike::standardize(input);
            if (m.cols() == 0) return m;
            const int top = m.rows() - 1;
            if (top < 0) return bmlike::drop_last_column(m);
            if (top < n) return bmlike::bm4_expand(m, index, shorter);
            return bhm_expand(m, index, shorter);
        }

    }  // namespace bhm_detail

    [[nodiscard]] inline std::string
        BHMNotation::expand_text(std::string_view text, int term) {
        return bmlike::expand_text_with(
            text, term, true,
            [](const bmlike::Matrix& m, int idx, bool) {
                return bhm_detail::bhm_expand(m, idx, true);
            });
    }

    [[nodiscard]] inline std::string
        BHMNotation::expand_text_alter(std::string_view text, int term) {
        return bmlike::expand_text_with(
            text, term, false,
            [](const bmlike::Matrix& m, int idx, bool) {
                return bhm_detail::bhm_expand(m, idx, false);
            });
    }

    // nBM-BHM：N 为 BM_BHM(N) 的行数参数（顶行小于 N 时走 BM4 展开）。
    template <int N>
    struct BM_BHMNotation {
        static constexpr int kRows = N;

        [[nodiscard]] static std::string
            expand_text(std::string_view text, int term) {
            return bmlike::expand_text_with(
                text, term, true,
                [](const bmlike::Matrix& m, int idx, bool) {
                    return bhm_detail::bm_bhm_expand(m, idx, N, true);
                });
        }

        [[nodiscard]] static std::string
            expand_text_alter(std::string_view text, int term) {
            return bmlike::expand_text_with(
                text, term, false,
                [](const bmlike::Matrix& m, int idx, bool) {
                    return bhm_detail::bm_bhm_expand(m, idx, N, false);
                });
        }

        [[nodiscard]] static std::string suffix() { return {}; }
    };

}  // namespace omegay::notation

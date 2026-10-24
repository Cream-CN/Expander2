// bsm.hpp
// BSM —— Bashicu sudden matrix（Bashicu 突变矩阵系统）。
//
// 逻辑移植自 ne-rewritten/src/notations/BM-like/BSM.ts，仅保留核心展开
// （Expand）逻辑与 BM 文本接口，舍弃记忆化缓存表（UI 层职责）。
// 父项链、规范化、比较与文本解析复用 bmlike_share.hpp。
//
// 记号接口走文本槽：BSMNotation::expand_text 对应 TS 中 BSM.FS 的完整语义
// （记号极限 → 极限基本列；后继型 → 删除末列；极限表达式 → expand）。
#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "Header/notation/bmlike_share.hpp"

namespace omegay::notation {

    struct BSMNotation {
        static constexpr const char* kName = "BSM";

        [[nodiscard]] static std::string
            expand_text(std::string_view text, int term);

        [[nodiscard]] static std::string suffix() { return {}; }
    };

    namespace bsm_detail {

        using bmlike::Expr;
        using bmlike::Matrix;

        // “上升”（ascending）判定：r <= x，且 x 本身是根，或 x 在第 y 行的
        // 父项相对 r 上升。对应 BSM.ts ascending，带按 (x,y) 的记忆化。
        class Ascending {
        public:
            Ascending(const std::vector<std::vector<int>>& P,
                      const std::vector<int>& roots, int nc)
                : P_(P), roots_(roots),
                  memo_(static_cast<std::size_t>(nc),
                        std::vector<signed char>(0, -1)) {}

            bool operator()(int r, int x, int y) {
                if (x < r) return false;
                if (x < 0) return false;
                if (y >= static_cast<int>(memo_[x].size()))
                    memo_[x].resize(static_cast<std::size_t>(y) + 1, -1);
                if (memo_[x][y] != -1) return memo_[x][y] != 0;

                const bool is_root =
                    std::find(roots_.begin(), roots_.end(), x) != roots_.end();
                bool result;
                if (is_root) {
                    result = true;
                } else {
                    const int p = bmlike::parent_at(P_, x, y);
                    result = (*this)(r, p, y);
                }
                memo_[x][y] = result ? 1 : 0;
                return result;
            }

        private:
            const std::vector<std::vector<int>>& P_;
            const std::vector<int>& roots_;
            std::vector<std::vector<signed char>> memo_;
        };

        // delta：末列相对根列 r 的增量；LNZ 行额外减 1（BSM.ts delta）。
        [[nodiscard]] inline std::vector<int>
            delta(const Matrix& m, int end_col, int r, int LNZ, int rows) {
            std::vector<int> d(static_cast<std::size_t>(rows), 0);
            for (int y = 0; y < rows; ++y) {
                const int cv = m(y, end_col);
                const int rv = bmlike::at(m, y, r);
                if (y < LNZ) d[y] = cv - rv;
                else if (y == LNZ) d[y] = cv - rv - 1;
            }
            return d;
        }

        // expansion：取前 end_col 列，随后复制 n 段 [r, end_col)，
        // 第 a 段按 a*delta、且仅在 ascending 成立的行上加增量。
        [[nodiscard]] inline Matrix
            expansion(const Matrix& m, int r, int n, int end_col, int rows,
                      const std::vector<int>& del,
                      const std::vector<std::vector<int>>& P,
                      const std::vector<int>& roots) {
            const int seg = end_col - r;
            Matrix out(rows, end_col + n * seg);
            for (int c = 0; c < end_col; ++c)
                for (int y = 0; y < rows; ++y) out(y, c) = m(y, c);

            Ascending asc(P, roots, m.cols());
            int dst = end_col;
            for (int a = 1; a <= n; ++a) {
                for (int x = r; x < end_col; ++x) {
                    for (int y = 0; y < rows; ++y) {
                        const int base = m(y, x);
                        const int v = asc(r, x, y) ? base + a * del[y] : base;
                        out(y, dst) = v;
                    }
                    ++dst;
                }
            }
            return out;
        }

        // expansion_append：expansion(r,1) 后再追加“提升后的末列”。
        [[nodiscard]] inline Matrix
            expansion_append(const Matrix& m, int r, int end_col, int rows,
                             const std::vector<int>& del,
                             const std::vector<std::vector<int>>& P,
                             const std::vector<int>& roots) {
            Matrix base = expansion(m, r, 1, end_col, rows, del, P, roots);
            Matrix out(rows, base.cols() + 1);
            for (int c = 0; c < base.cols(); ++c)
                for (int y = 0; y < rows; ++y) out(y, c) = base(y, c);

            Ascending asc(P, roots, m.cols());
            for (int y = 0; y < rows; ++y) {
                const int cv = m(y, end_col);
                out(y, base.cols()) = asc(r, end_col, y) ? cv + del[y] : cv;
            }
            return out;
        }

        // 核心展开（BSM.ts expand）。
        [[nodiscard]] inline Matrix expand(const Matrix& input, int term) {
            Matrix m = bmlike::standardize(input);
            const int nc = m.cols();
            if (nc == 0) return m;
            const int end_col = nc - 1;
            const int rows = m.rows();

            Matrix prefix(rows, end_col);
            for (int c = 0; c < end_col; ++c)
                for (int y = 0; y < rows; ++y) prefix(y, c) = m(y, c);

            // 末列最后一个正值所在行 LNZ；全零为后继型，删末列。
            int LNZ = -1;
            for (int y = rows - 1; y >= 0; --y) {
                if (m(y, end_col) > 0) { LNZ = y; break; }
            }
            if (LNZ < 0) return prefix;

            const int y_max = rows - 1;
            const auto P = bmlike::parents(m);

            // special_roots：末列在 LNZ 行的父项链，末端补 -1（TS 原语义）。
            std::vector<int> special_roots;
            for (int cur = end_col; cur >= 0;) {
                cur = bmlike::parent_at(P, cur, LNZ);
                special_roots.push_back(cur);
                if (cur < 0) break;
            }

            // roots：沿 LNZ-1 行（第 0 行则左邻列）回溯，凡其 LNZ 行父项
            // 落在 special_roots 中的列皆入选。
            std::vector<int> roots;
            for (int cur = special_roots.empty() ? -1 : special_roots[0];
                 cur >= 0;
                 cur = LNZ > 0 ? bmlike::parent_at(P, cur, LNZ - 1)
                               : cur - 1) {
                const int p = bmlike::parent_at(P, cur, LNZ);
                if (std::find(special_roots.begin(), special_roots.end(), p)
                    != special_roots.end()) {
                    roots.push_back(cur);
                }
            }
            if (roots.empty()) return prefix;  // 防御：标准输入下不应发生

            const auto del0 = delta(m, end_col, roots[0], LNZ, rows);
            const Matrix threshold = expansion_append(
                m, roots[0], end_col, rows, del0, P, roots);

            // test_root：根列 LNZ 行以下的尾部（用于 special root 的快速判定）。
            std::vector<int> test_root;
            for (int y = LNZ + 1; y < rows; ++y)
                test_root.push_back(bmlike::at(m, y, roots[0]));

            int chosen = static_cast<int>(roots.size());  // 找不到则取最后一个
            for (int i = 0; i < static_cast<int>(roots.size()); ++i) {
                const int r = roots[i];
                bool smaller = false;
                if (std::find(special_roots.begin(), special_roots.end(), r)
                    != special_roots.end()) {
                    for (std::size_t dy = 0; dy < test_root.size(); ++dy) {
                        if (bmlike::at(m, LNZ + 1 + static_cast<int>(dy), r)
                            != test_root[dy]) {
                            smaller = true;
                            break;
                        }
                    }
                } else {
                    const auto delr = delta(m, end_col, r, LNZ, rows);
                    const Matrix cand = expansion_append(
                        m, r, end_col, rows, delr, P, roots);
                    smaller = bmlike::compare(cand, threshold) < 0;
                }
                if (smaller) { chosen = i; break; }
            }

            const int r_actual = roots[chosen - 1];
            const auto del = delta(m, end_col, r_actual, LNZ, rows);
            Matrix res = expansion(m, r_actual, term, end_col, rows, del,
                                   P, roots);

            // 若顶行（y_max）在所有结果列中均为 0，则截去该尾行。
            if (y_max > 0) {
                bool all_zero = true;
                for (int c = 0; c < res.cols(); ++c)
                    if (res(y_max, c) != 0) { all_zero = false; break; }
                if (all_zero) {
                    Matrix trimmed(y_max, res.cols());
                    for (int c = 0; c < res.cols(); ++c)
                        for (int y = 0; y < y_max; ++y)
                            trimmed(y, c) = res(y, c);
                    res = trimmed;
                }
            }
            return bmlike::normalize(res);
        }

    }  // namespace bsm_detail

    [[nodiscard]] inline std::string
        BSMNotation::expand_text(std::string_view text, int term) {
        const auto e = bmlike::parse(text);
        if (!e) return bmlike::kParseError;
        if (e->infinity)
            return bmlike::matrix_to_bm_string(bmlike::infinity_fs(term));
        if (e->m.cols() == 0) return {};
        const bmlike::Matrix r = bsm_detail::expand(e->m, term < 1 ? 1 : term);
        return bmlike::matrix_to_bm_string(r);
    }

}  // namespace omegay::notation

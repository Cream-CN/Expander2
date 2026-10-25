// lpms.hpp
// LPMS —— Lifting Projection Matrix System（提升投影矩阵系统），
// LPTSS —— Lifting Projection Triple Sequence System（仅极限基本列不同）。
//
// 二者共用同一套展开引擎（移植自 ne-rewritten/src/notations/BM-like/LPMS.ts），
// 仅保留核心展开与 BM 文本接口，舍弃 0Y / simple 等显示层。
//
// 引擎要点：
//   makeCtx 维护多级“B 父列”与“A 祖先链”（带记忆化）；
//   upmsPrepare 计算好部 G、坏部 B、阶差 delta、UPMS 标记 v(r,c) 与 B1/B2；
//   bmsFS 是投影复制（按祖先是否含根决定是否施加 h·delta）；
//   lpmsSingle 用“阶段列（缺失视为 -1）”做五种情形的提升，把三行以上的
//   合法极限反复提升，直到落入可投影形态再交 bmsFS。
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "Header/notation/bmlike_share.hpp"

namespace omegay::notation {

    struct LPMSNotation;
    struct LPTSSNotation;

    namespace lpms_detail {

        using bmlike::Matrix;
        using Col = std::vector<int>;
        using FMat = std::vector<Col>;

        // ---------- 基础列工具 ----------
        [[nodiscard]] inline Col clone_col(const Col& c) { return c; }
        [[nodiscard]] inline int max_rows(const FMat& m) noexcept {
            int r = 0;
            for (const auto& c : m)
                r = std::max<int>(r, static_cast<int>(c.size()));
            return r;
        }
        [[nodiscard]] inline FMat standardize(const FMat& m, int rows) {
            FMat out;
            out.reserve(m.size());
            for (const auto& c : m) {
                Col v(static_cast<std::size_t>(rows), 0);
                for (int r = 0; r < rows; ++r)
                    v[r] = r < static_cast<int>(c.size()) ? c[r] : 0;
                out.push_back(std::move(v));
            }
            return out;
        }
        [[nodiscard]] inline FMat standardize(const FMat& m) {
            return standardize(m, max_rows(m));
        }
        [[nodiscard]] inline Col zero_col(int rows) {
            return Col(static_cast<std::size_t>(rows), 0);
        }
        [[nodiscard]] inline Col ones_col(int rows) {
            return Col(static_cast<std::size_t>(rows), 1);
        }
        [[nodiscard]] inline Col const_col(int a, int rows) {
            Col c = zero_col(rows);
            if (rows > 0) c[0] = a;
            return c;
        }
        // 列比较；a/b 缺省元素分别取 missingA/missingB。
        [[nodiscard]] inline int
            col_compare(const Col* a, const Col* b, int rows,
                        int missing_a = 0, int missing_b = 0) {
            for (int r = 0; r < rows; ++r) {
                const int av =
                    (a && r < static_cast<int>(a->size())) ? (*a)[r]
                                                            : missing_a;
                const int bv =
                    (b && r < static_cast<int>(b->size())) ? (*b)[r]
                                                            : missing_b;
                if (av < bv) return -1;
                if (av > bv) return 1;
            }
            return 0;
        }
        [[nodiscard]] inline bool is_nat(int x) noexcept {
            return x >= 0;  // 输入整数
        }
        [[nodiscard]] inline bool legal(const FMat& in) {
            if (in.empty()) return true;
            const int rows = max_rows(in);
            const FMat m = standardize(in, rows);
            for (int r = 0; r < rows; ++r)
                if (m[0][r] != 0) return false;
            for (int c = 0; c < static_cast<int>(m.size()); ++c)
                for (int r = 1; r < rows; ++r)
                    if (m[c][r] > m[c][r - 1]) return false;
            return true;
        }
        [[nodiscard]] inline int last_nonzero_row(const FMat& m) {
            if (m.empty()) return -1;
            const Col& last = m.back();
            for (int r = static_cast<int>(last.size()) - 1; r >= 0; --r)
                if (last[r] != 0) return r + 1;
            return -1;
        }
        [[nodiscard]] inline bool last_all_zero(const FMat& m) {
            if (m.empty()) return true;
            return std::all_of(m.back().begin(), m.back().end(),
                               [](int v) { return v == 0; });
        }

        // ---------- 上下文：多级父列 / 祖先链 ----------
        struct Anc {
            std::vector<int> list;
            std::vector<unsigned char> mask;
        };

        class Ctx {
        public:
            FMat m;
            int rows = 0;
            int cols = 0;

            Ctx() = default;
            explicit Ctx(const FMat& matrix, int rows_)
                : m(standardize(matrix, rows_)), rows(rows_),
                  cols(static_cast<int>(matrix.size())),
                  parent_cache_(static_cast<std::size_t>(rows) + 1,
                                std::vector<int>(matrix.size(), -2)),
                  anc_cache_(static_cast<std::size_t>(rows) + 1,
                             std::vector<Anc*>(matrix.size(), nullptr)) {}
            ~Ctx() {
                for (auto& row : anc_cache_)
                    for (Anc* p : row) delete p;
            }
            Ctx(const Ctx&) = delete;
            Ctx& operator=(const Ctx&) = delete;

            int get_b_parent(int col, int b) {
                if (b < 1 || b > rows || col < 0 || col >= cols) return -1;
                int cached = parent_cache_[b][col];
                if (cached != -2) return cached;
                const int row = b - 1;
                const int value = m[col][row];
                const Anc& a = get_a_ancestors(col, b - 1);
                int best = -1;
                for (int candidate : a.list) {
                    if (candidate >= col) continue;
                    if (m[candidate][row] < value) {
                        best = candidate;
                        break;
                    }
                }
                parent_cache_[b][col] = best;
                return best;
            }

            const Anc& get_a_ancestors(int col, int a) {
                static const Anc empty;
                if (a < 0 || a > rows || col < 0 || col >= cols)
                    return empty;
                Anc* cached = anc_cache_[a][col];
                if (cached) return *cached;
                auto* p = new Anc();
                p->mask.assign(static_cast<std::size_t>(cols), 0);
                int current = col, guard = 0;
                while (current != -1 && !p->mask[current] &&
                       guard++ <= cols + 2) {
                    p->list.push_back(current);
                    p->mask[current] = 1;
                    current = (a == 0)
                                  ? (current > 0 ? current - 1 : -1)
                                  : get_b_parent(current, a);
                }
                anc_cache_[a][col] = p;
                return *p;
            }

        private:
            std::vector<std::vector<int>> parent_cache_;
            std::vector<std::vector<Anc*>> anc_cache_;
        };

        struct Root {
            int root;
            int t;
        };

        [[nodiscard]] inline bool find_root(Ctx& ctx, Root& out) {
            const int t = last_nonzero_row(ctx.m);
            if (t < 1) return false;
            const int root = ctx.get_b_parent(ctx.cols - 1, t);
            if (root < 0) return false;
            out.root = root;
            out.t = t;
            return true;
        }

        [[nodiscard]] inline Col compute_delta(Ctx& ctx, int root, int t) {
            const int alpha = ctx.cols - 1;
            Col d = zero_col(ctx.rows);
            for (int r = 0; r < ctx.rows; ++r)
                d[r] = (r >= t - 1) ? 0
                                    : ctx.m[alpha][r] - ctx.m[root][r];
            return d;
        }
        [[nodiscard]] inline int max_entry(const FMat& m) {
            int mx = 0;
            for (const auto& c : m)
                for (int v : c) mx = std::max(mx, v);
            return mx;
        }

        struct XY {
            FMat X, Y;
            int u, j;
            bool u_missing;
        };

        struct Prep {
            Ctx ctx;
            int root = 0, t = 0, alpha = 0;
            FMat G, B, B1, B2;
            Col delta;
            int stride = 1;
            std::vector<int8_t> vr;  // -1 未定义

            Prep() = default;
            Prep(const Prep&) = delete;
            Prep& operator=(const Prep&) = delete;

            int get_vr(int c, int r) const {
                if (c >= root && c < alpha && r < t - 1)
                    return vr[c * stride + r];
                return -1;
            }
            void set_vr(int c, int r, int val) {
                vr[c * stride + r] = static_cast<int8_t>(val);
            }

            Col base_col_for(int col, int k) {
                Col base = zero_col(rows());
                for (int r = 0; r < rows(); ++r)
                    base[r] = (r <= k) ? ctx.m[col][r] : 0;
                for (int r = 0; r < k; ++r) base[r]++;
                return base;
            }

            int rows() const { return ctx.rows; }

            XY build_xy(int col, int k) {
                const int R = rows();
                Col base = base_col_for(col, k);
                int u = -1;
                bool u_missing = false;
                for (int c = col + 1; c <= alpha; ++c) {
                    if (col_compare(&ctx.m[c], &base, R) < 0) {
                        u = c;
                        break;
                    }
                }
                if (u < 0) {
                    u = alpha + 1;
                    u_missing = true;
                }
                const int ayk = ctx.m[root][k - 1];
                const Anc& alpha_anc = ctx.get_a_ancestors(alpha, k);
                int j = -1;
                for (int c : alpha_anc.list)
                    if (ctx.m[c][k - 1] == ayk + 1) {
                        j = c;
                        break;
                    }
                if (j < 0) j = alpha;

                const int max_twice = max_entry_cached_;
                XY xy;
                xy.u = u;
                xy.j = j;
                xy.u_missing = u_missing;
                for (int c = col; c <= u - 1; ++c) {
                    Col out = ctx.m[c];
                    for (int s = 1; s <= k - 1; ++s) {
                        const int r = s - 1;
                        if (get_vr(c, r) == 1)
                            out[r] += max_twice - ctx.m[col][r];
                    }
                    xy.X.push_back(std::move(out));
                }
                for (int c = j; c <= alpha; ++c) {
                    Col out = ctx.m[c];
                    for (int s = 1; s <= k - 1; ++s) {
                        const int r = s - 1;
                        if (c == j ||
                            ctx.get_a_ancestors(c, s).mask[j] == 1)
                            out[r] += max_twice - ctx.m[j][r];
                    }
                    xy.Y.push_back(std::move(out));
                }
                return xy;
            }

            int max_entry_cached_ = 0;
        };

        [[nodiscard]] inline Prep* make_prep(const FMat& matrix, int rows) {
            auto* prep = new Prep();
            prep->ctx.~Ctx();
            new (&prep->ctx) Ctx(matrix, rows);
            Ctx& ctx = prep->ctx;
            if (ctx.cols == 0) {
                delete prep;
                return nullptr;
            }
            Root fr;
            if (!find_root(ctx, fr)) {
                delete prep;
                return nullptr;
            }
            prep->root = fr.root;
            prep->t = fr.t;
            const int root = prep->root, t = prep->t;
            const int alpha = ctx.cols - 1;
            prep->alpha = alpha;
            for (int c = 0; c < root; ++c) prep->G.push_back(ctx.m[c]);
            for (int c = root; c < alpha; ++c) prep->B.push_back(ctx.m[c]);
            prep->delta = compute_delta(ctx, root, t);
            const int max_twice = max_entry(ctx.m) * 2;
            prep->max_entry_cached_ = max_twice;
            prep->stride = std::max(1, ctx.rows);
            prep->vr.assign(static_cast<std::size_t>(ctx.cols) *
                                std::max(1, ctx.rows),
                            static_cast<int8_t>(-1));

            auto matrix_lex = [&](const FMat& X, const FMat& Y) {
                const int len = static_cast<int>(std::max(X.size(), Y.size()));
                Col miss = zero_col(ctx.rows);
                for (int r = 0; r < ctx.rows; ++r) miss[r] = -1;
                for (int c = 0; c < len; ++c) {
                    const Col* xc =
                        c < static_cast<int>(X.size()) ? &X[c] : &miss;
                    const Col* yc =
                        c < static_cast<int>(Y.size()) ? &Y[c] : &miss;
                    const int cmp = col_compare(xc, yc, ctx.rows);
                    if (cmp) return cmp;
                }
                return 0;
            };

            for (int row = 0; row < t - 1; ++row) {
                const int k = row + 1;
                for (int col = root; col < alpha; ++col) {
                    if (col == root || row == 0) {
                        prep->set_vr(col, row, 1);
                        continue;
                    }
                    const Anc& ancestors = ctx.get_a_ancestors(col, k);
                    bool has0 = false;
                    for (int a : ancestors.list)
                        if (prep->get_vr(a, row) == 0) {
                            has0 = true;
                            break;
                        }
                    const int parent = ctx.get_b_parent(col, k);
                    if (ancestors.mask[root] != 1 || has0 || parent < 0) {
                        prep->set_vr(col, row, 0);
                        continue;
                    }
                    if (parent != root) {
                        prep->set_vr(col, row, 1);
                        continue;
                    }
                    bool earlier0 = false;
                    for (int r = 0; r < row; ++r)
                        if (prep->get_vr(col, r) == 0) {
                            earlier0 = true;
                            break;
                        }
                    if (earlier0) {
                        prep->set_vr(col, row, 0);
                        continue;
                    }
                    bool higher_escapes = false;
                    for (int r = row + 1; r < t; ++r)
                        if (ctx.get_b_parent(col, r + 1) != root) {
                            higher_escapes = true;
                            break;
                        }
                    if (higher_escapes) {
                        prep->set_vr(col, row, 0);
                        continue;
                    }
                    XY xy = prep->build_xy(col, k);
                    if (xy.u == alpha + 1 || xy.u_missing) {
                        prep->set_vr(col, row, 1);
                        continue;
                    }
                    prep->set_vr(col, row,
                                 matrix_lex(xy.X, xy.Y) < 0 ? 0 : 1);
                }
            }

            auto bhs = [&](int h) {
                FMat out;
                for (int local = 0;
                     local < static_cast<int>(prep->B.size()); ++local) {
                    const int original = root + local;
                    Col c(static_cast<std::size_t>(ctx.rows));
                    for (int r = 0; r < ctx.rows; ++r)
                        c[r] = prep->B[local][r] +
                               h * prep->delta[r] *
                                   (r < t - 1 &&
                                            prep->get_vr(original, r) == 1
                                        ? 1
                                        : 0);
                    out.push_back(std::move(c));
                }
                return out;
            };
            prep->B1 = bhs(1);
            prep->B2 = bhs(2);
            return prep;
        }

        // ---------- 投影复制 ----------
        [[nodiscard]] inline FMat
            upms_single(const FMat& matrix, int l, int rows,
                        bool l_finite) {
            const FMat m = standardize(matrix, rows);
            if (m.empty() || last_all_zero(m)) {
                FMat r;
                for (int i = 0; i < static_cast<int>(m.size()) - 1; ++i)
                    r.push_back(m[i]);
                return r;
            }
            Prep* prep = make_prep(m, rows);
            if (!prep) return {};
            const int take = std::min(
                static_cast<int>(prep->B1.size()),
                l_finite ? l : static_cast<int>(prep->B1.size()));
            FMat out;
            for (const auto& c : prep->G) out.push_back(c);
            for (const auto& c : prep->B) out.push_back(c);
            for (int i = 0; i < take; ++i) out.push_back(prep->B1[i]);
            delete prep;
            return standardize(out, rows);
        }

        [[nodiscard]] inline FMat bms_fs(const FMat& matrix, int n,
                                         int rows) {
            const FMat m = standardize(matrix, rows);
            if (m.empty()) return {};
            if (last_all_zero(m)) {
                FMat r;
                for (int i = 0; i < static_cast<int>(m.size()) - 1; ++i)
                    r.push_back(m[i]);
                return r;
            }
            Ctx ctx(m, rows);
            Root fr;
            if (!find_root(ctx, fr)) return {};
            const int root = fr.root, t = fr.t;
            const int alpha = ctx.cols - 1;
            FMat G, B;
            for (int c = 0; c < root; ++c) G.push_back(ctx.m[c]);
            for (int c = root; c < alpha; ++c) B.push_back(ctx.m[c]);
            const Col delta = compute_delta(ctx, root, t);
            auto contains_root = [&](int col, int row_label) {
                return ctx.get_a_ancestors(col, row_label).mask[root] == 1;
            };
            FMat result;
            for (const auto& c : G) result.push_back(c);
            for (const auto& c : B) result.push_back(c);
            for (int h = 1; h <= n; ++h)
                for (int local = 0;
                     local < static_cast<int>(B.size()); ++local) {
                    const int original = root + local;
                    Col out = B[local];
                    for (int r = 0; r < rows; ++r)
                        out[r] = B[local][r] +
                                 h * delta[r] *
                                     (r < t - 1 &&
                                              contains_root(original, r + 1)
                                          ? 1
                                          : 0);
                    result.push_back(std::move(out));
                }
            return standardize(result, rows);
        }

        // ---------- 阶段列辅助（缺失列 / 越界视为 -1） ----------
        [[nodiscard]] inline Col phase_column(const FMat& m, int index,
                                              int rows) {
            if (index >= 0 && index < static_cast<int>(m.size()))
                return m[index];
            return const_col(-1, rows);
        }
        [[nodiscard]] inline int
            phase_compare(const FMat& m, int index, const Col& col,
                          int rows) {
            Col pc = phase_column(m, index, rows);
            return col_compare(&pc, &col, rows, -1, 0);
        }
        [[nodiscard]] inline bool p_eq(const FMat& m, int i, const Col& c,
                                       int r) {
            return phase_compare(m, i, c, r) == 0;
        }
        [[nodiscard]] inline bool p_lt(const FMat& m, int i, const Col& c,
                                       int r) {
            return phase_compare(m, i, c, r) < 0;
        }
        [[nodiscard]] inline bool p_le(const FMat& m, int i, const Col& c,
                                       int r) {
            return phase_compare(m, i, c, r) <= 0;
        }
        [[nodiscard]] inline bool p_gt(const FMat& m, int i, const Col& c,
                                       int r) {
            return phase_compare(m, i, c, r) > 0;
        }
        [[nodiscard]] inline bool p_ge(const FMat& m, int i, const Col& c,
                                       int r) {
            return phase_compare(m, i, c, r) >= 0;
        }
        [[nodiscard]] inline bool
            phase_cols_lt(const FMat& m, int start,
                          const std::vector<Col>& cols, int rows) {
            for (int i = 0; i < static_cast<int>(cols.size()); ++i)
                if (phase_compare(m, start + i, cols[i], rows) != 0)
                    return phase_compare(m, start + i, cols[i], rows) < 0;
            return false;
        }
        [[nodiscard]] inline int
            first_diff_column(const FMat& X, const FMat& Y, int rows) {
            const int len = static_cast<int>(std::max(X.size(), Y.size()));
            Col miss = const_col(-1, rows);
            for (int i = 0; i < len; ++i) {
                const Col* xc =
                    i < static_cast<int>(X.size()) ? &X[i] : &miss;
                const Col* yc =
                    i < static_cast<int>(Y.size()) ? &Y[i] : &miss;
                if (col_compare(xc, yc, rows, -1, -1) != 0) return i + 1;
            }
            return static_cast<int>(X.size()) + 1;
        }
        [[nodiscard]] inline bool
            matrices_equal(const FMat& X, const FMat& Y, int rows) {
            return first_diff_column(X, Y, rows) ==
                       static_cast<int>(X.size()) + 1 &&
                   X.size() == Y.size();
        }
        [[nodiscard]] inline Col
            column_c_prime(const FMat& M, int r, int n, int rows) {
            Col out = zero_col(rows);
            for (int i = 0; i < rows; ++i)
                out[i] = i < n ? M[r][i] + 1 : 0;
            return out;
        }
        [[nodiscard]] inline Col column_c(const FMat& M, int t, int n,
                                          int rows) {
            Col out = M[t];
            for (int i = 0; i < rows; ++i) {
                if (i < n - 1) out[i]++;
                else if (i >= n) out[i] = 0;
            }
            return out;
        }
        inline void append_s(FMat& mp, const Prep& prep, int phase,
                             const std::vector<unsigned char>& desc) {
            const int local = phase - prep.root;
            if (local < 0 || local >= static_cast<int>(prep.B1.size()))
                return;
            Col col = prep.B1[local];
            for (int r = 0; r < prep.ctx.rows; ++r)
                if (desc[phase * prep.ctx.rows + r]) col[r]--;
            mp.push_back(std::move(col));
        }
        inline void mark_skip_range(std::vector<unsigned char>& skip,
                                    int from, int to, int alpha) {
            for (int c = from; c <= to && c < alpha; ++c)
                if (c >= 0) skip[c] = 1;
        }
        [[nodiscard]] inline int
            find_min_parent_root(Ctx& ctx, int col, int root) {
            for (int k = 1; k <= ctx.rows; ++k)
                if (ctx.get_b_parent(col, k) == root) return k;
            return ctx.rows;
        }
        [[nodiscard]] inline int find_adjusted_d(Ctx& ctx, int t, int d) {
            const int idx = t + d - 1;
            if (idx < 0 || idx >= ctx.cols) return d;
            const int p1 = ctx.get_b_parent(idx, 1);
            if (p1 <= t) return d;
            const Anc& a = ctx.get_a_ancestors(idx, 1);
            for (int tp : a.list)
                if (ctx.get_b_parent(tp, 1) == t) return tp + 1 - t;
            return d;
        }
        [[nodiscard]] inline const Col&
            at_or_last(const FMat& mp, int idx, const Col& fallback) {
            if (idx >= 0 && idx < static_cast<int>(mp.size())) return mp[idx];
            if (!mp.empty()) return mp.back();
            return fallback;
        }

        // ---------- 提升（核心五情形） ----------
        [[nodiscard]] inline FMat lpms_single(const FMat& matrix) {
            const int rows = max_rows(matrix);
            FMat M = standardize(matrix, rows);
            if (M.empty()) return {};
            if (last_all_zero(M)) {
                FMat r;
                for (int i = 0; i < static_cast<int>(M.size()) - 1; ++i)
                    r.push_back(M[i]);
                return r;
            }
            if (M.size() == 2 && rows > 1) {
                bool all0 = true, all1 = true;
                for (int r = 0; r < rows; ++r) {
                    if (M[0][r] != 0) all0 = false;
                    if (M[1][r] != 1) all1 = false;
                }
                if (all0 && all1) {
                    const int nr = rows - 1;
                    FMat r;
                    r.push_back(zero_col(nr));
                    r.push_back(ones_col(nr));
                    r.push_back(Col(static_cast<std::size_t>(nr), 2));
                    r.push_back(const_col(3, nr));
                    return r;
                }
            }

            Prep* prep_ptr = make_prep(M, rows);
            if (!prep_ptr) return {};
            Prep& prep = *prep_ptr;
            Ctx& ctx = prep.ctx;
            const int r = prep.root, n = prep.t, alpha = prep.alpha;

            FMat mp;
            for (const auto& c : prep.G) mp.push_back(c);
            for (const auto& c : prep.B) mp.push_back(c);
            bool m_switch = true, b_switch = true;
            int l = 0;
            std::vector<unsigned char> skip(
                static_cast<std::size_t>(ctx.cols), 0);
            std::vector<unsigned char> desc(
                static_cast<std::size_t>(ctx.cols) *
                    std::max(1, rows),
                0);
            const Col c_prime = column_c_prime(M, r, n, rows);
            const bool last2_root = ctx.get_b_parent(alpha, 2) == r;

            auto mark_descent = [&](int col, int row) {
                for (int c = 0; c < ctx.cols; ++c)
                    if (ctx.get_a_ancestors(c, row + 1).mask[col] == 1)
                        desc[c * rows + row] = 1;
            };

            for (int t = r; t < alpha; ++t) {
                if (skip[t]) continue;
                if (b_switch) {
                    append_s(mp, prep, t, desc);
                    if (n >= rows || M[r][n] == 0 ||
                        p_le(M, t + 1, c_prime, rows)) {
                        b_switch = false;
                        l = t + 1 - r;
                    }
                    continue;
                }
                const int n_minus_parent =
                    n > 1 ? ctx.get_b_parent(t, n - 1) : -1;
                const int n_minus_vr =
                    n > 1 ? prep.get_vr(t, n - 2) : 0;
                if (n_minus_parent != r || n_minus_vr == 0) {
                    append_s(mp, prep, t, desc);
                    continue;
                }
                const Col c = column_c(M, t, n, rows);
                if (last2_root) {
                    if (n < rows && M[t][n] > 0) {
                        append_s(mp, prep, t, desc);
                        continue;
                    }
                    if (p_eq(M, t + 1, c, rows) &&
                        p_le(M, t + 2, c, rows)) {
                        if (t + 1 < alpha) skip[t + 1] = 1;
                        append_s(mp, prep, t, desc);
                        continue;
                    }
                    const Col threshold = const_col(c[0] + 1, rows);
                    if (p_gt(M, t + 1, c, rows) ||
                        (p_eq(M, t + 1, c, rows) &&
                         p_ge(M, t + 2, threshold, rows))) {
                        append_s(mp, prep, t, desc);
                        continue;
                    }
                    if (p_lt(M, t + 1, c, rows)) {
                        append_s(mp, prep, t, desc);
                        m_switch = false;
                        int a = M[t][0];
                        if (phase_cols_lt(
                                M, t + 1,
                                { const_col(a + 1, rows),
                                  const_col(a + 2, rows) },
                                rows) &&
                            ctx.get_b_parent(t, 2) == r)
                            m_switch = true;
                        a = mp.empty() ? 0 : mp.back()[0];
                        mp = upms_single(mp, l, rows, true);
                        if (m_switch)
                            mp.push_back(const_col(a + 1, rows));
                        continue;
                    }
                }

                int k = find_min_parent_root(ctx, t, r);
                int kp = (k == 1) ? 2 : k;
                XY xy = prep.build_xy(t, kp);
                const bool eq_xy = matrices_equal(xy.X, xy.Y, rows);
                int d = eq_xy ? static_cast<int>(xy.X.size()) + 1
                              : first_diff_column(xy.X, xy.Y, rows);
                if (!eq_xy) {
                    if (xy.u_missing && t + d - 1 >= alpha)
                        d = static_cast<int>(xy.X.size()) + 1;
                    else
                        d = find_adjusted_d(ctx, t, d);
                }
                if (d == 1) {
                    append_s(mp, prep, t, desc);
                    continue;
                }
                if (p_lt(M, t + d - 1, c, rows)) {
                    mark_skip_range(skip, t + 1, t + d - 2, alpha);
                    m_switch = false;
                    int a = M[t][0];
                    if (kp == 2 &&
                        phase_cols_lt(
                            M, t + d - 1,
                            { const_col(a + 1, rows),
                              const_col(a + 2, rows) },
                            rows))
                        m_switch = true;
                    if (t != r + l) {
                        append_s(mp, prep, t, desc);
                        if (!mp.empty())
                            for (int rr = k; rr < rows; ++rr)
                                mp.back()[rr] = 0;
                        mp = upms_single(mp, l, rows, true);
                    }
                    a = (l > 0 && static_cast<int>(mp.size()) >= l)
                            ? mp[mp.size() - l][0]
                            : (mp.empty() ? 0 : mp.back()[0]);
                    if (m_switch)
                        mp.push_back(const_col(a + 1, rows));
                    for (int rr = k - 1; rr <= n - 2 && rr < rows; ++rr)
                        mark_descent(t, rr);
                    continue;
                }
                if (n == 3 && p_eq(M, t + d - 1, c, rows) &&
                    p_lt(M, t + d, c, rows)) {
                    mark_skip_range(skip, t + 1, t + d - 1, alpha);
                    append_s(mp, prep, t, desc);
                    m_switch = false;
                    int a = M[t][0];
                    if (phase_cols_lt(
                            M, t + d,
                            { const_col(a + 1, rows),
                              const_col(a + 2, rows) },
                            rows))
                        m_switch = true;
                    a = mp.empty() ? 0 : mp.back()[0];
                    mp = upms_single(mp, l, rows, true);
                    if (m_switch)
                        mp.push_back(const_col(a + 1, rows));
                    continue;
                }
                mark_skip_range(skip, t + 1, t + d - 2, alpha);
                if (n == 3 && p_eq(M, t + d - 1, c, rows) &&
                    p_eq(M, t + d, c, rows)) {
                    if (t + d - 1 < alpha) skip[t + d - 1] = 1;
                    if (t + d < alpha) skip[t + d] = 1;
                }
                if (n > 3 && p_eq(M, t + d - 1, c, rows) &&
                    p_lt(M, t + d, const_col(c[0] + 1, rows), rows)) {
                    if (t + d - 1 < alpha) skip[t + d - 1] = 1;
                }
                append_s(mp, prep, t, desc);
            }

            if (b_switch) l = ctx.cols - r - 1;

            if (last2_root) {
                const int take = std::min(l, static_cast<int>(prep.B2.size()));
                for (int i = 0; i < take; ++i)
                    mp.push_back(prep.B2[i]);
                const int a =
                    !prep.B2.empty()
                        ? prep.B2[0][0]
                        : (mp.empty() ? 0 : mp.back()[0]);
                mp.push_back(const_col(a + 1, rows));
            } else {
                const int b_len = ctx.cols - r - 1;
                if (l == b_len) {
                    static const Col kZero{ 0 };
                    const Col& src =
                        at_or_last(mp, ctx.cols - 1, kZero);
                    mp.push_back(const_col(src[0] + 1, rows));
                }
            }
            FMat out = standardize(mp, rows);
            delete prep_ptr;
            return out;
        }

        [[nodiscard]] inline FMat lpms_fs(const FMat& expr, int n) {
            n = std::max(0, n);
            if (!legal(expr)) return {};
            const int rows = max_rows(expr);
            const FMat M = standardize(expr, rows);
            if (M.empty()) return {};
            if (last_all_zero(M)) {
                FMat r;
                for (int i = 0; i < static_cast<int>(M.size()) - 1; ++i)
                    r.push_back(M[i]);
                return r;
            }
            if (rows < 3 || M.back()[2] == 0)
                return bms_fs(M, n, rows);
            return lpms_fs(lpms_single(M), n);
        }

        [[nodiscard]] inline FMat lpms_infinity_fs(int n) {
            FMat r;
            r.push_back(zero_col(n));
            r.push_back(ones_col(n));
            return r;
        }
        [[nodiscard]] inline FMat lptss_infinity_fs(int n) {
            FMat r;
            for (int i = 0; i <= n; ++i) r.push_back(Col{ i, i, i });
            return r;
        }

        [[nodiscard]] inline std::string fmat_to_string(const FMat& a) {
            std::string out;
            for (const Col& col : a) {
                int h = static_cast<int>(col.size());
                while (h > 0 && col[h - 1] == 0) --h;
                out += '(';
                if (h == 0) out += '0';
                for (int r = 0; r < h; ++r) {
                    if (r) out += ',';
                    out += std::to_string(col[r]);
                }
                out += ')';
            }
            return out;
        }

        [[nodiscard]] inline FMat from_matrix(const Matrix& m) {
            FMat a;
            for (int c = 0; c < m.cols(); ++c) {
                Col col(static_cast<std::size_t>(m.rows()));
                for (int r = 0; r < m.rows(); ++r) col[r] = m(r, c);
                a.push_back(std::move(col));
            }
            return a;
        }

        template <bool IsLptss>
        [[nodiscard]] inline std::string run(std::string_view text, int term) {
            const auto e = bmlike::parse(text);
            if (!e) return bmlike::kParseError;
            if (e->infinity) {
                const FMat b = IsLptss ? lptss_infinity_fs(term)
                                       : lpms_infinity_fs(term);
                return fmat_to_string(b);
            }
            if (e->m.cols() == 0) return {};
            const FMat in = from_matrix(e->m);
            // 分派：非极限（含非法）删末列。
            if (!(legal(in) && !last_all_zero(standardize(in))))
                return fmat_to_string(
                    standardize(FMat(in.begin(), in.end() - 1)));
            return fmat_to_string(lpms_fs(in, term));
        }

    }  // namespace lpms_detail

    struct LPMSNotation {
        static constexpr const char* kName = "LPMS";
        [[nodiscard]] static std::string
            expand_text(std::string_view text, int term) {
            return lpms_detail::run<false>(text, term);
        }
        [[nodiscard]] static std::string suffix() { return {}; }
    };

    struct LPTSSNotation {
        static constexpr const char* kName = "LPTSS";
        [[nodiscard]] static std::string
            expand_text(std::string_view text, int term) {
            return lpms_detail::run<true>(text, term);
        }
        [[nodiscard]] static std::string suffix() { return {}; }
    };

}  // namespace omegay::notation

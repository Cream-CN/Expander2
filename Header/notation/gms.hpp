// gms.hpp
// GMS —— General Matrix System（通用矩阵系统）族。
//
// 移植自 ne-rewritten/src/notations/BM-like/GMS.ts，仅保留核心 FS 展开：
// 三个核心系统  GBMS / UPMS / LPMS2，与五个“投影”极限构造器
//   ω-P / pQSS / QSS / Full / Weirdly Full，组合出 15 个记号，
// 由模板 GMSNotation<System, Projection> 生成。
//
// 原文件约 600 行 OCN / deluxe 文本渲染（α/Ω/ψ 等记号显示）属于显示层，
// 与展开无关，按任务要求舍弃；这里只保留矩阵 FS、比较与 BM 文本接口。
//
// 说明：原版 FS 在“后继型且请求下标 >=1”等无法生成更长项时会抛错
// （基本序列含过长重复段）。为保持“接口不抛异常”，这里内部用异常，
// 在 expand_text 边界统一转换为 bmlike::kParseError。
#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Header/notation/bmlike_share.hpp"

namespace omegay::notation {

    enum class GmsSystem { GBMS, UPMS, LPMS2 };
    enum class GmsProj { OmegaP, PQSS, QSS, Full, WeirdlyFull };

    namespace gms_detail {

        using bmlike::Matrix;
        using FMat = std::vector<std::vector<int>>;  // 矩形：列向量定长 rows
        using RowList = std::vector<std::vector<int>>;

        struct GmsError : std::runtime_error {
            explicit GmsError(const std::string& m) : std::runtime_error(m) {}
        };

        // ---- 基础矩阵工具（对应 bms* 系列）----
        [[nodiscard]] inline int val(const std::vector<int>& col, int r) noexcept {
            if (r < 0 || r >= static_cast<int>(col.size())) return 0;
            return col[r];
        }
        [[nodiscard]] inline int row_count(const FMat& a) noexcept {
            int n = 1;
            for (const auto& c : a)
                n = std::max(n, static_cast<int>(c.size()));
            return n;
        }
        [[nodiscard]] inline std::vector<int>
            clone_col(const std::vector<int>& col, int rows) {
            std::vector<int> out(static_cast<std::size_t>(rows), 0);
            for (int r = 0; r < rows; ++r) out[r] = val(col, r);
            return out;
        }
        [[nodiscard]] inline FMat
            clone(const FMat& a, int rows = -1) {
            if (rows < 0) rows = row_count(a);
            FMat out;
            out.reserve(a.size());
            for (const auto& c : a) out.push_back(clone_col(c, rows));
            return out;
        }
        [[nodiscard]] inline std::vector<int> zero_col(int rows) {
            return std::vector<int>(static_cast<std::size_t>(rows), 0);
        }
        [[nodiscard]] inline bool
            is_zero_col(const std::vector<int>& col, int rows) {
            for (int r = 0; r < rows; ++r)
                if (val(col, r) != 0) return false;
            return true;
        }
        [[nodiscard]] inline std::vector<int>
            add_cols(const std::vector<int>& a, const std::vector<int>& b,
                     int rows) {
            std::vector<int> out(static_cast<std::size_t>(rows));
            for (int r = 0; r < rows; ++r)
                out[r] = val(a, r) + val(b, r);
            return out;
        }
        [[nodiscard]] inline FMat
            add_mat(const FMat& a, const FMat& b, int rows) {
            if (a.size() != b.size())
                throw GmsError("GMS: 只能对同形矩阵做加法");
            FMat out;
            out.reserve(a.size());
            for (std::size_t c = 0; c < a.size(); ++c)
                out.push_back(add_cols(a[c], b[c], rows));
            return out;
        }
        [[nodiscard]] inline FMat
            scale_mat(const FMat& a, int scalar, int rows) {
            FMat out;
            out.reserve(a.size());
            for (const auto& c : a) {
                std::vector<int> v(static_cast<std::size_t>(rows));
                for (int r = 0; r < rows; ++r) v[r] = val(c, r) * scalar;
                out.push_back(std::move(v));
            }
            return out;
        }
        [[nodiscard]] inline int
            col_compare(const std::vector<int>& a, const std::vector<int>& b,
                        int rows) {
            for (int r = 0; r < rows; ++r) {
                const int av = val(a, r), bv = val(b, r);
                if (av != bv) return av < bv ? -1 : 1;
            }
            return 0;
        }
        [[nodiscard]] inline int mat_compare(const FMat& a, const FMat& b) {
            const int rows = std::max(row_count(a), row_count(b));
            const int common = static_cast<int>(std::min(a.size(), b.size()));
            for (int c = 0; c < common; ++c) {
                const int cmp = col_compare(a[c], b[c], rows);
                if (cmp != 0) return cmp;
            }
            if (a.size() == b.size()) return 0;
            return a.size() < b.size() ? -1 : 1;
        }
        [[nodiscard]] inline int
            suffix_compare(const std::vector<int>& a,
                           const std::vector<int>& b, int start_row, int rows) {
            for (int r = start_row; r < rows; ++r) {
                const int av = val(a, r), bv = val(b, r);
                if (av != bv) return av < bv ? -1 : 1;
            }
            return 0;
        }

        using Parents = std::vector<std::vector<int>>;

        [[nodiscard]] inline Parents
            compute_parents(const FMat& a, int rows) {
            Parents parents;
            std::vector<int> p1(a.size(), -1);
            for (std::size_t c = 0; c < a.size(); ++c)
                for (int q = static_cast<int>(c) - 1; q >= 0; --q)
                    if (val(a[q], 0) < val(a[c], 0)) {
                        p1[c] = q;
                        break;
                    }
            parents.push_back(std::move(p1));
            for (int level = 2; level <= rows; ++level) {
                const auto& previous = parents[level - 2];
                std::vector<int> current(a.size(), -1);
                const int suffix_start = level - 1;
                for (std::size_t c = 0; c < a.size(); ++c) {
                    int p = previous[c];
                    while (p != -1) {
                        if (suffix_compare(a[p], a[c], suffix_start, rows) <
                            0) {
                            current[c] = p;
                            break;
                        }
                        p = previous[p];
                    }
                }
                parents.push_back(std::move(current));
            }
            return parents;
        }
        [[nodiscard]] inline int
            parent(const Parents& parents, int level, int col) noexcept {
            if (level < 1 || level > static_cast<int>(parents.size()) ||
                col < 0)
                return -1;
            return parents[level - 1][col];
        }
        [[nodiscard]] inline bool
            is_ancestor(const Parents& parents, int level, int ancestor,
                        int col) {
            int p = parent(parents, level, col);
            while (p != -1) {
                if (p == ancestor) return true;
                p = parent(parents, level, p);
            }
            return false;
        }
        [[nodiscard]] inline int
            child_above_ancestor(const Parents& parents, int level,
                                 int descendant, int ancestor) {
            int current = descendant;
            while (current != -1) {
                const int p = parent(parents, level, current);
                if (p == ancestor) return current;
                if (p == -1) return -1;
                current = p;
            }
            return -1;
        }
        [[nodiscard]] inline int
            highest_nonzero_row(const std::vector<int>& col, int rows) {
            for (int r = rows - 1; r >= 0; --r)
                if (val(col, r) != 0) return r + 1;
            return 0;
        }
        [[nodiscard]] inline int
            max_entry(const FMat& a, int rows) {
            int result = 0;
            for (const auto& c : a)
                for (int r = 0; r < rows; ++r)
                    result = std::max(result, val(c, r));
            return result;
        }
        [[nodiscard]] inline FMat trim_zero_rows(const FMat& a) {
            if (a.empty()) return {};
            const int rows = row_count(a);
            std::vector<int> kept;
            for (int r = 0; r < rows; ++r) {
                bool nonzero = false;
                for (const auto& c : a)
                    if (val(c, r) != 0) {
                        nonzero = true;
                        break;
                    }
                if (nonzero) kept.push_back(r);
            }
            if (kept.empty()) kept.push_back(0);
            FMat out;
            out.reserve(a.size());
            for (const auto& c : a) {
                std::vector<int> v;
                v.reserve(kept.size());
                for (int r : kept) v.push_back(val(c, r));
                out.push_back(std::move(v));
            }
            return out;
        }

        struct LimitCtx {
            FMat a;
            int rows = 0;
            int last = 0;
            std::vector<int> x;
            int m = 0;
            Parents parents;
            int y_index = 0;
            std::vector<int> y;
            std::vector<int> d;
            int k = 0;
            FMat B;
            FMat C;
            FMat a_prime;
            FMat y_plus_c;
            FMat fs1;
        };

        [[nodiscard]] inline LimitCtx prepare_limit_context(const FMat& a) {
            if (a.empty())
                throw GmsError("GMS: 空矩阵不是极限表达式");
            LimitCtx ctx;
            ctx.rows = row_count(a);
            ctx.last = static_cast<int>(a.size()) - 1;
            ctx.x = a[ctx.last];
            ctx.m = highest_nonzero_row(ctx.x, ctx.rows);
            if (ctx.m == 0)
                throw GmsError("GMS: 全0末列是后继，不进入极限展开");
            ctx.parents = compute_parents(a, ctx.rows);
            ctx.y_index = parent(ctx.parents, ctx.m, ctx.last);
            if (ctx.y_index == -1)
                throw GmsError("GMS: 末列不存在父列");
            ctx.y = a[ctx.y_index];
            ctx.d = zero_col(ctx.rows);
            for (int r = 0; r < ctx.m - 1; ++r)
                ctx.d[r] = val(ctx.x, r) - val(ctx.y, r);
            ctx.k = highest_nonzero_row(ctx.d, ctx.rows);

            auto slice_clone = [&](int lo, int hi) {
                FMat part;
                for (int i = lo; i < hi; ++i)
                    part.push_back(clone_col(a[i], ctx.rows));
                return part;
            };
            ctx.B = slice_clone(0, ctx.y_index);
            ctx.C = slice_clone(ctx.y_index + 1, ctx.last);
            ctx.a_prime = slice_clone(0, ctx.last);
            ctx.y_plus_c = slice_clone(ctx.y_index, ctx.last);
            ctx.fs1 = clone(ctx.a_prime, ctx.rows);
            ctx.fs1.push_back(add_cols(ctx.y, ctx.d, ctx.rows));
            ctx.a = clone(a, ctx.rows);
            return ctx;
        }

        [[nodiscard]] inline FMat
            build_gbms_d(const LimitCtx& ctx) {
            FMat d;
            d.reserve(ctx.y_plus_c.size());
            for (std::size_t local = 0; local < ctx.y_plus_c.size();
                 ++local)
                d.push_back(zero_col(ctx.rows));
            for (std::size_t local = 0; local < ctx.y_plus_c.size();
                 ++local) {
                const int global = ctx.y_index + static_cast<int>(local);
                for (int row = 1; row <= ctx.k; ++row)
                    if (global == ctx.y_index ||
                        is_ancestor(ctx.parents, row, ctx.y_index, global))
                        d[local][row - 1] = ctx.d[row - 1];
            }
            return d;
        }

        [[nodiscard]] inline FMat
            projection_recurrence(const LimitCtx& ctx, const FMat& D,
                                  int index) {
            if (index == 0) return clone(ctx.a_prime, ctx.rows);
            if (index == 1) return clone(ctx.fs1, ctx.rows);
            FMat result = clone(ctx.a_prime, ctx.rows);
            for (int factor = 1; factor <= index - 1; ++factor) {
                FMat shifted = add_mat(
                    ctx.y_plus_c, scale_mat(D, factor, ctx.rows), ctx.rows);
                for (auto& c : shifted)
                    result.push_back(std::move(c));
            }
            return result;
        }

        struct UpmsCtx {
            int h = 0;
            FMat span;
            std::vector<FMat> Y;       // 下标 0..k
            std::vector<std::vector<int>> v;
            FMat D;
        };

        // buildAdjustedSuffix 需要 v 与缓存，单独建一个带状态对象。
        class UpmsBuilder {
        public:
            explicit UpmsBuilder(const LimitCtx& ctx) : ctx_(ctx) {
                u_.h = 2 * max_entry(ctx.a, ctx.rows);
                for (int i = ctx.y_index; i <= ctx.last; ++i)
                    u_.span.push_back(clone_col(ctx.a[i], ctx.rows));
                u_.Y.resize(static_cast<std::size_t>(ctx.k) + 1);
                u_.v.resize(static_cast<std::size_t>(ctx.k) + 1);
                build();
            }
            const UpmsCtx& get() const noexcept { return u_; }

            [[nodiscard]] FMat build_adjusted_suffix(int z_index, int level) {
                const std::string key =
                    std::to_string(z_index) + ':' + std::to_string(level);
                const auto it = cache_.find(key);
                if (it != cache_.end()) return clone(it->second, ctx_.rows);
                FMat suffix;
                for (int i = z_index; i <= ctx_.last; ++i)
                    suffix.push_back(clone_col(ctx_.a[i], ctx_.rows));
                FMat adjustment;
                for (std::size_t local = 0; local < suffix.size(); ++local)
                    adjustment.push_back(zero_col(ctx_.rows));
                for (std::size_t local = 0; local < suffix.size(); ++local) {
                    const int span_position =
                        z_index + static_cast<int>(local) - ctx_.y_index;
                    for (int row = 1; row < level; ++row) {
                        if (static_cast<std::size_t>(row) < u_.v.size() &&
                            span_position >= 0 &&
                            span_position <
                                static_cast<int>(u_.v[row].size()) &&
                            u_.v[row][span_position] == 1)
                            adjustment[local][row - 1] =
                                u_.h - val(ctx_.a[z_index], row - 1);
                    }
                }
                FMat result = add_mat(suffix, adjustment, ctx_.rows);
                cache_[key] = clone(result, ctx_.rows);
                return result;
            }

        private:
            const LimitCtx& ctx_;
            UpmsCtx u_;
            std::unordered_map<std::string, FMat> cache_;

            void build() {
                const int span_length = static_cast<int>(u_.span.size());
                for (int level = 2; level <= ctx_.k; ++level) {
                    const int z_index = child_above_ancestor(
                        ctx_.parents, level, ctx_.last, ctx_.y_index);
                    if (z_index == -1)
                        throw GmsError("UPMS: 无法构造 Y 中的 z");
                    FMat suffix;
                    for (int i = z_index; i <= ctx_.last; ++i)
                        suffix.push_back(clone_col(ctx_.a[i], ctx_.rows));
                    FMat adjustment;
                    for (std::size_t local = 0; local < suffix.size();
                         ++local)
                        adjustment.push_back(zero_col(ctx_.rows));
                    for (std::size_t local = 0; local < suffix.size();
                         ++local) {
                        const int global =
                            z_index + static_cast<int>(local);
                        for (int row = 1; row < level; ++row)
                            if (global == z_index ||
                                is_ancestor(ctx_.parents, row, z_index,
                                            global))
                                adjustment[local][row - 1] =
                                    u_.h - val(ctx_.a[z_index], row - 1);
                    }
                    u_.Y[level] =
                        add_mat(suffix, adjustment, ctx_.rows);
                }
                u_.v[1].assign(static_cast<std::size_t>(span_length), 1);
                for (int level = 2; level <= ctx_.k; ++level) {
                    std::vector<int> current(
                        static_cast<std::size_t>(span_length), 0);
                    current[0] = 1;
                    current[span_length - 1] = 1;
                    for (int pos = 1; pos < span_length - 1; ++pos) {
                        const int z_index = ctx_.y_index + pos;
                        if (!is_ancestor(ctx_.parents, level, ctx_.y_index,
                                         z_index))
                            continue;
                        if (u_.v[level - 1][pos] == 0) continue;
                        const int z_prime = child_above_ancestor(
                            ctx_.parents, level, z_index, ctx_.y_index);
                        if (z_prime == -1) continue;
                        FMat X = build_adjusted_suffix(z_prime, level);
                        current[pos] =
                            mat_compare(X, u_.Y[level]) < 0 ? 0 : 1;
                    }
                    u_.v[level] = std::move(current);
                }
                u_.D.clear();
                for (std::size_t pos = 0; pos < ctx_.y_plus_c.size();
                     ++pos)
                    u_.D.push_back(zero_col(ctx_.rows));
                for (int row = 1; row <= ctx_.k; ++row)
                    for (std::size_t pos = 0; pos < ctx_.y_plus_c.size();
                         ++pos)
                        u_.D[pos][row - 1] =
                            ctx_.d[row - 1] * u_.v[row][pos];
            }
        };

        [[nodiscard]] inline int
            first_different_column(const FMat& a, const FMat& b) {
            const int common = static_cast<int>(std::min(a.size(), b.size()));
            const int rows = std::max(row_count(a), row_count(b));
            for (int i = 0; i < common; ++i)
                if (col_compare(a[i], b[i], rows) != 0) return i;
            return common;
        }

        [[nodiscard]] inline FMat
            build_lpms2_x(const LimitCtx& ctx, const UpmsCtx& upms,
                          UpmsBuilder& builder) {
            const int c_length = static_cast<int>(ctx.C.size());
            std::vector<int> s(static_cast<std::size_t>(c_length) + 1, 0);
            FMat X;
            for (int round = 0; round < c_length; ++round) {
                if (s[round] == 1) continue;
                const int z_index = ctx.y_index + 1 + round;
                const std::vector<int>& z = ctx.a[z_index];
                const int span_position = round + 1;
                const bool passes_k =
                    parent(ctx.parents, ctx.k, z_index) == ctx.y_index &&
                    span_position <
                        static_cast<int>(upms.v[ctx.k].size()) &&
                    upms.v[ctx.k][span_position] == 1;
                if (passes_k) {
                    int level = -1;
                    for (int candidate = 2; candidate <= ctx.k; ++candidate)
                        if (parent(ctx.parents, candidate, z_index) ==
                            ctx.y_index) {
                            level = candidate;
                            break;
                        }
                    if (level != -1) {
                        FMat adjusted =
                            builder.build_adjusted_suffix(z_index, level);
                        const FMat& target = upms.Y[level];
                        const int difference =
                            first_different_column(adjusted, target);
                        const int w_index =
                            difference < static_cast<int>(adjusted.size())
                                ? z_index + difference
                                : ctx.last + 1;
                        if (w_index != z_index) {
                            int j_prime = -1;
                            if (w_index == ctx.last + 1) {
                                j_prime = c_length + 1;
                            } else {
                                for (int search = w_index;
                                     search >= ctx.y_index + 2; --search)
                                    if (val(ctx.a[search], 0) <=
                                        val(z, 0) + 1) {
                                        j_prime = search - ctx.y_index - 1;
                                        break;
                                    }
                            }
                            if (j_prime != -1)
                                for (int q = round + 1;
                                     q <= j_prime - 1 &&
                                     q < static_cast<int>(s.size());
                                     ++q)
                                    s[q] = 1;
                        }
                    }
                }
                X.push_back(add_cols(z, upms.D[round + 1], ctx.rows));
            }
            return X;
        }

        [[nodiscard]] inline FMat
            old_finite_fs(const FMat& a, int index, GmsSystem system) {
            if (index < 0)
                throw GmsError("GMS: 基本序列下标必须是非负整数");
            if (a.empty()) return {};
            const int rows = row_count(a);
            const int last = static_cast<int>(a.size()) - 1;
            if (is_zero_col(a[last], rows))
                return clone(FMat(a.begin(), a.begin() + last), rows);
            const LimitCtx ctx = prepare_limit_context(a);
            if (index == 0) return clone(ctx.a_prime, ctx.rows);
            if (index == 1) return clone(ctx.fs1, ctx.rows);
            const FMat gbms_d = build_gbms_d(ctx);
            if (system == GmsSystem::GBMS || ctx.k <= 1)
                return projection_recurrence(ctx, gbms_d, index);
            UpmsBuilder builder(ctx);
            const UpmsCtx& upms = builder.get();
            if (system == GmsSystem::UPMS)
                return projection_recurrence(ctx, upms.D, index);
            // LPMS2
            FMat X = build_lpms2_x(ctx, upms, builder);
            FMat next = clone(ctx.fs1, ctx.rows);
            for (auto& c : X) next.push_back(std::move(c));
            return old_finite_fs(next, index - 1, system);
        }

        // ---- 五个投影极限构造器 ----
        [[nodiscard]] inline FMat limit_omega_p(int index) {
            FMat out{ {0, 0, 0} };
            for (int n = 1; n <= index; ++n)
                out.push_back({ n, n / 2, (n + 1) / 2 });
            return out;
        }
        [[nodiscard]] inline FMat limit_pqss(int index) {
            FMat out{ {0, 0, 0, 0} };
            for (int n = 1; n <= index; ++n)
                out.push_back({ n, n / 2, (n - 1) / 2, 1 });
            return out;
        }
        [[nodiscard]] inline FMat limit_qss(int index) {
            FMat out{ {0, 0, 0, 0} };
            for (int n = 1; n <= index; ++n)
                out.push_back(
                    { n, (n + 1) / 3, n / 3, (n + 2) / 3 });
            return out;
        }
        [[nodiscard]] inline FMat limit_full(int index) {
            if (index == 0) return {};
            FMat out;
            out.push_back(std::vector<int>(
                static_cast<std::size_t>(index), 0));
            for (int stage = 2; stage <= index; ++stage) {
                const int triangular = ((stage - 2) * (stage - 1)) / 2;
                for (int j = 1; j <= stage - 1; ++j) {
                    std::vector<int> col;
                    col.push_back(triangular + j);
                    for (int r = 1; r <= stage - 1; ++r)
                        col.push_back(std::max(
                            1, stage - r - (r >= j ? 1 : 0)));
                    while (static_cast<int>(col.size()) < index)
                        col.push_back(0);
                    out.push_back(std::move(col));
                }
            }
            return out;
        }
        [[nodiscard]] inline FMat limit_weirdly_full(int index) {
            const int rows = index + 2;
            FMat out;
            out.push_back(std::vector<int>(
                static_cast<std::size_t>(rows), 0));
            for (int j = 1; j < rows; ++j) {
                std::vector<int> col;
                col.push_back(j);
                for (int r = 1; r < rows - 1; ++r)
                    col.push_back(r < j ? 1 : 0);
                col.push_back(1);
                out.push_back(std::move(col));
            }
            return out;
        }

        [[nodiscard]] inline FMat
            limit_builder(GmsProj proj, int index) {
            switch (proj) {
                case GmsProj::OmegaP: return limit_omega_p(index);
                case GmsProj::PQSS: return limit_pqss(index);
                case GmsProj::QSS: return limit_qss(index);
                case GmsProj::Full: return limit_full(index);
                case GmsProj::WeirdlyFull: return limit_weirdly_full(index);
            }
            return {};
        }
        [[nodiscard]] inline bool trim_enabled(GmsProj proj) noexcept {
            return proj == GmsProj::Full || proj == GmsProj::WeirdlyFull;
        }
        [[nodiscard]] inline int
            compare_for(GmsProj proj, const FMat& a, const FMat& b) {
            if (proj != GmsProj::WeirdlyFull) return mat_compare(a, b);
            if (a.empty() || b.empty()) return mat_compare(a, b);
            const int ra = row_count(a), rb = row_count(b);
            if (ra != rb) return ra < rb ? -1 : 1;
            return mat_compare(a, b);
        }

        [[nodiscard]] inline FMat
            raw_adjusted_fs(const FMat& a, bool infinity, int index,
                            GmsSystem system, GmsProj proj) {
            if (infinity) return limit_builder(proj, index);
            int old_index = index;
            if (index >= 2) {
                const FMat old1 = old_finite_fs(a, 1, system);
                const FMat old2 = old_finite_fs(a, 2, system);
                if (old2.size() <= old1.size()) old_index = index + 1;
            }
            FMat result = old_finite_fs(a, old_index, system);
            return trim_enabled(proj) ? trim_zero_rows(result) : result;
        }

        // 对外的有限/极限 FS；重复段保护失败时抛 GmsError。
        [[nodiscard]] inline FMat
            fs(const FMat& a, bool infinity, int index,
               GmsSystem system, GmsProj proj) {
            if (index < 0)
                throw GmsError("GMS: 基本序列下标必须是非负整数");
            FMat result = raw_adjusted_fs(a, infinity, 0, system, proj);
            if (index == 0) return result;
            int logical = 0, raw_index = 1;
            while (logical < index) {
                if (raw_index > index + 10000)
                    throw GmsError("GMS: 基本序列含有过长的重复段");
                FMat candidate = raw_adjusted_fs(a, infinity, raw_index++,
                                                 system, proj);
                if (compare_for(proj, candidate, result) == 0) continue;
                result = std::move(candidate);
                ++logical;
            }
            return result;
        }

        [[nodiscard]] inline std::string fmat_to_string(const FMat& a) {
            std::string out;
            for (const auto& col : a) {
                int h = static_cast<int>(col.size());
                while (h > 0 && col[h - 1] == 0) --h;  // normalize_col(>0)
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

        [[nodiscard]] inline std::string
            run(std::string_view text, int term,
                GmsSystem system, GmsProj proj) {
            const auto e = bmlike::parse(text);
            if (!e) return bmlike::kParseError;
            try {
                if (e->infinity) {
                    // 极限构造器严格递增，无需去重，等价于 fs 包装。
                    return fmat_to_string(limit_builder(proj, term));
                }
                if (e->m.cols() == 0) return {};
                FMat a;
                for (int c = 0; c < e->m.cols(); ++c)
                    a.push_back([&] {
                        std::vector<int> v(
                            static_cast<std::size_t>(e->m.rows()));
                        for (int r = 0; r < e->m.rows(); ++r)
                            v[r] = e->m(r, c);
                        return v;
                    }());
                const FMat result =
                    fs(a, false, term, system, proj);
                return fmat_to_string(result);
            } catch (const GmsError&) {
                return bmlike::kParseError;
            }
        }

    }  // namespace gms_detail

    template <GmsSystem SystemV, GmsProj ProjV>
    struct GMSNotation {
        [[nodiscard]] static std::string
            expand_text(std::string_view text, int term) {
            return gms_detail::run(text, term, SystemV, ProjV);
        }
        [[nodiscard]] static std::string suffix() { return {}; }
    };

    using GbmsOmegaP = GMSNotation<GmsSystem::GBMS, GmsProj::OmegaP>;
    using GbmsPQSS = GMSNotation<GmsSystem::GBMS, GmsProj::PQSS>;
    using GbmsQSS = GMSNotation<GmsSystem::GBMS, GmsProj::QSS>;
    using GbmsFull = GMSNotation<GmsSystem::GBMS, GmsProj::Full>;
    using GbmsWeirdlyFull = GMSNotation<GmsSystem::GBMS, GmsProj::WeirdlyFull>;
    using UpmsOmegaP = GMSNotation<GmsSystem::UPMS, GmsProj::OmegaP>;
    using UpmsPQSS = GMSNotation<GmsSystem::UPMS, GmsProj::PQSS>;
    using UpmsQSS = GMSNotation<GmsSystem::UPMS, GmsProj::QSS>;
    using UpmsFull = GMSNotation<GmsSystem::UPMS, GmsProj::Full>;
    using UpmsWeirdlyFull =
        GMSNotation<GmsSystem::UPMS, GmsProj::WeirdlyFull>;
    using Lpms2OmegaP = GMSNotation<GmsSystem::LPMS2, GmsProj::OmegaP>;
    using Lpms2PQSS = GMSNotation<GmsSystem::LPMS2, GmsProj::PQSS>;
    using Lpms2QSS = GMSNotation<GmsSystem::LPMS2, GmsProj::QSS>;
    using Lpms2Full = GMSNotation<GmsSystem::LPMS2, GmsProj::Full>;
    using Lpms2WeirdlyFull =
        GMSNotation<GmsSystem::LPMS2, GmsProj::WeirdlyFull>;

}  // namespace omegay::notation

// bmlike_share.hpp
// BM-like 系列平面矩阵记号（BMS 家族之外的 BSM / BHM / CMS / DSM / BLM /
// wMM / GMS / LPMS 等）的公共类型与工具。
//
// 逻辑移植自 ne-rewritten/src/notations/BM-like/BM.ts，仅保留展开（Expand）
// 所需的核心部分：父项链 parents、规范化 / 标准化、列字典序比较、极限判定、
// 极限基本列、BM 文本解析 / 序列化以及 FS / FS_alter 公共分派。
// 山脉图（mountain diagram）、0Y 等价表示、三角矩阵转换器（BM_converter）、
// display 变体与记忆化缓存等 UI 无关内容一律不带入。
//
// 约定与 BM.ts 一致：
//   - 表达式 Expr 为若干“列”，列内为非负整数行值；越界单元格一律视为 0
//     （对应 TS 的 m[p][j] ?? 0）。
//   - 规范化（normalize）截去每列尾随的 0；空列高度为 0，显示为 "(0)"。
//   - 记号极限单独用 Expr::infinity 表示（TS 中为 [[Infinity]]）。
#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Header/common/Matrix.hpp"

namespace omegay::notation::bmlike {

    using omegay::common::Matrix;

    // 解析失败时各记号 expand_text 统一返回的提示语（与项目内其他文本记号一致）。
    inline constexpr const char* kParseError =
        "解析失败，请确认你没有使用全角括号，逗号";

    // BM-like 表达式：要么是记号极限（Limit），要么是普通矩阵。
    struct Expr {
        bool infinity = false;
        Matrix m;
    };

    // ------------------------------------------------------------------
    // 单元格与列工具
    // ------------------------------------------------------------------

    // 越界（含行列越界）一律返回 0，对齐 TS 的 m[r][c] ?? 0。
    [[nodiscard]] inline int at(const Matrix& m, int r, int c) noexcept {
        if (r < 0 || c < 0 || r >= m.rows() || c >= m.cols()) return 0;
        return m(r, c);
    }

    // 第 c 列规范化后的高度：最后一个正值所在行 + 1；全零列为 0。
    // 对应 BM.ts normalize_col：slice(0, index_of_last(x => x > 0) + 1)。
    [[nodiscard]] inline int column_height(const Matrix& m, int c) noexcept {
        for (int r = m.rows() - 1; r >= 0; --r) {
            if (m(r, c) > 0) return r + 1;
        }
        return 0;
    }

    [[nodiscard]] inline std::vector<int>
        normalized_column(const Matrix& m, int c) {
        std::vector<int> out;
        const int h = column_height(m, c);
        out.reserve(static_cast<std::size_t>(h));
        for (int r = 0; r < h; ++r) out.push_back(m(r, c));
        return out;
    }

    // 规范化：各列截去尾随 0，矩阵行数取所有列高度的最大值（可为 0）。
    [[nodiscard]] inline Matrix normalize(const Matrix& m) {
        if (m.cols() == 0) return Matrix{};
        int rows = 0;
        for (int c = 0; c < m.cols(); ++c)
            rows = std::max(rows, column_height(m, c));
        Matrix out(rows, m.cols());
        for (int c = 0; c < m.cols(); ++c) {
            const int h = std::min(rows, column_height(m, c));
            for (int r = 0; r < h; ++r) out(r, c) = m(r, c);
        }
        return out;
    }

    // 标准化：把矩阵行数补齐到至少 min_rows 行（新增行以 0 填充）。
    // 对应 BM.ts standardize(m, min)。本工程的 Matrix 本就是矩形，
    // 因此除 min_rows 外通常是恒等变换。
    [[nodiscard]] inline Matrix
        standardize(const Matrix& m, int min_rows = 0) {
        if (m.cols() == 0) return m;
        const int rows = std::max(m.rows(), min_rows);
        if (rows == m.rows()) return m;
        Matrix out(rows, m.cols());
        for (int c = 0; c < m.cols(); ++c)
            for (int r = 0; r < m.rows(); ++r) out(r, c) = m(r, c);
        return out;
    }

    // ------------------------------------------------------------------
    // 父项链（BM.ts parents）
    // ------------------------------------------------------------------

    // P[i][j]：第 i 列第 j 行元素的父项列号。
    // 第 0 行父项为同行左侧值严格更小的最近列；第 j>0 行父项沿第 j-1 行的
    // 父项链寻找。某列某行找不到父项时，该列更高的行也不再生成（与 TS 中
    // result[i] 被截断、访问缺失项得到 undefined 的语义一致）。
    [[nodiscard]] inline std::vector<std::vector<int>> parents(const Matrix& m) {
        const int nc = m.cols();
        std::vector<std::vector<int>> P(static_cast<std::size_t>(nc));
        for (int i = 0; i < nc; ++i) {
            for (int j = 0; j < m.rows(); ++j) {
                int p = i;
                while (true) {
                    if (j > 0) {
                        p = (j - 1 < static_cast<int>(P[p].size()))
                            ? P[p][j - 1] : -1;
                    } else {
                        --p;
                    }
                    if (p < 0) break;
                    if (at(m, j, p) < at(m, j, i)) break;
                }
                if (p >= 0) P[i].push_back(p);
                else break;
            }
        }
        return P;
    }

    // 读取父项链中的父项；缺失（行超出该列已算出的父项）返回 -1。
    [[nodiscard]] inline int
        parent_at(const std::vector<std::vector<int>>& P, int c, int r) noexcept {
        if (c < 0 || c >= static_cast<int>(P.size())) return -1;
        if (r < 0 || r >= static_cast<int>(P[c].size())) return -1;
        return P[c][r];
    }

    // ------------------------------------------------------------------
    // 比较（BM.ts compare / lex_compare，longer_last=true）
    // ------------------------------------------------------------------

    // 单个值比较：-1 / 0 / 1。
    [[nodiscard]] inline int number_compare(int a, int b) noexcept {
        return a == b ? 0 : (a < b ? -1 : 1);
    }

    // 两个等长向量逐元素比较后再比长度（前缀更短者更小）。
    [[nodiscard]] inline int
        vector_compare(const std::vector<int>& a, const std::vector<int>& b) {
        const std::size_t n = std::min(a.size(), b.size());
        for (std::size_t i = 0; i < n; ++i)
            if (a[i] != b[i]) return number_compare(a[i], b[i]);
        return number_compare(static_cast<int>(a.size()),
                              static_cast<int>(b.size()));
    }

    // 两列按规范化（去尾随 0）后的字典序比较。
    [[nodiscard]] inline int
        column_compare(const Matrix& a, int ca, const Matrix& b, int cb) {
        return vector_compare(normalized_column(a, ca),
                              normalized_column(b, cb));
    }

    // 两个矩阵的 BM 字典序：逐列比较，再比列数。
    [[nodiscard]] inline int compare(const Matrix& a, const Matrix& b) {
        const int n = std::min(a.cols(), b.cols());
        for (int c = 0; c < n; ++c) {
            const int r = column_compare(a, c, b, c);
            if (r != 0) return r;
        }
        return number_compare(a.cols(), b.cols());
    }

    // ------------------------------------------------------------------
    // 极限判定与极限基本列
    // ------------------------------------------------------------------

    // BM.ts is_limit：末列存在且其第 0 行 > 0。
    [[nodiscard]] inline bool is_limit(const Matrix& m) noexcept {
        return m.cols() > 0 && m.rows() > 0 && m(0, m.cols() - 1) > 0;
    }

    // 极限的 FS 第 n 个基本列（BM.infinity_FS）：
    // [[], [1,1,...,1]]（第 1 列 n+1 个 1；第 0 列为空列）。
    [[nodiscard]] inline Matrix infinity_fs(int n) {
        if (n < 0) n = 0;
        Matrix m(n + 1, 2);
        for (int r = 0; r <= n; ++r) m(r, 1) = 1;
        return m;
    }

    // 三角 BMS 的极限基本列（BM.triangular_infinity_FS）：
    // [[], [1], [2,1], [3,2,1], ..., [n,...,1]]。
    [[nodiscard]] inline Matrix triangular_infinity_fs(int n) {
        if (n < 0) n = 0;
        Matrix m(n, n + 1);  // n=0 时为 0 行 1 列（即单独的空列）
        for (int i = 1; i <= n; ++i)
            for (int j = 0; j < i; ++j) m(j, i) = i - j;
        return m;
    }

    // 删除最后一列（后继型 / FS 截断的公共动作）。
    [[nodiscard]] inline Matrix drop_last_column(const Matrix& m) {
        if (m.cols() <= 1) return Matrix{};
        Matrix out(m.rows(), m.cols() - 1);
        for (int c = 0; c < m.cols() - 1; ++c)
            for (int r = 0; r < m.rows(); ++r) out(r, c) = m(r, c);
        return out;
    }

    // ------------------------------------------------------------------
    // FS / FS_alter 公共分派（对应 notation_utils.ts 的
    // sequence_FS_variants / Y_FS_variants，去掉纯 UI 用的记忆化缓存）
    // ------------------------------------------------------------------

    // sequence_FS_variants 语义：
    //   极限       → 极限基本列 infinity_fs(term)
    //   空矩阵     → 空矩阵
    //   非极限     → 删除末列（后继型）
    //   极限表达式 → core(m, term, shorter)
    // core 签名：Matrix(const Matrix& m, int term, bool shorter)。
    template <class CoreExpand>
    [[nodiscard]] Matrix fs_expand(const Expr& e, int term,
                                   bool shorter, CoreExpand&& core) {
        if (e.infinity) return infinity_fs(term);
        const Matrix& m = e.m;
        if (m.cols() == 0) return m;
        if (!is_limit(m)) return drop_last_column(m);
        return std::forward<CoreExpand>(core)(m, term, shorter);
    }

    // Y_FS_variants 语义：core 只生成“较长”结果（无 shorter 参数），
    // FS（shorter）在其基础上再删去最后一列。
    template <class CoreExpandLonger>
    [[nodiscard]] Matrix y_fs_expand(const Expr& e, int term,
                                     bool shorter, CoreExpandLonger&& core) {
        if (e.infinity) return infinity_fs(term);
        const Matrix& m = e.m;
        if (m.cols() == 0) return m;
        if (!is_limit(m)) return drop_last_column(m);
        Matrix out = std::forward<CoreExpandLonger>(core)(m, term);
        if (shorter) out = drop_last_column(out);
        return out;
    }

    // ------------------------------------------------------------------
    // 文本解析与序列化（BM.ts from_display(std=false) / display）
    // ------------------------------------------------------------------

    namespace detail {

        inline void trim_spaces(std::string_view& s) noexcept {
            while (!s.empty() &&
                   std::isspace(static_cast<unsigned char>(s.front())))
                s.remove_prefix(1);
            while (!s.empty() &&
                   std::isspace(static_cast<unsigned char>(s.back())))
                s.remove_suffix(1);
        }

    }  // namespace detail

    // 解析 BM 文本：列形如 "(v0,v1,...)"，允许列内 / 列间空格；空括号 "()"
    // 为空列；"Limit" 为记号极限；"" 为空矩阵。非法输入返回 nullopt
    // （TS 抛异常，本项目按约定不抛异常）。结果经 normalize。
    [[nodiscard]] inline std::optional<Expr> parse(std::string_view text) {
        std::string_view s = text;
        detail::trim_spaces(s);
        if (s == "Limit") return Expr{ true, Matrix{} };
        if (s.empty()) return Expr{ false, Matrix{} };

        std::vector<std::vector<int>> raw_cols;
        std::size_t i = 0;
        while (i < s.size()) {
            while (i < s.size() && s[i] == ' ') ++i;
            if (i >= s.size()) break;
            if (s[i] != '(') return std::nullopt;
            ++i;
            while (i < s.size() && s[i] == ' ') ++i;

            std::vector<int> col;
            if (i < s.size() && s[i] == ')') {
                ++i;  // 空列
            } else {
                while (true) {
                    while (i < s.size() && s[i] == ' ') ++i;
                    if (i >= s.size() ||
                        !std::isdigit(static_cast<unsigned char>(s[i])))
                        return std::nullopt;
                    int v = 0;
                    while (i < s.size() &&
                           std::isdigit(static_cast<unsigned char>(s[i]))) {
                        v = v * 10 + (s[i] - '0');
                        ++i;
                    }
                    col.push_back(v);
                    while (i < s.size() && s[i] == ' ') ++i;
                    if (i >= s.size()) return std::nullopt;
                    if (s[i] == ',') { ++i; continue; }
                    if (s[i] == ')') { ++i; break; }
                    return std::nullopt;
                }
            }
            raw_cols.push_back(std::move(col));
        }

        int rows = 0;
        for (const auto& col : raw_cols) {
            int h = static_cast<int>(col.size());
            while (h > 0 && col[h - 1] == 0) --h;
            rows = std::max(rows, h);
        }
        Matrix m(rows, static_cast<int>(raw_cols.size()));
        for (int c = 0; c < static_cast<int>(raw_cols.size()); ++c) {
            const int h = std::min(rows, static_cast<int>(raw_cols[c].size()));
            for (int r = 0; r < h; ++r) m(r, c) = raw_cols[c][r];
        }
        return Expr{ false, std::move(m) };
    }

    // 矩阵 → BM 文本。空列（规范化高度 0）显示为 "(0)"（BM.column_display）。
    [[nodiscard]] inline std::string matrix_to_bm_string(const Matrix& m) {
        std::string out;
        for (int c = 0; c < m.cols(); ++c) {
            out += '(';
            const int h = column_height(m, c);
            if (h == 0) out += '0';  // 空列显示为 "(0)"（BM.column_display）
            for (int r = 0; r < h; ++r) {
                if (r) out += ',';
                out += std::to_string(m(r, c));
            }
            out += ')';
        }
        return out;
    }

    [[nodiscard]] inline std::string to_string(const Expr& e) {
        if (e.infinity) return "Limit";
        return matrix_to_bm_string(e.m);
    }

    // 供各记号 Notation::expand_text / expand_text_alter 直接复用的封装：
    // 解析 → fs_expand → 序列化；shorter 区分 FS / FS_alter。
    template <class CoreExpand>
    [[nodiscard]] std::string expand_text_with(
        std::string_view text, int term, bool shorter, CoreExpand&& core) {
        const auto e = parse(text);
        if (!e) return kParseError;
        Matrix r = fs_expand(*e, term, shorter,
                             std::forward<CoreExpand>(core));
        return matrix_to_bm_string(normalize(r));
    }

    // Y_FS_variants 版本的文本封装。
    template <class CoreExpandLonger>
    [[nodiscard]] std::string y_expand_text_with(
        std::string_view text, int term, bool shorter,
        CoreExpandLonger&& core) {
        const auto e = parse(text);
        if (!e) return kParseError;
        Matrix r = y_fs_expand(*e, term, shorter,
                               std::forward<CoreExpandLonger>(core));
        return matrix_to_bm_string(normalize(r));
    }

}  // namespace omegay::notation::bmlike

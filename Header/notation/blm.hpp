// blm.hpp
// BLM —— Bashicu Large Matrix（Bashicu 大矩阵系统）。
//
// 逻辑机械移植自 ne-rewritten/src/notations/BM-like/BLM.ts 的历史大函数
// （原实现使用 d2..d22 风格的混淆变量名），仅保留核心展开与 BM 文本接口，
// 舍弃记忆化缓存。变量名与循环控制流刻意与原版保持一致，以便逐项核对。
//
// 需要特别复刻的原版语义：
//   b2 = Array(d3+1).fill(Array(d2+1).fill(0))
// 在 JS 中所有列共享同一个底层行数组；直到展开阶段 d3 增长后，新列才被赋成
// 独立的新数组。这里用 shared_ptr<vector<int>> 表达“共享 / 独立”。
#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Header/notation/bmlike_share.hpp"

namespace omegay::notation {

    struct BLMNotation {
        static constexpr const char* kName = "BLM";
        [[nodiscard]] static std::string expand_text(std::string_view, int);
        [[nodiscard]] static std::string suffix() { return {}; }
    };

    namespace blm_detail {

        using bmlike::Matrix;
        using Cols = std::vector<std::vector<int>>;
        using B2Col = std::shared_ptr<std::vector<int>>;

        [[nodiscard]] inline int gv(const Cols& b, int c, int r) noexcept {
            if (c < 0 || c >= static_cast<int>(b.size())) return 0;
            if (r < 0 || r >= static_cast<int>(b[c].size())) return 0;
            return b[c][r];
        }
        [[nodiscard]] inline int gv2(const std::vector<int>& v, int i) noexcept {
            if (i < 0 || i >= static_cast<int>(v.size())) return 0;
            return v[i];
        }

        [[nodiscard]] inline Cols expand(const Matrix& input, int a) {
            // standardize → 矩形变长列
            Cols b(static_cast<std::size_t>(input.cols()));
            for (int c = 0; c < input.cols(); ++c) {
                b[c].assign(static_cast<std::size_t>(input.rows()), 0);
                for (int r = 0; r < input.rows(); ++r) b[c][r] = input(r, c);
            }

            int d3 = static_cast<int>(b.size()) - 1;
            const int d2 = static_cast<int>(b[0].size()) - 1;

            // b2：初始 d3+1 列全部共享同一个全 0 行。
            auto shared_row =
                std::make_shared<std::vector<int>>(static_cast<std::size_t>(d2) + 1, 0);
            std::vector<B2Col> b2(static_cast<std::size_t>(d3) + 1, shared_row);

            std::vector<int> c(static_cast<std::size_t>(d2) + 2, 0);
            std::vector<int> c2(static_cast<std::size_t>(d3) + 2, 0);
            std::vector<int> c3(static_cast<std::size_t>(d2) + 2, 0);
            int d7 = 0, d8 = 0, d9 = 0, d18 = 0, d19 = 0;

            for (int d4 = 0; d4 <= d2; ++d4) {
                if (0 < b[d3][d4] && gv(b, d3, d4 + 1) == 0) {
                    for (int d5 = 0; d5 <= d3; ++d5) {
                        for (int d6 = 0; d6 <= d4; ++d6) {
                            if (gv(b, d3 - d5, d6) < b[d3][d6] - gv2(c, d6)) {
                                if (d6 < d4) {
                                    c[d6] = b[d3][d6] - gv(b, d3 - d5, d6);
                                } else {
                                    if (!d7) d8 = d5;
                                    ++d9;
                                    if (gv2(c, d4) + 1 <
                                        b[d3][d6] - gv(b, d3 - d5, d6))
                                        ++c[d4];
                                    if (d9 >= static_cast<int>(c2.size()))
                                        c2.push_back(0);
                                    c2[d9] = d5;
                                    for (int d10 = 0; d10 <= d4; ++d10)
                                        (*b2[d3 - d5])[d10] = d9;
                                    for (int d11 = 0; d11 <= d4; ++d11) {
                                        for (int d12 = d3 - d5 + 1; d12 <= d3; ++d12) {
                                            for (int d13 = d12; d13 >= d3 - d5; --d13) {
                                                for (int d14 = 0; d14 <= d11; ++d14) {
                                                    if (gv(b, d13, d14) <
                                                        b[d12][d14] - gv2(c3, d14)) {
                                                        if (d11 == d14) {
                                                            if (0 < (*b2[d13])[d11] &&
                                                                (*b2[d12])[d11] == 0)
                                                                (*b2[d12])[d11] = d9;
                                                            d13 = d3 - d5;
                                                        } else {
                                                            c3[d14] = b[d12][d14] - gv(b, d13, d14);
                                                        }
                                                    } else {
                                                        d14 = d11;
                                                    }
                                                }
                                            }
                                        }
                                        for (int d15 = 0; d15 <= d4; ++d15)
                                            c3[d15] = 0;
                                    }
                                    for (int d16 = 0; d16 <= d8; ++d16) {
                                        for (int d17 = 0; d17 <= d2; ++d17) {
                                            d18 = 0;
                                            if (0 < (*b2[d3 - d8 + d16])[d17]) {
                                                if (d17 < d4 + 1) {
                                                    // 原版：b[d3 - c2[b2[..][d17]]][d17]
                                                    const int src_col =
                                                        d3 - gv2(c2, (*b2[d3 - d8 + d16])[d17]);
                                                    d18 = gv(b, src_col, d17) -
                                                          gv(b, d3 - d5, d17);
                                                }
                                            }
                                            if (gv(b, d3 - d5 + d16, d17) <
                                                    gv(b, d3 - d8 + d16, d17) - d18 ||
                                                (1 < d5 - d7 && 0 < d7)) {
                                                d16 = d7;
                                                d17 = d2;
                                                d19 = 1;
                                                d5 = d3;
                                                --d9;
                                            } else if (gv(b, d3 - d8 + d16, d17) - d18 <
                                                       gv(b, d3 - d5 + d16, d17)) {
                                                d16 = d7;
                                                d17 = d2;
                                            }
                                        }
                                    }
                                    if (!d19) d7 = d5;
                                    else d19 = 0;
                                }
                            } else {
                                d6 = d4;
                            }
                        }
                    }
                    d4 = d2;
                }
            }

            for (int d20 = 0; d20 <= d2; ++d20) {
                if (0 < gv(b, d3, d20 + 1)) {
                    c[d20] = b[d3][d20] - gv(b, d3 - d7, d20);
                } else {
                    c[d20] = b[d3][d20] - gv(b, d3 - d7, d20) - 1;
                    d20 = d2;
                }
            }

            // result = b.slice(0, d3) 的逐列深拷贝
            Cols result;
            result.reserve(static_cast<std::size_t>(d3));
            for (int i = 0; i < d3; ++i) result.push_back(b[i]);

            for (int d21 = 1; d21 <= a * d7; ++d21) {
                while (static_cast<int>(result.size()) <= d3)
                    result.emplace_back(static_cast<std::size_t>(d2) + 1, 0);
                while (static_cast<int>(b2.size()) <= d3)
                    b2.push_back(std::make_shared<std::vector<int>>(
                        static_cast<std::size_t>(d2) + 1, 0));
                for (int d22 = 0; d22 <= d2; ++d22) {
                    if (0 < (*b2[d3 - d7])[d22] &&
                        (*b2[d3 - d7])[d22] < d9 + 1) {
                        result[d3][d22] = result[d3 - d7][d22] + c[d22];
                    } else {
                        result[d3][d22] = result[d3 - d7][d22];
                    }
                    (*b2[d3])[d22] = (*b2[d3 - d7])[d22];
                }
                ++d3;
            }

            if (d2 > 0) {
                bool all_zero = true;
                for (const auto& col : result)
                    if (static_cast<int>(col.size()) > d2 && col[d2] != 0) {
                        all_zero = false;
                        break;
                    }
                if (all_zero)
                    for (auto& col : result)
                        if (static_cast<int>(col.size()) > d2)
                            col.resize(static_cast<std::size_t>(d2));
            }
            return result;
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

    }  // namespace blm_detail

    [[nodiscard]] inline std::string
        BLMNotation::expand_text(std::string_view text, int term) {
        const auto e = bmlike::parse(text);
        if (!e) return bmlike::kParseError;
        if (e->infinity)
            return bmlike::matrix_to_bm_string(bmlike::infinity_fs(term));
        if (e->m.cols() == 0) return {};
        auto cols = blm_detail::expand(bmlike::standardize(e->m), term);
        blm_detail::normalize_cols(cols);
        return blm_detail::cols_to_string(cols);
    }

}  // namespace omegay::notation

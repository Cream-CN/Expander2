#pragma once
// ============================================================
// 0-Y 序列 <-> BMS 矩阵 互译器
//
// 原理（见用户给出的山脉图构造规则）：
//   前向 0-Y → BMS：
//     1. 把 0-Y 序列作为山脉图第 0 层，逐阶求阶差序列
//        （阶差项 = 元素 - 本层父项；无父项则取自身），
//        直到最上方一层全部为 1 为止。
//        第 0 层父项 = 左边第一个比它小的项；
//        第 l(l>=1) 层父项 = 左边第一个比它小、且其正下方项
//        是本层正下方项祖先项的项。
//     2. 重编号：无父项的项记 0，其余记为本层父项取值 +1
//        （即本层父项森林里的深度）；最顶层全为 0。
//     3. 删掉最顶层，把各层上下颠倒：BMS 第 0 行 = 原最底层上方一层。
//   反向 BMS → 0-Y：
//     1. BMS 上下颠倒得到各层重编号矩阵，补一个全 0 顶层。
//     2. 每层按深度反推父项列：深度为 0 ⇒ 无父项；
//        否则父项 = 左边第一个深度恰为 (本层深度-1) 的列。
//     3. 从顶层（全 1）逐层向下还原阶差值：
//        d[l-1][i] = d[l][i] + (有父项 ? d[l-1][父项] : 0)。
//        第 0 层即为原 0-Y 序列。
// ============================================================

#include "translator.hpp"
#include "../common/Matrix.hpp"
#include "../common/sequence.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::translator {
namespace detail_0ybms {

    // 山脉图的一层：val[i] = 阶差值，par[i] = 本层父项列（-1 表示无父项）
    struct Level {
        std::vector<int> val;
        std::vector<int> par;
    };

    // 在 levels 末尾的一层上计算本层父项。
    // level == 0：左边第一个更小者即可；
    // level >= 1：还要求正下方一层中，j 列是 i 列的祖先项。
    inline void computeParents(std::vector<Level>& levels) {
        const int lv = static_cast<int>(levels.size()) - 1;
        Level& cur = levels.back();
        const int n = static_cast<int>(cur.val.size());
        cur.par.assign(n, -1);
        for (int i = 0; i < n; ++i) {
            int found = -1;
            for (int j = i - 1; j >= 0; --j) {
                if (cur.val[j] >= cur.val[i]) continue;
                if (lv == 0) { found = j; break; }
                // 正下方一层（lv-1）：j 列是否在 i 列的祖先链上
                bool isAncestor = false;
                for (int c = i; c != -1; c = levels[lv - 1].par[c]) {
                    if (c == j) { isAncestor = true; break; }
                }
                if (isAncestor) { found = j; break; }
            }
            cur.par[i] = found;
        }
    }

    // 从输入文本中提取 0-Y 序列（容忍 "0-Y(1,2,3)" / "ω-Y(...)" / 裸逗号列表）。
    inline std::vector<int> extractSequence(std::string_view in) {
        std::string s{ in };
        // 若有括号，只取最外层括号内的内容
        const size_t lp = s.find('(');
        const size_t rp = s.rfind(')');
        if (lp != std::string_view::npos && rp != std::string_view::npos && rp > lp) {
            s = s.substr(lp + 1, rp - lp - 1);
        }
        return common::parse_sequence(s);
    }

    // ---------------- 0-Y -> BMS ----------------
    inline std::string zeroYToBms(const std::vector<int>& seq) {
        if (seq.empty()) return std::string(kParseError);
        for (int v : seq) if (v <= 0) return std::string(kParseError);

        std::vector<Level> levels;
        Level bottom;
        bottom.val = seq;
        levels.push_back(bottom);
        computeParents(levels);

        int guard = 0;
        while (true) {
            bool allOne = true;
            for (int v : levels.back().val)
                if (v != 1) { allOne = false; break; }
            if (allOne) break;
            if (++guard > 64) return std::string(kNotSupported);

            Level nx;
            const int n = static_cast<int>(levels.back().val.size());
            nx.val.assign(n, 0);
            const Level& cur = levels.back();
            for (int i = 0; i < n; ++i) {
                const int p = cur.par[i];
                nx.val[i] = (p < 0) ? cur.val[i] : cur.val[i] - cur.val[p];
            }
            levels.push_back(nx);
            computeParents(levels);
        }

        const int H = static_cast<int>(levels.size());
        // 重编号：R[l][i] = 本层森林深度
        std::vector<std::vector<int>> R(H);
        for (int l = 0; l < H; ++l) {
            R[l].assign(levels[l].val.size(), 0); // 顶层无父项，全 0
        }
        for (int l = 0; l < H - 1; ++l) {
            for (int i = 0; i < static_cast<int>(R[l].size()); ++i) {
                const int p = levels[l].par[i];
                R[l][i] = (p < 0) ? 0 : R[l][p] + 1;
            }
        }

        // 删掉最顶层（l = H-1），上下颠倒：BMS 行 r = R[H-2-r]
        const int rows = H - 1;
        const int cols = static_cast<int>(R[0].size());
        if (rows <= 0) return std::string(); // 原序列已全为 1 ⇒ 空 BMS
        std::string out;
        for (int c = 0; c < cols; ++c) {
            out += '(';
            for (int r = 0; r < rows; ++r) {
                if (r) out += ',';
                out += std::to_string(R[H - 2 - r][c]);
            }
            out += ')';
        }
        return out;
    }

    // ---------------- BMS -> 0-Y ----------------
    inline std::string bmsToZeroY(const common::Matrix& m) {
        const int rows = m.rows();
        const int cols = m.cols();
        if (rows == 0 || cols == 0) return std::string(kRangeError);

        // renum[l] = BMS 第 (rows-1-l) 行
        // 反推每层父项 p[l]：深度 0 ⇒ 无父项；否则左边第一个深度恰为 d-1 的列
        std::vector<std::vector<int>> p(rows);
        for (int l = 0; l < rows; ++l) {
            const int mrow = rows - 1 - l;
            p[l].assign(cols, -1);
            for (int i = 0; i < cols; ++i) {
                const int d = m(mrow, i);
                if (d <= 0) { p[l][i] = -1; continue; }
                int found = -1;
                for (int j = i - 1; j >= 0; --j) {
                    if (m(mrow, j) == d - 1) { found = j; break; }
                }
                if (found < 0) return std::string(kRangeError);
                p[l][i] = found;
            }
        }

        // 阶差数组 d[0..rows]：d[rows] 是被删掉的全 1 顶层
        std::vector<std::vector<int>> d(rows + 1);
        d[rows].assign(cols, 1);
        for (int l = rows; l >= 1; --l) {
            d[l - 1].assign(cols, 0);
            for (int i = 0; i < cols; ++i) {
                const int par = p[l - 1][i];
                d[l - 1][i] = d[l][i] + (par < 0 ? 0 : d[l - 1][par]);
            }
        }

        std::string out;
        for (int i = 0; i < cols; ++i) {
            if (i) out += ',';
            out += std::to_string(d[0][i]);
        }
        return out;
    }

} // namespace detail_0ybms

// ---------------- 对外互译函数（符合 TranslateFn 签名） ----------------
[[nodiscard]] inline std::string zeroYToBmsTranslate(std::string_view input) {
    try {
        const std::vector<int> seq = detail_0ybms::extractSequence(input);
        if (seq.empty()) return kParseError;
        return detail_0ybms::zeroYToBms(seq);
    }
    catch (...) {
        return kParseError;
    }
}

[[nodiscard]] inline std::string bmsToZeroYTranslate(std::string_view input) {
    try {
        long term = 0;
        const common::Matrix m = common::parse_matrix(input, term);
        if (m.empty()) return kParseError;
        return detail_0ybms::bmsToZeroY(m);
    }
    catch (...) {
        return kParseError;
    }
}

} // namespace omegay::translator

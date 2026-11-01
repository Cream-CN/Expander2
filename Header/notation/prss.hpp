#pragma once

#include <vector>
#include <string>

namespace omegay::notation {

    // PrSS（Primitive Sequence System，原始数列系统，Bashicu）。
    //
    // 定义：
    //   1. () = 0
    //   2. (#, 0) = (#) + 1，式中 # 为任意合法序列（后继序数）
    //   3. (#_1, a_i, #_2, a_k) = (#_1, a_i, #_2, a_i, #_2, ...)，式中 #_1,#_2 为
    //      任意两段合法序列，a_k > 0，a_i = a_k - 1 为 a_k 前首个小于 a_k 的数，
    //      省略号代表任意有限次循环的极限。
    //
    // 基本列 S[n]：
    //   - 若末项为 0：后继序数，S[n] = 去掉末项 0（即前驱）。
    //   - 否则：记末项 a_k > 0，取 a_k 前最右一个小于 a_k 的数 a_i 为坏根，令
    //       好部 G = (#_1)，坏部 B = (a_i, #_2)；S[n] = G + B 重复 n 次。
    //     这与第 3 条一致：极限表达式 ( #_1, a_i, #_2, a_i, #_2, ... ) 的第 n 项
    //     截断为 G + (a_i, #_2) * n。
    //
    // 本头文件采用与 PPS 相同的平面整数序列接口（std::vector<int>），
    // 输入 / 输出均为英文逗号分隔的非负整数序列，如 "0,1,2"。

    namespace prss_detail {
        // 末项为 0 → 后继序数。
        [[nodiscard]] inline bool prss_is_successor(const std::vector<int>& seq) {
            return !seq.empty() && seq.back() == 0;
        }

        // 取 a_k 前最右一个小于 a_k 的数的下标；不存在返回 -1。
        [[nodiscard]] inline int prss_bad_root(const std::vector<int>& seq) {
            if (seq.empty()) return -1;
            const int a_k = seq.back();
            for (int i = static_cast<int>(seq.size()) - 2; i >= 0; --i) {
                if (seq[i] < a_k) return i;
            }
            return -1;
        }

        [[nodiscard]] inline std::vector<int> expand_prss(
            const std::vector<int>& seq, int n)
        {
            if (seq.empty()) return seq;
            if (prss_is_successor(seq)) {
                // (#, 0) = (#) + 1，前驱 = 去掉末尾 0。
                return std::vector<int>(seq.begin(), seq.end() - 1);
            }

            const int r = prss_bad_root(seq);
            if (r < 0) {
                // 异常输入（不存在小于末项的数），退化为去掉末项，避免越界。
                return std::vector<int>(seq.begin(), seq.end() - 1);
            }

            if (n < 1) n = 1;

            const int a_k = seq.back();
            const std::size_t bad_len =
                static_cast<std::size_t>(seq.size()) - 1 - static_cast<std::size_t>(r);

            std::vector<int> res;
            res.reserve(static_cast<std::size_t>(r) + bad_len * static_cast<std::size_t>(n));

            // 好部 G = seq[0 .. r-1]
            res.insert(res.end(), seq.begin(), seq.begin() + r);
            // 坏部 B = seq[r .. size-2]（不含末项 a_k），重复 n 次
            for (int copy = 0; copy < n; ++copy) {
                res.insert(res.end(), seq.begin() + r, seq.end() - 1);
            }

            (void)a_k; // 留作语义说明：坏根值为 a_k - 1（标准表达式）。
            return res;
        }
    } // namespace prss_detail

    struct PrSSNotation {
        static constexpr const char* kName = "PrSS";
        [[nodiscard]] static std::vector<int> expand(
            const std::vector<int>& seq, int term)
        {
            return prss_detail::expand_prss(seq, term);
        }
        static std::string suffix() { return ""; }
    };

} // namespace omegay::notation

#pragma once

#include <vector>
#include <string>
#include <cstddef>

namespace omegay::notation {

    namespace pps_detail {

        // ------------------------------------------------------------------
        // 通用辅助：判断后继序数（末项为 0）
        // ------------------------------------------------------------------
        inline bool is_successor(const std::vector<int>& seq) {
            return !seq.empty() && seq.back() == 0;
        }
        inline std::vector<int> expand_pps1(const std::vector<int>& seq, int n) {
            if (seq.empty()) return seq;
            if (is_successor(seq)) {
                std::vector<int> res(seq.begin(), seq.end() - 1);
                return res;
            }

            const int y = static_cast<int>(seq.size()); // 末项列标（1-based）
            const int x = seq.back();                   // 末项值
            if (x <= 0 || x > y) return seq;            // 非法输入，原样返回

            const int bad_root_index = x - 1;           // 坏根 0-based 索引
            const int b = seq[bad_root_index];          // 坏根值
            const int L = y - x;                        // L = y - x
            if (L <= 0) return seq;

            // 构造初始序列：原序列前 y-1 项 + 新末项
            std::vector<int> res(seq.begin(), seq.end() - 1);

            // 判断弱展开 / 强展开
            bool weak = false;
            for (int i = bad_root_index + 1; i < y - 1; ++i) {
                if (seq[i] == b) {
                    weak = true;
                    break;
                }
            }

            int new_last;
            if (weak) {
                new_last = b;               // 弱展开：末项换成 b
            }
            else {
                new_last = seq.back() - 1;  // 强展开：末项减 1
            }
            res.push_back(new_last);

            // 目标长度：第 y + n*L - 1 项
            int target_len = y + n * L - 1;
            if (target_len < y) target_len = y;

            // 递归生成其他项
            for (int i = x + 1; i <= target_len - L; ++i) {
                int val_i;
                if (i < y) {
                    val_i = seq[i - 1];
                }
                else if (i == y) {
                    val_i = res[y - 1];     // 新末项
                }
                else {
                    if (i - 1 < static_cast<int>(res.size())) {
                        val_i = res[i - 1];
                    }
                    else {
                        continue;
                    }
                }

                int new_val = (val_i >= x) ? (val_i + L) : val_i;
                int target_index = i + L;   // 1-based
                if (target_index <= target_len) {
                    if (target_index - 1 >= static_cast<int>(res.size())) {
                        res.resize(target_index);
                    }
                    res[target_index - 1] = new_val;
                }
            }

            return res;
        }

        // ------------------------------------------------------------------
        // PPS4 家族展开
        // 对应 PPS-family.js 中的 expand_pps4(sequence, fs_term, variant)
        // ------------------------------------------------------------------
        enum class PPS4Variant {
            PPS4,     // 找 <= b
            WPPS4,    // 只找 === b
            TPPS4,    // 找 <= b，且找到后走 strong_expand 独立构造
            EWPPS4,   // 找 === b；遇到 < b 立即停止
        };

        inline std::vector<int> expand_pps4_variant(
            const std::vector<int>& seq, int n, PPS4Variant variant)
        {
            if (seq.empty()) return seq;

            // 后继序数：基本列为去掉末项
            if (is_successor(seq)) {
                std::vector<int> res(seq.begin(), seq.end() - 1);
                return res;
            }

            const int y = static_cast<int>(seq.size()); // 末项列标（1-based）
            const int x = seq.back();                   // 末项值
            if (x <= 0 || x > y) return seq;

            const int bad_root_index = x - 1;           // 坏根 0-based 索引
            const int b = seq[bad_root_index];          // 坏根值
            const int L = y - x;
            if (L <= 0) return seq;

            std::vector<int> res(seq.begin(), seq.end() - 1);

            // 判断弱展开：x+1 .. y-1 中是否存在 === b
            bool weak = false;
            for (int i = bad_root_index + 1; i < y - 1; ++i) {
                if (seq[i] == b) {
                    weak = true;
                    break;
                }
            }

            int new_last = b;
            bool strong_expand = false;

            if (!weak) {
                int found_col = -1;

                if (variant == PPS4Variant::EWPPS4) {
                    // 从 x-2 向左找 === b；遇到 < b 停止
                    for (int candidate = x - 2; candidate >= b; --candidate) {
                        if (candidate < 0 || candidate >= y) continue;
                        const int v = seq[candidate];
                        if (v == b) {
                            found_col = candidate + 1;
                            break;
                        }
                        if (v < b) break;
                    }
                }
                else if (variant == PPS4Variant::WPPS4) {
                    // 只找 === b
                    for (int candidate = x - 2; candidate >= b; --candidate) {
                        if (candidate < 0 || candidate >= y) continue;
                        if (seq[candidate] == b) {
                            found_col = candidate + 1;
                            break;
                        }
                    }
                }
                else {
                    // pps4 / tpps4：找 <= b
                    for (int candidate = x - 2; candidate >= b; --candidate) {
                        if (candidate < 0 || candidate >= y) continue;
                        if (seq[candidate] <= b) {
                            found_col = candidate + 1;
                            break;
                        }
                    }
                }

                if (found_col != -1) {
                    new_last = found_col;
                    strong_expand = (variant == PPS4Variant::TPPS4);
                }
                else {
                    new_last = b;           // 找不到则等同弱展开
                }
            }

            res.push_back(new_last);

            // ----------------------------------------------------------
            // TPPS4 的强展开：走独立构造
            // ----------------------------------------------------------
            if (strong_expand) {
                const int total_len = y + n * L;
                std::vector<int> out;
                out.reserve(total_len);

                for (int i = 0; i < y - 1; ++i) out.push_back(seq[i]);
                out.push_back(new_last);

                for (int position = y + 1; position <= total_len; ++position) {
                    const bool is_last_copy =
                        (position > y) && ((position - y) % L == 0);
                    if (is_last_copy) {
                        const int copy_number = (position - y) / L;
                        out.push_back(new_last + copy_number * L);
                    }
                    else {
                        const int source_position = position - L;
                        const int source_value = out[source_position - 1];
                        out.push_back(source_value >= x ? source_value + L
                            : source_value);
                    }
                }
                return out;
            }

            // ----------------------------------------------------------
            // 其余变体：逐项 + 偏移构造
            // ----------------------------------------------------------
            int target_len = y + n * L - 1;
            if (target_len < y) target_len = y;

            for (int i = x + 1; i <= target_len - L; ++i) {
                int val_i;
                if (i < y) {
                    val_i = seq[i - 1];
                }
                else if (i == y) {
                    val_i = res[y - 1];
                }
                else {
                    if (i - 1 < static_cast<int>(res.size())) {
                        val_i = res[i - 1];
                    }
                    else {
                        continue;
                    }
                }

                const int new_val = (val_i >= x) ? (val_i + L) : val_i;
                const int target_index = i + L;
                if (target_index <= target_len) {
                    if (target_index - 1 >= static_cast<int>(res.size())) {
                        res.resize(target_index);
                    }
                    res[target_index - 1] = new_val;
                }
            }

            return res;
        }

        // 保留原名字，默认等价于 pps4
        inline std::vector<int> expand_pps4(const std::vector<int>& seq, int n) {
            return expand_pps4_variant(seq, n, PPS4Variant::PPS4);
        }

        // ------------------------------------------------------------------
        // Second PPS4 展开
        // 对应 PPS-family.js 中的 expand_second_pps4
        // ------------------------------------------------------------------
        inline std::vector<int> expand_second_pps4(const std::vector<int>& seq, int count) {
            if (seq.empty()) return seq;

            const int y = static_cast<int>(seq.size());
            const int x = seq.back();
            if (x == 0) return std::vector<int>(seq.begin(), seq.end() - 1);
            if (x > y) return seq; // 原 JS 抛异常，这里按你的接口约定原样返回

            const int b = seq[x - 1];
            const int L = y - x;
            if (L <= 0) return seq;

            int value = b;
            bool strong_expand = false;
            bool found_less_or_equal = false;

            for (int column = y - 1; column >= x + 1; --column) {
                if (seq[column - 1] <= b) {
                    found_less_or_equal = true;
                    break;
                }
            }

            if (!found_less_or_equal) {
                int found_column = -1;
                const int strong_start = b + 1;
                const int strong_end = x - 1;
                if (strong_start <= strong_end) {
                    for (int candidate = strong_end; candidate >= strong_start; --candidate) {
                        if (seq[candidate - 1] == b) {
                            found_column = candidate;
                            break;
                        }
                    }
                }
                if (found_column != -1) {
                    value = found_column;
                    strong_expand = true;
                }
            }

            const int total_length = y + count * L - 1;
            if (total_length <= 0) return {};

            std::vector<int> result(total_length);

            for (int i = 0; i < x; ++i) result[i] = seq[i];
            for (int i = x; i < y - 1; ++i) result[i] = seq[i];
            result[y - 1] = value;

            for (int i = x; i < y; ++i) {
                const int base_value = (i == y - 1) ? value : seq[i];
                const int shifts = (i == y - 1) ? count - 1 : count;
                for (int copy = 1; copy <= shifts; ++copy) {
                    const int position = i + copy * L;
                    if (position >= total_length) continue;
                    if ((i == y - 1 && strong_expand) || base_value >= x) {
                        result[position] = base_value + copy * L;
                    }
                    else {
                        result[position] = base_value;
                    }
                }
            }

            return result;
        }

        // ------------------------------------------------------------------
        // 2-pps4 展开
        // 对应 PPS-family.js 中的 expandPPS / FS 对 [0,2] 的特判
        // ------------------------------------------------------------------
        inline std::vector<int> expand_2_pps4(const std::vector<int>& seq, int n) {
            if (seq.empty()) return seq;

            const int y = static_cast<int>(seq.size());
            const int x = seq.back();

            // 原 JS 中 n === 0 先于 [0,2] 特判
            if (n == 0) {
                return std::vector<int>(seq.begin(), seq.end() - 1);
            }

            if (x == 0) {
                return std::vector<int>(seq.begin(), seq.end() - 1);
            }
            if (x > y) return seq;

            // 特判 [0,2]
            if (y == 2 && seq[0] == 0 && seq[1] == 2) {
                std::vector<int> result;
                for (int i = 0; i <= n; ++i) result.push_back(i);
                return result;
            }

            const int b = seq[x - 1];
            const int L = y - x;
            if (L <= 0) return seq;

            int equal_count = 0;
            for (int col = x + 1; col <= y - 1; ++col) {
                if (seq[col - 1] == b) ++equal_count;
            }

            int v;
            if (equal_count >= 2) {
                v = b;
            }
            else {
                int found_col = -1;
                const int strong_start = b + 1;
                const int strong_end = x - 1;
                if (strong_start <= strong_end) {
                    for (int col = strong_end; col >= strong_start; --col) {
                        if (seq[col - 1] <= b) {
                            found_col = col;
                            break;
                        }
                    }
                }
                v = (found_col != -1) ? found_col : b;
            }

            const int total_len = y + n * L - 1;
            if (total_len <= 0) return {};

            std::vector<int> res(total_len);

            for (int i = 0; i < x; ++i) res[i] = seq[i];
            for (int i = x; i < y - 1; ++i) res[i] = seq[i];
            res[y - 1] = v;

            for (int i = x; i < y; ++i) {
                const int base_val = (i == y - 1) ? v : seq[i];
                const bool ge = base_val >= x;
                const int max_k = (i == y - 1) ? n - 1 : n;
                for (int k = 1; k <= max_k; ++k) {
                    const int pos = i + k * L;
                    if (pos >= total_len) continue;
                    res[pos] = ge ? (base_val + k * L) : base_val;
                }
            }

            return res;
        }

    } // namespace pps_detail

    // ==================================================================
    // PPS 家族接口定义
    // ==================================================================

    // PPS1（简称 PPS）
    struct PPSNotation {
        static constexpr const char* kName = "PPS";
        static std::vector<int> expand(const std::vector<int>& seq, int term) {
            return pps_detail::expand_pps1(seq, term);
        }
        static std::string suffix() { return ""; }
    };

    // PPS4
    struct PPS4Notation {
        static constexpr const char* kName = "PPS4";
        static std::vector<int> expand(const std::vector<int>& seq, int term) {
            return pps_detail::expand_pps4_variant(
                seq, term, pps_detail::PPS4Variant::PPS4);
        }
        static std::string suffix() { return ""; }
    };

    // Weak PPS4
    struct WPPS4Notation {
        static constexpr const char* kName = "Weak PPS4";
        static std::vector<int> expand(const std::vector<int>& seq, int term) {
            return pps_detail::expand_pps4_variant(
                seq, term, pps_detail::PPS4Variant::WPPS4);
        }
        static std::string suffix() { return ""; }
    };

    // Third PPS4
    struct TPPS4Notation {
        static constexpr const char* kName = "Third PPS4";
        static std::vector<int> expand(const std::vector<int>& seq, int term) {
            return pps_detail::expand_pps4_variant(
                seq, term, pps_detail::PPS4Variant::TPPS4);
        }
        static std::string suffix() { return ""; }
    };

    // Extremely Weak PPS4
    struct EWPPS4Notation {
        static constexpr const char* kName = "Extremely Weak PPS4";
        static std::vector<int> expand(const std::vector<int>& seq, int term) {
            return pps_detail::expand_pps4_variant(
                seq, term, pps_detail::PPS4Variant::EWPPS4);
        }
        static std::string suffix() { return ""; }
    };

    // Second PPS4
    struct SecondPPS4Notation {
        static constexpr const char* kName = "Second PPS4";
        static std::vector<int> expand(const std::vector<int>& seq, int term) {
            return pps_detail::expand_second_pps4(seq, term);
        }
        static std::string suffix() { return ""; }
    };

    // 2-pps4
    struct PPS2Notation {
        static constexpr const char* kName = "2-pps4";
        static std::vector<int> expand(const std::vector<int>& seq, int term) {
            return pps_detail::expand_2_pps4(seq, term);
        }
        static std::string suffix() { return ""; }
    };

} // namespace omegay::notation
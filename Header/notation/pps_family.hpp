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

        // ------------------------------------------------------------------
        // PPS1 展开
        // 规则来源：main.txt - PPS1
        // ------------------------------------------------------------------
        inline std::vector<int> expand_pps1(const std::vector<int>& seq, int n) {
            if (seq.empty()) return seq;

            // 后继序数：基本列为去掉末项
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
        // PPS4 展开
        // 规则来源：main.txt - PPS4
        // ------------------------------------------------------------------
        inline std::vector<int> expand_pps4(const std::vector<int>& seq, int n) {
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
                // 强展开：在第 b 列和第 x 列（都不含）之间
                // 找到最右侧的值 <= b 的项，将末项换为这个项的列标
                int found_col = -1;
                const int start_col = b + 1;
                const int end_col = x - 1;
                for (int col = end_col; col >= start_col; --col) {
                    if (col >= 1 && col <= y) {
                        if (seq[col - 1] <= b) {
                            found_col = col;
                            break;
                        }
                    }
                }

                if (found_col != -1) {
                    new_last = found_col;
                }
                else {
                    new_last = b;           // 找不到则等同弱展开
                }
            }
            res.push_back(new_last);

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

                int new_val = (val_i >= x) ? (val_i + L) : val_i;
                int target_index = i + L;
                if (target_index <= target_len) {
                    if (target_index - 1 >= static_cast<int>(res.size())) {
                        res.resize(target_index);
                    }
                    res[target_index - 1] = new_val;
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
            return pps_detail::expand_pps4(seq, term);
        }
        static std::string suffix() { return ""; }
    };

    // ------------------------------------------------------------------
    // 以下变体在 main.txt 中没有给出独立规则，
    // 暂以 PPS4 逻辑实现，保证 Expander.cpp 可编译运行。
    // 若后续补充规则，只需替换 expand 内部的调用。
    // ------------------------------------------------------------------

    struct WPPS4Notation {
        static constexpr const char* kName = "Weak PPS4";
        static std::vector<int> expand(const std::vector<int>& seq, int term) {
            return pps_detail::expand_pps4(seq, term);
        }
        static std::string suffix() { return ""; }
    };

    struct TPPS4Notation {
        static constexpr const char* kName = "Third PPS4";
        static std::vector<int> expand(const std::vector<int>& seq, int term) {
            return pps_detail::expand_pps4(seq, term);
        }
        static std::string suffix() { return ""; }
    };

    struct EWPPS4Notation {
        static constexpr const char* kName = "Extremely Weak PPS4";
        static std::vector<int> expand(const std::vector<int>& seq, int term) {
            return pps_detail::expand_pps4(seq, term);
        }
        static std::string suffix() { return ""; }
    };

    struct SecondPPS4Notation {
        static constexpr const char* kName = "Second PPS4";
        static std::vector<int> expand(const std::vector<int>& seq, int term) {
            return pps_detail::expand_pps4(seq, term);
        }
        static std::string suffix() { return ""; }
    };

    struct PPS2Notation {
        static constexpr const char* kName = "2-pps4";
        static std::vector<int> expand(const std::vector<int>& seq, int term) {
            return pps_detail::expand_pps4(seq, term);
        }
        static std::string suffix() { return ""; }
    };

} // namespace omegay::notation
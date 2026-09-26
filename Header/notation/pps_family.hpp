#pragma once
//各位贡献者请注意，这不只是PPS4一个
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

namespace omegay::notation {

inline constexpr int kInfinity = std::numeric_limits<int>::max();

namespace detail {

inline bool is_limit_sentinel(const std::vector<int>& seq) {
    return seq.size() == 1 && seq[0] == kInfinity;
}

inline void set_at(std::vector<int>& v, int index, int value) {
    if (index < 0) return;
    if (static_cast<std::size_t>(index) >= v.size()) {
        v.resize(static_cast<std::size_t>(index) + 1);
    }
    v[static_cast<std::size_t>(index)] = value;
}

inline std::vector<int> limit(int n) {
    std::vector<int> result;
    if (n < 0) return result;
    result.reserve(static_cast<std::size_t>(n) + 1);
    for (int i = 0; i <= n; ++i) result.push_back(i);
    return result;
}

// ---------- PPS ----------

inline std::vector<int> expand_pps(const std::vector<int>& sequence, int fs_term) {
    const int length = static_cast<int>(sequence.size());
    if (length == 0) return {};

    const int last = sequence.back();
    const int parent_column = last;

    int bad_root = 0;
    std::vector<int> bad_part;
    int copy_width = 0;
    bool weak_expand = false;

    if (parent_column >= 1 && parent_column <= length) {
        bad_root = parent_column;
        const int root_value = sequence[bad_root - 1];

        // JS: sequence.slice(bad_root, length - 1)
        if (bad_root < length - 1) {
            bad_part.assign(
                sequence.begin() + bad_root,
                sequence.begin() + (length - 1)
            );
        }

        copy_width = length - bad_root;
        for (int value : bad_part) {
            if (value == root_value) {
                weak_expand = true;
                break;
            }
        }
    } else {
        copy_width = length - parent_column;
    }

    std::vector<int> result(sequence.begin(), sequence.end() - 1);

    for (int copy = 1; copy <= fs_term; ++copy) {
        result.push_back(weak_expand ? sequence[bad_root - 1] : last - 1);
        for (int value : bad_part) {
            result.push_back(value < last ? value : value + copy_width * copy);
        }
    }

    return result;
}

inline std::vector<int> pps_fs(const std::vector<int>& sequence, int fs_term) {
    if (is_limit_sentinel(sequence)) return limit(fs_term);
    if (sequence.empty()) return {};
    return expand_pps(sequence, fs_term);
}

// ---------- PPS4 系列 ----------
// variant: 0=pps4, 1=wpps4, 2=tpps4, 3=ewpps4

inline std::vector<int> expand_pps4(
    const std::vector<int>& sequence,
    int fs_term,
    int variant
) {
    if (sequence.empty()) return {};

    const int y = static_cast<int>(sequence.size());
    const int x = sequence[y - 1];

    if (x == 0 || x > y) {
        return std::vector<int>(sequence.begin(), sequence.end() - 1);
    }

    const int b = sequence[x - 1];
    const int width = y - x;

    bool weak_expand = false;
    for (int column = x + 1; column < y; ++column) {
        if (sequence[column - 1] == b) {
            weak_expand = true;
            break;
        }
    }

    int value = 0;
    bool strong_expand = false;

    if (weak_expand) {
        value = b;
    } else {
        int found_column = -1;

        for (int candidate = x - 2; candidate >= b && candidate >= 0; --candidate) {
            const int candidate_value = sequence[candidate];

            if (variant == 0) {
                // pps4
                if (candidate_value <= b) {
                    found_column = candidate + 1;
                    break;
                }
            } else if (variant == 3) {
                // ewpps4
                if (candidate_value == b) {
                    found_column = candidate + 1;
                    break;
                }
                if (candidate_value < b) break;
            } else {
                // wpps4, tpps4
                if (candidate_value == b) {
                    found_column = candidate + 1;
                    break;
                }
            }
        }

        if (found_column != -1) {
            value = found_column;
            strong_expand = (variant == 2); // tpps4
        } else {
            value = b;
        }
    }

    const int total_length = y + fs_term * width;

    if (strong_expand) {
        std::vector<int> result(sequence.begin(), sequence.end() - 1);
        result.push_back(value);

        for (int position = y + 1; position <= total_length; ++position) {
            const bool is_last_copy =
                position > y && (position - y) % width == 0;

            if (is_last_copy) {
                const int copy_number = (position - y) / width;
                result.push_back(value + copy_number * width);
            } else {
                const int source_position = position - width;
                const int source_value = result[source_position - 1];
                result.push_back(source_value >= x ? source_value + width : source_value);
            }
        }

        return result;
    }

    std::vector<int> result(sequence.begin(), sequence.end() - 1);
    result.push_back(value);

    const int start_column = y - width + 1;
    for (int index = start_column; static_cast<int>(result.size()) < total_length; ++index) {
        const int source_index = index - 1;
        if (source_index >= static_cast<int>(result.size())) break;

        const int source = result[source_index];
        result.push_back(source >= x ? source + width : source);
    }

    return result;
}

inline std::vector<int> pps4_fs(
    const std::vector<int>& sequence,
    int fs_term,
    int variant
) {
    if (is_limit_sentinel(sequence)) return limit(fs_term);
    if (sequence.empty()) return {};
    return expand_pps4(sequence, fs_term, variant);
}

// ---------- Second PPS4 / spps4 ----------

inline bool is_second_pps4_infinity(const std::vector<int>& sequence) {
    return is_limit_sentinel(sequence);
}

inline std::vector<int> expand_second_pps4(
    const std::vector<int>& sequence,
    int count
) {
    if (sequence.empty()) return {};

    const int y = static_cast<int>(sequence.size());
    const int x = sequence[y - 1];

    if (x == 0) {
        return std::vector<int>(sequence.begin(), sequence.end() - 1);
    }
    if (x > y) {
        return {}; // 不抛异常；JS 中此处会抛，这里按 CONTRIBUTING 约定返回空
    }

    const int b = sequence[x - 1];
    const int width = y - x;

    int value = 0;
    bool strong_expand = false;
    bool found_less_or_equal = false;

    for (int column = y - 1; column >= x + 1; --column) {
        if (sequence[column - 1] <= b) {
            found_less_or_equal = true;
            break;
        }
    }

    if (found_less_or_equal) {
        value = b;
    } else {
        int found_column = -1;
        const int strong_start = b + 1;
        const int strong_end = x - 1;

        if (strong_start <= strong_end) {
            for (int candidate = strong_end; candidate >= strong_start; --candidate) {
                if (sequence[candidate - 1] == b) {
                    found_column = candidate;
                    break;
                }
            }
        }

        if (found_column != -1) {
            value = found_column;
            strong_expand = true;
        } else {
            value = b;
        }
    }

    const int total_length = y + count * width - 1;

    std::vector<int> result;
    if (total_length > 0) result.resize(static_cast<std::size_t>(total_length), 0);

    for (int i = 0; i < x; ++i) {
        set_at(result, i, sequence[i]);
    }
    for (int i = x; i < y - 1; ++i) {
        set_at(result, i, sequence[i]);
    }
    set_at(result, y - 1, value);

    for (int index = x; index < y; ++index) {
        const int base_value = (index == y - 1) ? value : sequence[index];
        const int shifts = (index == y - 1) ? count - 1 : count;

        for (int copy = 1; copy <= shifts; ++copy) {
            const int position = index + copy * width;
            if (position >= total_length) continue;

            if ((index == y - 1 && strong_expand) || base_value >= x) {
                set_at(result, position, base_value + copy * width);
            } else {
                set_at(result, position, base_value);
            }
        }
    }

    return result;
}

inline std::vector<int> second_pps4_fs(
    const std::vector<int>& sequence,
    int index
) {
    if (index < 0) return {};
    if (is_second_pps4_infinity(sequence)) return limit(index);
    if (sequence.empty()) return {};
    if (index == 0) {
        return std::vector<int>(sequence.begin(), sequence.end() - 1);
    }
    return expand_second_pps4(sequence, index);
}

// ---------- 2-pps4 ----------

inline std::vector<int> expand_pps2(
    const std::vector<int>& seq,
    int nCount,
    int maxLength = -1
) {
    if (seq.empty()) return {};

    const int Y = static_cast<int>(seq.size());
    const int X = seq[Y - 1];
    const bool hasMax = (maxLength >= 0);

    if (X == 0) {
        std::vector<int> zeroRes(seq.begin(), seq.end() - 1);
        if (hasMax && static_cast<int>(zeroRes.size()) > maxLength) {
            zeroRes.resize(static_cast<std::size_t>(maxLength));
        }
        return zeroRes;
    }

    if (X > Y) {
        return {}; // 不抛异常；JS 中此处会抛
    }

    const int B = seq[X - 1];
    const int L = Y - X;

    int v = 0;
    int equalCount = 0;

    for (int col = X + 1; col <= Y - 1; ++col) {
        if (seq[col - 1] == B) ++equalCount;
    }

    if (equalCount >= 2) {
        v = B;
    } else {
        const int strongStart = B + 1;
        const int strongEnd = X - 1;
        int foundCol = -1;

        if (strongStart <= strongEnd) {
            for (int col2 = strongEnd; col2 >= strongStart; --col2) {
                if (seq[col2 - 1] <= B) {
                    foundCol = col2;
                    break;
                }
            }
        }

        v = (foundCol != -1) ? foundCol : B;
    }

    const int totalLen = Y + nCount * L - 1;

    std::vector<int> res;
    if (totalLen > 0) res.resize(static_cast<std::size_t>(totalLen), 0);

    for (int i = 0; i < X; ++i) {
        set_at(res, i, seq[i]);
    }
    for (int i = X; i < Y - 1; ++i) {
        set_at(res, i, seq[i]);
    }
    set_at(res, Y - 1, v);

    for (int i = X; i < Y; ++i) {
        const int baseVal = (i == Y - 1) ? v : seq[i];
        const bool ge = baseVal >= X;
        const int maxK = (i == Y - 1) ? nCount - 1 : nCount;

        for (int k = 1; k <= maxK; ++k) {
            const int pos = i + k * L;
            if (pos >= totalLen) continue;

            if (ge) {
                set_at(res, pos, baseVal + k * L);
            } else {
                set_at(res, pos, baseVal);
            }
        }
    }

    if (hasMax && static_cast<int>(res.size()) > maxLength) {
        res.resize(static_cast<std::size_t>(maxLength));
    }

    return res;
}

inline std::vector<int> second_pps4_fs_2pps4(
    const std::vector<int>& arr,
    int nNum
) {
    if (arr.empty()) return {};
    if (nNum < 0) return {};
    if (is_limit_sentinel(arr)) return limit(nNum);
    if (nNum == 0) {
        return std::vector<int>(arr.begin(), arr.end() - 1);
    }

    if (arr.size() == 2 && arr[0] == 0 && arr[1] == 2) {
        std::vector<int> result;
        for (int i = 0; i <= nNum; ++i) result.push_back(i);
        return result;
    }

    return expand_pps2(arr, nNum);
}

} // namespace detail

// ---------- 对外记号接口 ----------

struct PPSNotation {
    static constexpr const char* kName = "PPS";

    [[nodiscard]] static std::vector<int> expand(
        const std::vector<int>& seq,
        int term
    ) {
        return detail::pps_fs(seq, term);
    }

    [[nodiscard]] static std::string suffix() { return {}; }
};

struct PPS4Notation {
    static constexpr const char* kName = "PPS4";

    [[nodiscard]] static std::vector<int> expand(
        const std::vector<int>& seq,
        int term
    ) {
        return detail::pps4_fs(seq, term, 0);
    }

    [[nodiscard]] static std::string suffix() { return {}; }
};

struct WPPS4Notation {
    static constexpr const char* kName = "Weak PPS4";

    [[nodiscard]] static std::vector<int> expand(
        const std::vector<int>& seq,
        int term
    ) {
        return detail::pps4_fs(seq, term, 1);
    }

    [[nodiscard]] static std::string suffix() { return {}; }
};

struct TPPS4Notation {
    static constexpr const char* kName = "Third PPS4";

    [[nodiscard]] static std::vector<int> expand(
        const std::vector<int>& seq,
        int term
    ) {
        return detail::pps4_fs(seq, term, 2);
    }

    [[nodiscard]] static std::string suffix() { return {}; }
};

struct EWPPS4Notation {
    static constexpr const char* kName = "Extremely Weak PPS4";

    [[nodiscard]] static std::vector<int> expand(
        const std::vector<int>& seq,
        int term
    ) {
        return detail::pps4_fs(seq, term, 3);
    }

    [[nodiscard]] static std::string suffix() { return {}; }
};

struct SecondPPS4Notation {
    static constexpr const char* kName = "Second PPS4";

    [[nodiscard]] static std::vector<int> expand(
        const std::vector<int>& seq,
        int term
    ) {
        return detail::second_pps4_fs(seq, term);
    }

    [[nodiscard]] static std::string suffix() { return {}; }
};

struct PPS2Notation {
    static constexpr const char* kName = "2-pps4";

    [[nodiscard]] static std::vector<int> expand(
        const std::vector<int>& seq,
        int term
    ) {
        return detail::second_pps4_fs_2pps4(seq, term);
    }

    [[nodiscard]] static std::string suffix() { return {}; }
};

} // namespace omegay::notation
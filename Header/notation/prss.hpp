#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

    struct PrSSNotation {
        static constexpr std::string_view kName = "PrSS";
        [[nodiscard]] static std::vector<int> expand(const std::vector<int>& seq, int term) {
            if (seq.empty()) return {};
            if (term <= 0) return seq;

            std::vector<int> result = seq;
            int k = static_cast<int>(result.size()) - 1;
            if (result[k] == 0) {
                result.pop_back();
                return result;
            }
            int i = -1;
            for (int j = k - 1; j >= 0; --j) {
                if (result[j] < result[k]) {
                    i = j;
                    break;
                }
            }

            if (i == -1) {
                int ak = result[k];
                result.pop_back();
                result.push_back(ak - 1);
                for (int t = 0; t < term; ++t) {
                    result.push_back(0);
                }
                return result;
            }

            int ai = result[i];
            std::vector<int> hash2(result.begin() + i + 1, result.begin() + k);
            result.pop_back();
            for (int t = 0; t < term; ++t) {
                result.push_back(ai);
                result.insert(result.end(), hash2.begin(), hash2.end());
            }

            return result;
        }

        [[nodiscard]] static std::string suffix() {
            return " [PrSS]";
        }
    };

} // namespace omegay::notation
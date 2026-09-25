#pragma once
#include <algorithm>
#include <cctype>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::common {

[[nodiscard]] inline std::vector<int> parse_sequence(std::string_view str) {
    std::vector<int> seq;
    for (auto part : str | std::views::split(',')) {
        std::string token{ part.begin(), part.end() };
        token.erase(std::remove_if(token.begin(), token.end(),
            [](unsigned char c) { return std::isspace(c); }), token.end());
        if (token.empty()) continue;
        try { seq.push_back(std::stoi(token)); } catch (...) {}
    }
    return seq;
}

[[nodiscard]] inline std::string seq_to_string(const std::vector<int>& seq) {
    if (seq.empty()) return "[]";
    std::string out;
    for (size_t i = 0; i < seq.size(); ++i) {
        if (i) out += ',';
        out += std::to_string(seq[i]);
    }
    return out;
}

} // namespace omegay::common
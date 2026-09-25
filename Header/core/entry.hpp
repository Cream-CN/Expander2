#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

namespace omegay::core {

struct Entry {
    int value = 0;
    int x = 0;
    std::vector<int> y;
    uint64_t ykey = 0;

    std::vector<Entry*> leftleg_up;
    Entry* rightleg_up   = nullptr;
    Entry* rightleg_down = nullptr;
    Entry* leftleg_down  = nullptr;

    Entry() = default;
    Entry(int v, int x_, std::vector<int> y_)
        : value(v), x(x_), y(std::move(y_)) { refresh_key(); }

    void refresh_key() noexcept {
        uint64_t k = static_cast<uint64_t>(y.size() & 0xFF) << 56;
        const size_t n = std::min<size_t>(y.size(), 7);
        for (size_t i = 0; i < n; ++i)
            k |= static_cast<uint64_t>(y[i] & 0xFFFF) << (i * 8);
        ykey = k;
    }
};

[[nodiscard]] inline uint64_t make_ykey(const std::vector<int>& y) noexcept {
    uint64_t k = static_cast<uint64_t>(y.size() & 0xFF) << 56;
    const size_t n = std::min<size_t>(y.size(), 7);
    for (size_t i = 0; i < n; ++i)
        k |= static_cast<uint64_t>(y[i] & 0xFFFF) << (i * 8);
    return k;
}

} // namespace omegay::core
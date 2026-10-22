#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

namespace omegay::core {

// 统一的 ykey 计算：refresh_key() 与 make_ykey() 共用同一实现，
// 保证两者的结果始终一致。
[[nodiscard]] inline uint64_t make_ykey(const std::vector<int>& y) noexcept;

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

    void refresh_key() noexcept { ykey = make_ykey(y); }
};

// 64 位 FNV-1a 哈希：
// 先以 y.size() 与 FNV 偏移基数异或作为初始值，再逐个元素
// （取 uint32 位模式，与负数、字节序无关）混入并乘 FNV 素数。
// 确定性、可移植、noexcept；仅存在 64 位空间内的理论碰撞，
// 不再有旧实现的截断、重叠与长度字节碰撞问题。
[[nodiscard]] inline uint64_t make_ykey(const std::vector<int>& y) noexcept {
    constexpr uint64_t kOffsetBasis = 14695981039346656037ULL; // FNV-1a offset basis
    constexpr uint64_t kPrime       = 1099511628211ULL;        // FNV-1a prime
    uint64_t k = kOffsetBasis ^ static_cast<uint64_t>(y.size());
    for (const int v : y)
        k = (k ^ static_cast<uint32_t>(v)) * kPrime;
    return k;
}

} // namespace omegay::core

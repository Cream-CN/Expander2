#pragma once
#include "entry.hpp"
#include <memory>
#include <vector>

namespace omegay::core {

class EntryArena {
public:
    Entry* make(int v = 0, int x = 0, std::vector<int> y = {}) {
        nodes_.push_back(std::make_unique<Entry>(v, x, std::move(y)));
        return nodes_.back().get();
    }
    Entry* make_empty() {
        nodes_.push_back(std::make_unique<Entry>());
        return nodes_.back().get();
    }
    void clear() noexcept { nodes_.clear(); }
    [[nodiscard]] size_t size() const noexcept { return nodes_.size(); }
private:
    std::vector<std::unique_ptr<Entry>> nodes_;
};

} // namespace omegay::core
#pragma once

#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

struct SrdNotation {
    static constexpr const char* kName = "SRD";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term);
    [[nodiscard]] static std::string suffix() { return {}; }
};

namespace srd_detail {

using Expr = std::vector<std::vector<int>>;

inline constexpr int kMaxColumns = 8192;
inline constexpr int kMaxCells = 250000;

struct Budget {
    int work = 0;
    void tick() {
        if (++work > kMaxCells) throw std::runtime_error("计算预算不足");
    }
};

[[nodiscard]] inline std::string plain(const Expr& g) {
    if (g.empty()) return "0";
    std::string out;
    for (const auto& col : g) {
        out.push_back('[');
        for (std::size_t i = 0; i < col.size(); ++i) {
            if (i) out.push_back(',');
            out += std::to_string(col[i]);
        }
        out.push_back(']');
    }
    return out;
}

[[nodiscard]] inline int natural_int(int v, int max) {
    if (v < 0) throw std::runtime_error("需要非负整数");
    if (v > max) throw std::runtime_error("输入过大");
    return v;
}
inline void check(const Expr& g) {
    if (g.size() > kMaxColumns) throw std::runtime_error("列数超出预算");
    int cells = 0;
    for (std::size_t j = 0; j < g.size(); ++j) {
        const auto& col = g[j];
        if ((cells += static_cast<int>(col.size())) > kMaxCells)
            throw std::runtime_error("单元数超出预算");
        for (std::size_t row = 0; row < col.size(); ++row) {
            int p = col[row];
            if (p < 0) throw std::runtime_error("需要非负整数");
            if (p >= static_cast<int>(j) + 1 || p < static_cast<int>(col.size()) - 1 ||
                (row > 0 && col[row - 1] < p))
                throw std::runtime_error("父列表须非增，且满足 高度−1≤父列<子列");
        }
    }
}

[[nodiscard]] inline const std::vector<int>& parent_column(const Expr& g, int p) {
    static const std::vector<int> empty;
    return p ? g[p - 1] : empty;
}

[[nodiscard]] inline Expr fs(const Expr& graph, int n, Budget& tick) {
    if (n < 0) throw std::runtime_error("需要非负整数");
    if (graph.empty() || n == 0 || graph.back().empty()) {
        Expr r = graph;
        if (!r.empty()) r.pop_back();
        return r;
    }
    Expr result = graph;
    const int span = static_cast<int>(graph.size()) - graph.back().back();
    if (static_cast<int>(graph.size()) - 1 + n * span > kMaxColumns)
        throw std::runtime_error("展开列数超出预算");
    int cells = 0;
    for (const auto& c : graph) cells += static_cast<int>(c.size());
    for (int block = 0; block < n; ++block) {
        tick.tick();
        const auto& column = result.back();
        const int parent = column.back();
        std::vector<Expr::value_type> copies;
        for (std::size_t s = static_cast<std::size_t>(parent); s < result.size(); ++s) {
            tick.tick();
            const auto& source = result[s];
            int size = static_cast<int>(source.size()) +
                       (source.size() > static_cast<std::size_t>(parent) ? span : 0);
            if ((cells += size) > kMaxCells) throw std::runtime_error("单元数超出预算");
            std::vector<int> moved;
            moved.reserve(source.size());
            for (int p : source) moved.push_back(p >= parent ? p + span : p);
            std::vector<int> col;
            for (std::size_t i = 0; i < static_cast<std::size_t>(parent) && i < moved.size(); ++i)
                col.push_back(moved[i]);
            if (moved.size() > static_cast<std::size_t>(parent))
                col.insert(col.end(), static_cast<std::size_t>(span), moved[parent]);
            for (std::size_t i = static_cast<std::size_t>(parent); i < moved.size(); ++i)
                col.push_back(moved[i]);
            copies.push_back(std::move(col));
        }
        std::vector<int> seam;
        for (std::size_t i = 0; i + 1 < column.size(); ++i) seam.push_back(column[i]);
        const auto& pc = parent_column(result, parent);
        for (std::size_t i = column.size() - 1; i < pc.size(); ++i) seam.push_back(pc[i]);
        cells += static_cast<int>(seam.size()) - static_cast<int>(column.size());
        if (cells > kMaxCells) throw std::runtime_error("单元数超出预算");
        result.back() = std::move(seam);
        for (auto& col : copies) result.push_back(std::move(col));
    }
    result.pop_back();
    return result;
}

[[nodiscard]] inline Expr parse(std::string_view raw) {
    std::string s;
    s.reserve(raw.size());
    for (char c : raw)
        if (!std::isspace(static_cast<unsigned char>(c))) s.push_back(c);

    // 路由：Limit / Limit of SRD[PD]? / RPD0、SRD、SRPD、Top、Ω，后接若干 [n]
    {
        std::size_t i = 0;
        auto eat = [&](const char* w) -> bool {
            std::size_t k = 0;
            while (w[k]) {
                if (i + k >= s.size() ||
                    std::tolower(static_cast<unsigned char>(s[i + k])) !=
                        std::tolower(static_cast<unsigned char>(w[k])))
                    return false;
                ++k;
            }
            i += k;
            return true;
        };
        bool route = false;
        if (eat("Limit")) {
            route = true;
            if (eat("of")) {
                if (!eat("SRD")) if (!eat("SRPD")) eat("RPD0");
            }
        } else if (eat("SRD") || eat("SRPD") || eat("Top") || eat("Ω")) {
            route = true;
        }
        if (route) {
            Expr result{{0}};
            int steps = 0;
            while (i < s.size() && s[i] == '[' && steps < 128) {
                ++i;
                int v = 0;
                bool any = false;
                while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
                    v = v * 10 + (s[i] - '0');
                    ++i;
                    any = true;
                }
                if (!any || i >= s.size() || s[i] != ']')
                    throw std::runtime_error("路由展开格式错误");
                ++i;
                Budget tick;
                result = fs(result, v, tick);
                ++steps;
            }
            if (i != s.size()) throw std::runtime_error("表达式后有未识别内容");
            return result;
        }
    }

    if (s.empty() || s == "∅") return {};

    // 纯数字：n 个空列
    bool allDigits = !s.empty();
    for (char c : s)
        if (c < '0' || c > '9') allDigits = false;
    if (allDigits) {
        int n = 0;
        for (char c : s) n = n * 10 + (c - '0');
        n = natural_int(n, kMaxColumns);
        return Expr(static_cast<std::size_t>(n));
    }

    Expr out;
    std::size_t i = 0;
    while (i < s.size()) {
        if (s[i] != '[') throw std::runtime_error("请输入父项列表，如 [][1][2,2]");
        ++i;
        std::vector<int> col;
        while (i < s.size() && s[i] != ']') {
            int v = 0;
            bool any = false;
            while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
                v = v * 10 + (s[i] - '0');
                ++i;
                any = true;
            }
            if (any) col.push_back(v);
            if (i < s.size() && s[i] == ',') {
                ++i;
                continue;
            }
            break;
        }
        if (i >= s.size() || s[i] != ']')
            throw std::runtime_error("请输入父项列表，如 [][1][2,2]");
        ++i;
        out.push_back(std::move(col));
    }
    check(out);
    return out;
}

} // namespace srd_detail

[[nodiscard]] inline std::string SrdNotation::expand_text(std::string_view text, int term) {
    try {
        auto g = srd_detail::parse(text);
        srd_detail::Budget tick;
        auto r = srd_detail::fs(g, term, tick);
        return srd_detail::plain(r);
    } catch (const std::exception& e) {
        return e.what();
    }
}

} // namespace omegay::notation

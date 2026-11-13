#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

struct RpdNotation {
    static constexpr const char* kName = "RPD";
    [[nodiscard]] static std::string expand_text(std::string_view text, int term);
    [[nodiscard]] static std::string suffix() { return {}; }
};
struct ArdNotation {
    static constexpr const char* kName = "ARD";
    [[nodiscard]] static std::string expand_text(std::string_view text, int term);
    [[nodiscard]] static std::string suffix() { return {}; }
};
struct Ard2Notation {
    static constexpr const char* kName = "ARD2";
    [[nodiscard]] static std::string expand_text(std::string_view text, int term);
    [[nodiscard]] static std::string suffix() { return {}; }
};

namespace graph_detail {

struct Edge {
    int k, p, q;
};
enum class Kind { RPD, ARD, ARD2 };

struct Graph {
    bool isLimit = false;
    std::vector<std::vector<Edge>> cols;
};

inline constexpr int kWidth = 8192;
inline constexpr int kCells = 250000;

struct Budget {
    int work = 0;
    void tick() {
        if (++work > kCells * 4) throw std::runtime_error("计算预算不足");
    }
};

[[nodiscard]] inline int pairCompare(const Edge& a, const Edge& b) {
    if (a.k != b.k) return a.k < b.k ? -1 : 1;
    if (a.q != b.q) return a.q < b.q ? -1 : 1;
    return 0;
}
[[nodiscard]] inline int edgeCompare(const Edge& a, const Edge& b) {
    if (a.p != b.p) return a.p < b.p ? -1 : 1;
    return pairCompare(a, b);
}

[[nodiscard]] inline std::vector<Edge> skyline(const std::vector<Edge>& col) {
    std::map<int, Edge> best;
    for (const auto& e : col) {
        auto it = best.find(e.p);
        if (it == best.end() || pairCompare(e, it->second) > 0) best[e.p] = e;
    }
    std::vector<Edge> byP;
    for (auto& [p, e] : best) byP.push_back(e);
    std::sort(byP.begin(), byP.end(), [](const Edge& a, const Edge& b) { return a.p > b.p; });
    std::vector<Edge> result;
    const Edge* high = nullptr;
    for (const auto& e : byP) {
        if (!high || pairCompare(e, *high) > 0) {
            result.push_back(e);
            high = &result.back();
        }
    }
    return result;
}

[[nodiscard]] inline std::vector<std::vector<Edge>> normalize(Kind kind, std::vector<std::vector<Edge>> columns, Budget& tick) {
    if (static_cast<int>(columns.size()) > kWidth) throw std::runtime_error("列数超出预算");
    int cells = 0;
    for (std::size_t j = 0; j < columns.size(); ++j) {
        auto& col = columns[j];
        std::map<std::string, Edge> records;
        for (const auto& e : col) {
            tick.tick();
            if (e.k < 0 || e.p < 0 || e.q < 0) throw std::runtime_error("需要非负整数");
            bool bad = e.p >= static_cast<int>(j);
            if (!bad) {
                if (kind == Kind::ARD2) bad = (e.k > static_cast<int>(j) || e.q > static_cast<int>(j));
                else bad = (e.q > e.p || (kind == Kind::ARD && e.k > e.p));
            }
            if (bad) throw std::runtime_error("父列、层标或根标超出本记号允许的范围");
            if (kind != Kind::RPD) continue;
            if (e.q >= kCells) throw std::runtime_error("计算预算不足");
            for (int q = 0; q <= e.q; ++q) {
                tick.tick();
                records[std::to_string(e.k) + "," + std::to_string(e.p) + "," + std::to_string(q)] =
                    Edge{e.k, e.p, q};
                if (++cells > kCells) throw std::runtime_error("计算预算不足");
            }
        }
        if (kind == Kind::RPD) {
            std::vector<Edge> rec;
            for (auto& [key, e] : records) rec.push_back(e);
            std::sort(rec.begin(), rec.end(), [](const Edge& a, const Edge& b) {
                return edgeCompare(a, b) > 0;
            });
            col = std::move(rec);
        } else {
            col = skyline(col);
            if ((cells += static_cast<int>(col.size())) > kCells)
                throw std::runtime_error("计算预算不足");
        }
    }
    return columns;
}

[[nodiscard]] inline std::vector<Edge> predecessor(Kind kind, const Edge& e, int child) {
    if (e.q) return {Edge{e.k, e.p, e.q - 1}};
    if (e.k) return {Edge{e.k - 1, e.p, kind == Kind::ARD2 ? child : e.p}};
    return {};
}

[[nodiscard]] inline Graph seed(Kind kind, int index) {
    Graph g;
    int cap = kind == Kind::RPD ? 4096 : kWidth;
    if (index < 0 || index > cap) throw std::runtime_error("输入过大");
    if (kind == Kind::RPD) {
        g.cols.resize(2);
        g.cols[0] = {};
        for (int k = index; k >= 0; --k) g.cols[1].push_back(Edge{k, 0, 0});
        return g;
    }
    g.cols.resize(static_cast<std::size_t>(index));
    for (int j = 1; j < index; ++j) {
        int v = j - (kind == Kind::ARD ? 1 : 0);
        g.cols[j] = {Edge{v, j - 1, v}};
    }
    return g;
}

[[nodiscard]] inline Graph fs(Kind kind, const Graph& graph, int n, Budget& tick) {
    if (n < 0) throw std::runtime_error("需要非负整数");
    if (graph.isLimit) return seed(kind, n);
    const int x = static_cast<int>(graph.cols.size()) - 1;
    if (x < 0) return {};
    if (n == 0 || graph.cols[x].empty()) {
        Graph r;
        r.cols.assign(graph.cols.begin(), graph.cols.begin() + x);
        return r;
    }
    const auto& last = graph.cols[x];
    Edge control{};
    if (kind == Kind::RPD) {
        auto sorted = last;
        std::sort(sorted.begin(), sorted.end(), [](const Edge& a, const Edge& b) {
            int c = pairCompare(a, b);
        return c != 0 ? c < 0 : a.p < b.p;
        });
        control = sorted.back();
    } else {
        control = last.back();
    }
    const int c = control.p;
    const int width = x - c;
    if (x + n * width > kWidth) throw std::runtime_error("展开列数超出预算");
    std::vector<std::vector<Edge>> result(graph.cols.begin(), graph.cols.begin() + x);
    int copied = 0;
    for (int v = 0; v < n; ++v) {
        tick.tick();
        auto phi = [&](int i, int block) { return i < c ? i : i + block * width; };
        auto move = [&](const Edge& e, int block) -> Edge {
            tick.tick();
            if (++copied > kCells) throw std::runtime_error("计算预算不足");
            if (kind == Kind::RPD) return Edge{e.k, phi(e.p, block), phi(e.q, block)};
            return Edge{phi(e.k, block), phi(e.p, block), phi(e.q, block)};
        };
        std::vector<Edge> seam;
        if (kind == Kind::RPD) {
            int cutRoot = phi(control.q, v);
            for (const auto& e : last) {
                Edge shifted = move(e, v);
                int mx;
                if (e.k < control.k) mx = shifted.q;
                else if (e.k == control.k) mx = std::min(shifted.q, cutRoot - 1);
                else mx = -1;
                if (mx >= 0) seam.push_back(Edge{shifted.k, shifted.p, mx});
            }
        } else {
            for (std::size_t i = 0; i + 1 < last.size(); ++i) seam.push_back(move(last[i], v));
            auto pred = predecessor(kind, move(control, v), x + v * width);
            seam.insert(seam.end(), pred.begin(), pred.end());
        }
        for (const auto& e : graph.cols[c]) seam.push_back(move(e, v + 1));
        result.push_back(std::move(seam));
        for (int j = c + 1; j < x; ++j) {
            std::vector<Edge> col;
            col.reserve(graph.cols[j].size());
            for (const auto& e : graph.cols[j]) col.push_back(move(e, v + 1));
            result.push_back(std::move(col));
        }
    }
    Graph r;
    r.cols = normalize(kind, std::move(result), tick);
    return r;
}

[[nodiscard]] inline std::string plain(const Graph& g) {
    if (g.isLimit) return "Limit";
    if (g.cols.empty()) return "0";
    std::string out;
    for (const auto& col : g.cols) {
        out.push_back('[');
        for (std::size_t i = 0; i < col.size(); ++i) {
            if (i) out.push_back(',');
            out += "(" + std::to_string(col[i].k) + "," + std::to_string(col[i].p) + "," +
                   std::to_string(col[i].q) + ")";
        }
        out.push_back(']');
    }
    return out;
}

[[nodiscard]] inline Graph parse(Kind kind, std::string_view raw) {
    std::string s;
    s.reserve(raw.size());
    for (char ch : raw)
        if (!std::isspace(static_cast<unsigned char>(ch))) s.push_back(ch);

    // 路由：Limit / Limit of (RPD|ARD|ARD2|Anchored-Rows) / Ω / Top / A<d>，后接 [n]*
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
        Graph g;
        if (eat("Limit")) {
            route = true;
            if (eat("of")) {
                if (!eat("RPD"))
                    if (!eat("ARD2")) if (!eat("ARD")) eat("Anchored-Rows");
            }
            g.isLimit = true;
        } else if (eat("Ω") || eat("Top")) {
            route = true;
            g.isLimit = true;
        } else if (eat("A")) {
            std::size_t b = i;
            while (i < s.size() && s[i] >= '0' && s[i] <= '9') ++i;
            if (i > b) {
                route = true;
                g = seed(kind, std::stoi(s.substr(b, i - b)));
            }
        }
        if (route) {
            int steps = 0;
            while (i < s.size() && s[i] == '[' && steps < 128) {
                ++i;
                int v = 0;
                while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
                    v = v * 10 + (s[i] - '0');
                    ++i;
                }
                if (i >= s.size() || s[i] != ']') throw std::runtime_error("路由展开格式错误");
                ++i;
                Budget tick;
                g = fs(kind, g, v, tick);
                ++steps;
            }
            if (i != s.size()) throw std::runtime_error("表达式后有未识别内容");
            return g;
        }
    }

    if (s.empty() || s == "∅") return {};

    bool allDigits = !s.empty();
    for (char ch : s)
        if (ch < '0' || ch > '9') allDigits = false;
    if (allDigits) {
        int n = 0;
        for (char ch : s) n = n * 10 + (ch - '0');
        if (n < 0 || n > kWidth) throw std::runtime_error("输入过大");
        return Graph{false, std::vector<std::vector<Edge>>(static_cast<std::size_t>(n))};
    }

    std::vector<std::vector<Edge>> columns;
    std::size_t i = 0;
    while (i < s.size()) {
        if (s[i] != '[') throw std::runtime_error("请输入 [(层,父列,根)] 列表或 Limit");
        ++i;
        std::vector<Edge> col;
        while (i < s.size() && s[i] != ']') {
            if (s[i] == '(') {
                ++i;
                int v[3] = {0, 0, 0};
                for (int t = 0; t < 3; ++t) {
                    bool any = false;
                    while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
                        v[t] = v[t] * 10 + (s[i] - '0');
                        ++i;
                        any = true;
                    }
                    if (!any) throw std::runtime_error("请输入 [(层,父列,根)] 列表");
                    if (t < 2) {
                        if (i >= s.size() || s[i] != ',')
                            throw std::runtime_error("请输入 [(层,父列,根)] 列表");
                        ++i;
                    }
                }
                col.push_back(Edge{v[0], v[1], v[2]});
                if (i < s.size() && s[i] == ')') ++i;
            }
            if (i < s.size() && s[i] == ',') {
                ++i;
                continue;
            }
            break;
        }
        if (i >= s.size() || s[i] != ']')
            throw std::runtime_error("请输入 [(层,父列,根)] 列表或 Limit");
        ++i;
        columns.push_back(std::move(col));
    }
    Budget tick;
    Graph r;
    r.cols = normalize(kind, std::move(columns), tick);
    return r;
}

[[nodiscard]] inline std::string expand(Kind kind, std::string_view text, int term) {
    Budget tick;
    auto g = parse(kind, text);
    auto r = fs(kind, g, term, tick);
    return plain(r);
}

} // namespace graph_detail

[[nodiscard]] inline std::string RpdNotation::expand_text(std::string_view text, int term) {
    try {
        return graph_detail::expand(graph_detail::Kind::RPD, text, term);
    } catch (const std::exception& e) {
        return e.what();
    }
}
[[nodiscard]] inline std::string ArdNotation::expand_text(std::string_view text, int term) {
    try {
        return graph_detail::expand(graph_detail::Kind::ARD, text, term);
    } catch (const std::exception& e) {
        return e.what();
    }
}
[[nodiscard]] inline std::string Ard2Notation::expand_text(std::string_view text, int term) {
    try {
        return graph_detail::expand(graph_detail::Kind::ARD2, text, term);
    } catch (const std::exception& e) {
        return e.what();
    }
}

} // namespace omegay::notation

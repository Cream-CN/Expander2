#pragma once

#include <array>
#include <cctype>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace omegay::notation {

struct IpdNotation {
    static constexpr const char* kName = "IPD";
    [[nodiscard]] static std::string expand_text(std::string_view text, int term);
    [[nodiscard]] static std::string suffix() { return {}; }
};

namespace ipd_detail {

inline constexpr int CAP = -1;
inline constexpr int kColumns = 4096;
inline constexpr int kEdges = 100000;
inline constexpr int kLevel = 1000000;
inline constexpr int kArity = 8192;
inline constexpr int kDepth = 160;
inline constexpr int kNodes = 60000;
inline constexpr int kSlots = 800000;
inline constexpr int kMaxWork = 1500000;

struct Edge {
    int p, d, t;
};
struct Node {
    int d, h;
    std::vector<int> children;
};

struct Graph {
    bool isLimit = false;
    std::vector<std::vector<Edge>> cols;
    std::vector<std::string> texts;
    std::string key;
};

class Core {
public:
    std::vector<Node> nodes; // nodes[0] 占位
    std::map<std::string, int> indexMap;
    std::map<std::string, int> cmpCache;
    std::map<std::string, int> lowerCache;
    int work = 0;
    int depth = 0;
    int slots = 0;

    Core() { nodes.push_back(Node{0, 0, {}}); }

    void tick() {
        if (++work > kMaxWork) throw std::runtime_error("计算预算不足");
    }
    void enter() {
        tick();
        if (++depth > kDepth) throw std::runtime_error("递归深度超出预算");
    }
    void leave() { --depth; }

    int make(int d, int h, std::vector<int> children) {
        tick();
        if (static_cast<int>(children.size()) > kArity)
            throw std::runtime_error("节点元数超出预算");
        std::string key = std::to_string(d) + ":" + std::to_string(h) + ":";
        for (std::size_t i = 0; i < children.size(); ++i) {
            if (i) key.push_back(',');
            key += std::to_string(children[i]);
        }
        auto it = indexMap.find(key);
        if (it != indexMap.end()) return it->second;
        if (static_cast<int>(nodes.size()) >= kNodes ||
            slots + static_cast<int>(children.size()) + 1 > kSlots)
            throw std::runtime_error("共享树容量超出预算");
        int id = static_cast<int>(nodes.size());
        nodes.push_back(Node{d, h, std::move(children)});
        indexMap.emplace(std::move(key), id);
        slots += static_cast<int>(nodes.back().children.size()) + 1;
        return id;
    }

    int seed(int d, int n, int p, int j) {
        if (d == 0) return n == 0 ? p : j;
        if (n == 0) return 0;
        return make(d, CAP, std::vector<int>(static_cast<std::size_t>(n) - 1, 0));
    }

    int cmp(int d, int a, int c) {
        tick();
        if (a == c) return 0;
        if (d == 0 || a == 0 || c == 0) return (a > c) - (a < c);
        std::string key = std::to_string(d) + ":" + std::to_string(a) + ":" + std::to_string(c);
        auto it = cmpCache.find(key);
        if (it != cmpCache.end()) return it->second;
        enter();
        int result;
        const Node& x = nodes[a];
        const Node& y = nodes[c];
        bool ge = false;
        for (int t : x.children)
            if (cmp(d, t, c) >= 0) { ge = true; break; }
        if (ge) {
            result = 1;
        } else {
            bool le = false;
            for (int t : y.children)
                if (cmp(d, t, a) >= 0) { le = true; break; }
            if (le) {
                result = -1;
            } else {
                if (x.h == y.h) result = 0;
                else if (x.h == CAP) result = 1;
                else if (y.h == CAP) result = -1;
                else result = cmp(d - 1, x.h, y.h);
                if (!result)
                    result = (static_cast<int>(x.children.size()) >
                              static_cast<int>(y.children.size())) -
                             (static_cast<int>(x.children.size()) <
                              static_cast<int>(y.children.size()));
                for (std::size_t i = 0; !result && i < x.children.size(); ++i)
                    result = cmp(d, x.children[i], y.children[i]);
                if (!result) throw std::runtime_error("共享树规范化内部错误");
            }
        }
        leave();
        if (static_cast<int>(cmpCache.size()) < 180000) cmpCache.emplace(std::move(key), result);
        return result;
    }

    int profile(const Edge& a, const Edge& c) {
        if (a.d != c.d) return (a.d > c.d) - (a.d < c.d);
        return cmp(a.d, a.t, c.t);
    }

    int maximum(int d, const std::vector<int>& values) {
        int m = 0;
        for (int v : values)
            if (cmp(d, v, m) > 0) m = v;
        return m;
    }

    std::optional<int> lower(int d, int t, int p, int j, int n) {
        tick();
        if (d == 0) {
            if (t == 0) return std::nullopt;
            return t == j ? std::optional<int>(p) : std::optional<int>(t - 1);
        }
        if (t == 0) return std::nullopt;
        std::string key = std::to_string(d) + ":" + std::to_string(t) + ":" +
                          std::to_string(p) + ":" + std::to_string(j) + ":" + std::to_string(n);
        auto it = lowerCache.find(key);
        if (it != lowerCache.end()) return it->second;
        enter();
        const Node& nd = nodes[t];
        const int k = static_cast<int>(nd.children.size());
        std::optional<int> smaller;
        if (nd.h == CAP) smaller = seed(d - 1, n, p, j);
        else smaller = lower(d - 1, nd.h, p, j, n);
        int m = 0;
        if (k) {
            std::vector<int> vals;
            vals.push_back(make(d, nd.h, {}));
            for (int ch : nd.children) vals.push_back(ch);
            m = maximum(d, vals);
        }
        std::vector<std::pair<int, int>> heads;
        if (k) heads.push_back({nd.h, k - 1});
        if (smaller) heads.push_back({*smaller, n});
        std::vector<int> positions;
        for (int i = k - 2; i >= 0; --i)
            if (nd.children[i] != 0) { positions.push_back(i); break; }
        if (k && nd.children[k - 1] != 0) positions.push_back(k - 1);
        std::vector<std::pair<int, int>> lowered;
        for (int i : positions) {
            auto low = lower(d, nd.children[i], p, j, n);
            lowered.push_back({i, low.value_or(0)});
        }
        for (int round = 0; round <= n; ++round) {
            tick();
            std::vector<int> candidates;
            candidates.push_back(m);
            for (auto& [head, arity] : heads)
                candidates.push_back(make(d, head, std::vector<int>(static_cast<std::size_t>(arity), m)));
            for (auto& [i, low] : lowered) {
                std::vector<int> ch(nd.children.begin(), nd.children.begin() + i);
                ch.push_back(low);
                for (int z = 0; z < k - i - 1; ++z) ch.push_back(m);
                candidates.push_back(make(d, nd.h, std::move(ch)));
            }
            m = maximum(d, candidates);
        }
        leave();
        if (static_cast<int>(lowerCache.size()) < 40000) lowerCache.emplace(std::move(key), m);
        return m;
    }

    std::optional<Edge> lowerEdge(const Edge& e, int j, int n) {
        auto low = lower(e.d, e.t, e.p, j, n);
        if (low) return Edge{e.p, e.d, *low};
        if (e.d) return Edge{e.p, e.d - 1, seed(e.d - 1, n, e.p, j)};
        return std::nullopt;
    }

    std::vector<Edge> normalize(const std::vector<Edge>& column) {
        std::map<int, Edge> values;
        for (const auto& e : column) {
            tick();
            auto it = values.find(e.p);
            if (it == values.end() || profile(e, it->second) > 0) values[e.p] = e;
        }
        std::vector<Edge> out;
        for (auto it = values.rbegin(); it != values.rend(); ++it) out.push_back(it->second);
        return out;
    }

    int edgeOrder(const Edge& e, const Edge* best) {
        if (!best) return 1;
        int c = profile(e, *best);
        if (c) return c;
        return (e.p > best->p) - (e.p < best->p);
    }
    const Edge* controller(const std::vector<Edge>& column) {
        const Edge* best = nullptr;
        for (const auto& e : column)
            if (edgeOrder(e, best) > 0) best = &e;
        return best;
    }

    std::string termText(int d, int t, int j, std::map<std::string, std::string>& cache) {
        tick();
        if (d == 0) return t == j ? "*" : std::to_string(t);
        if (t == 0) return ".";
        std::string key = std::to_string(d) + ":" + std::to_string(t) + ":" + std::to_string(j);
        auto it = cache.find(key);
        if (it != cache.end()) return it->second;
        enter();
        const Node& nd = nodes[t];
        std::string h;
        if (nd.h == CAP) h = "^";
        else {
            h = termText(d - 1, nd.h, j, cache);
            if (nd.h != CAP && d > 1) h = "{" + h + "}";
        }
        std::vector<std::string> chunks;
        for (std::size_t i = 0; i < nd.children.size();) {
            std::size_t end = i + 1;
            while (end < nd.children.size() && nd.children[end] == nd.children[i]) {
                tick();
                ++end;
            }
            std::string chunk = termText(d, nd.children[i], j, cache);
            if (end - i > 1) chunk += "~" + std::to_string(end - i);
            chunks.push_back(std::move(chunk));
            i = end;
        }
        std::string result = h;
        if (!chunks.empty()) {
            result.push_back('(');
            for (std::size_t i = 0; i < chunks.size(); ++i) {
                if (i) result.push_back(',');
                result += chunks[i];
            }
            result.push_back(')');
        }
        leave();
        cache.emplace(std::move(key), result);
        return result;
    }

    int blockVisit(int d, int t, const std::function<int(int)>& phi,
                   std::map<std::string, int>& memo) {
        tick();
        if (d == 0) return phi(t);
        if (t == 0) return 0;
        std::string key = std::to_string(d) + ":" + std::to_string(t);
        auto it = memo.find(key);
        if (it != memo.end()) return it->second;
        enter();
        const Node& nd = nodes[t];
        int h = nd.h == CAP ? CAP : blockVisit(d - 1, nd.h, phi, memo);
        std::vector<int> children;
        children.reserve(nd.children.size());
        for (int c : nd.children) children.push_back(blockVisit(d, c, phi, memo));
        int result = make(d, h, std::move(children));
        leave();
        memo.emplace(std::move(key), result);
        return result;
    }

    Graph makeGraph(const std::vector<std::vector<Edge>>& columns) {        if (static_cast<int>(columns.size()) > kColumns)
            throw std::runtime_error("列数超出预算");
        Graph g;
        int edges = 0, chars = 0;
        std::map<std::string, std::string> cache;
        for (std::size_t j = 0; j < columns.size(); ++j) {
            auto col = normalize(columns[j]);
            edges += static_cast<int>(col.size());
            if (edges > kEdges) throw std::runtime_error("边数超出预算");
            std::string text = "[";
            for (std::size_t i = 0; i < col.size(); ++i) {
                if (i) text.push_back(';');
                text += std::to_string(col[i].p) + ":" + std::to_string(col[i].d) + "/" +
                        termText(col[i].d, col[i].t, static_cast<int>(j), cache);
            }
            text.push_back(']');
            chars += static_cast<int>(text.size());
            if (chars > 1000000) throw std::runtime_error("表达式文本过长");
            g.cols.push_back(std::move(col));
            g.texts.push_back(std::move(text));
        }
        for (const auto& t : g.texts) g.key += t;
        if (g.key.empty()) g.key = "0";
        return g;
    }

    Graph seedGraph(int index) {
        if (index < 0 || index > kLevel) throw std::runtime_error("层号超出预算");
        return makeGraph({{}, {{0, index, 0}}});
    }

    Graph expandGraph(Graph g, int index) {
        if (g.isLimit) return seedGraph(index);
        if (g.cols.empty()) return g;
        if (index == 0 || g.cols.back().empty()) {
            std::vector<std::vector<Edge>> trimmed(g.cols.begin(), g.cols.end() - 1);
            return makeGraph(trimmed);
        }
        const int x = static_cast<int>(g.cols.size()) - 1;
        const int cut = controller(g.cols[x])->p;
        const int width = x - cut;
        if (x + index * width > kColumns) throw std::runtime_error("展开列数超出预算");
        std::vector<std::vector<Edge>> result(static_cast<std::size_t>(x + index * width));
        for (int j = 0; j < cut; ++j) result[j] = g.cols[j];
        for (int block = 0; block <= index; ++block) {
            auto phi = [&](int k) { return k < cut ? k : k + block * width; };
            std::map<std::string, int> memo;
            auto move = [&](const Edge& e) {
                return Edge{phi(e.p), e.d, blockVisit(e.d, e.t, phi, memo)};
            };
            for (int j = cut; j < x; ++j)
                for (const auto& e : g.cols[j]) result[phi(j)].push_back(move(e));
            if (block == index) break;
            for (const auto& e : g.cols[x]) {
                Edge moved = move(e);
                if (e.p == cut) {
                    auto low = lowerEdge(moved, phi(x), block);
                    if (!low) continue;
                    moved = *low;
                }
                result[phi(x)].push_back(moved);
            }
        }
        return makeGraph(result);
    }

    // ---- 解析 ----
    Graph parse(std::string_view raw) {
        tick();
        std::string s;
        s.reserve(raw.size());
        for (char c : raw)
            if (!std::isspace(static_cast<unsigned char>(c))) s.push_back(c);
        if (s.size() > 1000000) throw std::runtime_error("输入文本过长");
        std::size_t i = 0;

        auto startsCi = [&](const char* w) -> bool {
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
        auto integer = [&](int max = kLevel) -> int {
            std::size_t b = i;
            while (i < s.size() && s[i] >= '0' && s[i] <= '9') ++i;
            if (i == b) throw std::runtime_error("需要非负整数");
            if (i - b > 16) throw std::runtime_error("整数过长");
            int v = 0;
            for (std::size_t k = b; k < i; ++k) v = v * 10 + (s[k] - '0');
            if (v > max) throw std::runtime_error("整数范围超出预算");
            return v;
        };
        auto take = [&](char c) {
            if (i >= s.size() || s[i] != c) throw std::runtime_error("格式错误");
            ++i;
        };
        auto term = [&](auto& self, int d, int p, int j) -> int {
            enter();
            int result;
            if (d == 0) {
                if (i < s.size() && s[i] == '*') {
                    ++i;
                    result = j;
                } else {
                    int q = integer(kColumns);
                    if (!(q <= p || q == j))
                        throw std::runtime_error("引用必须是父列以前的 ROOT 或当前 SELF");
                    result = q;
                }
            } else if (i < s.size() && s[i] == '.') {
                ++i;
                result = 0;
            } else {
                int h;
                if (i < s.size() && s[i] == '^') {
                    ++i;
                    h = CAP;
                } else if (d > 1) {
                    take('{');
                    h = self(self, d - 1, p, j);
                    take('}');
                } else {
                    h = self(self, 0, p, j);
                }
                std::vector<int> children;
                if (i < s.size() && s[i] == '(') {
                    ++i;
                    if (i < s.size() && s[i] != ')') {
                        for (;;) {
                            int child = self(self, d, p, j);
                            int repeat = 1;
                            if (i < s.size() && s[i] == '~') {
                                ++i;
                                repeat = integer(kArity);
                                if (repeat == 0) throw std::runtime_error("重复次数不能为零");
                            }
                            if (static_cast<int>(children.size()) + repeat > kArity)
                                throw std::runtime_error("节点元数超出预算");
                            for (int k = 0; k < repeat; ++k) {
                                tick();
                                children.push_back(child);
                            }
                            if (i >= s.size() || s[i] != ',') break;
                            ++i;
                        }
                    }
                    take(')');
                }
                result = make(d, h, std::move(children));
            }
            leave();
            return result;
        };

        Graph g;
        bool aliasOrTop = false;
        bool isDigit = !s.empty() && s[0] >= '0' && s[0] <= '9';
        if (startsCi("LimitofIPD")) {
            g.isLimit = true;
            aliasOrTop = true;
        } else if (startsCi("Limit")) {
            g.isLimit = true;
            aliasOrTop = true;
        } else if (startsCi("TOP")) {
            g.isLimit = true;
            aliasOrTop = true;
        } else if ((s[0] == 'A' || s[0] == 'a' || s[0] == 'T' || s[0] == 't') && !isDigit &&
                   s.size() > 1 && s[1] >= '0' && s[1] <= '9') {
            ++i;
            int v = integer(kLevel);
            g = seedGraph(v);
            aliasOrTop = true;
        } else if (isDigit) {
            int n = integer(kColumns);
            g = makeGraph(std::vector<std::vector<Edge>>(static_cast<std::size_t>(n)));
        } else {
            std::vector<std::vector<Edge>> cols;
            while (i < s.size() && s[i] == '[') {
                if (static_cast<int>(cols.size()) >= kColumns)
                    throw std::runtime_error("列数超出预算");
                ++i;
                std::vector<Edge> column;
                int j = static_cast<int>(cols.size());
                if (i < s.size() && s[i] != ']') {
                    for (;;) {
                        int p = integer(kColumns);
                        if (p >= j) throw std::runtime_error("父列必须严格早于本列");
                        take(':');
                        int d = integer(kLevel);
                        take('/');
                        column.push_back(Edge{p, d, term(term, d, p, j)});
                        if (i >= s.size() || s[i] != ';') break;
                        ++i;
                    }
                }
                take(']');
                cols.push_back(std::move(column));
            }
            if (cols.empty()) throw std::runtime_error("请输入 A0、A1[3]、Limit 或完整列列表");
            g = makeGraph(cols);
        }

        int steps = 0;
        while (aliasOrTop && i < s.size() && s[i] == '[') {
            if (++steps > 256) throw std::runtime_error("展开路径过长");
            ++i;
            int v = integer();
            take(']');
            g = expandGraph(std::move(g), v);
        }
        if (i != s.size()) throw std::runtime_error("表达式后有未识别内容");
        return g;
    }
};

inline std::string expand(std::string_view text, int term) {
    Core core;
    Graph g = core.parse(text);
    Graph r = core.expandGraph(std::move(g), term < 0 ? 0 : term);
    if (r.isLimit) return "Limit";
    return r.key;
}

} // namespace ipd_detail

[[nodiscard]] inline std::string IpdNotation::expand_text(std::string_view text, int term) {
    try {
        return ipd_detail::expand(text, term);
    } catch (const std::exception& e) {
        return e.what();
    }
}

} // namespace omegay::notation

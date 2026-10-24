// Header/notation/up2mn_v1a.hpp
// UP2MN v1A 记号（display 名 "UP2MN v1A"，TS id "up2mn-v1a"）。
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/UPMN/UP2MN-v1a.ts
// （家族/类别：category-upmn-test，credit.upmn）。
// 表达式结构（TS）：与 nMN 相同 —— Sep=int；Vertical=Sep[]；Entry=[value(1-based),Sep]；
// Column=Entry[]；Mountain=Column[]。∞ 用 bool 标志表示（display "Limit"）。
// 复用了 smnshare（n-MN 全套）的显示/解析/vertical/parent/expand。
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "smnshare.hpp"

namespace omegay::notation {

using Up2MnV1aExpr = smnshare::Mountain;

struct UP2MNV1ANotation {
    static constexpr const char* kName = "UP2MN v1A";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/false);
    }
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term) {
        return expand_text_impl(text, term, /*longer=*/true);
    }
    [[nodiscard]] static std::string suffix() { return {}; }

    // parse：∞ 用空 Mountain + infinity 标志；此处以 optional 表示解析结果，
    // "Limit" 特殊处理放在 expand_text_impl。
    [[nodiscard]] static std::optional<Up2MnV1aExpr> parse(std::string_view text) {
        if (text == "Limit" || text == "limit" || text == "LIMIT") {
            return smnshare::Mountain{};
        }
        return smnshare::from_display(text);
    }

    [[nodiscard]] static std::string to_string(const Up2MnV1aExpr& e) {
        return smnshare::mountain_display(e, false);
    }

private:
    using Mountain = smnshare::Mountain;
    using Column = smnshare::Column;
    using Vertical = smnshare::Vertical;

    // TS UP2MN-v1a 的 INFINITY()：[[], [[1,1]], [[2,1]]]。
    [[nodiscard]] static Mountain infinity_value() {
        return Mountain{ Column{}, Column{ smnshare::Entry{ 1, 1 } },
                         Column{ smnshare::Entry{ 2, 1 } } };
    }

    [[nodiscard]] static bool is_infinity(const Mountain& m) {
        return smnshare::mountain_compare(m, infinity_value()) == 0;
    }

    // TS infinity_FS(index) = MN_expand(INFINITY(), index)。
    [[nodiscard]] static Mountain infinity_fs(int index) {
        return smnshare::expand(infinity_value(), index, false);
    }

    [[nodiscard]] static bool has_infinite(const Column& col) {
        return !col.empty() && col.back().sep > 0;
    }

    [[nodiscard]] static int finite_height(const Column& col) {
        return has_infinite(col) ? static_cast<int>(col.size()) - 1
                                 : static_cast<int>(col.size());
    }

    // TS copy_column。
    [[nodiscard]] static Column copy_column(const Column& col, int Ri, int Rj, int offset, bool up, int y_offset) {
        Column result;
        if (col.empty() && Rj == 0 && up) {
            for (int k = 0; k < y_offset; ++k) result.push_back(smnshare::Entry{ Ri + 1 + offset, 0 });
        }
        for (std::size_t j = 0; j < col.size(); ++j) {
            const smnshare::Entry& e = col[j];
            if (static_cast<int>(j) == Rj && up) {
                for (int k = 0; k < y_offset; ++k) result.push_back(smnshare::Entry{ e.value + offset, 0 });
            }
            if (e.value > Ri + 1 || (e.value == Ri + 1 && (up || static_cast<int>(j) < Rj))) {
                result.push_back(smnshare::Entry{ e.value + offset, e.sep });
            } else {
                result.push_back(e);
            }
        }
        return result;
    }

    // TS expand 的自定义分支（末列含无限条目时）。
    [[nodiscard]] static Mountain expand_infinite(const Mountain& m, int index, bool shorter) {
        const int right = static_cast<int>(m.size()) - 1;
        const int top = static_cast<int>(m[static_cast<std::size_t>(right)].size()) - 1;

        std::vector<std::vector<Vertical>> V;
        V.reserve(m.size());
        for (const Column& c : m) V.push_back(smnshare::column_verticals(c));

        const smnshare::Position pr = smnshare::parent(m, V, smnshare::Position{ right, top });
        const int Ri = pr.i, Rj = pr.j;
        const int offset = right - Ri;
        const int y_offset = top == Rj ? 1 : top - Rj;

        std::vector<int> up(m.size(), 0);
        for (int i = Ri + 1; i < right; ++i) {
            const Column& col = m[static_cast<std::size_t>(i)];
            if (!has_infinite(col)) {
                if (finite_height(col) <= Rj + 1) continue;
                const smnshare::Position p = smnshare::parent(m, V, smnshare::Position{ i, Rj });
                up[static_cast<std::size_t>(i)] = up[static_cast<std::size_t>(p.i)];
                continue;
            }
            if (finite_height(col) < Rj) continue;
            if (finite_height(col) > Rj) {
                const smnshare::Position p = smnshare::parent(m, V, smnshare::Position{ i, Rj });
                up[static_cast<std::size_t>(i)] = up[static_cast<std::size_t>(p.i)];
                continue;
            }
            // finite_height == Rj：threshold 比较
            Column threshold_col;
            for (int j = 0; j <= Rj; ++j) threshold_col.push_back(smnshare::Entry{ i, 0 });
            threshold_col.push_back(smnshare::Entry{ Ri, 1 });

            const int X_start = i;
            int X_end = i + 1;
            while (X_end < static_cast<int>(m.size()) &&
                   smnshare::column_compare(m[static_cast<std::size_t>(X_end)], threshold_col) >= 0) {
                ++X_end;
            }
            if (X_end == static_cast<int>(m.size())) {
                up[static_cast<std::size_t>(i)] = 1;
                continue;
            }
            const int Y_end = static_cast<int>(m.size());
            int Y_start = right;
            while (finite_height(m[static_cast<std::size_t>(Y_start)]) > Rj) {
                const smnshare::Position p = smnshare::parent(m, V, smnshare::Position{ Y_start, Rj });
                Y_start = p.i;
            }
            int up_i = -1;  // undefined
            for (int k = 1; k + X_start < X_end && k + Y_start < Y_end; ++k) {
                Column X_col;
                X_col.reserve(m[static_cast<std::size_t>(X_start + k)].size());
                for (const smnshare::Entry& e : m[static_cast<std::size_t>(X_start + k)]) {
                    X_col.push_back(smnshare::Entry{ e.value >= X_start ? e.value + Y_start - X_start : e.value, e.sep });
                }
                const Column& Y_col = m[static_cast<std::size_t>(Y_start + k)];
                const int cmp = smnshare::column_compare(X_col, Y_col);
                if (cmp != 0) {
                    up_i = cmp > 0 ? 1 : 0;
                    break;
                }
            }
            if (up_i < 0) up_i = (X_end - X_start >= Y_end - Y_start) ? 1 : 0;
            up[static_cast<std::size_t>(i)] = up_i;
        }
        up[static_cast<std::size_t>(right)] = 1;

        Mountain result = smnshare::subtract_1(m);
        for (int w = 1; w <= index; ++w) {
            for (int i = Ri + 1; i <= right; ++i) {
                result.push_back(copy_column(result[static_cast<std::size_t>(i)], Ri, Rj,
                                             offset * w, up[static_cast<std::size_t>(i)] != 0, y_offset * w));
            }
        }
        if (shorter) result.pop_back();
        return result;
    }

    // TS expand。
    [[nodiscard]] static Mountain expand(const Mountain& m, int index, bool shorter) {
        if (is_infinity(m)) return infinity_fs(index);
        if (m.empty()) return m;
        const int right = static_cast<int>(m.size()) - 1;
        if (m[static_cast<std::size_t>(right)].empty()) {
            return Mountain(m.begin(), m.end() - 1);
        }
        if (!has_infinite(m[static_cast<std::size_t>(right)])) {
            return smnshare::expand(m, index, shorter);
        }
        return expand_infinite(m, index, shorter);
    }

    static std::string expand_text_impl(std::string_view text, int term, bool longer) {
        if (term < 1) term = 1;
        const bool is_inf = text == "Limit" || text == "limit" || text == "LIMIT";
        auto parsed = parse(text);
        if (!parsed && !is_inf) {
            return "解析失败，请确认你没有使用全角括号，逗号，分号";
        }
        const Mountain& m = is_inf ? infinity_value() : *parsed;
        // 值域校验：value 1-based 作为父列索引（value-1），越界时 TS 在 expand 中抛错。
        if (!is_inf) {
            for (const Column& col : m) {
                for (const smnshare::Entry& en : col) {
                    if (en.value < 1 || en.value > static_cast<int>(m.size())) {
                        return "解析失败，请确认你没有使用全角括号，逗号，分号";
                    }
                }
            }
        }
        return smnshare::mountain_display(expand(m, term, !longer), false);
    }
};

}  // namespace omegay::notation

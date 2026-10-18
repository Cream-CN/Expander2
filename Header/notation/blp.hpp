// Header/notation/blp.hpp
// blp 记号（display 名为 "blp"）。
//
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/DEN/DEN2.ts。
// 表达式是“标记行”序列：每行 (step, 条目[])，条目带标记（display 中以 '*' 表示）。
// 记号极限以 expr.infinity 表示，display 为 "Limit"。
//
// 对外提供文本接口（expand_text / expand_text_alter，分别对应 FS / FS_alter），
// 纯函数、不抛异常。
#pragma once

#include "denshare.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::notation {

struct BlpNotation {
    static constexpr const char* kName = "blp";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term);
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term);
    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::optional<denshare::DenExpr> parse(std::string_view text);
    [[nodiscard]] static std::string to_string(const denshare::DenExpr& expr);
};

namespace blp_detail {

using denshare::DenEntry;
using denshare::DenExpr;
using denshare::DenRow;
using denshare::TextParser;

// ---------------------------------------------------------------------
// 显示与解析
// ---------------------------------------------------------------------

// TS: toShort —— entries.slice(0, -step) 再补最后一个值（step=0 时切片为空）。
[[nodiscard]] inline std::vector<int> to_short_row(const DenRow& row) {
    std::vector<int> out;
    if (row.step > 0 && static_cast<std::size_t>(row.step) < row.entries.size()) {
        const std::size_t end = row.entries.size() - static_cast<std::size_t>(row.step);
        for (std::size_t i = 0; i < end; ++i) out.push_back(row.entries[i].value);
    }
    if (!row.entries.empty()) out.push_back(row.entries.back().value);
    return out;
}

[[nodiscard]] inline std::string to_string(const DenExpr& expr) {
    if (expr.infinity) return "Limit";
    std::string out;
    for (const DenRow& row : expr.rows) {
        out += '(';
        for (std::size_t k = 0; k < row.entries.size(); ++k) {
            if (k > 0) out += ',';
            if (row.entries[k].marked) out += '*';
            out += std::to_string(row.entries[k].value);
        }
        out += ')';
        out += std::to_string(row.step);
    }
    return out;
}

// 解析（TS 的 from_display）：'(' 条目列表 ')' step 反复；
// 括号内不允许为空，非法输入返回 nullopt。
[[nodiscard]] inline std::optional<DenExpr> parse(std::string_view s) {
    TextParser p(s);
    p.skip_spaces();
    if (p.match("Limit")) {
        p.skip_spaces();
        if (!p.eof()) return std::nullopt;
        DenExpr e;
        e.infinity = true;
        return e;
    }

    DenExpr expr;
    while (!p.eof()) {
        p.skip_spaces();
        if (p.eof()) break;
        if (p.peek() != '(') return std::nullopt;
        p.advance(1);
        p.skip_spaces();
        DenRow row;
        if (p.eof() || p.peek() == ')') return std::nullopt;   // 括号内不允许为空
        while (true) {
            bool marked = false;
            if (!p.eof() && p.peek() == '*') { marked = true; p.advance(1); }
            auto v = p.parse_digits();
            if (!v) return std::nullopt;
            row.entries.push_back({ *v, marked });
            p.skip_spaces();
            if (!p.eof() && p.peek() == ',') { p.advance(1); continue; }
            break;
        }
        if (p.eof() || p.peek() != ')') return std::nullopt;
        p.advance(1);
        p.skip_spaces();
        auto step = p.parse_digits();
        if (!step) return std::nullopt;
        row.step = *step;
        expr.rows.push_back(std::move(row));
    }
    return expr;
}

// ---------------------------------------------------------------------
// 比较与极限判定
// ---------------------------------------------------------------------

[[nodiscard]] inline int compare(const DenExpr& a, const DenExpr& b) {
    if (a.infinity || b.infinity) return denshare::bool_compare(a.infinity, b.infinity);
    std::vector<std::vector<int>> sa, sb;
    for (const DenRow& row : a.rows) sa.push_back(to_short_row(row));
    for (const DenRow& row : b.rows) sb.push_back(to_short_row(row));
    return denshare::matrix_compare(sa, sb);
}

[[nodiscard]] inline bool is_limit(const DenExpr& expr) {
    if (expr.infinity) return true;
    if (expr.rows.empty()) return false;
    const DenRow& active = expr.rows.back();
    if (static_cast<std::size_t>(active.step) >= active.entries.size() ||
        active.entries[active.step].value == 0) return false;

    // pleasant_until(expr.slice(value-1, -1), active) === -1
    const int begin = active.entries[active.step].value;
    std::vector<std::vector<int>> rows;
    for (std::size_t i = static_cast<std::size_t>(begin - 1); i + 1 < expr.rows.size(); ++i) {
        rows.push_back(denshare::row_values(expr.rows[i]));
    }
    std::vector<int> tcheck;
    for (std::size_t k = static_cast<std::size_t>(active.step); k < active.entries.size(); ++k) {
        tcheck.push_back(active.entries[k].value);
    }
    return denshare::pleasant_until(rows, tcheck) == -1;
}

// ---------------------------------------------------------------------
// 展开核心
// ---------------------------------------------------------------------

// 删除最后一行（DEN2 无父标号列表，无需其他操作）。
[[nodiscard]] inline DenExpr cut(const DenExpr& raw) {
    DenExpr expr = raw;
    if (!expr.rows.empty()) expr.rows.pop_back();
    return expr;
}

// seqFrom 的产物项：(行, 条目下标)。
struct SeqItem {
    int row;   // 1 起的行下标（或 active 的值）
    int idx;   // 条目下标
};

// TS: seqFrom —— 从 (i, j) 出发沿 parent 链收集 [行, 下标] 对。
[[nodiscard]] inline std::vector<SeqItem> seq_from(const DenExpr& expr, int i, int j) {
    const DenRow& row = expr.rows[i];
    const int val0 = row.entries[j].value;
    const int threshold = (j + row.step < static_cast<int>(row.entries.size()))
        ? row.entries[j + row.step].value : 0;

    std::vector<SeqItem> record;
    record.push_back({ i + 1, j });
    record.push_back({ val0, 0 });
    int v = val0;
    while (v > threshold) {
        if (v - 1 < 0 || v - 1 >= static_cast<int>(expr.rows.size())) break;   // 非法输入保护
        const DenRow& r2 = expr.rows[v - 1];
        const int idx = r2.step;
        record.back().idx = idx;
        v = (idx >= 0 && idx < static_cast<int>(r2.entries.size())) ? r2.entries[idx].value : 0;
        record.push_back({ v, 0 });
    }
    record.pop_back();
    return record;
}

// ap：按 active 行的映射规则复制一行。
[[nodiscard]] inline DenRow apply_row(const DenRow& s, const DenRow& t) {
    const std::vector<int> s_vals = denshare::row_values(s);
    const std::vector<int> t_vals = denshare::row_values(t);
    DenRow out;
    out.step = s.step;
    out.entries.reserve(s.entries.size());
    for (const int x : s_vals) out.entries.push_back({ denshare::ap_value(x, t_vals, t.step), false });
    return out;
}

// copy：复制 [begin-1, end-1) 区间内的行并迁移标记。
[[nodiscard]] inline DenExpr copy(const DenExpr& raw, int flag) {
    const DenRow& active = raw.rows.back();
    DenExpr expr = cut(raw);
    const int begin = active.entries[active.step].value;
    const int end = flag >= 0 ? begin + flag : static_cast<int>(raw.rows.size()) + 1;
    const int offset = static_cast<int>(raw.rows.size()) - begin;
    const int last_active = active.entries.back().value;

    for (int i = begin - 1; i < end - 1; ++i) {
        expr.rows.push_back(apply_row(raw.rows[i], active));
    }
    for (int i = begin - 1; i < end - 1; ++i) {
        const DenRow& row = raw.rows[i];
        DenRow& target = expr.rows[i + offset];
        for (int j = 0; j < static_cast<int>(row.entries.size()); ++j) {
            if (!row.entries[j].marked) continue;
            const std::vector<SeqItem> seq = seq_from(raw, i, j);
            int no_move = -1;
            for (std::size_t k = 0; k < seq.size(); ++k) {
                if (seq[k].row < begin) { no_move = static_cast<int>(k); break; }
            }
            if (no_move == -1) { target.entries[j].marked = true; continue; }
            if (seq[no_move].row < last_active) { target.entries[j].marked = true; continue; }
            const int c = seq[no_move - 1].row + offset;
            if (c - 1 < 0 || c - 1 >= static_cast<int>(expr.rows.size())) continue;
            const DenRow& row_c = expr.rows[c - 1];
            const int b = (seq[no_move - 1].idx >= 0 &&
                seq[no_move - 1].idx < static_cast<int>(row_c.entries.size()))
                ? row_c.entries[seq[no_move - 1].idx].value : 0;
            const int tj = j + target.step - 1;
            const bool lhs = (tj >= 0 && tj < static_cast<int>(target.entries.size()) &&
                target.entries[tj].value <= last_active);
            bool rhs = false;
            for (const DenEntry& e : active.entries) {
                if (e.value == b && e.marked) { rhs = true; break; }
            }
            if (lhs && rhs) target.entries[j].marked = true;
        }
    }
    return expr;
}

// already：稀疏表，行下标 -> 该行压缩时构造的 T（尚未构造的为空）。
using Already = std::vector<std::optional<std::vector<int>>>;

// compTo：对第 r 行做“上向压缩”。
[[nodiscard]] inline DenExpr comp_to(const DenExpr& raw, int r, const Already& already) {
    DenExpr expr = raw;
    for (int j = static_cast<int>(raw.rows[r].entries.size()) - 1; j >= 0; --j) {
        if (!raw.rows[r].entries[j].marked) continue;
        const int n = raw.rows[r].entries[j].value;
        const std::vector<SeqItem> seq = seq_from(raw, r, j);
        const int t = seq.back().row;
        if (t - 1 < 0 || t - 1 >= static_cast<int>(already.size()) || !already[t - 1]) continue;
        const std::vector<int>& T = *already[t - 1];
        const int q = static_cast<int>(T.size());

        std::vector<DenEntry> entries = expr.rows[r].entries;
        for (const int x : T) entries.push_back({ x, false });
        for (int k = 0; k < q; ++k) entries.push_back({ n + 1 + k, true });
        std::stable_sort(entries.begin(), entries.end(),
            [](const DenEntry& a, const DenEntry& b) { return a.value > b.value; });
        expr.rows[r].step += q;
        expr.rows[r].entries = std::move(entries);
    }
    return expr;
}

// compFrom：以第 r 行为模板、T 为系数构造 q+1 行并平移后续行。
[[nodiscard]] inline DenExpr comp_from(const DenExpr& raw, int r, const std::vector<int>& T) {
    DenExpr expr;
    expr.rows.assign(raw.rows.begin(), raw.rows.begin() + r);
    const int q = static_cast<int>(T.size());
    const DenRow& rr = raw.rows[r];
    const int first_val = rr.entries.empty() ? 0 : rr.entries.front().value;
    const int lr = static_cast<int>(rr.entries.size()) < rr.step * 2 ? rr.step : rr.step + 1;

    // cr：slice(0, -step) concat slice(step)，或整行（长行情况）
    std::vector<DenEntry> cr;
    if (static_cast<int>(rr.entries.size()) < rr.step * 2) {
        denshare::append_slice_except_tail(cr, rr.entries, 0, rr.step);
        denshare::append_slice_from(cr, rr.entries, rr.step);
    } else {
        cr = rr.entries;
    }

    auto make_row = [first_val](int step, std::vector<DenEntry> entries) {
        std::stable_sort(entries.begin(), entries.end(),
            [](const DenEntry& a, const DenEntry& b) { return a.value > b.value; });
        DenRow row;
        row.step = step;
        row.entries = std::move(entries);
        return row;
    };

    for (int l = 0; l < q; ++l) {
        std::vector<DenEntry> entries = cr;
        for (int k = 0; k <= l; ++k) entries.push_back({ T[k], false });
        for (int k = 0; k < l; ++k) entries.push_back({ first_val + 1 + k, false });
        expr.rows.push_back(make_row(lr + l, std::move(entries)));
    }
    {
        std::vector<DenEntry> entries = rr.entries;
        for (const int x : T) entries.push_back({ x, false });
        for (int k = 0; k < q; ++k) entries.push_back({ first_val + 1 + k, false });
        expr.rows.push_back(make_row(rr.step + q, std::move(entries)));
    }
    // 新行标记：expr[r+l] 的条目 1..l 标真
    for (int l = 1; l <= q; ++l) {
        DenRow& row = expr.rows[r + l];
        for (int k = 1; k <= l && k < static_cast<int>(row.entries.size()); ++k) {
            row.entries[k].marked = true;
        }
    }
    // 后续行：值 > first_val 的平移 q，标记保留
    for (std::size_t ii = r + 1; ii < raw.rows.size(); ++ii) {
        DenRow nrow;
        nrow.step = raw.rows[ii].step;
        nrow.entries.reserve(raw.rows[ii].entries.size());
        for (const DenEntry& e : raw.rows[ii].entries) {
            DenEntry ne = e;
            if (ne.value > first_val) ne.value += q;
            nrow.entries.push_back(ne);
        }
        expr.rows.push_back(std::move(nrow));
    }
    return expr;
}

// expand：核心展开。shorter=false 对应 FS（较短），true 对应 FS_alter（较长）。
[[nodiscard]] inline DenExpr expand(const DenExpr& raw, int index, bool shorter) {
    if (raw.rows.empty()) return raw;
    const DenRow& active = raw.rows.back();
    if (static_cast<std::size_t>(active.step) >= active.entries.size() ||
        active.entries[active.step].value == 0) return cut(raw);

    const int begin = active.entries[active.step].value;
    std::vector<std::vector<int>> rows;
    for (std::size_t i = static_cast<std::size_t>(begin - 1); i + 1 < raw.rows.size(); ++i) {
        rows.push_back(denshare::row_values(raw.rows[i]));
    }
    std::vector<int> tcheck;
    for (std::size_t k = static_cast<std::size_t>(active.step); k < active.entries.size(); ++k) {
        tcheck.push_back(active.entries[k].value);
    }
    const int flag = denshare::pleasant_until(rows, tcheck);

    DenExpr expr = raw;
    if (flag >= 0) {
        expr = copy(expr, flag);
    } else {
        for (int n = 1; n <= index; ++n) expr = copy(expr, flag);   // flag = -1 → 复制到末尾（extend）
        expr = shorter ? cut(expr) : copy(expr, 1);
    }

    Already already(expr.rows.size() + 8);
    for (int r = static_cast<int>(raw.rows.size()) - 1; r < static_cast<int>(expr.rows.size()); ++r) {
        expr = comp_to(expr, r, already);
        if (!(expr.rows[r].entries.size() <= static_cast<std::size_t>(expr.rows[r].step) * 2)) {
            continue;
        }
        // 构造 T（do-while 链）
        const DenRow& row = expr.rows[r];
        std::vector<int> T;
        T.push_back((row.step - 1 >= 0 && row.step - 1 < static_cast<int>(row.entries.size()))
            ? row.entries[row.step - 1].value : 0);
        while (true) {
            const int row_idx = T[0] - 1;
            int v = 0;
            if (row_idx >= 0 && row_idx < static_cast<int>(expr.rows.size())) {
                const DenRow& r2 = expr.rows[row_idx];
                v = (1 < static_cast<int>(r2.entries.size())) ? r2.entries[1].value : 0;
            }
            T.insert(T.begin(), v);
            const int th = (row.step < static_cast<int>(row.entries.size()))
                ? row.entries[row.step].value : 0;
            if (!(T[0] > th)) break;
        }
        if (!T.empty()) T.erase(T.begin());
        if (!T.empty()) T.pop_back();
        if (T.empty()) continue;

        expr = comp_from(expr, r, T);
        if (r >= static_cast<int>(already.size())) already.resize(static_cast<std::size_t>(r) + 1);
        already[r] = T;
        r += static_cast<int>(T.size());
    }
    return expr;
}

// ---------------------------------------------------------------------
// 极限的 FS（TS 的 infinity_FS）
// ---------------------------------------------------------------------

// 注意：DEN2 的 Limit_row 与 DEN3 不同，末尾没有追加的 2（步骤值在 reversed 之外）。
[[nodiscard]] inline DenRow limit_row(int n) {
    DenRow row;
    row.step = 2;
    row.entries.reserve(static_cast<std::size_t>(3 + n));
    for (int nn = 2 + n; nn >= 0; --nn) {
        row.entries.push_back({ nn, 3 <= nn && nn < 2 + n });
    }
    return row;
}

[[nodiscard]] inline DenExpr infinity_fs(int n) {
    DenExpr expr;
    expr.rows.push_back({ 1, { {1, false}, {0, false} } });
    expr.rows.push_back({ 1, { {2, false}, {1, false}, {0, false} } });
    for (int i = 0; i < n; ++i) expr.rows.push_back(limit_row(1 + i));
    return expr;
}

// FS / FS_alter 的公共骨架。
[[nodiscard]] inline std::string expand_text_impl(std::string_view text, int term, bool shorter) {
    if (term < 1) term = 1;
    auto parsed = parse(text);
    if (!parsed) {
        return "解析失败，请确认你没有使用全角括号，逗号";
    }
    if (parsed->infinity) {
        return to_string(infinity_fs(term));
    }
    if (parsed->rows.empty()) {
        DenExpr e;
        return to_string(e);
    }
    if (!is_limit(*parsed)) {
        return to_string(cut(*parsed));
    }
    return to_string(expand(*parsed, term, shorter));
}

} // namespace blp_detail

// ---------------------------------------------------------------------
// 公共接口
// ---------------------------------------------------------------------

[[nodiscard]] inline std::string BlpNotation::expand_text(std::string_view text, int term) {
    return blp_detail::expand_text_impl(text, term, /*shorter=*/true);
}

[[nodiscard]] inline std::string BlpNotation::expand_text_alter(std::string_view text, int term) {
    return blp_detail::expand_text_impl(text, term, /*shorter=*/false);
}

[[nodiscard]] inline std::optional<denshare::DenExpr> BlpNotation::parse(std::string_view text) {
    return blp_detail::parse(text);
}

[[nodiscard]] inline std::string BlpNotation::to_string(const denshare::DenExpr& expr) {
    return blp_detail::to_string(expr);
}

} // namespace omegay::notation

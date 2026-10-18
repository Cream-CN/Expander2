// Header/notation/iblp.hpp
// iblp 记号（display 名为 "iblp"）。
//
// 逻辑移植自 notation-explorer（hypcos）的 src/notations/DEN/DEN3.ts。
// 表达式是“带标记的扁平行”序列：每行是扁平数组，行内位置 0 是 step，
// 位置 1.. 是条目（entry，带标记，display 中以 '*' 表示）。
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

struct IblpNotation {
    static constexpr const char* kName = "iblp";

    [[nodiscard]] static std::string expand_text(std::string_view text, int term);
    [[nodiscard]] static std::string expand_text_alter(std::string_view text, int term);
    [[nodiscard]] static std::string suffix() { return {}; }

    [[nodiscard]] static std::optional<denshare::Den3Expr> parse(std::string_view text);
    [[nodiscard]] static std::string to_string(const denshare::Den3Expr& expr);
};

namespace iblp_detail {

using denshare::Den3Expr;
using denshare::Den3Row;
using denshare::DenEntry;
using denshare::TextParser;

// ---------------------------------------------------------------------
// 显示与解析
// ---------------------------------------------------------------------

// TS: toShort —— 行内位置 1.. 的 slice(1, -step) 再补最后一个条目
// （step=0 时切片为空，-0 按 0 处理）。
[[nodiscard]] inline std::vector<int> to_short_row(const Den3Row& row) {
    std::vector<int> out;
    const int step = row.empty() ? 0 : row[0].value;
    if (step >= 1) {
        const int end = static_cast<int>(row.size()) - step;
        for (int i = 1; i < end; ++i) out.push_back(row[i].value);
    }
    if (!row.empty()) out.push_back(row.back().value);
    return out;
}

[[nodiscard]] inline std::string to_string(const Den3Expr& expr) {
    if (expr.infinity) return "Limit";
    std::string out;
    for (const Den3Row& row : expr.rows) {
        out += '(';
        for (std::size_t k = 1; k < row.size(); ++k) {
            if (k > 1) out += ',';
            if (row[k].marked) out += '*';
            out += std::to_string(row[k].value);
        }
        out += ')';
        out += std::to_string(row[0].value);
    }
    return out;
}

// 解析（TS 的 from_display）：'(' 条目列表 ')' step 反复。
// 与 DEN2 不同，这里括号内允许为空；非法输入返回 nullopt。
[[nodiscard]] inline std::optional<Den3Expr> parse(std::string_view s) {
    TextParser p(s);
    p.skip_spaces();
    if (p.match("Limit")) {
        p.skip_spaces();
        if (!p.eof()) return std::nullopt;
        Den3Expr e;
        e.infinity = true;
        return e;
    }

    Den3Expr expr;
    while (!p.eof()) {
        p.skip_spaces();
        if (p.eof()) break;
        if (p.peek() != '(') return std::nullopt;
        p.advance(1);
        p.skip_spaces();
        Den3Row row;
        if (!p.eof() && p.peek() != ')') {
            while (true) {
                bool marked = false;
                if (!p.eof() && p.peek() == '*') { marked = true; p.advance(1); }
                auto v = p.parse_digits();
                if (!v) return std::nullopt;
                row.push_back({ *v, marked });
                p.skip_spaces();
                if (!p.eof() && p.peek() == ',') { p.advance(1); continue; }
                break;
            }
        }
        if (p.eof() || p.peek() != ')') return std::nullopt;
        p.advance(1);
        p.skip_spaces();
        auto step = p.parse_digits();
        if (!step) return std::nullopt;
        row.insert(row.begin(), { *step, false });
        expr.rows.push_back(std::move(row));
    }
    return expr;
}

// ---------------------------------------------------------------------
// 比较与极限判定
// ---------------------------------------------------------------------

[[nodiscard]] inline int compare(const Den3Expr& a, const Den3Expr& b) {
    if (a.infinity || b.infinity) return denshare::bool_compare(a.infinity, b.infinity);
    std::vector<std::vector<int>> sa, sb;
    for (const Den3Row& row : a.rows) sa.push_back(to_short_row(row));
    for (const Den3Row& row : b.rows) sb.push_back(to_short_row(row));
    return denshare::matrix_compare(sa, sb);
}

[[nodiscard]] inline bool is_limit(const Den3Expr& expr) {
    if (expr.infinity) return true;
    if (expr.rows.empty()) return false;
    const Den3Row& active = expr.rows.back();
    const std::size_t idx = 1 + static_cast<std::size_t>(active[0].value);
    if (idx >= active.size() || active[idx].value == 0) return false;

    // pleasant_until(expr.slice(value-1, -1), active) === -1
    const int begin = active[idx].value;
    std::vector<std::vector<int>> rows;
    for (std::size_t i = static_cast<std::size_t>(begin - 1); i + 1 < expr.rows.size(); ++i) {
        std::vector<int> vals;
        for (std::size_t e = 1; e < expr.rows[i].size(); ++e) vals.push_back(expr.rows[i][e].value);
        rows.push_back(std::move(vals));
    }
    std::vector<int> tcheck;
    for (std::size_t e = idx; e < active.size(); ++e) tcheck.push_back(active[e].value);
    return denshare::pleasant_until(rows, tcheck) == -1;
}

// ---------------------------------------------------------------------
// 展开核心
// ---------------------------------------------------------------------

// 删除最后一行。
[[nodiscard]] inline Den3Expr cut(const Den3Expr& raw) {
    Den3Expr expr = raw;
    if (!expr.rows.empty()) expr.rows.pop_back();
    return expr;
}

// seqFrom 的产物项：(行, 条目下标)。行是 1 起的下标（或 active 的值）。
struct SeqItem {
    int row;
    int idx;
};

// TS: seqFrom —— 与 DEN2 不同：
//   - threshold 为 0 时返回空；
//   - 沿 parent 链走到 val != threshold 时返回空；
//   - 返回 record.slice(1, -1)（去掉首尾两项）。
[[nodiscard]] inline std::optional<std::vector<SeqItem>> seq_from(
    const Den3Expr& expr, int i, int j) {
    const Den3Row& row = expr.rows[i];
    if (j < 0 || j >= static_cast<int>(row.size())) return std::nullopt;
    const int val0 = row[j].value;
    const int threshold = (j + row[0].value >= 0 && j + row[0].value < static_cast<int>(row.size()))
        ? row[j + row[0].value].value : 0;
    if (!threshold) return std::nullopt;

    std::vector<SeqItem> record;
    record.push_back({ i + 1, j });
    record.push_back({ val0, 0 });
    int v = val0;
    while (v > threshold) {
        if (v - 1 < 0 || v - 1 >= static_cast<int>(expr.rows.size())) return std::nullopt;
        const Den3Row& r2 = expr.rows[v - 1];
        const int idx = 1 + r2[0].value;
        record.back().idx = idx;
        v = (idx >= 0 && idx < static_cast<int>(r2.size())) ? r2[idx].value : 0;
        record.push_back({ v, 0 });
    }
    if (v != threshold) return std::nullopt;

    record.erase(record.begin());
    record.pop_back();
    return record;
}

// ap：按 active 行的映射规则复制一行（step 不变，条目逐个映射）。
[[nodiscard]] inline Den3Row apply_row(const Den3Row& s, const Den3Row& t) {
    std::vector<int> s_vals, t_vals;
    for (std::size_t e = 1; e < s.size(); ++e) s_vals.push_back(s[e].value);
    for (std::size_t e = 1; e < t.size(); ++e) t_vals.push_back(t[e].value);
    Den3Row out;
    out.reserve(s.size());
    out.push_back({ s[0].value, false });
    for (const int x : s_vals) out.push_back({ denshare::ap_value(x, t_vals, t[0].value), false });
    return out;
}

// copy：复制 [begin-1, end-1) 区间内的行并迁移标记。
// 注意标记迁移的 seq 取自新表达式（expr），而非 raw。
[[nodiscard]] inline Den3Expr copy(const Den3Expr& raw, int flag) {
    const Den3Row& active = raw.rows.back();
    Den3Expr expr = cut(raw);
    const int begin = active[1 + active[0].value].value;
    const int end = flag >= 0 ? begin + flag : static_cast<int>(raw.rows.size()) + 1;
    const int offset = static_cast<int>(raw.rows.size()) - begin;
    const int a1 = active.back().value;

    for (int i = begin - 1; i < end - 1; ++i) {
        expr.rows.push_back(apply_row(raw.rows[i], active));
    }
    for (int i = begin - 1; i < end - 1; ++i) {
        const Den3Row& row = raw.rows[i];
        Den3Row& target = expr.rows[i + offset];
        for (int j = 1; j < static_cast<int>(row.size()); ++j) {
            if (!row[j].marked) continue;
            auto seq = seq_from(expr, i + offset, j);
            if (!seq) continue;
            int nomove = -1;
            for (std::size_t k = 0; k < seq->size(); ++k) {
                if ((*seq)[k].row < begin) { nomove = static_cast<int>(k); break; }
            }
            if (nomove == -1) { target[j].marked = true; continue; }
            const int y0 = (*seq)[nomove].row;
            if (y0 < a1) { target[j].marked = true; continue; }
            // k = 1 + active.slice(1).findIndex(x => x[0] === y0)；找不到时为 0
            int k = 0;
            for (int e = 1; e < static_cast<int>(active.size()); ++e) {
                if (active[e].value == y0) { k = e; break; }
            }
            const int kk = k - active[0].value;
            const bool active_mark = (kk >= 1 && kk < static_cast<int>(active.size()))
                ? active[kk].marked : false;
            // !(target[j + step - 1]?.[0] > a1)：越界视为满足
            const int tj = j + target[0].value - 1;
            const bool lhs = (tj < 0 || tj >= static_cast<int>(target.size()))
                ? true : (target[tj].value <= a1);
            if (active_mark && lhs) target[j].marked = true;
        }
    }
    return expr;
}

// Rec：行下标 -> 该行压缩时构造的 T（尚未构造的为空）。
using Rec = std::vector<std::optional<std::vector<int>>>;

// compTo：对第 r 行做“上向压缩”。只处理位置 1.. 的标记条目。
[[nodiscard]] inline Den3Expr comp_to(const Den3Expr& raw, int r, const Rec& rec) {
    Den3Expr expr = raw;
    for (int i = static_cast<int>(raw.rows[r].size()) - 1; i > 0; --i) {
        if (!raw.rows[r][i].marked) continue;
        const int bi = raw.rows[r][i].value;
        auto seq = seq_from(expr, r, i);
        if (!seq) continue;
        const int t = seq->back().row;
        if (t - 1 < 0 || t - 1 >= static_cast<int>(rec.size()) || !rec[t - 1]) continue;
        // 原 JS 中有一处对内层 j 循环自身的 continue（对结果无影响），此处略去。
        const std::vector<int>& T = *rec[t - 1];
        const int q = static_cast<int>(T.size());

        std::vector<DenEntry> entries;
        for (std::size_t e = 1; e < expr.rows[r].size(); ++e) entries.push_back(expr.rows[r][e]);
        for (const int x : T) entries.push_back({ x, false });
        for (int uu = 0; uu < q; ++uu) entries.push_back({ bi + 1 + uu, true });
        std::stable_sort(entries.begin(), entries.end(),
            [](const DenEntry& a, const DenEntry& b) { return a.value > b.value; });
        Den3Row nrow;
        nrow.reserve(entries.size() + 1);
        nrow.push_back({ expr.rows[r][0].value + q, false });
        nrow.insert(nrow.end(), entries.begin(), entries.end());
        expr.rows[r] = std::move(nrow);
    }
    return expr;
}

// compFrom：以第 r 行为模板、T 为系数构造 q+1 行并平移后续行。
// 与 DEN2 不同：长行判据带 +1；新行标记区间是 [2, 1+l]；
// 后续行的位置 0（step）不参与平移。
[[nodiscard]] inline Den3Expr comp_from(const Den3Expr& raw, int r, const std::vector<int>& T) {
    Den3Expr expr;
    expr.rows.assign(raw.rows.begin(), raw.rows.begin() + r);
    const int q = static_cast<int>(T.size());
    const Den3Row& rr = raw.rows[r];
    const int first_val = rr.size() > 1 ? rr[1].value : 0;
    const bool short_row = static_cast<int>(rr.size()) < rr[0].value * 2 + 1;
    const int lr = short_row ? rr[0].value : rr[0].value + 1;

    // cr：短行 slice(1, -step) concat slice(1+step)；长行 slice(1)
    std::vector<DenEntry> cr;
    if (short_row) {
        denshare::append_slice_except_tail(cr, rr, 1, rr[0].value);
        denshare::append_slice_from(cr, rr, 1 + rr[0].value);
    } else {
        denshare::append_slice_from(cr, rr, 1);
    }

    auto make_row = [](int step, std::vector<DenEntry> entries) {
        std::stable_sort(entries.begin(), entries.end(),
            [](const DenEntry& a, const DenEntry& b) { return a.value > b.value; });
        Den3Row row;
        row.reserve(entries.size() + 1);
        row.push_back({ step, false });
        row.insert(row.end(), entries.begin(), entries.end());
        return row;
    };

    for (int qq = 0; qq < q; ++qq) {
        std::vector<DenEntry> entries = cr;
        for (int k = 0; k <= qq; ++k) entries.push_back({ T[k], false });
        for (int uu = 0; uu < qq; ++uu) entries.push_back({ first_val + 1 + uu, false });
        expr.rows.push_back(make_row(lr + qq, std::move(entries)));
    }
    {
        std::vector<DenEntry> entries;
        for (std::size_t e = 1; e < rr.size(); ++e) entries.push_back(rr[e]);
        for (const int x : T) entries.push_back({ x, false });
        for (int uu = 0; uu < q; ++uu) entries.push_back({ first_val + 1 + uu, false });
        expr.rows.push_back(make_row(rr[0].value + q, std::move(entries)));
    }
    // 新行标记：expr[r+qq] 的位置 2..1+qq 标真
    for (int qq = 1; qq <= q; ++qq) {
        Den3Row& row = expr.rows[r + qq];
        for (int uu = 2; uu <= 1 + qq && uu < static_cast<int>(row.size()); ++uu) {
            row[uu].marked = true;
        }
    }
    // 后续行：位置 0（step）不动，其余位置值 > first_val 的平移 q，标记保留
    for (std::size_t ii = r + 1; ii < raw.rows.size(); ++ii) {
        Den3Row nrow;
        nrow.reserve(raw.rows[ii].size());
        nrow.push_back(raw.rows[ii][0]);
        for (std::size_t e = 1; e < raw.rows[ii].size(); ++e) {
            DenEntry ne = raw.rows[ii][e];
            if (ne.value > first_val) ne.value += q;
            nrow.push_back(ne);
        }
        expr.rows.push_back(std::move(nrow));
    }
    return expr;
}

// expand：核心展开。longer=false 对应 FS（较短），true 对应 FS_alter（较长）。
// FS_alter 在全部压缩完成后会把结果裁剪回 len0 行。
[[nodiscard]] inline Den3Expr expand(const Den3Expr& raw, int fs_term, bool longer) {
    if (raw.rows.empty()) return raw;
    const Den3Row& active = raw.rows.back();
    const std::size_t idx = 1 + static_cast<std::size_t>(active[0].value);
    if (idx >= active.size() || active[idx].value == 0) return cut(raw);

    const int begin = active[idx].value;
    std::vector<std::vector<int>> rows;
    for (std::size_t i = static_cast<std::size_t>(begin - 1); i + 1 < raw.rows.size(); ++i) {
        std::vector<int> vals;
        for (std::size_t e = 1; e < raw.rows[i].size(); ++e) vals.push_back(raw.rows[i][e].value);
        rows.push_back(std::move(vals));
    }
    std::vector<int> tcheck;
    for (std::size_t e = idx; e < active.size(); ++e) tcheck.push_back(active[e].value);
    const int flag = denshare::pleasant_until(rows, tcheck);

    Den3Expr expr = raw;
    int len0 = -1;
    if (flag >= 0) {
        expr = copy(expr, flag);
    } else {
        for (int n = 1; n <= fs_term; ++n) expr = copy(expr, flag);
        if (longer) {
            len0 = static_cast<int>(expr.rows.size());
            expr = copy(expr, 1);
        } else {
            expr = cut(expr);
        }
    }

    Rec rec;
    for (int r = static_cast<int>(raw.rows.size()) - 1; r < static_cast<int>(expr.rows.size()); ++r) {
        expr = comp_to(expr, r, rec);
        const Den3Row& row = expr.rows[r];
        if (!(row.size() <= static_cast<std::size_t>(row[0].value) * 2 + 1)) continue;
        const int pr = (1 + row[0].value < static_cast<int>(row.size()))
            ? row[1 + row[0].value].value : 0;
        std::vector<int> T;
        T.push_back((row[0].value < static_cast<int>(row.size())) ? row[row[0].value].value : 0);
        while (true) {
            // expr[T[0] - 1][2]：行内固定位置 2
            const int row_idx = T[0] - 1;
            int v = 0;
            if (row_idx >= 0 && row_idx < static_cast<int>(expr.rows.size()) &&
                expr.rows[row_idx].size() > 2) {
                v = expr.rows[row_idx][2].value;
            }
            T.insert(T.begin(), v);
            if (!(T[0] > pr)) break;
        }
        if (!T.empty()) T.erase(T.begin());
        if (!T.empty()) T.pop_back();
        if (T.empty()) continue;

        if (r >= static_cast<int>(rec.size())) rec.resize(static_cast<std::size_t>(r) + 1);
        rec[r] = T;
        expr = comp_from(expr, r, T);
        r += static_cast<int>(T.size());
    }
    if (longer && len0 >= 0) {
        while (static_cast<int>(expr.rows.size()) > len0) expr = cut(expr);
    }
    return expr;
}

// ---------------------------------------------------------------------
// 极限的 FS（TS 的 Limit）
// ---------------------------------------------------------------------

// DEN3 的 Limit_row 与 DEN1 同构：末尾追加 2 再整体反转；
// 反转后位置 0 恰好是 step=2。
[[nodiscard]] inline Den3Row limit_row(int n) {
    return denshare::limit_row_entries(n);
}

[[nodiscard]] inline Den3Expr limit(int n) {
    Den3Expr expr;
    expr.rows.push_back({ {1, false}, {1, false}, {0, false} });
    expr.rows.push_back({ {1, false}, {2, false}, {1, false}, {0, false} });
    for (int i = 0; i < n; ++i) expr.rows.push_back(limit_row(1 + i));
    return expr;
}

// FS / FS_alter 的公共骨架。与 DEN2 不同，DEN3 的 FS 不做 is_limit 预判，
// 直接交给 expand（expand 内部会处理非极限情形）。
[[nodiscard]] inline std::string expand_text_impl(std::string_view text, int term, bool longer) {
    if (term < 1) term = 1;
    auto parsed = parse(text);
    if (!parsed) {
        return "解析失败，请确认你没有使用全角括号，逗号";
    }
    if (parsed->infinity) {
        return to_string(limit(term));
    }
    if (parsed->rows.empty()) {
        Den3Expr e;
        return to_string(e);
    }
    return to_string(expand(*parsed, term, longer));
}

} // namespace iblp_detail

// ---------------------------------------------------------------------
// 公共接口
// ---------------------------------------------------------------------

[[nodiscard]] inline std::string IblpNotation::expand_text(std::string_view text, int term) {
    return iblp_detail::expand_text_impl(text, term, /*longer=*/false);
}

[[nodiscard]] inline std::string IblpNotation::expand_text_alter(std::string_view text, int term) {
    return iblp_detail::expand_text_impl(text, term, /*longer=*/true);
}

[[nodiscard]] inline std::optional<denshare::Den3Expr> IblpNotation::parse(std::string_view text) {
    return iblp_detail::parse(text);
}

[[nodiscard]] inline std::string IblpNotation::to_string(const denshare::Den3Expr& expr) {
    return iblp_detail::to_string(expr);
}

} // namespace omegay::notation

#pragma once
//============================================================================
// OOP 内核（2026-11 深度面向对象重构）
//
//  统一记号接口（INotation）→ 能力适配器（模板，包住既有 static-struct 记号）
//  → 特殊实现（BMS / UPMS / MrSS）→ 注册中心（NotationRegistry）→ 唯一注册表
//  （register_all_notations，114 项）。
//
//  设计要点：
//    · 130+ 个记号头文件的内部实现零改动，通过模板适配器（静态多态）接入
//      运行时多态接口；
//    · GUI 的 NotationEntry 表 / CLI 的 CliEntry 表都从本注册中心派生（单一
//      事实来源），对外行为逐字节不变；
//    · 原 GUI/CLI 重复的 FS 分派 / FS 截断 / BMS/UPMS 文本⇄Matrix 逻辑收敛
//      到 INotation::expand() 与 truncate_fs()（策略虚函数）；
//    · header-only，兼容 GUI（MSVC /std:c++20）与 CLI（cl.exe / g++ 单文件）。
//============================================================================

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "empty.hpp"
#include "prss.hpp"
#include "PPS-family.hpp"
#include "Omega-YMagma.hpp"
#include "mrss121.hpp"
#include "omega_y.hpp"
#include "epsilon-y.hpp"
#include "BMSFamily.hpp"
#include "upms.hpp"
#include "bsm.hpp"
#include "bhm.hpp"
#include "cms.hpp"
#include "dsm.hpp"
#include "blm.hpp"
#include "wmm.hpp"
#include "gms.hpp"
#include "lpms.hpp"
#include "tbm.hpp"
#include "tupms.hpp"
#include "btbm.hpp"
#include "btbw.hpp"
#include "bbm.hpp"
#include "m1ynss.hpp"
#include "tm1ynss.hpp"
#include "bm1ynss.hpp"
#include "bt1ynss.hpp"
#include "bt2ynss.hpp"
#include "bt3ynss.hpp"
#include "btlynss.hpp"
#include "gy.hpp"
#include "xy-v1.hpp"
#include "xy-v2.hpp"
#include "xy-v3.hpp"
#include "den1.hpp"
#include "blp.hpp"
#include "iblp.hpp"
#include "ton.hpp"
#include "ton_i.hpp"
#include "ton_dor.hpp"
#include "ton_drc.hpp"
#include "ton_drp.hpp"
#include "ton_drpc.hpp"
#include "ton_ibp.hpp"
#include "ton_mc.hpp"
#include "ton_mpc.hpp"
#include "e0mn.hpp"
#include "strong_e0mn.hpp"
#include "omega_mn.hpp"
#include "pcf_omega_mn.hpp"
#include "t_omega_mn.hpp"
#include "n_mn.hpp"
#include "s_omega2_mn.hpp"
#include "sa_omega2_mn.hpp"
#include "s_omega_pow_omega_mn.hpp"
#include "s1dbms.hpp"
#include "s2dbms.hpp"
#include "s_omega_dbms_v1.hpp"
#include "s_omega_dbms_v2.hpp"
#include "s_omega_dbms_v3.hpp"
#include "s_omega_p1_dbms.hpp"
#include "s_omega2_dbms.hpp"
#include "s_omega_square_dbms.hpp"
#include "ts2dbms.hpp"
#include "ts_omega_dbms.hpp"
#include "up1mn.hpp"
#include "up1dbms.hpp"
#include "up2mn_v1a.hpp"
#include "up2mn_v1b.hpp"
#include "up2mn_v1b_plus.hpp"
#include "up2dbms_v1.hpp"
#include "up2dbms_v1b_plus.hpp"
#include "up2dbms_v1c.hpp"
#include "up2dbms_v2.hpp"
#include "aw2mn2.hpp"
#include "aw2mn3.hpp"
#include "waw2mn2.hpp"
#include "waw2mn3.hpp"
#include "../common/Matrix.hpp"
#include "../common/sequence.hpp"
#include "../../Resource.h"   // IDR_DEF_* 定义资源 id（纯宏头，跨平台）

namespace omegay::notation {

//--------------------------------------------------------------------------
// 展开模式：Fs = “展开”按钮（含 FS 通用截断）；FsAlter = “FS_alter”按钮。
//--------------------------------------------------------------------------
enum class ExpandMode { Fs, FsAlter };

// 一次展开的结果：成功返回 text；失败置 ok=false 并给出人可读错误（与
// 原 GUI/CLI 的错误提示逐字一致）。
struct ExpandResult {
    std::string text;
    bool ok = true;
    std::string err;
};

// 记号元数据（UTF-8 窄字符存储，跨平台；GUI 壳层在构建 NotationEntry 时
// 一次性转换为宽字符并静态驻留）。
struct NotationMeta {
    std::string_view id;                 // 唯一英文 id（表内查找用）
    std::string_view display_name;       // 展示名
    int definition_resource_id = 0;      // 统一定义文本资源 id（Definitions.h）
    std::array<std::string_view, 16> tags{};  // 搜索/展示标签；空槽为空 string_view
    int param_min = 0;                   // 参数化记号：参数允许的最小值（含）
    int param_max = 25;                  // 参数化记号：参数允许的最大值（含）
    int param_default = 0;               // 参数输入框默认值
    std::string_view param_label = "n:"; // 参数输入框标签
};

// 能力函数指针（旧壳层 NotationEntry / CliEntry 各函数指针字段的单一来源；
// 由适配器从记号实现提取，或由特殊实现提供 trampoline）。
struct NotationCaps {
    std::vector<int> (*expand_seq)(const std::vector<int>&, int) = nullptr;
    std::string (*suffix)() = nullptr;
    std::string (*text)(std::string_view, int) = nullptr;
    std::string (*text_alter)(std::string_view, int) = nullptr;
    std::string (*text_n)(std::string_view, int, int) = nullptr;
    std::string (*text_n_alter)(std::string_view, int, int) = nullptr;
};

//==========================================================================
// 抽象记号接口：元数据 + 能力虚函数 + 统一展开入口 + FS 截断策略。
// 展开语义复刻原 GUI IDM_FS / CLI expandFs 的分派逻辑，逐字节等价。
//==========================================================================
class INotation {
public:
    explicit INotation(NotationMeta meta) noexcept : meta_(meta) {}
    virtual ~INotation() = default;

    // ---- 元数据 ----
    [[nodiscard]] const NotationMeta& meta() const noexcept { return meta_; }
    [[nodiscard]] std::string_view id() const noexcept { return meta_.id; }
    [[nodiscard]] int definition_resource_id() const noexcept {
        return meta_.definition_resource_id;
    }
    [[nodiscard]] bool is_parametric() const noexcept { return has_text_n(); }
    [[nodiscard]] const NotationCaps& caps() const noexcept { return caps_; }

    // ---- 能力（默认无） ----
    [[nodiscard]] virtual bool has_seq() const noexcept { return false; }
    [[nodiscard]] virtual bool has_text() const noexcept { return false; }
    [[nodiscard]] virtual bool has_text_alter() const noexcept { return false; }
    [[nodiscard]] virtual bool has_text_n() const noexcept { return false; }
    [[nodiscard]] virtual bool has_text_n_alter() const noexcept { return false; }

    // ---- 展开入口（未支持时返回空，调用方按能力查询） ----
    [[nodiscard]] virtual std::vector<int> expand_seq(
        const std::vector<int>& seq, int term) const { (void)seq; (void)term; return {}; }
    [[nodiscard]] virtual std::string expand_text(
        std::string_view text, int term) const { (void)text; (void)term; return {}; }
    [[nodiscard]] virtual std::string expand_text_alter(
        std::string_view text, int term) const { (void)text; (void)term; return {}; }
    [[nodiscard]] virtual std::string expand_text_n(
        std::string_view text, int term, int param) const {
        (void)text; (void)term; (void)param; return {}; }
    [[nodiscard]] virtual std::string expand_text_n_alter(
        std::string_view text, int term, int param) const {
        (void)text; (void)term; (void)param; return {}; }
    [[nodiscard]] virtual std::string suffix() const { return {}; }

    // ---- 统一展开入口（原 GUI/CLI 分派语义的收敛；返回含 suffix 的最终文本）----
    [[nodiscard]] ExpandResult expand(ExpandMode mode, std::string_view text,
                                      int term, int param) const {
        // 文本型记号（优先于参数化/序列型，与原分派顺序一致）
        if (has_text()) {
            if (mode == ExpandMode::FsAlter && has_text_alter())
                return { expand_text_alter(text, term) + suffix(), true, {} };

            std::string out = expand_text(text, term);
            // 仅 FS 且记号未自带 FS_alter 语义时做通用截断
            if (mode == ExpandMode::Fs && !has_text_alter())
                out = truncate_fs(std::move(out), term);
            out += suffix();
            return { std::move(out), true, {} };
        }

        // 参数化记号
        if (has_text_n()) {
            if (param < meta_.param_min || param > meta_.param_max) {
                ExpandResult r;
                r.ok = false;
                r.err = "参数必须是 " + std::to_string(meta_.param_min)
                      + " 到 " + std::to_string(meta_.param_max)
                      + " 之间的整数";
                return r;
            }
            std::string out = (mode == ExpandMode::FsAlter && has_text_n_alter())
                ? expand_text_n_alter(text, term, param)
                : expand_text_n(text, term, param);
            out += suffix();
            return { std::move(out), true, {} };
        }

        // 序列型记号
        if (term < 1)
            return { {}, false, "项数必须为正整数" };
        std::vector<int> seq = common::parse_sequence(text);
        if (seq.empty())
            return { {}, false, "序列不能为空" };
        std::vector<int> result = expand_seq(seq, term);
        if (mode == ExpandMode::Fs && result.size() > 1)
            result.pop_back();
        return { common::seq_to_string(result) + suffix(), true, {} };
    }

protected:
    // ---- FS 通用截断策略（虚函数；BMS / UPMS 覆盖为删末列） ----
    // 默认：MrSS 向量语义尝试（复刻原 else 分支——对 cms/gms/blm 等文本
    // 记号尝试 MrSS 解析，失败则原样返回）。
    [[nodiscard]] virtual std::string truncate_fs(std::string out, int term) const {
        (void)term;
        auto parsed = Mrss121Notation::parse(out);
        if (parsed && parsed->size() > 1) {
            parsed->pop_back();
            return Mrss121Notation::to_string(*parsed);
        }
        return out;
    }

    NotationMeta meta_;
    NotationCaps caps_;
};

//==========================================================================
// 适配器模板：把既有 static-struct 记号（T::expand / T::expand_text / ...）
// 适配成 INotation。记号头文件本身零改动。
//==========================================================================
template<class T>
class SequenceNotation : public INotation {
public:
    explicit SequenceNotation(NotationMeta meta) : INotation(std::move(meta)) {
        caps_.expand_seq = &T::expand;
        caps_.suffix = &T::suffix;
    }
    [[nodiscard]] bool has_seq() const noexcept override { return true; }
    [[nodiscard]] std::vector<int> expand_seq(
        const std::vector<int>& seq, int term) const override {
        return T::expand(seq, term);
    }
    [[nodiscard]] std::string suffix() const override { return T::suffix(); }
};

template<class T>
class TextNotation : public INotation {
public:
    explicit TextNotation(NotationMeta meta) : INotation(std::move(meta)) {
        caps_.text = &T::expand_text;
        caps_.suffix = &T::suffix;
    }
    [[nodiscard]] bool has_text() const noexcept override { return true; }
    [[nodiscard]] std::string expand_text(
        std::string_view text, int term) const override {
        return T::expand_text(text, term);
    }
    [[nodiscard]] std::string suffix() const override { return T::suffix(); }
};

template<class T>
class TextAlterNotation : public INotation {
public:
    explicit TextAlterNotation(NotationMeta meta) : INotation(std::move(meta)) {
        caps_.text = &T::expand_text;
        caps_.suffix = &T::suffix;
        // BSM 类记号没有独立 expand_text_alter（alter 与 text 同函数），
        // 此处 if constexpr 自动选择，保证旧表字段语义不变。
        if constexpr (requires { T::expand_text_alter(std::string_view{}, 0); })
            caps_.text_alter = &T::expand_text_alter;
        else
            caps_.text_alter = &T::expand_text;
    }
    [[nodiscard]] bool has_text() const noexcept override { return true; }
    [[nodiscard]] bool has_text_alter() const noexcept override { return true; }
    [[nodiscard]] std::string expand_text(
        std::string_view text, int term) const override {
        return T::expand_text(text, term);
    }
    [[nodiscard]] std::string expand_text_alter(
        std::string_view text, int term) const override {
        if constexpr (requires { T::expand_text_alter(std::string_view{}, 0); })
            return T::expand_text_alter(text, term);
        else
            return T::expand_text(text, term);
    }
    [[nodiscard]] std::string suffix() const override { return T::suffix(); }
};

template<class T>
class ParametricNotation : public INotation {
public:
    explicit ParametricNotation(NotationMeta meta) : INotation(std::move(meta)) {
        caps_.text_n = &T::expand_text_n;
        caps_.suffix = &T::suffix;
    }
    [[nodiscard]] bool has_text_n() const noexcept override { return true; }
    [[nodiscard]] std::string expand_text_n(
        std::string_view text, int term, int param) const override {
        return T::expand_text_n(text, term, param);
    }
    [[nodiscard]] std::string suffix() const override { return T::suffix(); }
};

template<class T>
class ParametricAlterNotation : public INotation {
public:
    explicit ParametricAlterNotation(NotationMeta meta) : INotation(std::move(meta)) {
        caps_.text_n = &T::expand_text_n;
        caps_.text_n_alter = &T::expand_text_n_alter;
        caps_.suffix = &T::suffix;
    }
    [[nodiscard]] bool has_text_n() const noexcept override { return true; }
    [[nodiscard]] bool has_text_n_alter() const noexcept override { return true; }
    [[nodiscard]] std::string expand_text_n(
        std::string_view text, int term, int param) const override {
        return T::expand_text_n(text, term, param);
    }
    [[nodiscard]] std::string expand_text_n_alter(
        std::string_view text, int term, int param) const override {
        return T::expand_text_n_alter(text, term, param);
    }
    [[nodiscard]] std::string suffix() const override { return T::suffix(); }
};

//==========================================================================
// 特殊实现：BMS 族（版本化）、UPMS、MrSS。
// 原 GUI/CLI 的 bmsExpandV1..V4 / upmsExpandText / parseUpmsText /
// formatUpmsText / isBmsTextEntry 等游离代码全部收敛于此。
//==========================================================================

// ---- BMS 文本 ⇄ Matrix 的基础函数（由 BmsNotation 与壳层字段共用） ----
[[nodiscard]] inline std::string bmsExpandTextWith(
    BMSFamilyNotation::Version ver, std::string_view text, int term) {
    long parsedTerm = term;                 // parse_matrix 需要 long&
    omegay::common::Matrix m =
        omegay::common::parse_matrix(text, parsedTerm);

    if (m.empty())
        return "解析失败，请确认你没有使用全角括号，逗号";

    if (!BMSFamilyNotation::is_standard(m, ver))
        return "输入的矩阵为非标准表达式";

    const int t = parsedTerm < 1 ? 1 : static_cast<int>(parsedTerm);

    auto result = BMSFamilyNotation::expand(m, t, ver);
    return omegay::common::matrix_to_string(result);
}
inline std::string bmsExpandV1(std::string_view t, int n) { return bmsExpandTextWith(BMSFamilyNotation::Version::V1, t, n); }
inline std::string bmsExpandV2(std::string_view t, int n) { return bmsExpandTextWith(BMSFamilyNotation::Version::V2, t, n); }
inline std::string bmsExpandV21(std::string_view t, int n) { return bmsExpandTextWith(BMSFamilyNotation::Version::V21, t, n); }
inline std::string bmsExpandV22(std::string_view t, int n) { return bmsExpandTextWith(BMSFamilyNotation::Version::V22, t, n); }
inline std::string bmsExpandV23(std::string_view t, int n) { return bmsExpandTextWith(BMSFamilyNotation::Version::V23, t, n); }
inline std::string bmsExpandV3(std::string_view t, int n) { return bmsExpandTextWith(BMSFamilyNotation::Version::V3, t, n); }
inline std::string bmsExpandV31(std::string_view t, int n) { return bmsExpandTextWith(BMSFamilyNotation::Version::V31, t, n); }
inline std::string bmsExpandV32(std::string_view t, int n) { return bmsExpandTextWith(BMSFamilyNotation::Version::V32, t, n); }
inline std::string bmsExpandV33(std::string_view t, int n) { return bmsExpandTextWith(BMSFamilyNotation::Version::V33, t, n); }
inline std::string bmsExpandV4(std::string_view t, int n) { return bmsExpandTextWith(BMSFamilyNotation::Version::V4, t, n); }

class BmsNotation : public INotation {
public:
    explicit BmsNotation(BMSFamilyNotation::Version ver, NotationMeta meta)
        : INotation(std::move(meta)), ver_(ver) {
        // 壳层函数指针字段：按版本绑定对应 trampoline（与旧表字段值一致）
        switch (ver_) {
        case BMSFamilyNotation::Version::V1:  caps_.text = &bmsExpandV1; break;
        case BMSFamilyNotation::Version::V2:  caps_.text = &bmsExpandV2; break;
        case BMSFamilyNotation::Version::V21: caps_.text = &bmsExpandV21; break;
        case BMSFamilyNotation::Version::V22: caps_.text = &bmsExpandV22; break;
        case BMSFamilyNotation::Version::V23: caps_.text = &bmsExpandV23; break;
        case BMSFamilyNotation::Version::V3:  caps_.text = &bmsExpandV3; break;
        case BMSFamilyNotation::Version::V31: caps_.text = &bmsExpandV31; break;
        case BMSFamilyNotation::Version::V32: caps_.text = &bmsExpandV32; break;
        case BMSFamilyNotation::Version::V33: caps_.text = &bmsExpandV33; break;
        case BMSFamilyNotation::Version::V4:  caps_.text = &bmsExpandV4; break;
        default: break;
        }
        caps_.suffix = &BMSFamilyNotation::suffix;
    }
    [[nodiscard]] bool has_text() const noexcept override { return true; }
    [[nodiscard]] std::string expand_text(
        std::string_view text, int term) const override {
        return bmsExpandTextWith(ver_, text, term);
    }
    [[nodiscard]] std::string suffix() const override {
        return BMSFamilyNotation::suffix();
    }

protected:
    // FS 截断：删除最后一列（BMS 标准文本）
    [[nodiscard]] std::string truncate_fs(std::string out, int term) const override {
        long t = term;
        omegay::common::Matrix m = omegay::common::parse_matrix(out, t);
        if (m.cols() > 1) {
            m.resize_cols(m.cols() - 1);
            return omegay::common::matrix_to_string(m);
        }
        return out;
    }

private:
    BMSFamilyNotation::Version ver_;
};

// ---- UPMS 文本 ⇄ Matrix（省略尾随零的写法） ----
[[nodiscard]] inline omegay::common::Matrix parseUpmsText(std::string_view s) {
    std::vector<std::vector<int>> cols;
    const std::size_t n = s.size();
    std::size_t i = 0;

    while (i < n) {
        while (i < n && s[i] != '(') ++i;
        if (i >= n) break;
        ++i; // 跳过 '('

        std::vector<int> col;
        int  cur = 0;
        bool neg = false;
        bool in_num = false;

        while (i < n && s[i] != ')') {
            const char c = s[i];
            if (c >= '0' && c <= '9') {
                cur = cur * 10 + (c - '0');
                in_num = true;
            }
            else if (c == '-' && !in_num) {
                neg = true;
                in_num = true;
            }
            else if (c == ',') {
                if (in_num) col.push_back(neg ? -cur : cur);
                cur = 0; neg = false; in_num = false;
            }
            ++i;
        }
        if (in_num) col.push_back(neg ? -cur : cur);
        if (col.empty()) col.push_back(0);
        cols.push_back(std::move(col));

        if (i < n) ++i; // 跳过 ')'
    }

    if (cols.empty()) return {};

    std::size_t max_rows = 0;
    for (const auto& c : cols)
        if (c.size() > max_rows) max_rows = c.size();
    const int nr = static_cast<int>(max_rows);

    omegay::common::Matrix m(nr, static_cast<int>(cols.size()));
    for (int c = 0; c < static_cast<int>(cols.size()); ++c)
        for (int r = 0; r < static_cast<int>(cols[c].size()); ++r)
            m(r, c) = cols[c][r];
    return m;
}

[[nodiscard]] inline std::string formatUpmsText(const omegay::common::Matrix& m) {
    if (m.empty()) return {};
    std::string out;
    for (int c = 0; c < m.cols(); ++c) {
        int last_nonzero = 0;
        for (int r = m.rows() - 1; r >= 0; --r) {
            if (m(r, c) != 0) { last_nonzero = r; break; }
        }
        out += '(';
        for (int r = 0; r <= last_nonzero; ++r) {
            if (r > 0) out += ',';
            out += std::to_string(m(r, c));
        }
        out += ')';
    }
    return out;
}

inline std::string upmsExpandText(std::string_view text, int term) {
    const auto m = parseUpmsText(text);
    if (m.empty())
        return "解析失败，请确认你没有使用全角括号，逗号";

    const auto result = UPMSNotation::expand(m, term);
    const auto out = formatUpmsText(result);
    if (out.empty())
        return "后继形展开为空矩阵，或未能定位坏根";
    return out;
}

class UpmsNotation : public INotation {
public:
    explicit UpmsNotation(NotationMeta meta) : INotation(std::move(meta)) {
        caps_.text = &upmsExpandText;
        caps_.suffix = &UPMSNotation::suffix;
    }
    [[nodiscard]] bool has_text() const noexcept override { return true; }
    [[nodiscard]] std::string expand_text(
        std::string_view text, int term) const override {
        return upmsExpandText(text, term);
    }
    [[nodiscard]] std::string suffix() const override {
        return UPMSNotation::suffix();
    }

protected:
    // FS 截断：删除最后一列（省略尾随零文本）
    [[nodiscard]] std::string truncate_fs(std::string out, int term) const override {
        (void)term;
        auto m = parseUpmsText(out);
        if (m.cols() > 1) {
            m.resize_cols(m.cols() - 1);
            return formatUpmsText(m);
        }
        return out;
    }
};

// MrSS1.2.1：文本 + 序列双入口；FS 截断沿用默认（MrSS pop_back 语义）
class MrssNotation : public INotation {
public:
    explicit MrssNotation(NotationMeta meta) : INotation(std::move(meta)) {
        caps_.text = &Mrss121Notation::expand_string;
        caps_.expand_seq = &Mrss121Notation::expand;
        caps_.suffix = &Mrss121Notation::suffix;
    }
    [[nodiscard]] bool has_text() const noexcept override { return true; }
    [[nodiscard]] bool has_seq() const noexcept override { return true; }
    [[nodiscard]] std::string expand_text(
        std::string_view text, int term) const override {
        return Mrss121Notation::expand_string(text, term);
    }
    [[nodiscard]] std::vector<int> expand_seq(
        const std::vector<int>& seq, int term) const override {
        return Mrss121Notation::expand(seq, term);
    }
    [[nodiscard]] std::string suffix() const override {
        return Mrss121Notation::suffix();
    }
};

//==========================================================================
// 注册中心（单例）：唯一事实来源。GUI / CLI 壳层均从这里派生各自的表。
//==========================================================================
class NotationRegistry {
public:
    static NotationRegistry& instance() {
        static NotationRegistry reg;
        return reg;
    }

    void add(std::unique_ptr<INotation> n) { items_.push_back(std::move(n)); }

    [[nodiscard]] const INotation* by_id(std::string_view id) const {
        for (const auto& p : items_)
            if (p->id() == id) return p.get();
        return nullptr;
    }
    [[nodiscard]] const INotation* by_index(std::size_t i) const {
        return i < items_.size() ? items_[i].get() : nullptr;
    }
    [[nodiscard]] std::size_t size() const noexcept { return items_.size(); }
    [[nodiscard]] const std::vector<std::unique_ptr<INotation>>& all() const noexcept {
        return items_;
    }

private:
    std::vector<std::unique_ptr<INotation>> items_;
};

    //==========================================================================
    // 编译期注册表排序（2026-11）
    //   注册表不再按源代码手写顺序，而在编译期按 display_name 首字符的
    //   UTF-8 码位稳定升序：主键 = 首字符完整码位；同码位保持源代码顺序
    //   （稳定）。排序在编译期完成：kSortedOrder 是 constexpr，注册顺序
    //   编译期确定。
    //   实现：meta 放在普通 constexpr 数组 kMetaArray（字符串字面量指针在
    //   常量初始化中合法）；Reg<Factory, I> 只带工厂类型 + 索引，规避
    //   MSVC 对 string_view 作非类型模板参数的限制（C7658）。
    //==========================================================================

    // ---- 类型列表基础 ----
    template<class...> struct TypeList {};

    // 首字符 UTF-8 码位（constexpr；多字节按完整码位比较，即字节序=码位序）
    constexpr std::uint32_t first_utf8_codepoint(std::string_view s) noexcept {
        if (s.empty()) return 0;
        const auto b0 = static_cast<unsigned char>(s[0]);
        if (b0 < 0x80) return b0;
        if (b0 >= 0xC0 && b0 < 0xE0)
            return ((b0 & 0x1Fu) << 6u)
                 | (static_cast<unsigned char>(s[1]) & 0x3Fu);
        if (b0 >= 0xE0 && b0 < 0xF0)
            return ((b0 & 0x0Fu) << 12u)
                 | ((static_cast<unsigned char>(s[1]) & 0x3Fu) << 6u)
                 | (static_cast<unsigned char>(s[2]) & 0x3Fu);
        if (b0 >= 0xF0)
            return ((b0 & 0x07u) << 18u)
                 | ((static_cast<unsigned char>(s[1]) & 0x3Fu) << 12u)
                 | ((static_cast<unsigned char>(s[2]) & 0x3Fu) << 6u)
                 | (static_cast<unsigned char>(s[3]) & 0x3Fu);
        return 0;
    }

    // >>> 生成元数据数组开始（tools/gen_registry.ps1 输出） >>>
#ifndef EXPANDER_GEN_REGISTRY_META
inline constexpr std::array<NotationMeta, 114> kMetaArray = {
    NotationMeta{ .id = "empty", .display_name = "空记号", .definition_resource_id = IDR_DEF_EMPTY, .tags = { "测试", "基础" } },
    NotationMeta{ .id = "prss", .display_name = "PrSS", .definition_resource_id = IDR_DEF_PRSS, .tags = { "PrSS", "原始数列", "数列", "入门", "基础" } },
    NotationMeta{ .id = "pps", .display_name = "PPS", .definition_resource_id = IDR_DEF_PPS, .tags = { "PPS", "原始数列", "数列", "入门" } },
    NotationMeta{ .id = "pps4", .display_name = "PPS4", .definition_resource_id = IDR_DEF_PPS4, .tags = { "PPS", "原始数列", "4行", "数列" } },
    NotationMeta{ .id = "wpps4", .display_name = "Weak PPS4", .definition_resource_id = IDR_DEF_WPPS4, .tags = { "PPS", "弱", "4行", "数列" } },
    NotationMeta{ .id = "tpps4", .display_name = "Third PPS4", .definition_resource_id = IDR_DEF_TPPS4, .tags = { "PPS", "第三", "4行", "数列" } },
    NotationMeta{ .id = "ewpps4", .display_name = "Ex. Weak PPS4", .definition_resource_id = IDR_DEF_EWPPS4, .tags = { "PPS", "扩展弱", "4行", "数列" } },
    NotationMeta{ .id = "spps4", .display_name = "Second PPS4", .definition_resource_id = IDR_DEF_SPPS4, .tags = { "PPS", "第二", "4行", "数列" } },
    NotationMeta{ .id = "2-pps4", .display_name = "2-pps4", .definition_resource_id = IDR_DEF_2PPS4, .tags = { "PPS", "2行", "4行", "数列" } },
    NotationMeta{ .id = "omega-y-medium", .display_name = "ω-Y (medium)", .definition_resource_id = IDR_DEF_OMEGA_Y_MEDIUM, .tags = { "Y序列", "ω-Y", "中等", "数列" } },
    NotationMeta{ .id = "omega-y-strong", .display_name = "ω-Y (strong)", .definition_resource_id = IDR_DEF_OMEGA_Y_STRONG, .tags = { "Y序列", "ω-Y", "强", "数列" } },
    NotationMeta{ .id = "mrss121", .display_name = "MrSS1.2.1", .definition_resource_id = IDR_DEF_MRSS121, .tags = { "MrSS", "矩阵", "数列" } },
    NotationMeta{ .id = "bms-v1", .display_name = "BMS v1.0", .definition_resource_id = IDR_DEF_BMS_V1, .tags = { "BMS", "矩阵", "旧版" } },
    NotationMeta{ .id = "bms-v2", .display_name = "BMS v2.0", .definition_resource_id = IDR_DEF_BMS_V2, .tags = { "BMS", "矩阵" } },
    NotationMeta{ .id = "bms-v21", .display_name = "BMS v2.1", .definition_resource_id = IDR_DEF_BMS_V21, .tags = { "BMS", "矩阵" } },
    NotationMeta{ .id = "bms-v22", .display_name = "BMS v2.2", .definition_resource_id = IDR_DEF_BMS_V22, .tags = { "BMS", "矩阵" } },
    NotationMeta{ .id = "bms-v23", .display_name = "BMS v2.3", .definition_resource_id = IDR_DEF_BMS_V23, .tags = { "BMS", "矩阵" } },
    NotationMeta{ .id = "bms-v3", .display_name = "BMS v3.0", .definition_resource_id = IDR_DEF_BMS_V3, .tags = { "BMS", "矩阵" } },
    NotationMeta{ .id = "bms-v31", .display_name = "BMS v3.1", .definition_resource_id = IDR_DEF_BMS_V31, .tags = { "BMS", "矩阵" } },
    NotationMeta{ .id = "bms-v32", .display_name = "BMS v3.2", .definition_resource_id = IDR_DEF_BMS_V32, .tags = { "BMS", "矩阵" } },
    NotationMeta{ .id = "bms-v33", .display_name = "BMS v3.3", .definition_resource_id = IDR_DEF_BMS_V33, .tags = { "BMS", "矩阵" } },
    NotationMeta{ .id = "bms-v4", .display_name = "BMS v4.0", .definition_resource_id = IDR_DEF_BMS_V4, .tags = { "BMS", "矩阵", "最新" } },
    NotationMeta{ .id = "omega-y-sequence", .display_name = "ω-Y sequence", .definition_resource_id = IDR_DEF_OMEGA_Y_SEQUENCE, .tags = { "Y序列", "ω-Y", "数列" } },
    NotationMeta{ .id = "epsilon-y", .display_name = "ε-Y", .definition_resource_id = IDR_DEF_EPSILON_Y, .tags = { "Y序列", "ε-Y", "数列" } },
    NotationMeta{ .id = "upms", .display_name = "UPMS", .definition_resource_id = IDR_DEF_UPMS, .tags = { "UPMS", "矩阵", "文本" } },
    NotationMeta{ .id = "bsm", .display_name = "BSM", .definition_resource_id = IDR_DEF_BSM, .tags = { "BSM", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "bhm", .display_name = "BHM", .definition_resource_id = IDR_DEF_BHM, .tags = { "BHM", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "1-bm-bhm", .display_name = "1BM-BHM", .definition_resource_id = IDR_DEF_1BM_BHM, .tags = { "BM-BHM", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "2-bm-bhm", .display_name = "2BM-BHM", .definition_resource_id = IDR_DEF_2BM_BHM, .tags = { "BM-BHM", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "3-bm-bhm", .display_name = "3BM-BHM", .definition_resource_id = IDR_DEF_3BM_BHM, .tags = { "BM-BHM", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "cms", .display_name = "CMS", .definition_resource_id = IDR_DEF_CMS, .tags = { "CMS", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "dsm", .display_name = "DSM", .definition_resource_id = IDR_DEF_DSM, .tags = { "DSM", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "blm", .display_name = "BLM", .definition_resource_id = IDR_DEF_BLM, .tags = { "BLM", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "wmm", .display_name = "wMM", .definition_resource_id = IDR_DEF_WMM, .tags = { "wMM", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "gms-gbms-omega-p", .display_name = "GMS GBMS ω-P", .definition_resource_id = IDR_DEF_GMS_GBMS_OMEGA_P, .tags = { "GMS", "GBMS", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "gms-gbms-pqss", .display_name = "GMS GBMS pQSS", .definition_resource_id = IDR_DEF_GMS_GBMS_PQSS, .tags = { "GMS", "GBMS", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "gms-gbms-qss", .display_name = "GMS GBMS QSS", .definition_resource_id = IDR_DEF_GMS_GBMS_QSS, .tags = { "GMS", "GBMS", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "gms-gbms-full", .display_name = "GMS GBMS Full", .definition_resource_id = IDR_DEF_GMS_GBMS_FULL, .tags = { "GMS", "GBMS", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "gms-gbms-weird", .display_name = "GMS GBMS Weirdly Full", .definition_resource_id = IDR_DEF_GMS_GBMS_WEIRD, .tags = { "GMS", "GBMS", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "gms-upms-omega-p", .display_name = "GMS UPMS ω-P", .definition_resource_id = IDR_DEF_GMS_UPMS_OMEGA_P, .tags = { "GMS", "UPMS", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "gms-upms-pqss", .display_name = "GMS UPMS pQSS", .definition_resource_id = IDR_DEF_GMS_UPMS_PQSS, .tags = { "GMS", "UPMS", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "gms-upms-qss", .display_name = "GMS UPMS QSS", .definition_resource_id = IDR_DEF_GMS_UPMS_QSS, .tags = { "GMS", "UPMS", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "gms-upms-full", .display_name = "GMS UPMS Full", .definition_resource_id = IDR_DEF_GMS_UPMS_FULL, .tags = { "GMS", "UPMS", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "gms-upms-weird", .display_name = "GMS UPMS Weirdly Full", .definition_resource_id = IDR_DEF_GMS_UPMS_WEIRD, .tags = { "GMS", "UPMS", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "gms-lpms2-omega-p", .display_name = "GMS LPMS2 ω-P", .definition_resource_id = IDR_DEF_GMS_LPMS2_OMEGA_P, .tags = { "GMS", "LPMS2", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "gms-lpms2-pqss", .display_name = "GMS LPMS2 pQSS", .definition_resource_id = IDR_DEF_GMS_LPMS2_PQSS, .tags = { "GMS", "LPMS2", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "gms-lpms2-qss", .display_name = "GMS LPMS2 QSS", .definition_resource_id = IDR_DEF_GMS_LPMS2_QSS, .tags = { "GMS", "LPMS2", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "gms-lpms2-full", .display_name = "GMS LPMS2 Full", .definition_resource_id = IDR_DEF_GMS_LPMS2_FULL, .tags = { "GMS", "LPMS2", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "gms-lpms2-weird", .display_name = "GMS LPMS2 Weirdly Full", .definition_resource_id = IDR_DEF_GMS_LPMS2_WEIRD, .tags = { "GMS", "LPMS2", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "lpms", .display_name = "LPMS", .definition_resource_id = IDR_DEF_LPMS, .tags = { "LPMS", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "lptss", .display_name = "LPTSS", .definition_resource_id = IDR_DEF_LPTSS, .tags = { "LPTSS", "LPMS", "BM-like", "矩阵", "文本" } },
    NotationMeta{ .id = "tbm", .display_name = "TBM 超限巴斯丘矩阵", .definition_resource_id = IDR_DEF_TBM, .tags = { "TBM", "BM-like", "树形矩阵", "文本" } },
    NotationMeta{ .id = "tupms", .display_name = "TUPMS 超限 UPMS", .definition_resource_id = IDR_DEF_TUPMS, .tags = { "TUPMS", "BM-like", "树形矩阵", "文本" } },
    NotationMeta{ .id = "btbm", .display_name = "BTBM Bubby3 超限巴斯丘矩阵", .definition_resource_id = IDR_DEF_BTBM, .tags = { "BTBM", "BM-like", "分层树形矩阵", "文本" } },
    NotationMeta{ .id = "btbm-weak", .display_name = "BTBM-weak 弱版 Bubby3 超限巴斯丘矩阵", .definition_resource_id = IDR_DEF_BTBM_WEAK, .tags = { "BTBM-weak", "BM-like", "分层树形矩阵", "文本" } },
    NotationMeta{ .id = "bbm", .display_name = "BBM 分支巴斯丘矩阵", .definition_resource_id = IDR_DEF_BBM, .tags = { "BBM", "BM-like", "分层树形矩阵", "文本" } },
    NotationMeta{ .id = "-1y-(n+1)ss", .display_name = "(-1)Y-(n+1)SS 负一 Y 序列系统", .definition_resource_id = IDR_DEF_M1YNSS, .tags = { "Minus1", "BM-like", "参数化", "文本" }, .param_min=0, .param_max=25, .param_default=0, .param_label="n (0-25):" },
    NotationMeta{ .id = "t--1y-(n+1)ss", .display_name = "T(n+1)SS 超限负一 Y 序列系统", .definition_resource_id = IDR_DEF_TM1YNSS, .tags = { "Minus1", "BM-like", "参数化", "递归嵌套", "文本" }, .param_min=0, .param_max=25, .param_default=0, .param_label="n (0-25):" },
    NotationMeta{ .id = "bt1ynss", .display_name = "BT*(n+1)SS v1 超限星负一 Y 序列系统", .definition_resource_id = IDR_DEF_BT1YNSS, .tags = { "Minus1", "BM-like", "参数化", "递归嵌套", "文本" }, .param_min=1, .param_max=25, .param_default=1, .param_label="n (1-25):" },
    NotationMeta{ .id = "bt2ynss", .display_name = "strong BT*(n+1)SS v2 超限星负一 Y 序列系统", .definition_resource_id = IDR_DEF_BT2YNSS, .tags = { "Minus1", "BM-like", "参数化", "递归嵌套", "文本" }, .param_min=1, .param_max=25, .param_default=1, .param_label="n (1-25):" },
    NotationMeta{ .id = "bt3ynss", .display_name = "weak BT*(n+1)SS v3 超限星负一 Y 序列系统", .definition_resource_id = IDR_DEF_BT3YNSS, .tags = { "Minus1", "BM-like", "参数化", "递归嵌套", "文本" }, .param_min=1, .param_max=25, .param_default=1, .param_label="n (1-25):" },
    NotationMeta{ .id = "btlynss", .display_name = "BTL(n+1)SS 带标记超限星负一 Y 序列系统", .definition_resource_id = IDR_DEF_BTLYNSS, .tags = { "Minus1", "BM-like", "参数化", "递归嵌套", "文本" }, .param_min=1, .param_max=25, .param_default=1, .param_label="n (1-25):" },
    NotationMeta{ .id = "abs-bm1ynss", .display_name = "AbsB(n+1)SS 绝对分支负一 Y 序列系统", .definition_resource_id = IDR_DEF_ABS_BM1YNSS, .tags = { "Minus1", "BM-like", "参数化", "递归嵌套", "文本" }, .param_min=0, .param_max=25, .param_default=0, .param_label="n (0-25):" },
    NotationMeta{ .id = "rel-bm1ynss", .display_name = "RelB(n+1)SS 相对分支负一 Y 序列系统", .definition_resource_id = IDR_DEF_REL_BM1YNSS, .tags = { "Minus1", "BM-like", "参数化", "递归嵌套", "文本" }, .param_min=0, .param_max=25, .param_default=0, .param_label="n (0-25):" },
    NotationMeta{ .id = "g-y", .display_name = "G-Y", .definition_resource_id = IDR_DEF_GY, .tags = { "Y序列", "G-Y", "数列" } },
    NotationMeta{ .id = "xy-v1", .display_name = "X-Y V1", .definition_resource_id = IDR_DEF_XY_V1, .tags = { "Y序列", "X-Y", "数列" } },
    NotationMeta{ .id = "xy-v2", .display_name = "X-Y V2", .definition_resource_id = IDR_DEF_XY_V2, .tags = { "Y序列", "X-Y", "数列" } },
    NotationMeta{ .id = "xy-v3", .display_name = "X-Y V3", .definition_resource_id = IDR_DEF_XY_V3, .tags = { "Y序列", "X-Y", "数列" } },
    NotationMeta{ .id = "den1", .display_name = "DEN1", .definition_resource_id = IDR_DEF_DEN1, .tags = { "DEN", "文本", "旧版" } },
    NotationMeta{ .id = "blp", .display_name = "blp", .definition_resource_id = IDR_DEF_BLP, .tags = { "blp", "文本" } },
    NotationMeta{ .id = "iblp", .display_name = "iblp", .definition_resource_id = IDR_DEF_IBLP, .tags = { "iblp", "文本", "逆" } },
    NotationMeta{ .id = "ton-m", .display_name = "TON", .definition_resource_id = IDR_DEF_TON_M, .tags = { "TON", "序数记号", "文本" } },
    NotationMeta{ .id = "ton-i", .display_name = "TON_I", .definition_resource_id = IDR_DEF_TON_I, .tags = { "TON", "n-built", "文本" } },
    NotationMeta{ .id = "ton-dr", .display_name = "TON_DoR", .definition_resource_id = IDR_DEF_TON_DR, .tags = { "TON", "反射度", "文本" } },
    NotationMeta{ .id = "ton-drc", .display_name = "TON_DRC", .definition_resource_id = IDR_DEF_TON_DRC, .tags = { "TON", "反射配置", "文本" } },
    NotationMeta{ .id = "ton-drp", .display_name = "TON_DRP", .definition_resource_id = IDR_DEF_TON_DRP, .tags = { "TON", "passthrough", "文本" } },
    NotationMeta{ .id = "ton-drpc", .display_name = "TON_DRPC", .definition_resource_id = IDR_DEF_TON_DRPC, .tags = { "TON", "反射配置", "passthrough", "文本" } },
    NotationMeta{ .id = "ton-ibp", .display_name = "TON_IBP", .definition_resource_id = IDR_DEF_TON_IBP, .tags = { "TON", "n-built", "文本" } },
    NotationMeta{ .id = "ton-mc", .display_name = "TON_MC", .definition_resource_id = IDR_DEF_TON_MC, .tags = { "TON", "反射配置", "文本" } },
    NotationMeta{ .id = "ton-mpc", .display_name = "TON_MPC", .definition_resource_id = IDR_DEF_TON_MPC, .tags = { "TON", "反射配置", "passthrough", "文本" } },
    NotationMeta{ .id = "e0mn", .display_name = "e0MN", .definition_resource_id = IDR_DEF_E0MN, .tags = { "MN", "文本" } },
    NotationMeta{ .id = "strong-e0mn", .display_name = "strong e0MN", .definition_resource_id = IDR_DEF_STRONG_E0MN, .tags = { "MN", "文本" } },
    NotationMeta{ .id = "omega-mn", .display_name = "ωMN", .definition_resource_id = IDR_DEF_OMEGA_MN, .tags = { "MN", "文本" } },
    NotationMeta{ .id = "pcf-omega-mn", .display_name = "PCF's ωMN", .definition_resource_id = IDR_DEF_PCF_OMEGA_MN, .tags = { "MN", "文本" } },
    NotationMeta{ .id = "t-omega-mn", .display_name = "TωMN", .definition_resource_id = IDR_DEF_T_OMEGA_MN, .tags = { "MN", "文本" } },
    NotationMeta{ .id = "1-mn", .display_name = "1MN", .definition_resource_id = IDR_DEF_1MN, .tags = { "MN", "文本" } },
    NotationMeta{ .id = "2-mn", .display_name = "2MN", .definition_resource_id = IDR_DEF_2MN, .tags = { "MN", "文本" } },
    NotationMeta{ .id = "3-mn", .display_name = "3MN", .definition_resource_id = IDR_DEF_3MN, .tags = { "MN", "文本" } },
    NotationMeta{ .id = "s-omega2-mn", .display_name = "Sω2MN", .definition_resource_id = IDR_DEF_S_OMEGA2_MN, .tags = { "MN", "文本" } },
    NotationMeta{ .id = "sa-omega2-mn", .display_name = "SAω2MN", .definition_resource_id = IDR_DEF_SA_OMEGA2_MN, .tags = { "MN", "文本" } },
    NotationMeta{ .id = "s-omega-pow-omega-mn", .display_name = "Sω^ωMN", .definition_resource_id = IDR_DEF_S_OMEGA_POW_OMEGA_MN, .tags = { "MN", "文本" } },
    NotationMeta{ .id = "up1mn", .display_name = "UP1MN", .definition_resource_id = IDR_DEF_UP1MN, .tags = { "MN", "UPMN", "文本" } },
    NotationMeta{ .id = "up2mn-v1a", .display_name = "UP2MN v1A", .definition_resource_id = IDR_DEF_UP2MN_V1A, .tags = { "MN", "UPMN", "文本" } },
    NotationMeta{ .id = "up2mn-v1b", .display_name = "UP2MN v1B", .definition_resource_id = IDR_DEF_UP2MN_V1B, .tags = { "MN", "UPMN", "文本" } },
    NotationMeta{ .id = "up2mn-v1b-plus", .display_name = "UP2MN v1B+", .definition_resource_id = IDR_DEF_UP2MN_V1B_PLUS, .tags = { "MN", "UPMN", "文本" } },
    NotationMeta{ .id = "up1dbms", .display_name = "UP1DBMS", .definition_resource_id = IDR_DEF_UP1DBMS, .tags = { "MN", "UPMN", "DBMS", "文本" } },
    NotationMeta{ .id = "up2dbms-v1", .display_name = "UP2DBMS v1", .definition_resource_id = IDR_DEF_UP2DBMS_V1, .tags = { "MN", "UPMN", "DBMS", "文本" } },
    NotationMeta{ .id = "up2dbms-v1b-plus", .display_name = "UP2DBMS v1B+", .definition_resource_id = IDR_DEF_UP2DBMS_V1B_PLUS, .tags = { "MN", "UPMN", "DBMS", "文本" } },
    NotationMeta{ .id = "up2dbms-v1c", .display_name = "UP2DBMS v1C", .definition_resource_id = IDR_DEF_UP2DBMS_V1C, .tags = { "MN", "UPMN", "DBMS", "文本" } },
    NotationMeta{ .id = "up2dbms-v2", .display_name = "UP2DBMS v2", .definition_resource_id = IDR_DEF_UP2DBMS_V2, .tags = { "MN", "UPMN", "DBMS", "文本" } },
    NotationMeta{ .id = "s1dbms", .display_name = "S1DBMS", .definition_resource_id = IDR_DEF_S1DBMS, .tags = { "MN", "SDBMS", "文本" } },
    NotationMeta{ .id = "s2dbms", .display_name = "S2DBMS", .definition_resource_id = IDR_DEF_S2DBMS, .tags = { "MN", "SDBMS", "文本" } },
    NotationMeta{ .id = "s-omega-dbms", .display_name = "SωDBMS v1", .definition_resource_id = IDR_DEF_S_OMEGA_DBMS, .tags = { "MN", "SDBMS", "文本" } },
    NotationMeta{ .id = "s-omega-dbms-v2", .display_name = "SωDBMS v2", .definition_resource_id = IDR_DEF_S_OMEGA_DBMS_V2, .tags = { "MN", "SDBMS", "文本" } },
    NotationMeta{ .id = "s-omega-dbms-v3", .display_name = "SωDBMS v3", .definition_resource_id = IDR_DEF_S_OMEGA_DBMS_V3, .tags = { "MN", "SDBMS", "文本" } },
    NotationMeta{ .id = "s-omega-p1-dbms", .display_name = "Sω+1DBMS", .definition_resource_id = IDR_DEF_S_OMEGA_P1_DBMS, .tags = { "MN", "SDBMS", "文本" } },
    NotationMeta{ .id = "s-omega2-dbms", .display_name = "Sω2DBMS", .definition_resource_id = IDR_DEF_S_OMEGA2_DBMS, .tags = { "MN", "SDBMS", "文本" } },
    NotationMeta{ .id = "s-omega-square-dbms", .display_name = "Sω^2DBMS", .definition_resource_id = IDR_DEF_S_OMEGA_SQUARE_DBMS, .tags = { "MN", "SDBMS", "文本" } },
    NotationMeta{ .id = "ts-2-dbms", .display_name = "TS2DBMS", .definition_resource_id = IDR_DEF_TS2DBMS, .tags = { "MN", "SDBMS", "文本" } },
    NotationMeta{ .id = "ts-omega-dbms", .display_name = "TSωDBMS", .definition_resource_id = IDR_DEF_TS_OMEGA_DBMS, .tags = { "MN", "SDBMS", "文本" } },
    NotationMeta{ .id = "a-omega2-mn-2", .display_name = "Aω2MN2", .definition_resource_id = IDR_DEF_A_OMEGA2_MN_2, .tags = { "MN", "文本" } },
    NotationMeta{ .id = "weak-a-omega2-mn-2", .display_name = "wAω2MN2", .definition_resource_id = IDR_DEF_WEAK_A_OMEGA2_MN_2, .tags = { "MN", "文本" } },
    NotationMeta{ .id = "a-omega2-mn-3", .display_name = "Aω2MN3", .definition_resource_id = IDR_DEF_A_OMEGA2_MN_3, .tags = { "MN", "文本" } },
    NotationMeta{ .id = "weak-a-omega2-mn-3", .display_name = "wAω2MN3", .definition_resource_id = IDR_DEF_WEAK_A_OMEGA2_MN_3, .tags = { "MN", "文本" } },
};
#endif
    // <<< 生成元数据数组结束 <<<

    // 注册条目：Factory（编译期构造适配器）+ 索引 I（运行时取 kMetaArray 的 meta）。
    template<class Factory, std::size_t I>
    struct Reg {
        static std::unique_ptr<INotation> make() {
            return Factory{}(kMetaArray[I]);
        }
    };

    // ---- 适配器工厂（按能力选择包装类型，与旧表字段绑定一致） ----
    template<class T> struct MakeSeq {
        std::unique_ptr<INotation> operator()(const NotationMeta& m) const {
            return std::make_unique<SequenceNotation<T>>(m);
        }
    };
    template<class T> struct MakeText {
        std::unique_ptr<INotation> operator()(const NotationMeta& m) const {
            return std::make_unique<TextNotation<T>>(m);
        }
    };
    template<class T> struct MakeTextAlter {
        std::unique_ptr<INotation> operator()(const NotationMeta& m) const {
            return std::make_unique<TextAlterNotation<T>>(m);
        }
    };
    template<class T> struct MakeParametric {
        std::unique_ptr<INotation> operator()(const NotationMeta& m) const {
            return std::make_unique<ParametricNotation<T>>(m);
        }
    };
    template<class T> struct MakeParametricAlter {
        std::unique_ptr<INotation> operator()(const NotationMeta& m) const {
            return std::make_unique<ParametricAlterNotation<T>>(m);
        }
    };
    template<BMSFamilyNotation::Version V> struct MakeBms {
        std::unique_ptr<INotation> operator()(const NotationMeta& m) const {
            return std::make_unique<BmsNotation>(V, m);
        }
    };
    struct MakeMrss {
        std::unique_ptr<INotation> operator()(const NotationMeta& m) const {
            return std::make_unique<MrssNotation>(m);
        }
    };
    struct MakeUpms {
        std::unique_ptr<INotation> operator()(const NotationMeta& m) const {
            return std::make_unique<UpmsNotation>(m);
        }
    };

    // >>> 生成类型列表开始（tools/gen_registry.ps1 输出：AllNotations 定义） >>>
using AllNotations = TypeList<
    Reg<MakeSeq<notation::EmptyNotation>, 0>,
    Reg<MakeSeq<notation::PrSSNotation>, 1>,
    Reg<MakeSeq<notation::PPSNotation>, 2>,
    Reg<MakeSeq<notation::PPS4Notation>, 3>,
    Reg<MakeSeq<notation::WPPS4Notation>, 4>,
    Reg<MakeSeq<notation::TPPS4Notation>, 5>,
    Reg<MakeSeq<notation::EWPPS4Notation>, 6>,
    Reg<MakeSeq<notation::SecondPPS4Notation>, 7>,
    Reg<MakeSeq<notation::PPS2Notation>, 8>,
    Reg<MakeSeq<notation::OmegaYMediumNotation>, 9>,
    Reg<MakeSeq<notation::OmegaYStrongNotation>, 10>,
    Reg<MakeMrss, 11>,
    Reg<MakeBms<notation::BMSFamilyNotation::Version::V1>, 12>,
    Reg<MakeBms<notation::BMSFamilyNotation::Version::V2>, 13>,
    Reg<MakeBms<notation::BMSFamilyNotation::Version::V21>, 14>,
    Reg<MakeBms<notation::BMSFamilyNotation::Version::V22>, 15>,
    Reg<MakeBms<notation::BMSFamilyNotation::Version::V23>, 16>,
    Reg<MakeBms<notation::BMSFamilyNotation::Version::V3>, 17>,
    Reg<MakeBms<notation::BMSFamilyNotation::Version::V31>, 18>,
    Reg<MakeBms<notation::BMSFamilyNotation::Version::V32>, 19>,
    Reg<MakeBms<notation::BMSFamilyNotation::Version::V33>, 20>,
    Reg<MakeBms<notation::BMSFamilyNotation::Version::V4>, 21>,
    Reg<MakeSeq<notation::OmegaYNotation>, 22>,
    Reg<MakeSeq<notation::EpsilonYNotation>, 23>,
    Reg<MakeUpms, 24>,
    Reg<MakeTextAlter<notation::BSMNotation>, 25>,
    Reg<MakeTextAlter<notation::BHMNotation>, 26>,
    Reg<MakeTextAlter<notation::BM_BHMNotation<1>>, 27>,
    Reg<MakeTextAlter<notation::BM_BHMNotation<2>>, 28>,
    Reg<MakeTextAlter<notation::BM_BHMNotation<3>>, 29>,
    Reg<MakeText<notation::CMSNotation>, 30>,
    Reg<MakeTextAlter<notation::DSMNotation>, 31>,
    Reg<MakeText<notation::BLMNotation>, 32>,
    Reg<MakeTextAlter<notation::WMMNotation>, 33>,
    Reg<MakeText<notation::GbmsOmegaP>, 34>,
    Reg<MakeText<notation::GbmsPQSS>, 35>,
    Reg<MakeText<notation::GbmsQSS>, 36>,
    Reg<MakeText<notation::GbmsFull>, 37>,
    Reg<MakeText<notation::GbmsWeirdlyFull>, 38>,
    Reg<MakeText<notation::UpmsOmegaP>, 39>,
    Reg<MakeText<notation::UpmsPQSS>, 40>,
    Reg<MakeText<notation::UpmsQSS>, 41>,
    Reg<MakeText<notation::UpmsFull>, 42>,
    Reg<MakeText<notation::UpmsWeirdlyFull>, 43>,
    Reg<MakeText<notation::Lpms2OmegaP>, 44>,
    Reg<MakeText<notation::Lpms2PQSS>, 45>,
    Reg<MakeText<notation::Lpms2QSS>, 46>,
    Reg<MakeText<notation::Lpms2Full>, 47>,
    Reg<MakeText<notation::Lpms2WeirdlyFull>, 48>,
    Reg<MakeText<notation::LPMSNotation>, 49>,
    Reg<MakeText<notation::LPTSSNotation>, 50>,
    Reg<MakeText<notation::TBMNotation>, 51>,
    Reg<MakeText<notation::TUPMSNotation>, 52>,
    Reg<MakeText<notation::BTBMNotation>, 53>,
    Reg<MakeText<notation::BTBMWeakNotation>, 54>,
    Reg<MakeText<notation::BBMNotation>, 55>,
    Reg<MakeParametric<notation::M1YnssNotation>, 56>,
    Reg<MakeParametric<notation::TM1YnssNotation>, 57>,
    Reg<MakeParametric<notation::BT1Notation>, 58>,
    Reg<MakeParametric<notation::BT2Notation>, 59>,
    Reg<MakeParametricAlter<notation::BT3Notation>, 60>,
    Reg<MakeParametric<notation::BTlNotation>, 61>,
    Reg<MakeParametric<notation::AbsBM1Notation>, 62>,
    Reg<MakeParametric<notation::RelBM1Notation>, 63>,
    Reg<MakeSeq<notation::GYNotation>, 64>,
    Reg<MakeSeq<notation::XYV1Notation>, 65>,
    Reg<MakeSeq<notation::XYV2Notation>, 66>,
    Reg<MakeSeq<notation::XYV3Notation>, 67>,
    Reg<MakeTextAlter<notation::Den1Notation>, 68>,
    Reg<MakeTextAlter<notation::BlpNotation>, 69>,
    Reg<MakeTextAlter<notation::IblpNotation>, 70>,
    Reg<MakeTextAlter<notation::TonNotation>, 71>,
    Reg<MakeTextAlter<notation::TonINotation>, 72>,
    Reg<MakeTextAlter<notation::TonDoRNotation>, 73>,
    Reg<MakeTextAlter<notation::TonDRCNotation>, 74>,
    Reg<MakeTextAlter<notation::TonDRPNotation>, 75>,
    Reg<MakeTextAlter<notation::TonDRPCNotation>, 76>,
    Reg<MakeTextAlter<notation::TonIbpNotation>, 77>,
    Reg<MakeTextAlter<notation::TonMcNotation>, 78>,
    Reg<MakeTextAlter<notation::TonMpcNotation>, 79>,
    Reg<MakeTextAlter<notation::E0MNNotation>, 80>,
    Reg<MakeTextAlter<notation::StrongE0MNNotation>, 81>,
    Reg<MakeTextAlter<notation::OmegaMNNotation>, 82>,
    Reg<MakeTextAlter<notation::PcfOmegaMNNotation>, 83>,
    Reg<MakeTextAlter<notation::TOmegaMNNotation>, 84>,
    Reg<MakeTextAlter<notation::NMnNotation<1>>, 85>,
    Reg<MakeTextAlter<notation::NMnNotation<2>>, 86>,
    Reg<MakeTextAlter<notation::NMnNotation<3>>, 87>,
    Reg<MakeTextAlter<notation::SOmega2MNNotation>, 88>,
    Reg<MakeTextAlter<notation::SAOmega2MNNotation>, 89>,
    Reg<MakeTextAlter<notation::SOmegaPowOmegaMNNotation>, 90>,
    Reg<MakeTextAlter<notation::UP1MNNotation>, 91>,
    Reg<MakeTextAlter<notation::UP2MNV1ANotation>, 92>,
    Reg<MakeTextAlter<notation::UP2MNV1BNotation>, 93>,
    Reg<MakeTextAlter<notation::UP2MNV1BPlusNotation>, 94>,
    Reg<MakeTextAlter<notation::UP1DBMSNotation>, 95>,
    Reg<MakeTextAlter<notation::UP2DBMSV1Notation>, 96>,
    Reg<MakeTextAlter<notation::UP2DBMSV1BPlusNotation>, 97>,
    Reg<MakeTextAlter<notation::UP2DBMSV1CNotation>, 98>,
    Reg<MakeTextAlter<notation::Up2DbmsV2Notation>, 99>,
    Reg<MakeTextAlter<notation::S1DBMSNotation>, 100>,
    Reg<MakeTextAlter<notation::S2DBMSNotation>, 101>,
    Reg<MakeTextAlter<notation::SOmegaDBMSV1Notation>, 102>,
    Reg<MakeTextAlter<notation::SOmegaDBMSV2Notation>, 103>,
    Reg<MakeTextAlter<notation::SOmegaDBMSV3Notation>, 104>,
    Reg<MakeTextAlter<notation::SOmegaP1DBMSNotation>, 105>,
    Reg<MakeTextAlter<notation::SOmega2DBMSNotation>, 106>,
    Reg<MakeTextAlter<notation::SOmegaSquareDBMSNotation>, 107>,
    Reg<MakeTextAlter<notation::Ts2DbmsNotation>, 108>,
    Reg<MakeTextAlter<notation::TsOmegaDbmsNotation>, 109>,
    Reg<MakeTextAlter<notation::Aw2MN2Notation>, 110>,
    Reg<MakeTextAlter<notation::WAw2MN2Notation>, 111>,
    Reg<MakeTextAlter<notation::Aw2MN3Notation>, 112>,
    Reg<MakeTextAlter<notation::WAw2MN3Notation>, 113>
>;
    // <<< 生成类型列表结束 <<<

    inline constexpr std::size_t kNotationCount = kMetaArray.size();

    // 编译期排序索引：按 display_name 首字符 UTF-8 码位稳定升序
    constexpr std::array<std::size_t, kNotationCount> sorted_order() {
        std::array<std::size_t, kNotationCount> order{};
        for (std::size_t i = 0; i < kNotationCount; ++i) order[i] = i;
        std::sort(order.begin(), order.end(), [](std::size_t i, std::size_t j) {
            const auto a = first_utf8_codepoint(kMetaArray[i].display_name);
            const auto b = first_utf8_codepoint(kMetaArray[j].display_name);
            if (a != b) return a < b;
            return i < j;  // 稳定：同码位保持源代码顺序
        });
        return order;
    }
    inline constexpr std::array<std::size_t, kNotationCount> kSortedOrder = sorted_order();

    // 取类型列表第 K 个元素
    template<class TL, std::size_t K> struct Get;
    template<class T0, class... Ts> struct Get<TypeList<T0, Ts...>, 0> { using type = T0; };
    template<class T0, class... Ts, std::size_t K>
    struct Get<TypeList<T0, Ts...>, K> {
        using type = typename Get<TypeList<Ts...>, K - 1>::type;
    };

    // 按编译期排序索引注册（递归，到 kNotationCount 终止）
    template<std::size_t I> struct RegStep {
        static void apply() {
            using E = typename Get<AllNotations, kSortedOrder[I]>::type;
            NotationRegistry::instance().add(E::make());
            RegStep<I + 1>::apply();
        }
    };
    template<> struct RegStep<kNotationCount> { static void apply() {} };

    // 注册全部记号（幂等；顺序由编译期按 display_name 首码位排序确定）。
    inline void register_all_notations() {
        auto& r = NotationRegistry::instance();
        if (!r.all().empty()) return;
        RegStep<0>::apply();
    }

// 取已注册记号列表（懒初始化；GUI/CLI 共用）。
[[nodiscard]] inline const std::vector<std::unique_ptr<INotation>>&
registered_notations() {
    register_all_notations();
    return NotationRegistry::instance().all();
}

} // namespace omegay::notation

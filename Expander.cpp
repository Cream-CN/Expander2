//Copyright(c) Cream-CN 2026
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>

#pragma comment(lib, "comctl32.lib")

#include "Header/common/utf8.hpp"
#include "Header/common/sequence.hpp"
#include "Header/common/Matrix.hpp"
#include "Header/core/arena.hpp"
#include "Header/core/entry.hpp"
#include "Header/notation/empty.hpp"
#include "Header/notation/PPS-family.hpp"
#include "Header/notation/Omega-YMagma.hpp"
#include "Header/notation/mrss121.hpp"
#include "Header/notation/omega_y.hpp"
#include "Header/notation/epsilon-y.hpp"
#include "Header/notation/BMSFamily.hpp"
#include "Header/notation/upms.hpp"
#include "Header/notation/bsm.hpp"
#include "Header/notation/bhm.hpp"
#include "Header/notation/cms.hpp"
#include "Header/notation/dsm.hpp"
#include "Header/notation/blm.hpp"
#include "Header/notation/gy.hpp"
#include "Header/notation/xy-v1.hpp"
#include "Header/notation/xy-v2.hpp"
#include "Header/notation/xy-v3.hpp"
#include "Header/notation/den1.hpp"
#include "Header/notation/blp.hpp"
#include "Header/notation/iblp.hpp"
#include "Header/notation/ton.hpp"
#include "Header/notation/ton_i.hpp"
#include "Header/notation/ton_dor.hpp"
#include "Header/notation/ton_drc.hpp"
#include "Header/notation/ton_drp.hpp"
#include "Header/notation/ton_drpc.hpp"
#include "Header/notation/ton_ibp.hpp"
#include "Header/notation/ton_mc.hpp"
#include "Header/notation/ton_mpc.hpp"
#include "Header/notation/e0mn.hpp"
#include "Header/notation/strong_e0mn.hpp"
#include "Header/notation/omega_mn.hpp"
#include "Header/notation/pcf_omega_mn.hpp"
#include "Header/notation/t_omega_mn.hpp"
#include "Header/notation/n_mn.hpp"
#include "Header/notation/s_omega2_mn.hpp"
#include "Header/notation/sa_omega2_mn.hpp"
#include "Header/notation/s_omega_pow_omega_mn.hpp"
#include "Header/notation/s1dbms.hpp"
#include "Header/notation/s2dbms.hpp"
#include "Header/notation/s_omega_dbms_v1.hpp"
#include "Header/notation/s_omega_dbms_v2.hpp"
#include "Header/notation/s_omega_dbms_v3.hpp"
#include "Header/notation/s_omega_p1_dbms.hpp"
#include "Header/notation/s_omega2_dbms.hpp"
#include "Header/notation/s_omega_square_dbms.hpp"
#include "Header/notation/up1mn.hpp"
#include "Header/notation/up1dbms.hpp"
#include "Header/notation/up2mn_v1a.hpp"
#include "Header/notation/up2mn_v1b.hpp"
#include "Header/notation/up2mn_v1b_plus.hpp"
#include "Header/notation/up2dbms_v1.hpp"
#include "Header/notation/up2dbms_v1b_plus.hpp"
#include "Header/notation/up2dbms_v1c.hpp"
#include "Header/notation/up2dbms_v2.hpp"
#include "Header/notation/aw2mn2.hpp"
#include "Header/notation/aw2mn3.hpp"
#include "Header/notation/waw2mn2.hpp"
#include "Header/notation/waw2mn3.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <algorithm>
#include <cwctype>
#include <cstdint>
#include "Resource.h"
#include "Definitions.h"   // 记号定义文本由标头编译进程序，绕过 rc.exe

using namespace omegay;
using namespace omegay::common;
using namespace omegay::notation;

namespace {
    using Bms = omegay::notation::BMSFamilyNotation;

    /*constexpr int IDM_ABOUT = 1001;
    constexpr int IDM_EXIT = 1002;*/
    constexpr int IDM_HELP = 1003;
    constexpr int IDM_LEGAL = 1004;
    constexpr int IDM_FS = 101;
    constexpr int IDM_FSALTER = 102;
    constexpr int IDM_ENTRY_DEMO = 103;
    constexpr int IDC_NOTATION_BTN = 2100;   // 主窗口“选择记号…”按钮
    constexpr int IDM_DEFINITION_PICK = 4000; // “定义(&D)…”菜单：打开记号选择窗体
    constexpr wchar_t kDefWindowClass[] = L"OmegaYDefPopup";
    constexpr int IDC_POPUP_DEF_EDIT = 5001;
    constexpr int IDC_POPUP_DEF_CLOSE = 5002;
    constexpr wchar_t kLegalWindowClass[] = L"OmegaYLegal";
    constexpr int IDC_LEGAL_EDIT = 6001;
    HWND g_hLegalWindow = nullptr;

    // ---- 记号选择窗体 ----
    constexpr wchar_t kPickerWindowClass[] = L"OmegaYPicker";
    constexpr int IDC_PICKER_SEARCH = 7101;
    constexpr int IDC_PICKER_LIST = 7102;
    constexpr int IDC_PICKER_STATUS = 7103;

    struct NotationEntry {
        const wchar_t* display_name;
        const char* id;
        std::vector<int>(*expand)(const std::vector<int>&, int);
        std::string(*suffix)();
        int definition_resource_id;

        // 可选的文本展开入口。若不为空，UI 会优先用它处理输入文本，
        // 签名约定：std::string expand_string(std::string_view, int)
        std::string(*expand_text)(std::string_view, int) = nullptr;

        // 可选的 FS_alter（较长）文本展开入口。
        // 非空时，FS 使用 expand_text、FS_alter 使用本入口，两者均由记号自身实现，
        // UI 不再对 FS 结果做通用截断（DEN1 / blp / iblp 即属此类）。
        std::string(*expand_text_alter)(std::string_view, int) = nullptr;

        // 记号标签（用于选择窗体的搜索与展示）。最多 16 个，以 nullptr 结尾；
        // 未用的槽位保持 nullptr。可在下表按需增删。
        std::array<const wchar_t*, 16> tags{};
    };

    std::string bmsExpandTextWith(
        Bms::Version ver, std::string_view text, int term)
    {
        long parsedTerm = term;                 // parse_matrix 需要 long&
        omegay::common::Matrix m =
            omegay::common::parse_matrix(text, parsedTerm);

        if (m.empty())
            return "解析失败，请确认你没有使用全角括号，逗号";

        if (!omegay::notation::BMSFamilyNotation::is_standard(m, ver))
            return "输入的矩阵为非标准表达式";

        const int t = parsedTerm < 1 ? 1 : static_cast<int>(parsedTerm);

        auto result = omegay::notation::BMSFamilyNotation::expand(m, t, ver);
        return omegay::common::matrix_to_string(result);
    }
    std::string bmsExpandV1(std::string_view t, int n) { return bmsExpandTextWith(Bms::Version::V1, t, n); }
    std::string bmsExpandV2(std::string_view t, int n) { return bmsExpandTextWith(Bms::Version::V2, t, n); }
    std::string bmsExpandV21(std::string_view t, int n) { return bmsExpandTextWith(Bms::Version::V21, t, n); }
    std::string bmsExpandV22(std::string_view t, int n) { return bmsExpandTextWith(Bms::Version::V22, t, n); }
    std::string bmsExpandV23(std::string_view t, int n) { return bmsExpandTextWith(Bms::Version::V23, t, n); }
    std::string bmsExpandV3(std::string_view t, int n) { return bmsExpandTextWith(Bms::Version::V3, t, n); }
    std::string bmsExpandV31(std::string_view t, int n) { return bmsExpandTextWith(Bms::Version::V31, t, n); }
    std::string bmsExpandV32(std::string_view t, int n) { return bmsExpandTextWith(Bms::Version::V32, t, n); }
    std::string bmsExpandV33(std::string_view t, int n) { return bmsExpandTextWith(Bms::Version::V33, t, n); }
    std::string bmsExpandV4(std::string_view t, int n) { return bmsExpandTextWith(Bms::Version::V4, t, n); }
    inline bool isBmsTextEntry(const NotationEntry& e) {
        return e.expand_text == &bmsExpandV1
            || e.expand_text == &bmsExpandV2
            || e.expand_text == &bmsExpandV21
            || e.expand_text == &bmsExpandV22
            || e.expand_text == &bmsExpandV23
            || e.expand_text == &bmsExpandV3
            || e.expand_text == &bmsExpandV31
            || e.expand_text == &bmsExpandV32
            || e.expand_text == &bmsExpandV33
            || e.expand_text == &bmsExpandV4;
    }

    // ---- UPMS 文本 ⇄ Matrix（省略尾随零的写法） ----
    [[nodiscard]] omegay::common::Matrix
        parseUpmsText(std::string_view s) {
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

    [[nodiscard]] std::string
        formatUpmsText(const omegay::common::Matrix& m) {
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

    std::string upmsExpandText(std::string_view text, int term) {
        const auto m = parseUpmsText(text);
        if (m.empty())
            return "解析失败，请确认你没有使用全角括号，逗号";

        const auto result = omegay::notation::UPMSNotation::expand(m, term);
        const auto out = formatUpmsText(result);
        if (out.empty())
            return "后继形展开为空矩阵，或未能定位坏根";
        return out;
    }

    [[nodiscard]] const std::vector<NotationEntry>& notationTable() {
        static const std::vector<NotationEntry> table = {
            { .display_name = L"空记号", .id = "empty",
              .expand = &notation::EmptyNotation::expand,
              .suffix = &notation::EmptyNotation::suffix,
              .definition_resource_id = IDR_DEF_EMPTY,
              .tags = { L"测试", L"基础" } },

            { .display_name = L"PPS", .id = "pps",
              .expand = &notation::PPSNotation::expand,
              .suffix = &notation::PPSNotation::suffix,
              .definition_resource_id = IDR_DEF_PPS,
              .tags = { L"PPS", L"原始数列", L"数列", L"入门" } },

            { .display_name = L"PPS4", .id = "pps4",
              .expand = &notation::PPS4Notation::expand,
              .suffix = &notation::PPS4Notation::suffix,
              .definition_resource_id = IDR_DEF_PPS4,
              .tags = { L"PPS", L"原始数列", L"4行", L"数列" } },

            { .display_name = L"Weak PPS4", .id = "wpps4",
              .expand = &notation::WPPS4Notation::expand,
              .suffix = &notation::WPPS4Notation::suffix,
              .definition_resource_id = IDR_DEF_WPPS4,
              .tags = { L"PPS", L"弱", L"4行", L"数列" } },

            { .display_name = L"Third PPS4", .id = "tpps4",
              .expand = &notation::TPPS4Notation::expand,
              .suffix = &notation::TPPS4Notation::suffix,
              .definition_resource_id = IDR_DEF_TPPS4,
              .tags = { L"PPS", L"第三", L"4行", L"数列" } },

            { .display_name = L"Ex. Weak PPS4", .id = "ewpps4",
              .expand = &notation::EWPPS4Notation::expand,
              .suffix = &notation::EWPPS4Notation::suffix,
              .definition_resource_id = IDR_DEF_EWPPS4,
              .tags = { L"PPS", L"扩展弱", L"4行", L"数列" } },

            { .display_name = L"Second PPS4", .id = "spps4",
              .expand = &notation::SecondPPS4Notation::expand,
              .suffix = &notation::SecondPPS4Notation::suffix,
              .definition_resource_id = IDR_DEF_SPPS4,
              .tags = { L"PPS", L"第二", L"4行", L"数列" } },

            { .display_name = L"2-pps4", .id = "2-pps4",
              .expand = &notation::PPS2Notation::expand,
              .suffix = &notation::PPS2Notation::suffix,
              .definition_resource_id = IDR_DEF_2PPS4,
              .tags = { L"PPS", L"2行", L"4行", L"数列" } },

            { .display_name = L"ω-Y (medium)", .id = "omega-y-medium",
              .expand = &notation::OmegaYMediumNotation::expand,
              .suffix = &notation::OmegaYMediumNotation::suffix,
              .definition_resource_id = IDR_DEF_OMEGA_Y_MEDIUM,
              .tags = { L"Y序列", L"ω-Y", L"中等", L"数列" } },

            { .display_name = L"ω-Y (strong)", .id = "omega-y-strong",
              .expand = &notation::OmegaYStrongNotation::expand,
              .suffix = &notation::OmegaYStrongNotation::suffix,
              .definition_resource_id = IDR_DEF_OMEGA_Y_STRONG,
              .tags = { L"Y序列", L"ω-Y", L"强", L"数列" } },

            { .display_name = L"MrSS1.2.1", .id = "mrss121",
              .expand = &notation::Mrss121Notation::expand,
              .suffix = &notation::Mrss121Notation::suffix,
              .definition_resource_id = IDR_DEF_MRSS121,
              .expand_text = &notation::Mrss121Notation::expand_string,
              .tags = { L"MrSS", L"矩阵", L"数列" } },

            { .display_name = L"BMS v1.0", .id = "bms-v1",
              .expand = nullptr, .suffix = &notation::BMSFamilyNotation::suffix,
              .definition_resource_id = IDR_DEF_BMS_V1,
              .expand_text = &bmsExpandV1,
              .tags = { L"BMS", L"矩阵", L"旧版" } },

            { .display_name = L"BMS v2.0", .id = "bms-v2",
              .expand = nullptr, .suffix = &notation::BMSFamilyNotation::suffix,
              .definition_resource_id = IDR_DEF_BMS_V2,
              .expand_text = &bmsExpandV2,
              .tags = { L"BMS", L"矩阵" } },

            { .display_name = L"BMS v2.1", .id = "bms-v21",
              .expand = nullptr, .suffix = &notation::BMSFamilyNotation::suffix,
              .definition_resource_id = IDR_DEF_BMS_V21,
              .expand_text = &bmsExpandV21,
              .tags = { L"BMS", L"矩阵" } },

            { .display_name = L"BMS v2.2", .id = "bms-v22",
              .expand = nullptr, .suffix = &notation::BMSFamilyNotation::suffix,
              .definition_resource_id = IDR_DEF_BMS_V22,
              .expand_text = &bmsExpandV22,
              .tags = { L"BMS", L"矩阵" } },

            { .display_name = L"BMS v2.3", .id = "bms-v23",
              .expand = nullptr, .suffix = &notation::BMSFamilyNotation::suffix,
              .definition_resource_id = IDR_DEF_BMS_V23,
              .expand_text = &bmsExpandV23,
              .tags = { L"BMS", L"矩阵" } },

            { .display_name = L"BMS v3.0", .id = "bms-v3",
              .expand = nullptr, .suffix = &notation::BMSFamilyNotation::suffix,
              .definition_resource_id = IDR_DEF_BMS_V3,
              .expand_text = &bmsExpandV3,
              .tags = { L"BMS", L"矩阵" } },

            { .display_name = L"BMS v3.1", .id = "bms-v31",
              .expand = nullptr, .suffix = &notation::BMSFamilyNotation::suffix,
              .definition_resource_id = IDR_DEF_BMS_V31,
              .expand_text = &bmsExpandV31,
              .tags = { L"BMS", L"矩阵" } },

            { .display_name = L"BMS v3.2", .id = "bms-v32",
              .expand = nullptr, .suffix = &notation::BMSFamilyNotation::suffix,
              .definition_resource_id = IDR_DEF_BMS_V32,
              .expand_text = &bmsExpandV32,
              .tags = { L"BMS", L"矩阵" } },

            { .display_name = L"BMS v3.3", .id = "bms-v33",
              .expand = nullptr, .suffix = &notation::BMSFamilyNotation::suffix,
              .definition_resource_id = IDR_DEF_BMS_V33,
              .expand_text = &bmsExpandV33,
              .tags = { L"BMS", L"矩阵" } },

            { .display_name = L"BMS v4.0", .id = "bms-v4",
              .expand = nullptr, .suffix = &notation::BMSFamilyNotation::suffix,
              .definition_resource_id = IDR_DEF_BMS_V4,
              .expand_text = &bmsExpandV4,
              .tags = { L"BMS", L"矩阵", L"最新" } },

            { .display_name = L"ω-Y sequence", .id = "omega-y-sequence",
              .expand = &notation::OmegaYNotation::expand,
              .suffix = &notation::OmegaYNotation::suffix,
              .definition_resource_id = IDR_DEF_OMEGA_Y_SEQUENCE,
              .tags = { L"Y序列", L"ω-Y", L"数列" } },

            { .display_name = L"ε-Y", .id = "epsilon-y",
              .expand = &notation::EpsilonYNotation::expand,
              .suffix = &notation::EpsilonYNotation::suffix,
              .definition_resource_id = IDR_DEF_EPSILON_Y,
              .tags = { L"Y序列", L"ε-Y", L"数列" } },

            { .display_name = L"UPMS", .id = "upms",
              .expand = nullptr, .suffix = &notation::UPMSNotation::suffix,
              .definition_resource_id = IDR_DEF_UPMS,
              .expand_text = &upmsExpandText,
              .tags = { L"UPMS", L"矩阵", L"文本" } },

            { .display_name = L"BSM", .id = "bsm",
              .expand = nullptr, .suffix = &notation::BSMNotation::suffix,
              .definition_resource_id = IDR_DEF_BSM,
              .expand_text = &notation::BSMNotation::expand_text,
              .expand_text_alter = &notation::BSMNotation::expand_text,
              .tags = { L"BSM", L"BM-like", L"矩阵", L"文本" } },

            { .display_name = L"BHM", .id = "bhm",
              .expand = nullptr, .suffix = &notation::BHMNotation::suffix,
              .definition_resource_id = IDR_DEF_BHM,
              .expand_text = &notation::BHMNotation::expand_text,
              .expand_text_alter = &notation::BHMNotation::expand_text_alter,
              .tags = { L"BHM", L"BM-like", L"矩阵", L"文本" } },

            { .display_name = L"1BM-BHM", .id = "1-bm-bhm",
              .expand = nullptr, .suffix = &notation::BM_BHMNotation<1>::suffix,
              .definition_resource_id = IDR_DEF_1BM_BHM,
              .expand_text = &notation::BM_BHMNotation<1>::expand_text,
              .expand_text_alter = &notation::BM_BHMNotation<1>::expand_text_alter,
              .tags = { L"BM-BHM", L"BM-like", L"矩阵", L"文本" } },

            { .display_name = L"2BM-BHM", .id = "2-bm-bhm",
              .expand = nullptr, .suffix = &notation::BM_BHMNotation<2>::suffix,
              .definition_resource_id = IDR_DEF_2BM_BHM,
              .expand_text = &notation::BM_BHMNotation<2>::expand_text,
              .expand_text_alter = &notation::BM_BHMNotation<2>::expand_text_alter,
              .tags = { L"BM-BHM", L"BM-like", L"矩阵", L"文本" } },

            { .display_name = L"3BM-BHM", .id = "3-bm-bhm",
              .expand = nullptr, .suffix = &notation::BM_BHMNotation<3>::suffix,
              .definition_resource_id = IDR_DEF_3BM_BHM,
              .expand_text = &notation::BM_BHMNotation<3>::expand_text,
              .expand_text_alter = &notation::BM_BHMNotation<3>::expand_text_alter,
              .tags = { L"BM-BHM", L"BM-like", L"矩阵", L"文本" } },

            { .display_name = L"CMS", .id = "cms",
              .expand = nullptr, .suffix = &notation::CMSNotation::suffix,
              .definition_resource_id = IDR_DEF_CMS,
              .expand_text = &notation::CMSNotation::expand_text,
              .tags = { L"CMS", L"BM-like", L"矩阵", L"文本" } },

            { .display_name = L"DSM", .id = "dsm",
              .expand = nullptr, .suffix = &notation::DSMNotation::suffix,
              .definition_resource_id = IDR_DEF_DSM,
              .expand_text = &notation::DSMNotation::expand_text,
              .expand_text_alter = &notation::DSMNotation::expand_text_alter,
              .tags = { L"DSM", L"BM-like", L"矩阵", L"文本" } },

            { .display_name = L"BLM", .id = "blm",
              .expand = nullptr, .suffix = &notation::BLMNotation::suffix,
              .definition_resource_id = IDR_DEF_BLM,
              .expand_text = &notation::BLMNotation::expand_text,
              .tags = { L"BLM", L"BM-like", L"矩阵", L"文本" } },

            { .display_name = L"G-Y", .id = "g-y",
              .expand = &notation::GYNotation::expand,
              .suffix = &notation::GYNotation::suffix,
              .definition_resource_id = IDR_DEF_GY,
              .tags = { L"Y序列", L"G-Y", L"数列" } },

            { .display_name = L"X-Y V1", .id = "xy-v1",
              .expand = &notation::XYV1Notation::expand,
              .suffix = &notation::XYV1Notation::suffix,
              .definition_resource_id = IDR_DEF_XY_V1,
              .tags = { L"Y序列", L"X-Y", L"数列" } },

            { .display_name = L"X-Y V2", .id = "xy-v2",
              .expand = &notation::XYV2Notation::expand,
              .suffix = &notation::XYV2Notation::suffix,
              .definition_resource_id = IDR_DEF_XY_V2,
              .tags = { L"Y序列", L"X-Y", L"数列" } },

            { .display_name = L"X-Y V3", .id = "xy-v3",
              .expand = &notation::XYV3Notation::expand,
              .suffix = &notation::XYV3Notation::suffix,
              .definition_resource_id = IDR_DEF_XY_V3,
              .tags = { L"Y序列", L"X-Y", L"数列" } },

            { .display_name = L"DEN1", .id = "den1",
              .expand = nullptr, .suffix = &notation::Den1Notation::suffix,
              .definition_resource_id = IDR_DEF_DEN1,
              .expand_text = &notation::Den1Notation::expand_text,
              .expand_text_alter = &notation::Den1Notation::expand_text_alter,
              .tags = { L"DEN", L"文本", L"旧版" } },

            { .display_name = L"blp", .id = "blp",
              .expand = nullptr, .suffix = &notation::BlpNotation::suffix,
              .definition_resource_id = IDR_DEF_BLP,
              .expand_text = &notation::BlpNotation::expand_text,
              .expand_text_alter = &notation::BlpNotation::expand_text_alter,
              .tags = { L"blp", L"文本" } },

            { .display_name = L"iblp", .id = "iblp",
              .expand = nullptr, .suffix = &notation::IblpNotation::suffix,
              .definition_resource_id = IDR_DEF_IBLP,
              .expand_text = &notation::IblpNotation::expand_text,
              .expand_text_alter = &notation::IblpNotation::expand_text_alter,
              .tags = { L"iblp", L"文本", L"逆" } },

            { .display_name = L"TON", .id = "ton-m",
              .expand = nullptr, .suffix = &notation::TonNotation::suffix,
              .definition_resource_id = IDR_DEF_TON_M,
              .expand_text = &notation::TonNotation::expand_text,
              .expand_text_alter = &notation::TonNotation::expand_text_alter,
              .tags = { L"TON", L"序数记号", L"文本" } },

            { .display_name = L"TON_I", .id = "ton-i",
              .expand = nullptr, .suffix = &notation::TonINotation::suffix,
              .definition_resource_id = IDR_DEF_TON_I,
              .expand_text = &notation::TonINotation::expand_text,
              .expand_text_alter = &notation::TonINotation::expand_text_alter,
              .tags = { L"TON", L"n-built", L"文本" } },

            { .display_name = L"TON_DoR", .id = "ton-dr",
              .expand = nullptr, .suffix = &notation::TonDoRNotation::suffix,
              .definition_resource_id = IDR_DEF_TON_DR,
              .expand_text = &notation::TonDoRNotation::expand_text,
              .expand_text_alter = &notation::TonDoRNotation::expand_text_alter,
              .tags = { L"TON", L"反射度", L"文本" } },

            { .display_name = L"TON_DRC", .id = "ton-drc",
              .expand = nullptr, .suffix = &notation::TonDRCNotation::suffix,
              .definition_resource_id = IDR_DEF_TON_DRC,
              .expand_text = &notation::TonDRCNotation::expand_text,
              .expand_text_alter = &notation::TonDRCNotation::expand_text_alter,
              .tags = { L"TON", L"反射配置", L"文本" } },

            { .display_name = L"TON_DRP", .id = "ton-drp",
              .expand = nullptr, .suffix = &notation::TonDRPNotation::suffix,
              .definition_resource_id = IDR_DEF_TON_DRP,
              .expand_text = &notation::TonDRPNotation::expand_text,
              .expand_text_alter = &notation::TonDRPNotation::expand_text_alter,
              .tags = { L"TON", L"passthrough", L"文本" } },

            { .display_name = L"TON_DRPC", .id = "ton-drpc",
              .expand = nullptr, .suffix = &notation::TonDRPCNotation::suffix,
              .definition_resource_id = IDR_DEF_TON_DRPC,
              .expand_text = &notation::TonDRPCNotation::expand_text,
              .expand_text_alter = &notation::TonDRPCNotation::expand_text_alter,
              .tags = { L"TON", L"反射配置", L"passthrough", L"文本" } },

            { .display_name = L"TON_IBP", .id = "ton-ibp",
              .expand = nullptr, .suffix = &notation::TonIbpNotation::suffix,
              .definition_resource_id = IDR_DEF_TON_IBP,
              .expand_text = &notation::TonIbpNotation::expand_text,
              .expand_text_alter = &notation::TonIbpNotation::expand_text_alter,
              .tags = { L"TON", L"n-built", L"文本" } },

            { .display_name = L"TON_MC", .id = "ton-mc",
              .expand = nullptr, .suffix = &notation::TonMcNotation::suffix,
              .definition_resource_id = IDR_DEF_TON_MC,
              .expand_text = &notation::TonMcNotation::expand_text,
              .expand_text_alter = &notation::TonMcNotation::expand_text_alter,
              .tags = { L"TON", L"反射配置", L"文本" } },

            { .display_name = L"TON_MPC", .id = "ton-mpc",
              .expand = nullptr, .suffix = &notation::TonMpcNotation::suffix,
              .definition_resource_id = IDR_DEF_TON_MPC,
              .expand_text = &notation::TonMpcNotation::expand_text,
              .expand_text_alter = &notation::TonMpcNotation::expand_text_alter,
              .tags = { L"TON", L"反射配置", L"passthrough", L"文本" } },

            // ---- MN 系列（e0MN / TωMN / SDBMS / SMN / UPMN / Aw2MN）----
            { .display_name = L"e0MN", .id = "e0mn",
              .expand = nullptr, .suffix = &notation::E0MNNotation::suffix,
              .definition_resource_id = IDR_DEF_E0MN,
              .expand_text = &notation::E0MNNotation::expand_text,
              .expand_text_alter = &notation::E0MNNotation::expand_text_alter,
              .tags = { L"MN", L"文本" } },

            { .display_name = L"strong e0MN", .id = "strong-e0mn",
              .expand = nullptr, .suffix = &notation::StrongE0MNNotation::suffix,
              .definition_resource_id = IDR_DEF_STRONG_E0MN,
              .expand_text = &notation::StrongE0MNNotation::expand_text,
              .expand_text_alter = &notation::StrongE0MNNotation::expand_text_alter,
              .tags = { L"MN", L"文本" } },

            { .display_name = L"ωMN", .id = "omega-mn",
              .expand = nullptr, .suffix = &notation::OmegaMNNotation::suffix,
              .definition_resource_id = IDR_DEF_OMEGA_MN,
              .expand_text = &notation::OmegaMNNotation::expand_text,
              .expand_text_alter = &notation::OmegaMNNotation::expand_text_alter,
              .tags = { L"MN", L"文本" } },

            { .display_name = L"PCF's ωMN", .id = "pcf-omega-mn",
              .expand = nullptr, .suffix = &notation::PcfOmegaMNNotation::suffix,
              .definition_resource_id = IDR_DEF_PCF_OMEGA_MN,
              .expand_text = &notation::PcfOmegaMNNotation::expand_text,
              .expand_text_alter = &notation::PcfOmegaMNNotation::expand_text_alter,
              .tags = { L"MN", L"文本" } },

            { .display_name = L"TωMN", .id = "t-omega-mn",
              .expand = nullptr, .suffix = &notation::TOmegaMNNotation::suffix,
              .definition_resource_id = IDR_DEF_T_OMEGA_MN,
              .expand_text = &notation::TOmegaMNNotation::expand_text,
              .expand_text_alter = &notation::TOmegaMNNotation::expand_text_alter,
              .tags = { L"MN", L"文本" } },

            { .display_name = L"1MN", .id = "1-mn",
              .expand = nullptr, .suffix = &notation::NMnNotation<1>::suffix,
              .definition_resource_id = IDR_DEF_1MN,
              .expand_text = &notation::NMnNotation<1>::expand_text,
              .expand_text_alter = &notation::NMnNotation<1>::expand_text_alter,
              .tags = { L"MN", L"文本" } },

            { .display_name = L"2MN", .id = "2-mn",
              .expand = nullptr, .suffix = &notation::NMnNotation<2>::suffix,
              .definition_resource_id = IDR_DEF_2MN,
              .expand_text = &notation::NMnNotation<2>::expand_text,
              .expand_text_alter = &notation::NMnNotation<2>::expand_text_alter,
              .tags = { L"MN", L"文本" } },

            { .display_name = L"3MN", .id = "3-mn",
              .expand = nullptr, .suffix = &notation::NMnNotation<3>::suffix,
              .definition_resource_id = IDR_DEF_3MN,
              .expand_text = &notation::NMnNotation<3>::expand_text,
              .expand_text_alter = &notation::NMnNotation<3>::expand_text_alter,
              .tags = { L"MN", L"文本" } },

            { .display_name = L"Sω2MN", .id = "s-omega2-mn",
              .expand = nullptr, .suffix = &notation::SOmega2MNNotation::suffix,
              .definition_resource_id = IDR_DEF_S_OMEGA2_MN,
              .expand_text = &notation::SOmega2MNNotation::expand_text,
              .expand_text_alter = &notation::SOmega2MNNotation::expand_text_alter,
              .tags = { L"MN", L"文本" } },

            { .display_name = L"SAω2MN", .id = "sa-omega2-mn",
              .expand = nullptr, .suffix = &notation::SAOmega2MNNotation::suffix,
              .definition_resource_id = IDR_DEF_SA_OMEGA2_MN,
              .expand_text = &notation::SAOmega2MNNotation::expand_text,
              .expand_text_alter = &notation::SAOmega2MNNotation::expand_text_alter,
              .tags = { L"MN", L"文本" } },

            { .display_name = L"Sω^ωMN", .id = "s-omega-pow-omega-mn",
              .expand = nullptr, .suffix = &notation::SOmegaPowOmegaMNNotation::suffix,
              .definition_resource_id = IDR_DEF_S_OMEGA_POW_OMEGA_MN,
              .expand_text = &notation::SOmegaPowOmegaMNNotation::expand_text,
              .expand_text_alter = &notation::SOmegaPowOmegaMNNotation::expand_text_alter,
              .tags = { L"MN", L"文本" } },

            { .display_name = L"UP1MN", .id = "up1mn",
              .expand = nullptr, .suffix = &notation::UP1MNNotation::suffix,
              .definition_resource_id = IDR_DEF_UP1MN,
              .expand_text = &notation::UP1MNNotation::expand_text,
              .expand_text_alter = &notation::UP1MNNotation::expand_text_alter,
              .tags = { L"MN", L"UPMN", L"文本" } },

            { .display_name = L"UP2MN v1A", .id = "up2mn-v1a",
              .expand = nullptr, .suffix = &notation::UP2MNV1ANotation::suffix,
              .definition_resource_id = IDR_DEF_UP2MN_V1A,
              .expand_text = &notation::UP2MNV1ANotation::expand_text,
              .expand_text_alter = &notation::UP2MNV1ANotation::expand_text_alter,
              .tags = { L"MN", L"UPMN", L"文本" } },

            { .display_name = L"UP2MN v1B", .id = "up2mn-v1b",
              .expand = nullptr, .suffix = &notation::UP2MNV1BNotation::suffix,
              .definition_resource_id = IDR_DEF_UP2MN_V1B,
              .expand_text = &notation::UP2MNV1BNotation::expand_text,
              .expand_text_alter = &notation::UP2MNV1BNotation::expand_text_alter,
              .tags = { L"MN", L"UPMN", L"文本" } },

            { .display_name = L"UP2MN v1B+", .id = "up2mn-v1b-plus",
              .expand = nullptr, .suffix = &notation::UP2MNV1BPlusNotation::suffix,
              .definition_resource_id = IDR_DEF_UP2MN_V1B_PLUS,
              .expand_text = &notation::UP2MNV1BPlusNotation::expand_text,
              .expand_text_alter = &notation::UP2MNV1BPlusNotation::expand_text_alter,
              .tags = { L"MN", L"UPMN", L"文本" } },

            { .display_name = L"UP1DBMS", .id = "up1dbms",
              .expand = nullptr, .suffix = &notation::UP1DBMSNotation::suffix,
              .definition_resource_id = IDR_DEF_UP1DBMS,
              .expand_text = &notation::UP1DBMSNotation::expand_text,
              .expand_text_alter = &notation::UP1DBMSNotation::expand_text_alter,
              .tags = { L"MN", L"UPMN", L"DBMS", L"文本" } },

            { .display_name = L"UP2DBMS v1", .id = "up2dbms-v1",
              .expand = nullptr, .suffix = &notation::UP2DBMSV1Notation::suffix,
              .definition_resource_id = IDR_DEF_UP2DBMS_V1,
              .expand_text = &notation::UP2DBMSV1Notation::expand_text,
              .expand_text_alter = &notation::UP2DBMSV1Notation::expand_text_alter,
              .tags = { L"MN", L"UPMN", L"DBMS", L"文本" } },

            { .display_name = L"UP2DBMS v1B+", .id = "up2dbms-v1b-plus",
              .expand = nullptr, .suffix = &notation::UP2DBMSV1BPlusNotation::suffix,
              .definition_resource_id = IDR_DEF_UP2DBMS_V1B_PLUS,
              .expand_text = &notation::UP2DBMSV1BPlusNotation::expand_text,
              .expand_text_alter = &notation::UP2DBMSV1BPlusNotation::expand_text_alter,
              .tags = { L"MN", L"UPMN", L"DBMS", L"文本" } },

            { .display_name = L"UP2DBMS v1C", .id = "up2dbms-v1c",
              .expand = nullptr, .suffix = &notation::UP2DBMSV1CNotation::suffix,
              .definition_resource_id = IDR_DEF_UP2DBMS_V1C,
              .expand_text = &notation::UP2DBMSV1CNotation::expand_text,
              .expand_text_alter = &notation::UP2DBMSV1CNotation::expand_text_alter,
              .tags = { L"MN", L"UPMN", L"DBMS", L"文本" } },

            { .display_name = L"UP2DBMS v2", .id = "up2dbms-v2",
              .expand = nullptr, .suffix = &notation::Up2DbmsV2Notation::suffix,
              .definition_resource_id = IDR_DEF_UP2DBMS_V2,
              .expand_text = &notation::Up2DbmsV2Notation::expand_text,
              .expand_text_alter = &notation::Up2DbmsV2Notation::expand_text_alter,
              .tags = { L"MN", L"UPMN", L"DBMS", L"文本" } },

            { .display_name = L"S1DBMS", .id = "s1dbms",
              .expand = nullptr, .suffix = &notation::S1DBMSNotation::suffix,
              .definition_resource_id = IDR_DEF_S1DBMS,
              .expand_text = &notation::S1DBMSNotation::expand_text,
              .expand_text_alter = &notation::S1DBMSNotation::expand_text_alter,
              .tags = { L"MN", L"SDBMS", L"文本" } },

            { .display_name = L"S2DBMS", .id = "s2dbms",
              .expand = nullptr, .suffix = &notation::S2DBMSNotation::suffix,
              .definition_resource_id = IDR_DEF_S2DBMS,
              .expand_text = &notation::S2DBMSNotation::expand_text,
              .expand_text_alter = &notation::S2DBMSNotation::expand_text_alter,
              .tags = { L"MN", L"SDBMS", L"文本" } },

            { .display_name = L"SωDBMS v1", .id = "s-omega-dbms",
              .expand = nullptr, .suffix = &notation::SOmegaDBMSV1Notation::suffix,
              .definition_resource_id = IDR_DEF_S_OMEGA_DBMS,
              .expand_text = &notation::SOmegaDBMSV1Notation::expand_text,
              .expand_text_alter = &notation::SOmegaDBMSV1Notation::expand_text_alter,
              .tags = { L"MN", L"SDBMS", L"文本" } },

            { .display_name = L"SωDBMS v2", .id = "s-omega-dbms-v2",
              .expand = nullptr, .suffix = &notation::SOmegaDBMSV2Notation::suffix,
              .definition_resource_id = IDR_DEF_S_OMEGA_DBMS_V2,
              .expand_text = &notation::SOmegaDBMSV2Notation::expand_text,
              .expand_text_alter = &notation::SOmegaDBMSV2Notation::expand_text_alter,
              .tags = { L"MN", L"SDBMS", L"文本" } },

            { .display_name = L"SωDBMS v3", .id = "s-omega-dbms-v3",
              .expand = nullptr, .suffix = &notation::SOmegaDBMSV3Notation::suffix,
              .definition_resource_id = IDR_DEF_S_OMEGA_DBMS_V3,
              .expand_text = &notation::SOmegaDBMSV3Notation::expand_text,
              .expand_text_alter = &notation::SOmegaDBMSV3Notation::expand_text_alter,
              .tags = { L"MN", L"SDBMS", L"文本" } },

            { .display_name = L"Sω+1DBMS", .id = "s-omega-p1-dbms",
              .expand = nullptr, .suffix = &notation::SOmegaP1DBMSNotation::suffix,
              .definition_resource_id = IDR_DEF_S_OMEGA_P1_DBMS,
              .expand_text = &notation::SOmegaP1DBMSNotation::expand_text,
              .expand_text_alter = &notation::SOmegaP1DBMSNotation::expand_text_alter,
              .tags = { L"MN", L"SDBMS", L"文本" } },

            { .display_name = L"Sω2DBMS", .id = "s-omega2-dbms",
              .expand = nullptr, .suffix = &notation::SOmega2DBMSNotation::suffix,
              .definition_resource_id = IDR_DEF_S_OMEGA2_DBMS,
              .expand_text = &notation::SOmega2DBMSNotation::expand_text,
              .expand_text_alter = &notation::SOmega2DBMSNotation::expand_text_alter,
              .tags = { L"MN", L"SDBMS", L"文本" } },

            { .display_name = L"Sω^2DBMS", .id = "s-omega-square-dbms",
              .expand = nullptr, .suffix = &notation::SOmegaSquareDBMSNotation::suffix,
              .definition_resource_id = IDR_DEF_S_OMEGA_SQUARE_DBMS,
              .expand_text = &notation::SOmegaSquareDBMSNotation::expand_text,
              .expand_text_alter = &notation::SOmegaSquareDBMSNotation::expand_text_alter,
              .tags = { L"MN", L"SDBMS", L"文本" } },

            { .display_name = L"Aω2MN2", .id = "a-omega2-mn-2",
              .expand = nullptr, .suffix = &notation::Aw2MN2Notation::suffix,
              .definition_resource_id = IDR_DEF_A_OMEGA2_MN_2,
              .expand_text = &notation::Aw2MN2Notation::expand_text,
              .expand_text_alter = &notation::Aw2MN2Notation::expand_text_alter,
              .tags = { L"MN", L"文本" } },

            { .display_name = L"wAω2MN2", .id = "weak-a-omega2-mn-2",
              .expand = nullptr, .suffix = &notation::WAw2MN2Notation::suffix,
              .definition_resource_id = IDR_DEF_WEAK_A_OMEGA2_MN_2,
              .expand_text = &notation::WAw2MN2Notation::expand_text,
              .expand_text_alter = &notation::WAw2MN2Notation::expand_text_alter,
              .tags = { L"MN", L"文本" } },

            { .display_name = L"Aω2MN3", .id = "a-omega2-mn-3",
              .expand = nullptr, .suffix = &notation::Aw2MN3Notation::suffix,
              .definition_resource_id = IDR_DEF_A_OMEGA2_MN_3,
              .expand_text = &notation::Aw2MN3Notation::expand_text,
              .expand_text_alter = &notation::Aw2MN3Notation::expand_text_alter,
              .tags = { L"MN", L"文本" } },

            { .display_name = L"wAω2MN3", .id = "weak-a-omega2-mn-3",
              .expand = nullptr, .suffix = &notation::WAw2MN3Notation::suffix,
              .definition_resource_id = IDR_DEF_WEAK_A_OMEGA2_MN_3,
              .expand_text = &notation::WAw2MN3Notation::expand_text,
              .expand_text_alter = &notation::WAw2MN3Notation::expand_text_alter,
              .tags = { L"MN", L"文本" } },
        };
        return table;
    }

    struct MenuDeleter { void operator()(HMENU m)  const noexcept { if (m) ::DestroyMenu(m); } };
    struct BrushDeleter { void operator()(HBRUSH b) const noexcept { if (b) ::DeleteObject(b); } };
    using MenuPtr = std::unique_ptr<std::remove_pointer_t<HMENU>, MenuDeleter>;
    using BrushPtr = std::unique_ptr<std::remove_pointer_t<HBRUSH>, BrushDeleter>;

    // ================= 记号选择窗体 =================

    [[nodiscard]] std::wstring pickerLower(std::wstring_view s) {
        std::wstring out;
        out.reserve(s.size());
        for (wchar_t c : s) out.push_back(static_cast<wchar_t>(std::towlower(c)));
        return out;
    }

    [[nodiscard]] std::wstring joinTags(const NotationEntry& e) {
        std::wstring out;
        for (const wchar_t* t : e.tags) {
            if (t == nullptr) break;
            if (!out.empty()) out += L", ";
            out += t;
        }
        return out;
    }

    // 搜索打分：0 = 不匹配；分值越高排序越靠前。
    // 支持四种命中：记号全名(=)、Tag全名(=)、记号前缀、记号部分名(包含)、Tag部分名(包含)。
    [[nodiscard]] int pickerMatchScore(const NotationEntry& e, const std::wstring& lq) {
        const std::wstring lname = pickerLower(e.display_name);
        if (lname == lq) return 5;
        for (const wchar_t* t : e.tags) {
            if (t == nullptr) break;
            if (pickerLower(t) == lq) return 4;
        }
        if (lname.size() >= lq.size() && lname.compare(0, lq.size(), lq) == 0) return 3;
        if (lname.find(lq) != std::wstring::npos) return 2;
        for (const wchar_t* t : e.tags) {
            if (t == nullptr) break;
            if (pickerLower(t).find(lq) != std::wstring::npos) return 1;
        }
        return 0;
    }

    struct PickerLaunch {
        int* result;
        int current;
    };

    struct PickerState {
        HWND hSearch{};
        HWND hList{};
        HWND hStatus{};
        BrushPtr background;
        std::vector<int> shown;   // 当前列表行 → 记号表索引
        int current = 0;          // 打开窗体时的当前选择（用于首屏高亮）
        int* pResult = nullptr;   // 确认时写入选中的记号表索引
    };

    [[nodiscard]] PickerState* getPicker(HWND h) {
        return reinterpret_cast<PickerState*>(::GetWindowLongPtrW(h, GWLP_USERDATA));
    }

    void refreshPickerList(PickerState* st) {
        const auto& table = notationTable();

        wchar_t buf[256]{};
        ::GetWindowTextW(st->hSearch, buf, 256);
        std::wstring q = buf;
        while (!q.empty() && std::iswspace(q.front())) q.erase(q.begin());
        while (!q.empty() && std::iswspace(q.back())) q.pop_back();

        struct Scored { int index; int score; };
        std::vector<Scored> scored;
        if (q.empty()) {
            scored.reserve(table.size());
            for (int i = 0; i < static_cast<int>(table.size()); ++i)
                scored.push_back({ i, 1 });
        }
        else {
            const std::wstring lq = pickerLower(q);
            for (int i = 0; i < static_cast<int>(table.size()); ++i) {
                const int s = pickerMatchScore(table[i], lq);
                if (s > 0) scored.push_back({ i, s });
            }
            std::stable_sort(scored.begin(), scored.end(),
                [](const Scored& a, const Scored& b) { return a.score > b.score; });
        }

        // 记住原选中记号，刷新后尽量恢复
        const int prevRow = ListView_GetNextItem(st->hList, -1, LVNI_SELECTED);
        int prevIndex = -1;
        if (prevRow >= 0 && prevRow < static_cast<int>(st->shown.size()))
            prevIndex = st->shown[prevRow];

        ListView_DeleteAllItems(st->hList);
        st->shown.clear();
        st->shown.reserve(scored.size());

        for (const Scored& sc : scored) {
            const int idx = sc.index;
            st->shown.push_back(idx);
            const NotationEntry& e = table[idx];

            LVITEMW it{};
            it.mask = LVIF_TEXT | LVIF_PARAM;
            it.iItem = static_cast<int>(st->shown.size()) - 1;
            it.pszText = const_cast<wchar_t*>(e.display_name);
            it.lParam = idx;
            const int row = static_cast<int>(
                ::SendMessageW(st->hList, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&it)));

            const std::wstring tags = joinTags(e);
            if (!tags.empty()) {
                LVITEMW si{};
                si.mask = LVIF_TEXT;
                si.iItem = row;
                si.iSubItem = 1;
                si.pszText = const_cast<wchar_t*>(tags.c_str());
                ::SendMessageW(st->hList, LVM_SETITEMW, 0, reinterpret_cast<LPARAM>(&si));
            }
        }

        int selectRow = -1;
        if (prevIndex >= 0) {
            for (int r = 0; r < static_cast<int>(st->shown.size()); ++r) {
                if (st->shown[r] == prevIndex) { selectRow = r; break; }
            }
        }
        if (selectRow < 0 && !st->shown.empty()) {
            for (int r = 0; r < static_cast<int>(st->shown.size()); ++r) {
                if (st->shown[r] == st->current) { selectRow = r; break; }
            }
            if (selectRow < 0) selectRow = 0;
        }
        if (selectRow >= 0) {
            ListView_SetItemState(st->hList, selectRow,
                LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(st->hList, selectRow, FALSE);
        }

        std::wstring status = L"匹配 " + std::to_wstring(st->shown.size()) +
            L" / " + std::to_wstring(table.size()) + L" 个记号";
        ::SetWindowTextW(st->hStatus, status.c_str());
    }

    void acceptPicker(HWND hwnd) {
        auto* st = getPicker(hwnd);
        if (!st) return;
        const int row = ListView_GetNextItem(st->hList, -1, LVNI_SELECTED);
        if (row < 0 || row >= static_cast<int>(st->shown.size())) return;
        if (st->pResult) *st->pResult = st->shown[row];
        ::DestroyWindow(hwnd);
    }

    LRESULT CALLBACK PickerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            auto* launch = reinterpret_cast<PickerLaunch*>(cs->lpCreateParams);
            auto* st = new PickerState{};
            st->background.reset(::CreateSolidBrush(::GetSysColor(COLOR_WINDOW)));
            if (launch) {
                st->current = launch->current;
                st->pResult = launch->result;
            }
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(st));

            HINSTANCE hInst = cs->hInstance;

            ::CreateWindowExW(0, L"STATIC", L"搜索:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                12, 14, 42, 18, hwnd, nullptr, hInst, nullptr);

            st->hSearch = ::CreateWindowExW(0, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP |
                ES_LEFT | ES_AUTOHSCROLL,
                56, 10, 492, 22, hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_PICKER_SEARCH)),
                hInst, nullptr);

            st->hList = ::CreateWindowExW(
                WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
                LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
                12, 40, 536, 330, hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_PICKER_LIST)),
                hInst, nullptr);
            ::SendMessageW(st->hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

            LVCOLUMNW col{};
            col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            col.pszText = const_cast<wchar_t*>(L"记号");
            col.cx = 170;
            ::SendMessageW(st->hList, LVM_INSERTCOLUMNW, 0,
                reinterpret_cast<LPARAM>(&col));
            col.pszText = const_cast<wchar_t*>(L"标签");
            col.cx = 366;
            ::SendMessageW(st->hList, LVM_INSERTCOLUMNW, 1,
                reinterpret_cast<LPARAM>(&col));

            st->hStatus = ::CreateWindowExW(0, L"STATIC", L"",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                12, 376, 300, 18, hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_PICKER_STATUS)),
                hInst, nullptr);

            ::CreateWindowExW(0, L"BUTTON", L"确定",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                360, 400, 90, 26, hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDOK)),
                hInst, nullptr);

            ::CreateWindowExW(0, L"BUTTON", L"取消",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                458, 400, 90, 26, hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDCANCEL)),
                hInst, nullptr);

            refreshPickerList(st);
            break;
        }
        case WM_COMMAND: {
            auto* st = getPicker(hwnd);
            if (!st) break;
            const int id = LOWORD(wParam);
            if (id == IDC_PICKER_SEARCH && HIWORD(wParam) == EN_CHANGE) {
                refreshPickerList(st);
                return 0;
            }
            if (id == IDOK) { acceptPicker(hwnd); return 0; }
            if (id == IDCANCEL) { ::DestroyWindow(hwnd); return 0; }
            break;
        }
        case WM_NOTIFY: {
            auto* nmh = reinterpret_cast<NMHDR*>(lParam);
            if (nmh->idFrom == IDC_PICKER_LIST && nmh->code == NM_DBLCLK) {
                acceptPicker(hwnd);
                return 0;
            }
            break;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN: {
            auto* st = getPicker(hwnd);
            if (!st) return ::DefWindowProcW(hwnd, msg, wParam, lParam);
            HDC hdc = reinterpret_cast<HDC>(wParam);
            ::SetBkMode(hdc, TRANSPARENT);
            ::SetTextColor(hdc, ::GetSysColor(COLOR_WINDOWTEXT));
            return reinterpret_cast<LRESULT>(st->background.get());
        }
        case WM_CLOSE:
            ::DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            delete getPicker(hwnd);
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            break;
        default:
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    void registerPickerClass(HINSTANCE hInst) {
        static bool registered = false;
        if (registered) return;

        WNDCLASSW wc{};
        wc.lpfnWndProc = PickerProc;
        wc.hInstance = hInst;
        wc.lpszClassName = kPickerWindowClass;
        wc.hbrBackground = ::GetSysColorBrush(COLOR_WINDOW);
        wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        ::RegisterClassW(&wc);
        registered = true;
    }

    // 模态显示记号选择窗体；返回选中的记号表索引，取消返回 -1。
    int showNotationPicker(HINSTANCE hInst, HWND hParent, int current,
        const wchar_t* title = L"选择记号") {
        registerPickerClass(hInst);

        int result = -1;
        PickerLaunch launch{ &result, current };

        HWND hDlg = ::CreateWindowExW(
            WS_EX_DLGMODALFRAME,
            kPickerWindowClass, title,
            WS_POPUP | WS_CAPTION | WS_SYSMENU,
            0, 0, 560, 440,
            hParent, nullptr, hInst, &launch);
        if (!hDlg) return -1;

        // 居中于父窗口，并整体下移 100px
        RECT pr{}, dr{};
        ::GetWindowRect(hParent, &pr);
        ::GetWindowRect(hDlg, &dr);
        const int w = dr.right - dr.left, h = dr.bottom - dr.top;
        const int x = pr.left + ((pr.right - pr.left) - w) / 2;
        const int y = pr.top + ((pr.bottom - pr.top) - h) / 2 + 100;
        ::SetWindowPos(hDlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        ::EnableWindow(hParent, FALSE);
        ::ShowWindow(hDlg, SW_SHOW);
        ::UpdateWindow(hDlg);
        ::SetForegroundWindow(hDlg);

        MSG msg;
        while (::GetMessageW(&msg, nullptr, 0, 0)) {
            if (!::IsDialogMessageW(hDlg, &msg)) {
                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
            }
            if (!::IsWindow(hDlg)) break;
        }

        ::EnableWindow(hParent, TRUE);
        ::SetForegroundWindow(hParent);
        return result;
    }

    struct UiState {
        HWND hEditSeq{};
        HWND hEditTerm{};
        HWND hBtnFS{};
        HWND hBtnFSalter{};
        HWND hBtnNotation{};    // “选择记号…”按钮
        HWND hNotationLabel{};  // 当前记号（含标签）显示
        int notation_index = 0; // 当前选中的记号表索引
        BrushPtr background;
    };

    void updateNotationLabel(UiState* ui) {
        const auto& table = notationTable();
        int idx = ui->notation_index;
        if (idx < 0 || idx >= static_cast<int>(table.size())) idx = 0;
        const NotationEntry& e = table[idx];
        std::wstring label = e.display_name;
        const std::wstring tags = joinTags(e);
        if (!tags.empty()) {
            label += L"　[";
            label += tags;
            label += L"]";
        }
        ::SetWindowTextW(ui->hNotationLabel, label.c_str());
    }

    [[nodiscard]] UiState* getUi(HWND h) {
        return reinterpret_cast<UiState*>(::GetWindowLongPtrW(h, GWLP_USERDATA));
    }

    [[nodiscard]] std::wstring loadDefinitionText(HINSTANCE, int resId) {
        // 定义文本不再经过 rc.exe 的资源编译：直接从 Definitions.h 标头查表，
        // 规避 rc.exe 对 UTF-8 源文件的编码误读(首尾乱码)与 \n 不转义的问题。
        std::string_view utf8 = expander_defs::lookup(resId);
        if (utf8.empty()) return L"暂缺，有提供的联系作者邮箱";
        return utf8_to_wstring(utf8);
    }

    [[nodiscard]] HMENU createMenuBar() {
        HMENU hMenu = ::CreateMenu();

        HMENU hFile = ::CreatePopupMenu();
        ::AppendMenuW(hFile, MF_STRING, IDM_EXIT, L"退出(&X)");
        ::AppendMenuW(hMenu, MF_POPUP,
            reinterpret_cast<UINT_PTR>(hFile), L"文件(&F)");

        // 定义改为直接命令项：点击后打开记号选择窗体，选定后弹出对应定义
        ::AppendMenuW(hMenu, MF_STRING, IDM_DEFINITION_PICK, L"定义(&D)...");

        HMENU hHelp = ::CreatePopupMenu();
        ::AppendMenuW(hHelp, MF_STRING, IDM_HELP, L"帮助(&H)...");
        ::AppendMenuW(hHelp, MF_STRING, IDM_ABOUT, L"关于(&A)...");
        ::AppendMenuW(hHelp, MF_STRING, IDM_LEGAL, L"法律声明(&L)...");
        ::AppendMenuW(hMenu, MF_POPUP,
            reinterpret_cast<UINT_PTR>(hHelp), L"帮助(&H)");

        return hMenu;
    }

    [[nodiscard]] bool readSeqAndTerm(
        HWND hwnd, UiState* ui,
        std::vector<int>& outSeq, int& outTerm)
    {
        wchar_t bufSeq[256]{}, bufTerm[64]{};
        ::GetWindowTextW(ui->hEditSeq, bufSeq, 256);
        ::GetWindowTextW(ui->hEditTerm, bufTerm, 64);

        outSeq = parse_sequence(wstring_to_utf8(bufSeq));

        try {
            outTerm = std::stoi(wstring_to_utf8(bufTerm));
        }
        catch (...) {
            ::MessageBoxW(hwnd, L"项数必须是整数", L"错误",
                MB_OK | MB_ICONERROR);
            return false;
        }
        if (outTerm < 1) {
            ::MessageBoxW(hwnd, L"项数必须为正整数", L"错误",
                MB_OK | MB_ICONERROR);
            return false;
        }
        if (outSeq.empty()) {
            ::MessageBoxW(hwnd, L"序列不能为空", L"错误",
                MB_OK | MB_ICONERROR);
            return false;
        }
        return true;
    }

    void applyRichEdit10pt(HWND hRich, const wchar_t* face = L"Microsoft YaHei UI") {
        const LONG sizeTwips = 10 * 20;

        CHARFORMAT2W cf{};
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_SIZE | CFM_FACE;
        cf.yHeight = sizeTwips;
        if (face != nullptr && face[0] != L'\0') {
            wcscpy_s(cf.szFaceName, face);
        }

        ::SendMessageW(hRich, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&cf));
    }

    void fillPopupDefinition(HWND hRich, const NotationEntry& entry, HINSTANCE hInst) {
        std::wstring text;
        text += L"【";
        text += entry.display_name;
        text += L"】\r\n\r\n";
        text += loadDefinitionText(hInst, entry.definition_resource_id);

        applyRichEdit10pt(hRich);
        ::SetWindowTextW(hRich, text.c_str());
        applyRichEdit10pt(hRich);

        ::SendMessageW(hRich, EM_SETSEL, 0, 0);
        ::SendMessageW(hRich, EM_SCROLLCARET, 0, 0);
    }

    LRESULT CALLBACK DefPopupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            const int index = static_cast<int>(reinterpret_cast<INT_PTR>(cs->lpCreateParams));
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, static_cast<LONG_PTR>(index));

            HINSTANCE hInst = cs->hInstance;

            HWND hRich = ::CreateWindowExW(
                0, MSFTEDIT_CLASS, L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
                ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                10, 10, 380, 260,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_POPUP_DEF_EDIT)),
                hInst, nullptr);

            HWND hClose = ::CreateWindowExW(
                0, L"BUTTON", L"关闭",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                320, 280, 70, 25,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_POPUP_DEF_CLOSE)),
                hInst, nullptr);

            const auto& table = notationTable();
            if (index >= 0 && index < static_cast<int>(table.size())) {
                fillPopupDefinition(hRich, table[index], hInst);
            }
            (void)hClose;
            break;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDC_POPUP_DEF_CLOSE) {
                ::DestroyWindow(hwnd);
            }
            break;
        }
        case WM_CLOSE:
            ::DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            break;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            ::SetBkMode(hdc, TRANSPARENT);
            ::SetTextColor(hdc, ::GetSysColor(COLOR_WINDOWTEXT));
            return reinterpret_cast<LRESULT>(::GetSysColorBrush(COLOR_WINDOW));
        }
        default:
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    void registerDefPopupClass(HINSTANCE hInst) {
        static bool registered = false;
        if (registered) return;

        WNDCLASSW wc{};
        wc.lpfnWndProc = DefPopupProc;
        wc.hInstance = hInst;
        wc.lpszClassName = kDefWindowClass;
        wc.hbrBackground = ::GetSysColorBrush(COLOR_WINDOW);
        wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        ::RegisterClassW(&wc);
        registered = true;
    }

    void showDefinitionPopup(HINSTANCE hInst, HWND hParent, int index) {
        const auto& table = notationTable();
        if (index < 0 || index >= static_cast<int>(table.size())) return;

        registerDefPopupClass(hInst);

        std::wstring title = L"定义 - ";
        title += table[index].display_name;

        HWND hPopup = ::CreateWindowExW(
            WS_EX_TOOLWINDOW,
            kDefWindowClass, title.c_str(),
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
            CW_USEDEFAULT, CW_USEDEFAULT, 420, 360,
            hParent, nullptr, hInst,
            reinterpret_cast<LPVOID>(static_cast<INT_PTR>(index)));

        if (hPopup) {
            // 在系统默认位置的基础上整体下移 100px
            RECT r{};
            if (::GetWindowRect(hPopup, &r)) {
                const int w = r.right - r.left, h = r.bottom - r.top;
                ::MoveWindow(hPopup, r.left, r.top + 100, w, h, TRUE);
            }
            ::ShowWindow(hPopup, SW_SHOW);
            ::UpdateWindow(hPopup);
        }
    }

    constexpr WORD kLicenseResourceId = 131;
    [[nodiscard]] std::wstring loadLicenseText(HINSTANCE hInst) {
        HRSRC hRes = ::FindResourceW(
            hInst, MAKEINTRESOURCEW(kLicenseResourceId), RT_RCDATA);
        HGLOBAL hGlobal = hRes ? ::LoadResource(hInst, hRes) : nullptr;
        const auto* data = hGlobal
            ? static_cast<const char*>(::LockResource(hGlobal)) : nullptr;
        const DWORD size = hRes ? ::SizeofResource(hInst, hRes) : 0;
        if (!data || size == 0) return L"无法加载许可证文本资源。";

        std::string_view utf8(data, size);
        if (utf8.size() >= 3 &&
            static_cast<unsigned char>(utf8[0]) == 0xEF &&
            static_cast<unsigned char>(utf8[1]) == 0xBB &&
            static_cast<unsigned char>(utf8[2]) == 0xBF) {
            utf8.remove_prefix(3); // 跳过 UTF-8 BOM
        }
        std::wstring w = utf8_to_wstring(utf8);

        std::wstring out;
        out.reserve(w.size() + 64);
        for (size_t i = 0; i < w.size(); ++i) {
            if (w[i] == L'\r') {
                if (i + 1 < w.size() && w[i + 1] == L'\n') ++i;
                out += L"\r\n";
            }
            else if (w[i] == L'\n') out += L"\r\n";
            else out += w[i];
        }
        return out;
    }

    LRESULT CALLBACK LegalWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            auto* text = reinterpret_cast<std::wstring*>(cs->lpCreateParams);

            HWND hRich = ::CreateWindowExW(
                0, MSFTEDIT_CLASS, L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_HSCROLL |
                ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                2, 2, 10, 10,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_LEGAL_EDIT)),
                cs->hInstance, nullptr);

            if (hRich && text) {
                ::SetWindowTextW(hRich, text->c_str());
                applyRichEdit10pt(hRich, L"Microsoft YaHei");
                ::SendMessageW(hRich, EM_SETMARGINS,
                    EC_LEFTMARGIN | EC_RIGHTMARGIN,
                    MAKELPARAM(0, 0));
                ::SendMessageW(hRich, EM_SETTARGETDEVICE, 0, 0);
                ::SendMessageW(hRich, EM_SETSEL, 0, 0);
            }
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(hRich));
            delete text;
            break;
        }
        case WM_SIZE: {
            HWND hRich = reinterpret_cast<HWND>(
                ::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (hRich) {
                ::MoveWindow(hRich, 2, 2,
                    LOWORD(lParam) - 4, HIWORD(lParam) - 4, TRUE);
            }
            break;
        }
        case WM_CLOSE:
            ::DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            g_hLegalWindow = nullptr;
            break;
        default:
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    void registerLegalClass(HINSTANCE hInst) {
        static bool registered = false;
        if (registered) return;

        WNDCLASSW wc{};
        wc.lpfnWndProc = LegalWndProc;
        wc.hInstance = hInst;
        wc.lpszClassName = kLegalWindowClass;
        wc.hbrBackground = ::GetSysColorBrush(COLOR_WINDOW);
        wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        ::RegisterClassW(&wc);
        registered = true;
    }

    void showLegalWindow(HINSTANCE hInst, HWND hParent) {
        if (::IsWindow(g_hLegalWindow)) {
            ::ShowWindow(g_hLegalWindow, SW_RESTORE);
            ::SetForegroundWindow(g_hLegalWindow);
            return;
        }

        registerLegalClass(hInst);
        auto* text = new std::wstring(loadLicenseText(hInst));

        HWND hwnd = ::CreateWindowExW(
            0, kLegalWindowClass, L"法律声明",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 760, 640,
            hParent, nullptr, hInst, text);

        if (!hwnd) {
            delete text;
            return;
        }
        g_hLegalWindow = hwnd;
        ::ShowWindow(hwnd, SW_SHOW);
        ::UpdateWindow(hwnd);
    }

    /*void showEntryDemo(HWND hwnd, UiState* ui) {
        std::vector<int> seq;
        int term = 0;
        if (!readSeqAndTerm(hwnd, ui, seq, term)) return;

        omegay::core::EntryArena arena;

        auto* root = arena.make(
            seq.empty() ? 0 : seq[0],
            term,
            seq);

        std::vector<int> childY = seq;
        if (childY.size() > 1) childY.pop_back();
        auto* child = arena.make(
            seq.back(),
            1,
            childY);

        root->rightleg_down = child;
        child->leftleg_up.push_back(root);

        child->y = childY;
        child->refresh_key();

        const std::uint64_t rootKey = omegay::core::make_ykey(root->y);
        const std::uint64_t childKey = omegay::core::make_ykey(child->y);

        std::wstring msg;
        msg += L"Entry 演示\n\n";
        msg += L"arena.size() = ";
        msg += std::to_wstring(arena.size());
        msg += L"\n\n";

        msg += L"root:\n";
        msg += L"  value = " + std::to_wstring(root->value) + L"\n";
        msg += L"  x     = " + std::to_wstring(root->x) + L"\n";
        msg += L"  y     = " + utf8_to_wstring(seq_to_string(root->y)) + L"\n";
        msg += L"  ykey  = " + std::to_wstring(root->ykey) + L"\n";
        msg += L"  make_ykey(y) = " + std::to_wstring(rootKey) + L"\n";
        msg += L"\n";

        msg += L"child:\n";
        msg += L"  value = " + std::to_wstring(child->value) + L"\n";
        msg += L"  x     = " + std::to_wstring(child->x) + L"\n";
        msg += L"  y     = " + utf8_to_wstring(seq_to_string(child->y)) + L"\n";
        msg += L"  ykey  = " + std::to_wstring(child->ykey) + L"\n";
        msg += L"  make_ykey(y) = " + std::to_wstring(childKey) + L"\n";
        msg += L"\n";

        msg += L"链接:\n";
        msg += L"  root->rightleg_down = child\n";
        msg += L"  child->leftleg_up.size() = ";
        msg += std::to_wstring(child->leftleg_up.size());
        msg += L"\n";

        ::MessageBoxW(hwnd, msg.c_str(), L"Entry 演示",
            MB_OK | MB_ICONINFORMATION);
    }*/

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE: {
            auto* ui = new UiState{};
            ui->background.reset(::CreateSolidBrush(::GetSysColor(COLOR_WINDOW)));
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(ui));

            MenuPtr menu(createMenuBar());
            ::SetMenu(hwnd, menu.release());

            HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
                ::GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
            constexpr int mh = 25;

            ::CreateWindowExW(0, L"STATIC", L"序列 (用逗号分隔):",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, mh + 10, 150, 20,
                hwnd, nullptr, hInst, nullptr);

            ui->hEditSeq = ::CreateWindowExW(0, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                10, mh + 30, 200, 20,
                hwnd, nullptr, hInst, nullptr);

            ::CreateWindowExW(0, L"STATIC", L"项数:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, mh + 55, 50, 20,
                hwnd, nullptr, hInst, nullptr);

            ui->hEditTerm = ::CreateWindowExW(0, L"EDIT", L"1",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                60, mh + 55, 80, 20,
                hwnd, nullptr, hInst, nullptr);

            ::CreateWindowExW(0, L"STATIC", L"记号:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, mh + 85, 80, 20,
                hwnd, nullptr, hInst, nullptr);

            ui->hBtnNotation = ::CreateWindowExW(0, L"BUTTON", L"选择记号…",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                60, mh + 83, 100, 25,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_NOTATION_BTN)),
                hInst, nullptr);

            ui->hNotationLabel = ::CreateWindowExW(0, L"STATIC", L"",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                170, mh + 87, 280, 20,
                hwnd, nullptr, hInst, nullptr);
            updateNotationLabel(ui);

            ui->hBtnFSalter = ::CreateWindowExW(0, L"BUTTON", L"展开",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, mh + 115, 200, 25,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDM_FSALTER)),
                hInst, nullptr);

            break;
        }
        case WM_ERASEBKGND: {
            if (auto* ui = getUi(hwnd)) {
                HDC hdc = reinterpret_cast<HDC>(wParam);
                RECT r; ::GetClientRect(hwnd, &r);
                ::FillRect(hdc, &r, ui->background.get());
                return 1;
            }
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc = ::BeginPaint(hwnd, &ps);
            if (auto* ui = getUi(hwnd)) {
                RECT r; ::GetClientRect(hwnd, &r);
                ::FillRect(hdc, &r, ui->background.get());
            }
            ::EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN: {
            auto* ui = getUi(hwnd);
            if (!ui) return ::DefWindowProcW(hwnd, msg, wParam, lParam);
            HDC hdc = reinterpret_cast<HDC>(wParam);
            ::SetBkMode(hdc, TRANSPARENT);
            ::SetTextColor(hdc, ::GetSysColor(COLOR_WINDOWTEXT));
            return reinterpret_cast<LRESULT>(ui->background.get());
        }
        case WM_COMMAND: {
            auto* ui = getUi(hwnd);
            if (!ui) break;

            const int id = LOWORD(wParam);
            const auto& table = notationTable();

            if (id == IDC_NOTATION_BTN) {
                HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
                    ::GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
                const int sel = showNotationPicker(hInst, hwnd, ui->notation_index);
                if (sel >= 0 && sel < static_cast<int>(table.size())) {
                    ui->notation_index = sel;
                    updateNotationLabel(ui);
                }
                break;
            }
            if (id == IDM_DEFINITION_PICK) {
                HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
                    ::GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
                const int sel = showNotationPicker(hInst, hwnd, ui->notation_index,
                    L"选择要查看定义的记号");
                if (sel >= 0 && sel < static_cast<int>(table.size()))
                    showDefinitionPopup(hInst, hwnd, sel);
                break;
            }

            switch (id) {
            case IDM_HELP:
                ::MessageBoxW(hwnd,
                    L"代码署名\n"
                    L"部分记号的代码修改自hypcos的notation-explorer\n"
                    L"部分记号的代码修改自笑姐姐(SmileLee-lyx)的NER\n"
                    L"曹知秋 记号提供给AI的定义使用《大数理论》的原文\n"
                    L"MrSS的定义来自 AAA滚木批发 (QQ3682911373)\n"
                    L"ε-Y,X-Y的代码修改自Go men的代码gomen520/gomen520.github.io"
                    L"Den的代码修改自SmileLee/ne-rewritten"
                    L"UPMS由test_alpha0定义\n"
                    L"\n"
                    L"BMS的代码修改自bmsmat，源仓库由Fish,kotetian,kyodaisuu提交\n"
                    L"请注意 代码系利用人工智能技术生成",
                    L"帮助", MB_OK | MB_ICONINFORMATION);
                break;

            case IDM_ABOUT:
                ::MessageBoxW(hwnd,
                    L"展开器\n\n"
                    L"本软件是可自由分发的自由软件，使用 GNU 通用公共许可证 第三版本 授权"
                    L"By Cream-CN\n",
                    L"关于", MB_OK | MB_ICONINFORMATION);
                break;

            case IDM_LEGAL: {
                HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
                    ::GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
                showLegalWindow(hInst, hwnd);
                break;
            }

            case IDM_EXIT:
                ::PostQuitMessage(0);
                break;

            /*case IDM_ENTRY_DEMO:
                showEntryDemo(hwnd, ui);
                break;*/

            case IDM_FS:
            case IDM_FSALTER: {
                int sel = ui->notation_index;
                if (sel < 0 || sel >= static_cast<int>(table.size())) sel = 0;
                const NotationEntry& entry = table[sel];

                std::string text;

                if (entry.expand_text) {
                    wchar_t bufSeq[512]{};
                    wchar_t bufTerm[64]{};
                    ::GetWindowTextW(ui->hEditSeq, bufSeq, 512);
                    ::GetWindowTextW(ui->hEditTerm, bufTerm, 64);

                    std::string utf8Seq = wstring_to_utf8(bufSeq);
                    int term = 0;
                    try {
                        term = std::stoi(wstring_to_utf8(bufTerm));
                    }
                    catch (...) {
                        ::MessageBoxW(hwnd, L"项数必须是整数，\n所以说你为什么要这样输入", L"错误",
                            MB_OK | MB_ICONERROR);
                        break;
                    }
                    if (term < 1) {
                        ::MessageBoxW(hwnd, L"项数必须为正整数，\n你难道要尝试向内存反方向写入序列吗", L"错误",
                            MB_OK | MB_ICONERROR);
                        break;
                    }
                    if (utf8Seq.empty()) {
                        ::MessageBoxW(hwnd, L"序列不能为空", L"错误",
                            MB_OK | MB_ICONERROR);
                        break;
                    }

                    if (entry.expand_text_alter && id == IDM_FSALTER) {
                        // 记号自带 FS_alter（较长）展开：直接使用，不做通用截断
                        text = entry.expand_text_alter(utf8Seq, term);
                    }
                    else {
                        text = entry.expand_text(utf8Seq, term);

                        // [FS 语义分派]（仅 IDM_FS 且记号未自带 FS 语义时截断）
                        //   BMS  → 删除最后一列（BMS 标准文本）
                        //   UPMS → 删除最后一列（省略尾随零文本）
                        //   MrSS → pop_back（向量语义）
                        if (id == IDM_FS && !entry.expand_text_alter) {
                            if (isBmsTextEntry(entry)) {
                                long t = term;
                                omegay::common::Matrix m =
                                    omegay::common::parse_matrix(text, t);
                                if (m.cols() > 1) {
                                    m.resize_cols(m.cols() - 1);
                                    text = omegay::common::matrix_to_string(m);
                                }
                            }
                            else if (entry.expand_text == &upmsExpandText) {
                                auto m = parseUpmsText(text);
                                if (m.cols() > 1) {
                                    m.resize_cols(m.cols() - 1);
                                    text = formatUpmsText(m);
                                }
                            }
                            else {
                                auto parsed = notation::Mrss121Notation::parse(text);
                                if (parsed && parsed->size() > 1) {
                                    parsed->pop_back();
                                    text = notation::Mrss121Notation::to_string(*parsed);
                                }
                            }
                        }
                    }
                }
                else {
                    std::vector<int> seq;
                    int term = 0;
                    if (!readSeqAndTerm(hwnd, ui, seq, term)) break;

                    std::vector<int> result = entry.expand(seq, term);

                    if (id == IDM_FS && result.size() > 1)
                        result.pop_back();

                    text = seq_to_string(result);
                }

                text += entry.suffix();
                std::wstring wtext = utf8_to_wstring(text);
                ::MessageBoxW(hwnd, wtext.c_str(), L"展开结果",
                    MB_OK | MB_ICONINFORMATION);
                break;
            }

            default:
                break;
            }
            break;
        }
        case WM_DESTROY: {
            delete getUi(hwnd);
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            ::PostQuitMessage(0);
            break;
        }
        default:
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

} // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    ::LoadLibraryW(L"Msftedit.dll");

    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES;
    ::InitCommonControlsEx(&icc);

    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OmegaY";
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!::RegisterClassW(&wc)) return 0;

    HWND hwnd = ::CreateWindowExW(
        0, L"OmegaY", L"Expander",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 470, 250,
        nullptr, nullptr, hInstance, nullptr);
    if (!hwnd) return 0;

    ::ShowWindow(hwnd, nCmdShow);
    ::UpdateWindow(hwnd);

    MSG msg;
    while (::GetMessageW(&msg, nullptr, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
    return 0;
}
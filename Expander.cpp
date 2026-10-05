//Copyright(c) Cream-CN 2026
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <richedit.h>

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
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <unordered_map>

using namespace omegay;
using namespace omegay::common;
using namespace omegay::notation;

namespace {
    using Bms = omegay::notation::BMSFamilyNotation;

    constexpr int IDM_ABOUT = 1001;
    constexpr int IDM_EXIT = 1002;
    constexpr int IDM_HELP = 1003;
    constexpr int IDM_LEGAL = 1004;
    constexpr int IDM_FS = 101;
    constexpr int IDM_FSALTER = 102;
    constexpr int IDM_ENTRY_DEMO = 103;
    constexpr int IDC_NOTATION_COMBO = 2001;
    constexpr int IDM_DEFINITION_BASE = 3000;
    constexpr int IDC_DEFINITION_EDIT = 4001;
    constexpr wchar_t kDefWindowClass[] = L"OmegaYDefPopup";
    constexpr int IDC_POPUP_DEF_EDIT = 5001;
    constexpr int IDC_POPUP_DEF_CLOSE = 5002;
    constexpr wchar_t kLegalWindowClass[] = L"OmegaYLegal";
    constexpr int IDC_LEGAL_EDIT = 6001;
    HWND g_hLegalWindow = nullptr;

    struct NotationEntry {
        const wchar_t* display_name;
        const char* id;
        std::vector<int>(*expand)(const std::vector<int>&, int);
        std::string(*suffix)();
        // 定义文本的键。空表示无定义。文本从内置表懒加载，
        // 见 DefinitionStore。约定通常直接等于 id。
        const char* def_key;

        // 可选的文本展开入口。若不为空，UI 会优先用它处理输入文本，
        // 签名约定：std::string expand_string(std::string_view, int)
        std::string(*expand_text)(std::string_view, int) = nullptr;
    };

    std::string bmsExpandTextWith(
        Bms::Version ver, std::string_view text, int term)
    {
        long parsedTerm = term;                 // parse_matrix 需要 long&
        omegay::common::Matrix m =
            omegay::common::parse_matrix(text, parsedTerm);

        if (m.empty())
            return "（无法解析出任何列，请检查 BMS 文本格式）";

        if (!omegay::notation::BMSFamilyNotation::is_standard(m, ver))
            return "（非标准形式，BMS 拒绝展开）";

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
            return "（无法解析出任何列，请检查 UPMS 文本格式）";

        const auto result = omegay::notation::UPMSNotation::expand(m, term);
        const auto out = formatUpmsText(result);
        if (out.empty())
            return "（后继形展开为空矩阵，或未能定位坏根）";
        return out;
    }

    // ---- 定义文本内置存储 ----
    //
    // 文本不再从 RT_RCDATA 资源读取，而是写死在本文件的 builtin 表中。
    // key 与 notationTable() 里的 def_key 一一对应。
    // 换行统一用 \r\n，供 RichEdit 显示。
    // 找不到 key 时返回空串，由 get() 回退到"（暂无定义）"。
    class DefinitionStore {
    public:
        static DefinitionStore& instance() {
            static DefinitionStore s;
            return s;
        }

        [[nodiscard]] const std::wstring& get(const char* key) {
            if (key == nullptr || key[0] == '\0') return missing_;

            auto it = cache_.find(key);
            if (it != cache_.end()) return it->second;

            std::wstring text = loadBuiltin(key);
            if (text.empty()) text = missing_;

            return cache_.emplace(key, std::move(text)).first->second;
        }

    private:
        DefinitionStore() : missing_(L"（暂无定义）") {}
        DefinitionStore(const DefinitionStore&) = delete;
        DefinitionStore& operator=(const DefinitionStore&) = delete;

        [[nodiscard]] static std::wstring loadBuiltin(const char* key) {
            static const std::unordered_map<std::string, std::wstring> builtin = {
                { "empty", L"空记号的定义。\r\n"
                           L"\r\n"
                           L"（在此填写 EmptyNotation 的定义正文。）\r\n" },

                { "pps", L"PPS 的定义。\r\n"
                         L"\r\n"
                         L"（在此填写 PPSNotation 的定义正文。）\r\n" },

                { "pps4", L"PPS4 的定义。\r\n"
                          L"\r\n"
                          L"（在此填写 PPS4Notation 的定义正文。）\r\n" },

                { "wpps4", L"Weak PPS4 的定义。\r\n"
                           L"\r\n"
                           L"（在此填写 WPPS4Notation 的定义正文。）\r\n" },

                { "tpps4", L"Third PPS4 的定义。\r\n"
                           L"\r\n"
                           L"（在此填写 TPPS4Notation 的定义正文。）\r\n" },

                { "ewpps4", L"Ex. Weak PPS4 的定义。\r\n"
                            L"\r\n"
                            L"（在此填写 EWPPS4Notation 的定义正文。）\r\n" },

                { "spps4", L"Second PPS4 的定义。\r\n"
                           L"\r\n"
                           L"（在此填写 SecondPPS4Notation 的定义正文。）\r\n" },

                { "2-pps4", L"2-pps4 的定义。\r\n"
                            L"\r\n"
                            L"（在此填写 PPS2Notation 的定义正文。）\r\n" },

                { "omega-y-medium", L"ω-Y (medium) 的定义。\r\n"
                                    L"\r\n"
                                    L"（在此填写 OmegaYMediumNotation 的定义正文。）\r\n" },

                { "omega-y-strong", L"ω-Y (strong) 的定义。\r\n"
                                    L"\r\n"
                                    L"（在此填写 OmegaYStrongNotation 的定义正文。）\r\n" },

                { "mrss121", L"MrSS1.2.1 的定义。\r\n"
                             L"\r\n"
                             L"（在此填写 Mrss121Notation 的定义正文。）\r\n" },

                { "bms-v1",  L"BMS v1.0 的定义。\r\n\r\n（在此填写定义正文。）\r\n" },
                { "bms-v2",  L"BMS v2.0 的定义。\r\n\r\n（在此填写定义正文。）\r\n" },
                { "bms-v21", L"BMS v2.1 的定义。\r\n\r\n（在此填写定义正文。）\r\n" },
                { "bms-v22", L"BMS v2.2 的定义。\r\n\r\n（在此填写定义正文。）\r\n" },
                { "bms-v23", L"BMS v2.3 的定义。\r\n\r\n（在此填写定义正文。）\r\n" },
                { "bms-v3",  L"BMS v3.0 的定义。\r\n\r\n（在此填写定义正文。）\r\n" },
                { "bms-v31", L"BMS v3.1 的定义。\r\n\r\n（在此填写定义正文。）\r\n" },
                { "bms-v32", L"BMS v3.2 的定义。\r\n\r\n（在此填写定义正文。）\r\n" },
                { "bms-v33", L"BMS v3.3 的定义。\r\n\r\n（在此填写定义正文。）\r\n" },
                { "bms-v4",  L"BMS v4.0 的定义。\r\n\r\n（在此填写定义正文。）\r\n" },

                { "omega-y-sequence", L"ω-Y sequence 的定义。\r\n"
                                      L"\r\n"
                                      L"（在此填写 OmegaYNotation 的定义正文。）\r\n" },

                { "epsilon-y", L"ε-Y 的定义。\r\n"
                               L"\r\n"
                               L"（在此填写 EpsilonYNotation 的定义正文。）\r\n" },

                { "upms", L"UPMS 的定义。\r\n"
                          L"\r\n"
                          L"（在此填写 UPMSNotation 的定义正文。）\r\n" },
            };

            if (key == nullptr || key[0] == '\0') return {};

            auto it = builtin.find(key);
            if (it == builtin.end()) return {};
            return it->second;
        }

        std::unordered_map<std::string, std::wstring> cache_;
        std::wstring missing_;
    };

    // 拼装"【名称】\r\n\r\n + 定义正文"
    [[nodiscard]] std::wstring formatDefinition(const NotationEntry& entry) {
        std::wstring text;
        text += L"【";
        text += entry.display_name;
        text += L"】\r\n\r\n";
        text += DefinitionStore::instance().get(entry.def_key);
        return text;
    }

    [[nodiscard]] const std::vector<NotationEntry>& notationTable() {
        static const std::vector<NotationEntry> table = {
            { L"空记号", "empty",
              &notation::EmptyNotation::expand,
              &notation::EmptyNotation::suffix,
              "empty" },

            { L"PPS", "pps",
              &notation::PPSNotation::expand,
              &notation::PPSNotation::suffix,
              "pps" },

            { L"PPS4", "pps4",
              &notation::PPS4Notation::expand,
              &notation::PPS4Notation::suffix,
              "pps4" },

            { L"Weak PPS4", "wpps4",
              &notation::WPPS4Notation::expand,
              &notation::WPPS4Notation::suffix,
              "wpps4" },

            { L"Third PPS4", "tpps4",
              &notation::TPPS4Notation::expand,
              &notation::TPPS4Notation::suffix,
              "tpps4" },

            { L"Ex. Weak PPS4", "ewpps4",
              &notation::EWPPS4Notation::expand,
              &notation::EWPPS4Notation::suffix,
              "ewpps4" },

            { L"Second PPS4", "spps4",
              &notation::SecondPPS4Notation::expand,
              &notation::SecondPPS4Notation::suffix,
              "spps4" },

            { L"2-pps4", "2-pps4",
              &notation::PPS2Notation::expand,
              &notation::PPS2Notation::suffix,
              "2-pps4" },

            { L"ω-Y (medium)", "omega-y-medium",
              &notation::OmegaYMediumNotation::expand,
              &notation::OmegaYMediumNotation::suffix,
              "omega-y-medium" },

            { L"ω-Y (strong)", "omega-y-strong",
              &notation::OmegaYStrongNotation::expand,
              &notation::OmegaYStrongNotation::suffix,
              "omega-y-strong" },

            { L"MrSS1.2.1", "mrss121",
              &notation::Mrss121Notation::expand,
              &notation::Mrss121Notation::suffix,
              "mrss121",
              &notation::Mrss121Notation::expand_string },

              // [BMS] 每个版本一条独立记号：expand 置 nullptr，走 expand_text 文本入口
              { L"BMS v1.0", "bms-v1",
                nullptr, &notation::BMSFamilyNotation::suffix,
                "bms-v1", &bmsExpandV1 },

              { L"BMS v2.0", "bms-v2",
                nullptr, &notation::BMSFamilyNotation::suffix,
                "bms-v2", &bmsExpandV2 },

              { L"BMS v2.1", "bms-v21",
                nullptr, &notation::BMSFamilyNotation::suffix,
                "bms-v21", &bmsExpandV21 },

              { L"BMS v2.2", "bms-v22",
                nullptr, &notation::BMSFamilyNotation::suffix,
                "bms-v22", &bmsExpandV22 },

              { L"BMS v2.3", "bms-v23",
                nullptr, &notation::BMSFamilyNotation::suffix,
                "bms-v23", &bmsExpandV23 },

              { L"BMS v3.0", "bms-v3",
                nullptr, &notation::BMSFamilyNotation::suffix,
                "bms-v3", &bmsExpandV3 },

              { L"BMS v3.1", "bms-v31",
                nullptr, &notation::BMSFamilyNotation::suffix,
                "bms-v31", &bmsExpandV31 },

              { L"BMS v3.2", "bms-v32",
                nullptr, &notation::BMSFamilyNotation::suffix,
                "bms-v32", &bmsExpandV32 },

              { L"BMS v3.3", "bms-v33",
                nullptr, &notation::BMSFamilyNotation::suffix,
                "bms-v33", &bmsExpandV33 },

              { L"BMS v4.0", "bms-v4",
                nullptr, &notation::BMSFamilyNotation::suffix,
                "bms-v4", &bmsExpandV4 },

              { L"ω-Y sequence", "omega-y-sequence",
                &notation::OmegaYNotation::expand,
                &notation::OmegaYNotation::suffix,
                "omega-y-sequence" },

              { L"ε-Y", "epsilon-y",
                &notation::EpsilonYNotation::expand,
                &notation::EpsilonYNotation::suffix,
                "epsilon-y" },

              { L"UPMS", "upms",
                nullptr, &notation::UPMSNotation::suffix,
                "upms", &upmsExpandText },
        };
        return table;
    }

    struct MenuDeleter { void operator()(HMENU m)  const noexcept { if (m) ::DestroyMenu(m); } };
    struct BrushDeleter { void operator()(HBRUSH b) const noexcept { if (b) ::DeleteObject(b); } };
    using MenuPtr = std::unique_ptr<std::remove_pointer_t<HMENU>, MenuDeleter>;
    using BrushPtr = std::unique_ptr<std::remove_pointer_t<HBRUSH>, BrushDeleter>;

    struct UiState {
        HWND hEditSeq{};
        HWND hEditTerm{};
        HWND hBtnFS{};
        HWND hBtnFSalter{};
        HWND hComboNotation{};
        HWND hRichDef{};
        BrushPtr background;
    };

    [[nodiscard]] UiState* getUi(HWND h) {
        return reinterpret_cast<UiState*>(::GetWindowLongPtrW(h, GWLP_USERDATA));
    }
    [[nodiscard]] HMENU createMenuBar() {
        HMENU hMenu = ::CreateMenu();

        HMENU hFile = ::CreatePopupMenu();
        ::AppendMenuW(hFile, MF_STRING, IDM_EXIT, L"退出(&X)");
        ::AppendMenuW(hMenu, MF_POPUP,
            reinterpret_cast<UINT_PTR>(hFile), L"文件(&F)");
        HMENU hDef = ::CreatePopupMenu();
        {
            const auto& table = notationTable();
            for (int i = 0; i < static_cast<int>(table.size()); ++i) {
                std::wstring label = table[i].display_name;
                label += L" 定义(&";
                label += static_cast<wchar_t>(L'A' + (i % 26));
                label += L")";
                ::AppendMenuW(hDef, MF_STRING,
                    static_cast<UINT_PTR>(IDM_DEFINITION_BASE + i),
                    label.c_str());
            }
            ::AppendMenuW(hDef, MF_SEPARATOR, 0, nullptr);
            ::AppendMenuW(hDef, MF_STRING, IDM_ENTRY_DEMO, L"Entry 演示(&E)...");
        }
        ::AppendMenuW(hMenu, MF_POPUP,
            reinterpret_cast<UINT_PTR>(hDef), L"定义(&D)");

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
    void showDefinition(UiState* ui, int index) {
        const auto& table = notationTable();
        if (!ui->hRichDef) return;
        if (index < 0 || index >= static_cast<int>(table.size())) return;

        const std::wstring text = formatDefinition(table[index]);
        ::SetWindowTextW(ui->hRichDef, text.c_str());

        ::SendMessageW(ui->hRichDef, EM_SETSEL, 0, 0);
        ::SendMessageW(ui->hRichDef, EM_SCROLLCARET, 0, 0);
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

    void fillPopupDefinition(HWND hRich, const NotationEntry& entry) {
        const std::wstring text = formatDefinition(entry);
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
                fillPopupDefinition(hRich, table[index]);
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
            ::ShowWindow(hPopup, SW_SHOW);
            ::UpdateWindow(hPopup);
        }
    }

    // 内置 GPLv3 文本。不再从 RT_RCDATA 资源读取。
    // 换行统一用 \r\n，供 RichEdit 显示。
    [[nodiscard]] std::wstring loadLicenseText(HINSTANCE /*hInst*/) {
        return
            L"GNU GENERAL PUBLIC LICENSE\r\n"
            L"Version 3, 29 June 2007\r\n"
            L"\r\n"
            L"（此处为 GPLv3 全文占位。可将完整文本粘贴到此字符串中，\r\n"
            L"换行请使用 \\r\\n。）\r\n";
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
                // 预格式化文本：清零 RichEdit 默认段落边距，并关闭自动换行
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
            0, kLegalWindowClass, L"法律声明 - GNU GPLv3",
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
    void showEntryDemo(HWND hwnd, UiState* ui) {
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
    }
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

            ui->hEditSeq = ::CreateWindowExW(0, L"EDIT", L"1,2,3",
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

            ui->hComboNotation = ::CreateWindowExW(
                0, L"COMBOBOX", nullptr,
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                100, mh + 85, 220, 200,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_NOTATION_COMBO)),
                hInst, nullptr);
            {
                const auto& table = notationTable();
                for (int i = 0; i < static_cast<int>(table.size()); ++i) {
                    ::SendMessageW(ui->hComboNotation, CB_ADDSTRING, 0,
                        reinterpret_cast<LPARAM>(table[i].display_name));
                }
                ::SendMessageW(ui->hComboNotation, CB_SETCURSEL, 0, 0);
            }

            ui->hBtnFS = ::CreateWindowExW(0, L"BUTTON", L"移除末项",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, mh + 115, 120, 25,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDM_FS)),
                hInst, nullptr);

            ui->hBtnFSalter = ::CreateWindowExW(0, L"BUTTON", L"保留末项",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                140, mh + 115, 120, 25,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDM_FSALTER)),
                hInst, nullptr);
            ui->hRichDef = ::CreateWindowExW(
                0, MSFTEDIT_CLASS, L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
                ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                10, mh + 150, 360, 110,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_DEFINITION_EDIT)),
                hInst, nullptr);
            applyRichEdit10pt(ui->hRichDef);
            showDefinition(ui, 0);

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
            if (id == IDC_NOTATION_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
                int sel = static_cast<int>(
                    ::SendMessageW(ui->hComboNotation, CB_GETCURSEL, 0, 0));
                showDefinition(ui, sel);
                break;
            }
            if (id >= IDM_DEFINITION_BASE &&
                id < IDM_DEFINITION_BASE + static_cast<int>(table.size())) {
                const int idx = id - IDM_DEFINITION_BASE;
                HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
                    ::GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
                showDefinitionPopup(hInst, hwnd, idx);
                break;
            }

            switch (id) {
            case IDM_HELP:
                ::MessageBoxW(hwnd,
                    L"代码署名\n"
                    L"Hypcos 部分记号的代码修改自notation-explorer\n"
                    L"SmileLee-lyx 部分记号的代码修改自 NER\n"
                    L"曹知秋 记号提供给AI的定义使用《大数理论》的原文\n"
                    L"MrSS的定义来自 AAA滚木批发 (QQ3682911373)\n"
                    L"ε-Y的代码修改自Go men的代码"
                    L"UPMS由test_alpha0定义"
                    L"BMS的代码修改自bmsmat，源仓库由Fish,kotetian,kyodaisuu"
                    L"请注意 代码系利用人工智能技术生成",
                    L"帮助", MB_OK | MB_ICONINFORMATION);
                break;

            case IDM_ABOUT:
                ::MessageBoxW(hwnd,
                    L"ω-Y 展开器\n\n"
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

            case IDM_ENTRY_DEMO:
                showEntryDemo(hwnd, ui);
                break;

            case IDM_FS:
            case IDM_FSALTER: {
                int sel = static_cast<int>(
                    ::SendMessageW(ui->hComboNotation, CB_GETCURSEL, 0, 0));
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
                        ::MessageBoxW(hwnd, L"项数必须是整数", L"错误",
                            MB_OK | MB_ICONERROR);
                        break;
                    }
                    if (term < 1) {
                        ::MessageBoxW(hwnd, L"项数必须为正整数", L"错误",
                            MB_OK | MB_ICONERROR);
                        break;
                    }
                    if (utf8Seq.empty()) {
                        ::MessageBoxW(hwnd, L"序列不能为空", L"错误",
                            MB_OK | MB_ICONERROR);
                        break;
                    }

                    text = entry.expand_text(utf8Seq, term);

                    // [FS 语义分派]
                    //   BMS  → 删除最后一列（BMS 标准文本）
                    //   UPMS → 删除最后一列（省略尾随零文本）
                    //   MrSS → pop_back（向量语义）
                    if (id == IDM_FS) {
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

    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OmegaY";
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!::RegisterClassW(&wc)) return 0;

    HWND hwnd = ::CreateWindowExW(
        0, L"OmegaY", L"ω-Y 展开器 (重构中)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 400,
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
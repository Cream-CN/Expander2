//Copyright(c) Cream-CN 2026
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <gdiplus.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")

#include "Header/common/utf8.hpp"
#include "Header/core/arena.hpp"
#include "Header/core/entry.hpp"
#include "Header/notation/registry.hpp"   // OOP 内核：全部记号 + 统一接口 + 注册中心
#include "Header/notation/translator.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <array>
#include <algorithm>
#include <cwctype>
#include <cstdint>
#include <shellapi.h>   // ShellExecuteW：打开友情链接
#include "Resource.h"
#include "Definitions.h"   // 记号定义文本由标头编译进程序，绕过 rc.exe
#include "HandA.h"         // 关于(&A)：展示软件信息（向下滚动的只读文本框）
#include "linksFreind.h"   // 友情链接(&F)：showsOfFriLnks 结构体数组

using namespace omegay;
using namespace omegay::common;
using namespace omegay::notation;

namespace {
    using Bms = omegay::notation::BMSFamilyNotation;

    /*constexpr int IDM_ABOUT = 1001;
    constexpr int IDM_EXIT = 1002;*/
    constexpr int IDM_LEGAL = 1004;
    constexpr int IDM_THIRDPARTY_LICENSE = 1005; // 帮助菜单：第三方开源软件许可证
    constexpr int IDM_FRIENDLINK = 1006;         // 帮助菜单：友情链接
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
    constexpr wchar_t kLicenseWindowClass[] = L"OmegaYLicense";
    constexpr int IDC_LICENSE_EDIT = 6002;
    HWND g_hLicenseWindow = nullptr;
    // ---- 展开结果展示窗体：只读 RichEdit 5.0（Msftedit.dll / MSFTEDIT_CLASS） ----
    constexpr wchar_t kResultWindowClass[] = L"OmegaYResult";
    constexpr int IDC_RESULT_EDIT = 6003;
    HWND g_hResultWindow = nullptr;

    // ---- 记号选择窗体 ----
    constexpr wchar_t kPickerWindowClass[] = L"OmegaYPicker";
    constexpr int IDC_PICKER_SEARCH = 7101;
    constexpr int IDC_PICKER_LIST = 7102;
    constexpr int IDC_PICKER_STATUS = 7103;

    // ---- 互译器页面 ----
    constexpr wchar_t kTranslatorWindowClass[] = L"OmegaYTranslator";
    constexpr int IDC_TRANSLATOR_COMBO = 7201;   // 互译器下拉选择
    constexpr int IDC_TRANSLATOR_INPUT = 7202;   // 输入（源记号表达式）
    constexpr int IDC_TRANSLATOR_OUTPUT = 7203;  // 输出（目标记号表达式，只读）
    constexpr int IDC_TRANSLATOR_BTN = 7204;     // “互译”按钮
    constexpr int IDC_TRANSLATOR_STATUS = 7205;  // 状态栏
    constexpr int IDC_TRANSLATOR_DESC = 7206;    // 互译器说明
    constexpr int IDC_TRANSLATOR_DIR = 7207;     // 方向（源 → 目标）
    constexpr int IDM_TRANSLATOR = 110;          // 工具菜单：“互译器”
    HWND g_hTranslatorWindow = nullptr;

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

        // 可选的“参数化族”文本展开入口。非空时，选中该记号会在主窗口显示一个
        // 额外的整数参数输入框（如 Minus1 系列的元组宽度 n），展开时把参数一并传入。
        // 签名约定：std::string expand_text_n(std::string_view text, int term, int param)
        std::string(*expand_text_n)(std::string_view, int, int) = nullptr;
        // 可选的“参数化族”FS_alter 文本展开入口（如 BT v3 的 FS_short）。
        // 非空且按下 FS_alter 按钮时使用本入口，否则使用 expand_text_n。
        std::string(*expand_text_n_alter)(std::string_view, int, int) = nullptr;
        int param_min = 0;          // 参数允许的最小整数值（含）
        int param_max = 25;         // 参数允许的最大整数值（含）
        int param_default = 0;      // 选中该记号时输入框的默认值
        const wchar_t* param_label = L"n:";  // 参数输入框前的标签

        // 记号标签（用于选择窗体的搜索与展示）。最多 16 个，以 nullptr 结尾；
        // 未用的槽位保持 nullptr。可在下表按需增删。
        std::array<const wchar_t*, 16> tags{};

        // OOP 内核指针（2026-11 重构）：指向注册中心中的 INotation 实例。
        // 上面的函数指针字段保留（由注册中心派生、语义等价），但展开调用
        // 统一走本指针的 INotation::expand()（含 FS 截断与后缀语义）。
        const omegay::notation::INotation* notation = nullptr;
    };

    // 记号表由 OOP 注册中心派生（2026-11 深度重构）：
    //   单一事实来源 = Header/notation/registry.hpp 的 register_all_notations()；
    //   本表保留 NotationEntry 结构（对外接口不变），字段由注册中心派生，
    //   宽字符元数据在此一次性转换并静态驻留。展开调用统一走 entry.notation
    //   ->expand()（含 FS 截断与后缀语义），函数指针字段仅供 UI 能力判断。
    [[nodiscard]] const std::vector<NotationEntry>& notationTable() {
        static const std::vector<NotationEntry> notation_table = [] {
            struct WideStore {
                // deque（而非 vector）：元素插入不会使已 push 的 wstring 的 c_str() 指针失效。
                // 用 vector 混存 display_name / tags 时，后续 push_back 触发重分配会让此前
                // 保存的指针悬垂 → 列表显示乱码/不可见。display_name 与 tags 必须存同一容器，
                // 且该容器须在插入时保持已有元素指针有效，故用 deque。
                std::deque<std::wstring> names;
                std::deque<std::wstring> labels;
                std::vector<std::array<const wchar_t*, 16>> tags;
            };
            static WideStore store;

            const auto& notations = omegay::notation::registered_notations();
            std::vector<NotationEntry> t;
            t.reserve(notations.size());
            for (const auto& p : notations) {
                const auto& meta = p->meta();
                const auto& caps = p->caps();
                NotationEntry e{};
                store.names.push_back(utf8_to_wstring(meta.display_name));
                e.display_name = store.names.back().c_str();
                e.id = meta.id.data();
                e.expand = caps.expand_seq;
                e.suffix = caps.suffix;
                e.definition_resource_id = meta.definition_resource_id;
                e.expand_text = caps.text;
                e.expand_text_alter = caps.text_alter;
                e.expand_text_n = caps.text_n;
                e.expand_text_n_alter = caps.text_n_alter;
                e.param_min = meta.param_min;
                e.param_max = meta.param_max;
                e.param_default = meta.param_default;
                store.labels.push_back(utf8_to_wstring(meta.param_label));
                e.param_label = store.labels.back().c_str();
                std::array<const wchar_t*, 16> tagArr{};
                int slot = 0;
                for (const auto& tag : meta.tags) {
                    if (tag.empty() || slot >= 16) break;
                    store.names.push_back(utf8_to_wstring(tag));
                    tagArr[slot++] = store.names.back().c_str();
                }
                store.tags.push_back(std::move(tagArr));
                e.tags = store.tags.back();
                e.notation = p.get();
                t.push_back(std::move(e));
            }
            return t;
        }();
        return notation_table;
    }

    // ---- 互译器表 ----
    // 登记在“互译器页面”中的所有互译器。契约见 Header/notation/translator.hpp，
    // 规范见 Docs/互译器规范.md。表很小，直接用普通静态数组承载。
    static const omegay::translator::Translator kTranslatorTable[] = {
            { .display_name = L"恒等互译器",
              .id = "identity",
              .source = L"任意",
              .target = L"相同（不改变）",
              .description = L"占位互译器：把输入原样返回，作为互译器框架的基线与调试工具。"
                             L"选中它输入任意文本，点击“互译”后输出应与输入逐字节一致。",
              .translate = &omegay::translator::identityTranslate },
        };

    static const std::vector<omegay::translator::Translator> translator_table(
        kTranslatorTable,
        kTranslatorTable + sizeof(kTranslatorTable) / sizeof(kTranslatorTable[0]));

    [[nodiscard]] const std::vector<omegay::translator::Translator>& translatorTable() {
        return translator_table;
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

    struct IconStrip; // 顶部图标条（Expander.ico + MFC.ico），定义见互译器页面前的辅助代码

    struct UiState {
        HWND hEditSeq{};
        HWND hEditTerm{};
        HWND hBtnFS{};
        HWND hBtnFSalter{};
        HWND hBtnNotation{};    // “选择记号…”按钮
        HWND hNotationLabel{};  // 当前记号（含标签）显示
        HWND hStaticParam{};    // 参数化记号的参数标签
        HWND hEditParam{};      // 参数化记号的参数输入框
        int notation_index = 0; // 当前选中的记号表索引
        IconStrip* icons{};     // 顶部图标条（Expander.ico + MFC.ico）
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

        // 仅参数化记号显示额外的参数输入框，并在切入时填入默认值。
        if (ui->hStaticParam && ui->hEditParam) {
            const bool hasParam = (e.expand_text_n != nullptr);
            const int show = hasParam ? SW_SHOW : SW_HIDE;
            ::ShowWindow(ui->hStaticParam, show);
            ::ShowWindow(ui->hEditParam, show);
            if (hasParam) {
                ::SetWindowTextW(ui->hStaticParam, e.param_label);
                ::SetWindowTextW(ui->hEditParam,
                    std::to_wstring(e.param_default).c_str());
            }
        }
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

        // 工具：互译器等独立功能页面
        HMENU hTools = ::CreatePopupMenu();
        ::AppendMenuW(hTools, MF_STRING, IDM_TRANSLATOR, L"互译器(&T)...");
        ::AppendMenuW(hMenu, MF_POPUP,
            reinterpret_cast<UINT_PTR>(hTools), L"工具(&T)");

        HMENU hHelp = ::CreatePopupMenu();
        ::AppendMenuW(hHelp, MF_STRING, IDM_ABOUT, L"关于(&A)...");
        ::AppendMenuW(hHelp, MF_STRING, IDM_FRIENDLINK, L"友情链接(&F)...");
        ::AppendMenuW(hHelp, MF_STRING, IDM_LEGAL, L"法律声明(&L)...");
        ::AppendMenuW(hHelp, MF_STRING, IDM_THIRDPARTY_LICENSE,
            L"第三方开源软件许可证(&T)...");
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

    void fillPopupDefinition(HWND hRich, const NotationEntry& entry, HINSTANCE hInst) {
        std::wstring text;
        text += L"【";
        text += entry.display_name;
        text += L"】\r\n\r\n";
        text += loadDefinitionText(hInst, entry.definition_resource_id);

        expander_help_about::applyRichEdit10pt(hRich);
        ::SetWindowTextW(hRich, text.c_str());
        expander_help_about::applyRichEdit10pt(hRich);

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
                expander_help_about::applyRichEdit10pt(hRich, L"Microsoft YaHei");
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

    // ---- 第三方开源软件许可证：内容读取自 defs/law.txt ----
    [[nodiscard]] std::wstring loadThirdPartyLicenseText() {
        // 候选路径：exe 所在目录的 defs/law.txt，其次当前工作目录的 defs/law.txt
        wchar_t exePath[MAX_PATH]{};
        ::GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring exeDir;
        if (exePath[0] != L'\0') {
            exeDir = exePath;
            const size_t slash = exeDir.find_last_of(L"\\/");
            if (slash != std::wstring::npos) exeDir.resize(slash + 1);
        }
        const std::wstring paths[] = {
            exeDir + L"defs\\law.txt",
            L"defs\\law.txt",
        };

        for (const std::wstring& path : paths) {
            HANDLE hFile = ::CreateFileW(
                path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile == INVALID_HANDLE_VALUE) continue;

            DWORD size = ::GetFileSize(hFile, nullptr);
            std::string utf8;
            if (size > 0) {
                utf8.resize(size);
                DWORD read = 0;
                if (!::ReadFile(hFile, utf8.data(), size, &read, nullptr))
                    utf8.clear();
                else
                    utf8.resize(read);
            }
            ::CloseHandle(hFile);

            std::string_view view(utf8);
            if (view.size() >= 3 &&
                static_cast<unsigned char>(view[0]) == 0xEF &&
                static_cast<unsigned char>(view[1]) == 0xBB &&
                static_cast<unsigned char>(view[2]) == 0xBF) {
                view.remove_prefix(3); // 跳过 UTF-8 BOM
            }

            std::wstring w = utf8_to_wstring(view);
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
        return L"未找到 defs/law.txt 文件。";
    }

    LRESULT CALLBACK LicenseWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_LICENSE_EDIT)),
                cs->hInstance, nullptr);

            if (hRich && text) {
                ::SetWindowTextW(hRich, text->c_str());
                expander_help_about::applyRichEdit10pt(hRich, L"Microsoft YaHei");
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
            g_hLicenseWindow = nullptr;
            break;
        default:
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    void registerLicenseClass(HINSTANCE hInst) {
        static bool registered = false;
        if (registered) return;

        WNDCLASSW wc{};
        wc.lpfnWndProc = LicenseWndProc;
        wc.hInstance = hInst;
        wc.lpszClassName = kLicenseWindowClass;
        wc.hbrBackground = ::GetSysColorBrush(COLOR_WINDOW);
        wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        ::RegisterClassW(&wc);
        registered = true;
    }

    void showLicenseWindow(HINSTANCE hInst, HWND hParent) {
        if (::IsWindow(g_hLicenseWindow)) {
            ::ShowWindow(g_hLicenseWindow, SW_RESTORE);
            ::SetForegroundWindow(g_hLicenseWindow);
            return;
        }

        registerLicenseClass(hInst);
        auto* text = new std::wstring(loadThirdPartyLicenseText());
        if (text->empty())
            *text = L"（defs/law.txt 当前为空，第三方开源软件许可证文本尚未填写。）";

        HWND hwnd = ::CreateWindowExW(
            0, kLicenseWindowClass, L"第三方开源软件许可证",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 760, 640,
            hParent, nullptr, hInst, text);

        if (!hwnd) {
            delete text;
            return;
        }
        g_hLicenseWindow = hwnd;
        ::ShowWindow(hwnd, SW_SHOW);
        ::UpdateWindow(hwnd);
    }

    // ---- 展开结果展示窗体：只读 RichEdit 5.0（Msftedit.dll / MSFTEDIT_CLASS） ----
    // 取代原 MessageBox 弹窗：结果可能极长，RichEdit 支持选择/复制与双向滚动。
    void setResultText(HWND hRich, const std::wstring& text) {
        expander_help_about::applyRichEdit10pt(hRich);
        ::SetWindowTextW(hRich, text.c_str());
        expander_help_about::applyRichEdit10pt(hRich);
        ::SendMessageW(hRich, EM_SETMARGINS,
            EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(0, 0));
        ::SendMessageW(hRich, EM_SETTARGETDEVICE, 0, 0);
        ::SendMessageW(hRich, EM_SETSEL, 0, 0);
        ::SendMessageW(hRich, EM_SCROLLCARET, 0, 0);
    }

    LRESULT CALLBACK ResultWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_RESULT_EDIT)),
                cs->hInstance, nullptr);

            if (hRich && text) setResultText(hRich, *text);
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
            g_hResultWindow = nullptr;
            break;
        default:
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    void registerResultClass(HINSTANCE hInst) {
        static bool registered = false;
        if (registered) return;

        WNDCLASSW wc{};
        wc.lpfnWndProc = ResultWndProc;
        wc.hInstance = hInst;
        wc.lpszClassName = kResultWindowClass;
        wc.hbrBackground = ::GetSysColorBrush(COLOR_WINDOW);
        wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        ::RegisterClassW(&wc);
        registered = true;
    }

    void showResultWindow(HINSTANCE hInst, HWND hParent,
        std::wstring text, std::wstring title)
    {
        if (::IsWindow(g_hResultWindow)) {
            HWND hRich = reinterpret_cast<HWND>(
                ::GetWindowLongPtrW(g_hResultWindow, GWLP_USERDATA));
            if (hRich) setResultText(hRich, text);
            if (!title.empty())
                ::SetWindowTextW(g_hResultWindow, title.c_str());
            ::ShowWindow(g_hResultWindow, SW_RESTORE);
            ::SetForegroundWindow(g_hResultWindow);
            return;
        }

        registerResultClass(hInst);
        auto* pText = new std::wstring(std::move(text));
        if (title.empty()) title = L"展开结果";

        HWND hwnd = ::CreateWindowExW(
            0, kResultWindowClass, title.c_str(),
            WS_OVERLAPPEDWINDOW,
            150, 150, 500, 500,   // 固定位置 (150,150)、固定尺寸 500×500
            hParent, nullptr, hInst, pText);

        if (!hwnd) {
            delete pText;
            return;
        }
        g_hResultWindow = hwnd;

        ::ShowWindow(hwnd, SW_SHOW);
        ::UpdateWindow(hwnd);
    }

    // ================= 顶栏图标条（Expander.ico + MFC.ico + namePic.jpg） =================
    // “关于 / 互译器 / 主界面”共用：顶部并排显示软件图标 Expander.ico 与 MFC.ico，
    // 以及右侧的 Assets/namePic.jpg 名字图（等比缩放到与图标同高）。
    // 统一按 48×48 显示。图标/位图句柄归本结构所有，父窗口销毁时须调用 destroyIconStrip 释放。
    constexpr int IDC_ICON_EXPANDER = 7301;
    constexpr int IDC_ICON_MFC      = 7302;
    constexpr int IDC_ICON_NAMEPIC  = 7303;   // Assets/namePic.jpg 名字图

    struct IconStrip {
        HWND  hIcon1{};      // Expander.ico
        HWND  hIcon2{};      // MFC.ico
        HWND  hNamePic{};    // Assets/namePic.jpg（名字图）
        HICON hIcoExpander{};
        HICON hIcoMfc{};
        HBITMAP hBmpNamePic{};  // namePic.jpg 等比缩放后的位图（归本结构所有）
    };

    // 定位资源文件：优先 exe 所在目录，其次当前工作目录（与 defs/law.txt 同款策略）。
    [[nodiscard]] std::wstring findAssetFile(const wchar_t* fileName) {
        wchar_t exePath[MAX_PATH]{};
        ::GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring exeDir;
        if (exePath[0] != L'\0') {
            exeDir = exePath;
            const size_t slash = exeDir.find_last_of(L"\\/");
            if (slash != std::wstring::npos) exeDir.resize(slash + 1);
        }
        const std::wstring candidates[] = { exeDir + fileName, fileName };
        for (const std::wstring& p : candidates) {
            if (::GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES)
                return p;
        }
        return std::wstring();
    }

    // 用 GDI+ 从文件加载 JPG，并按目标高度等比缩放为 DIB 位图；失败返回 nullptr。
    [[nodiscard]] HBITMAP loadJpegScaled(const std::wstring& path,
                                         int dstH, int& outW) {
        Gdiplus::Bitmap src(path.c_str());
        if (src.GetLastStatus() != Gdiplus::Ok) return nullptr;
        const int sw = src.GetWidth();
        const int sh = src.GetHeight();
        if (sw <= 0 || sh <= 0) return nullptr;
        const int dstW = std::max(1, (dstH * sw) / sh);
        Gdiplus::Bitmap dst(dstW, dstH, PixelFormat32bppARGB);
        Gdiplus::Graphics g(&dst);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g.Clear(Gdiplus::Color(255, 255, 255, 255));
        g.DrawImage(&src, Gdiplus::Rect(0, 0, dstW, dstH),
                    0, 0, sw, sh, Gdiplus::UnitPixel);
        HBITMAP hbmp = nullptr;
        if (dst.GetHBITMAP(Gdiplus::Color(255, 255, 255), &hbmp) != Gdiplus::Ok)
            return nullptr;
        outW = dstW;
        return hbmp;
    }

    [[nodiscard]] IconStrip* createIconStrip(HWND hParent, HINSTANCE hInst,
                                             int x, int y, int size = 48) {
        auto* s = new IconStrip{};
        s->hIcoExpander = static_cast<HICON>(::LoadImageW(
            hInst, MAKEINTRESOURCEW(IDI_EXPANDER),
            IMAGE_ICON, size, size, LR_DEFAULTCOLOR));
        s->hIcoMfc = static_cast<HICON>(::LoadImageW(
            hInst, MAKEINTRESOURCEW(IDI_MFC),
            IMAGE_ICON, size, size, LR_DEFAULTCOLOR));

        s->hIcon1 = ::CreateWindowExW(0, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | SS_ICON,
            x, y, size, size, hParent,
            reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_ICON_EXPANDER)),
            hInst, nullptr);
        if (s->hIcon1 && s->hIcoExpander) {
            ::SendMessageW(s->hIcon1, STM_SETICON,
                reinterpret_cast<WPARAM>(s->hIcoExpander), 0);
        }

        s->hIcon2 = ::CreateWindowExW(0, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | SS_ICON,
            x + size + 12, y, size, size, hParent,
            reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_ICON_MFC)),
            hInst, nullptr);
        if (s->hIcon2 && s->hIcoMfc) {
            ::SendMessageW(s->hIcon2, STM_SETICON,
                reinterpret_cast<WPARAM>(s->hIcoMfc), 0);
        }

        // 第三个：Assets/namePic.jpg 名字图，等比缩放到与图标同高，位于 MFC.ico 右侧。
        // 文件缺失或解码失败时静默跳过，不影响原有两个图标。
        const std::wstring assetPath = findAssetFile(L"Assets\\namePic.jpg");
        if (!assetPath.empty()) {
            int nameW = 0;
            s->hBmpNamePic = loadJpegScaled(assetPath, size, nameW);
            if (s->hBmpNamePic && nameW > 0) {
                s->hNamePic = ::CreateWindowExW(0, L"STATIC", L"",
                    WS_CHILD | WS_VISIBLE | SS_BITMAP,
                    x + 2 * (size + 12), y, nameW, size, hParent,
                    reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_ICON_NAMEPIC)),
                    hInst, nullptr);
                if (!s->hNamePic) {
                    ::DeleteObject(s->hBmpNamePic);
                    s->hBmpNamePic = nullptr;
                }
                else {
                    ::SendMessageW(s->hNamePic, STM_SETIMAGE,
                        IMAGE_BITMAP, reinterpret_cast<LPARAM>(s->hBmpNamePic));
                }
            }
        }
        return s;
    }

    void destroyIconStrip(IconStrip* s) {
        if (!s) return;
        if (s->hIcoExpander) ::DestroyIcon(s->hIcoExpander);
        if (s->hIcoMfc)     ::DestroyIcon(s->hIcoMfc);
        if (s->hBmpNamePic) ::DeleteObject(s->hBmpNamePic);
    }

    // ================= 互译器页面 =================

    struct TranslatorState {
        HWND hCombo{};    // 互译器下拉选择
        HWND hInput{};    // 输入（源记号表达式）
        HWND hOutput{};   // 输出（目标记号表达式，只读 RichEdit）
        HWND hBtn{};      // “互译”按钮
        HWND hStatus{};   // 状态栏
        HWND hDesc{};     // 互译器说明
        HWND hDir{};      // 方向（源 → 目标）
        IconStrip* icons{}; // 顶部图标条（Expander.ico + MFC.ico）
        BrushPtr background;
        int index = 0;    // 当前选中的互译器表索引
    };

    [[nodiscard]] TranslatorState* getTranslator(HWND h) {
        return reinterpret_cast<TranslatorState*>(::GetWindowLongPtrW(h, GWLP_USERDATA));
    }

    [[nodiscard]] const omegay::translator::Translator& currentTranslator(
        const TranslatorState* st) {
        const auto& table = translatorTable();
        int idx = st ? st->index : 0;
        if (idx < 0 || idx >= static_cast<int>(table.size())) idx = 0;
        return table[idx];
    }

    void updateTranslatorUI(HWND hwnd, TranslatorState* st) {
        const auto& e = currentTranslator(st);
        std::wstring dir = e.source;
        dir += L"  →  ";
        dir += e.target;
        ::SetWindowTextW(st->hDir, dir.c_str());
        ::SetWindowTextW(st->hDesc, e.description);
    }

    void runTranslator(HWND hwnd, TranslatorState* st) {
        const auto& e = currentTranslator(st);
        wchar_t buf[1024]{};
        ::GetWindowTextW(st->hInput, buf, 1024);
        std::string utf8in = wstring_to_utf8(buf);
        if (utf8in.empty()) {
            ::SetWindowTextW(st->hStatus, L"请输入要互译的表达式");
            return;
        }
        std::string result;
        try {
            result = e.translate(utf8in);
        }
        catch (...) {
            // 契约要求互译函数不得抛异常；此处兜底，避免互译器缺陷崩溃程序。
            result = omegay::translator::kParseError;
        }
        setResultText(st->hOutput, utf8_to_wstring(result));
        std::wstring status = L"互译完成：";
        status += e.display_name;
        ::SetWindowTextW(st->hStatus, status.c_str());
    }

    LRESULT CALLBACK TranslatorProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            auto* st = new TranslatorState{};
            st->background.reset(::CreateSolidBrush(::GetSysColor(COLOR_WINDOW)));
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(st));

            HINSTANCE hInst = cs->hInstance;

            // 顶部图标条：Expander.ico + MFC.ico（48×48）；下方内容整体下移 60 像素让位
            st->icons = createIconStrip(hwnd, hInst, 12, 12, 48);

            ::CreateWindowExW(0, L"STATIC", L"互译器:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                12, 74, 56, 18, hwnd, nullptr, hInst, nullptr);

            st->hCombo = ::CreateWindowExW(0, WC_COMBOBOXW, L"",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                68, 70, 250, 220, hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_TRANSLATOR_COMBO)),
                hInst, nullptr);

            st->hDir = ::CreateWindowExW(0, L"STATIC", L"",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                328, 74, 240, 18, hwnd, nullptr, hInst, nullptr);

            st->hDesc = ::CreateWindowExW(0, L"STATIC", L"",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                12, 98, 556, 40, hwnd, nullptr, hInst, nullptr);

            ::CreateWindowExW(0, L"STATIC", L"输入 (源记号表达式):",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                12, 144, 300, 18, hwnd, nullptr, hInst, nullptr);

            st->hInput = ::CreateWindowExW(0, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP |
                ES_LEFT | ES_AUTOHSCROLL,
                12, 164, 556, 24, hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_TRANSLATOR_INPUT)),
                hInst, nullptr);

            st->hBtn = ::CreateWindowExW(0, L"BUTTON", L"互译",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                12, 198, 90, 28, hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_TRANSLATOR_BTN)),
                hInst, nullptr);

            st->hStatus = ::CreateWindowExW(0, L"STATIC", L"",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                112, 204, 456, 22, hwnd, nullptr, hInst, nullptr);

            ::CreateWindowExW(0, L"STATIC", L"输出 (目标记号表达式):",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                12, 240, 300, 18, hwnd, nullptr, hInst, nullptr);

            st->hOutput = ::CreateWindowExW(0, MSFTEDIT_CLASS, L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_HSCROLL |
                ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                12, 260, 556, 248, hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_TRANSLATOR_OUTPUT)),
                hInst, nullptr);

            // 填充互译器下拉框并默认选中第一个
            const auto& table = translatorTable();
            for (const auto& t : table)
                ::SendMessageW(st->hCombo, CB_ADDSTRING, 0,
                    reinterpret_cast<LPARAM>(t.display_name));
            ::SendMessageW(st->hCombo, CB_SETCURSEL, 0, 0);
            updateTranslatorUI(hwnd, st);

            std::wstring ready = L"就绪：共 ";
            ready += std::to_wstring(table.size());
            ready += L" 个互译器";
            ::SetWindowTextW(st->hStatus, ready.c_str());
            break;
        }
        case WM_COMMAND: {
            auto* st = getTranslator(hwnd);
            if (!st) break;
            const int id = LOWORD(wParam);
            if (id == IDC_TRANSLATOR_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
                const int sel = static_cast<int>(
                    ::SendMessageW(st->hCombo, CB_GETCURSEL, 0, 0));
                if (sel >= 0) {
                    st->index = sel;
                    updateTranslatorUI(hwnd, st);
                }
                return 0;
            }
            if (id == IDC_TRANSLATOR_BTN) {
                runTranslator(hwnd, st);
                return 0;
            }
            break;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN: {
            auto* st = getTranslator(hwnd);
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
            {
                TranslatorState* st = getTranslator(hwnd);
                if (st) destroyIconStrip(st->icons);
            }
            delete getTranslator(hwnd);
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            g_hTranslatorWindow = nullptr;
            break;
        default:
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    void registerTranslatorClass(HINSTANCE hInst) {
        static bool registered = false;
        if (registered) return;

        WNDCLASSW wc{};
        wc.lpfnWndProc = TranslatorProc;
        wc.hInstance = hInst;
        wc.lpszClassName = kTranslatorWindowClass;
        wc.hbrBackground = ::GetSysColorBrush(COLOR_WINDOW);
        wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        ::RegisterClassW(&wc);
        registered = true;
    }

    // 打开“互译器”页面（单例：已打开则带回前台）。
    void showTranslatorWindow(HINSTANCE hInst, HWND hParent) {
        if (::IsWindow(g_hTranslatorWindow)) {
            ::ShowWindow(g_hTranslatorWindow, SW_RESTORE);
            ::SetForegroundWindow(g_hTranslatorWindow);
            return;
        }

        registerTranslatorClass(hInst);

        HWND hwnd = ::CreateWindowExW(
            0, kTranslatorWindowClass, L"互译器",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
            CW_USEDEFAULT, CW_USEDEFAULT, 600, 580,   // 600：为顶部 namePic 名字图让位
            hParent, nullptr, hInst, nullptr);
        if (!hwnd) return;

        g_hTranslatorWindow = hwnd;
        ::ShowWindow(hwnd, SW_SHOW);
        ::UpdateWindow(hwnd);
    }

    // ================= 友情链接页面 =================
    constexpr wchar_t kLinksWindowClass[] = L"OmegaYLinks";
    constexpr int IDC_LINKS_LIST = 7401;
    HWND g_hLinksWindow = nullptr;

    struct LinksState {
        HWND hList{};
    };

    [[nodiscard]] LinksState* getLinks(HWND h) {
        return reinterpret_cast<LinksState*>(::GetWindowLongPtrW(h, GWLP_USERDATA));
    }

    // 用系统默认浏览器打开第 index 条友情链接的 URL。
    void openFriendLink(int index) {
        if (index < 0 || index >= expander_links::kFriendLinkCount) return;
        const std::wstring url = utf8_to_wstring(expander_links::kFriendLinks[index].Link);
        ::ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    LRESULT CALLBACK LinksProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            auto* st = new LinksState{};
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(st));

            HINSTANCE hInst = cs->hInstance;

            // 列表：两列（名称 / URL），点击第一列（名称）跳转
            st->hList = ::CreateWindowExW(
                WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
                LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
                12, 12, 476, 360, hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_LINKS_LIST)),
                hInst, nullptr);
            ::SendMessageW(st->hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

            LVCOLUMNW col{};
            col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            col.pszText = const_cast<wchar_t*>(L"名称");
            col.cx = 200;
            ::SendMessageW(st->hList, LVM_INSERTCOLUMNW, 0,
                reinterpret_cast<LPARAM>(&col));
            col.pszText = const_cast<wchar_t*>(L"URL");
            col.cx = 276;
            ::SendMessageW(st->hList, LVM_INSERTCOLUMNW, 1,
                reinterpret_cast<LPARAM>(&col));

            for (int i = 0; i < expander_links::kFriendLinkCount; ++i) {
                const std::wstring name = utf8_to_wstring(expander_links::kFriendLinks[i].showName);
                const std::wstring link = utf8_to_wstring(expander_links::kFriendLinks[i].Link);
                LVITEMW it{};
                it.mask = LVIF_TEXT | LVIF_PARAM;
                it.iItem = i;
                it.pszText = const_cast<wchar_t*>(name.c_str());
                it.lParam = i;
                const int row = static_cast<int>(
                    ::SendMessageW(st->hList, LVM_INSERTITEMW, 0,
                        reinterpret_cast<LPARAM>(&it)));
                LVITEMW si{};
                si.mask = LVIF_TEXT;
                si.iItem = row;
                si.iSubItem = 1;
                si.pszText = const_cast<wchar_t*>(link.c_str());
                ::SendMessageW(st->hList, LVM_SETITEMW, 0,
                    reinterpret_cast<LPARAM>(&si));
            }
            break;
        }
        case WM_NOTIFY: {
            auto* nmh = reinterpret_cast<NMHDR*>(lParam);
            if (nmh->idFrom == IDC_LINKS_LIST && nmh->code == NM_CLICK) {
                // 单击第一列（名称）跳转到对应 URL
                auto* na = reinterpret_cast<NMITEMACTIVATE*>(lParam);
                if (na->iItem >= 0 && na->iSubItem == 0) {
                    openFriendLink(na->iItem);
                }
                return 0;
            }
            break;
        }
        case WM_CLOSE:
            ::DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            delete getLinks(hwnd);
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            g_hLinksWindow = nullptr;
            break;
        default:
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    void registerLinksClass(HINSTANCE hInst) {
        static bool registered = false;
        if (registered) return;

        WNDCLASSW wc{};
        wc.lpfnWndProc = LinksProc;
        wc.hInstance = hInst;
        wc.lpszClassName = kLinksWindowClass;
        wc.hbrBackground = ::GetSysColorBrush(COLOR_WINDOW);
        wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        ::RegisterClassW(&wc);
        registered = true;
    }

    // 打开“友情链接”页面（单例：已打开则带回前台）。
    void showLinksWindow(HINSTANCE hInst, HWND hParent) {
        if (::IsWindow(g_hLinksWindow)) {
            ::ShowWindow(g_hLinksWindow, SW_RESTORE);
            ::SetForegroundWindow(g_hLinksWindow);
            return;
        }

        registerLinksClass(hInst);

        HWND hwnd = ::CreateWindowExW(
            0, kLinksWindowClass, L"友情链接",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
            CW_USEDEFAULT, CW_USEDEFAULT, 520, 440,
            hParent, nullptr, hInst, nullptr);
        if (!hwnd) return;

        g_hLinksWindow = hwnd;
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
            // 顶部图标条：Expander.ico + MFC.ico（48×48）。mh 由 25 改为 60，
            // 使下方控件整体下移 35 像素，为图标条让位。
            ui->icons = createIconStrip(hwnd, hInst, 10, 10, 48);
            constexpr int mh = 60;

            ::CreateWindowExW(0, L"STATIC", L"序列 (用逗号分隔):",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, mh + 10, 150, 20,
                hwnd, nullptr, hInst, nullptr);

            ui->hEditSeq = ::CreateWindowExW(0, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                10, mh + 30, 420, 20,
                hwnd, nullptr, hInst, nullptr);

            ::CreateWindowExW(0, L"STATIC", L"项数:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, mh + 55, 50, 20,
                hwnd, nullptr, hInst, nullptr);

            ui->hEditTerm = ::CreateWindowExW(0, L"EDIT", L"1",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                60, mh + 55, 80, 20,
                hwnd, nullptr, hInst, nullptr);

            // 参数化记号（如 Minus1 系列）使用的额外整数参数输入框，默认隐藏；
            // 选中带 expand_text_n 的记号时由 updateNotationLabel 显示。
            ui->hStaticParam = ::CreateWindowExW(0, L"STATIC", L"",
                WS_CHILD | SS_LEFT,
                150, mh + 55, 100, 20,
                hwnd, nullptr, hInst, nullptr);
            ui->hEditParam = ::CreateWindowExW(0, L"EDIT", L"",
                WS_CHILD | WS_BORDER | ES_LEFT,
                250, mh + 55, 60, 20,
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
                170, mh + 87, 400, 20,
                hwnd, nullptr, hInst, nullptr);
            updateNotationLabel(ui);

            ui->hBtnFSalter = ::CreateWindowExW(0, L"BUTTON", L"展开",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, mh + 115, 420, 25,
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
            case IDM_ABOUT: {
                HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
                    ::GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
                expander_help_about::showSourceWindow(hInst, hwnd);
                break;
            }

            case IDM_TRANSLATOR: {
                HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
                    ::GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
                showTranslatorWindow(hInst, hwnd);
                break;
            }

            case IDM_FRIENDLINK: {
                HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
                    ::GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
                showLinksWindow(hInst, hwnd);
                break;
            }

            case IDM_LEGAL: {
                HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
                    ::GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
                showLegalWindow(hInst, hwnd);
                break;
            }

            case IDM_THIRDPARTY_LICENSE: {
                HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
                    ::GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
                showLicenseWindow(hInst, hwnd);
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

                const omegay::notation::ExpandMode mode =
                    (id == IDM_FSALTER) ? omegay::notation::ExpandMode::FsAlter
                                        : omegay::notation::ExpandMode::Fs;

                if (entry.notation->has_text()) {
                    // 文本型记号：输入读取与校验保持 UI 原行为；
                    // 展开统一走 OOP 接口（含 FS_alter 分支与 FS 通用截断）。
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

                    omegay::notation::ExpandResult r =
                        entry.notation->expand(mode, utf8Seq, term, 0);
                    if (!r.ok) {
                        ::MessageBoxW(hwnd, utf8_to_wstring(r.err).c_str(), L"错误",
                            MB_OK | MB_ICONERROR);
                        break;
                    }
                    text = std::move(r.text);
                }
                else if (entry.notation->has_text_n()) {
                    // 参数化记号：读取序列、项数与额外整数参数 n。
                    wchar_t bufSeq[512]{};
                    wchar_t bufTerm[64]{};
                    wchar_t bufParam[64]{};
                    ::GetWindowTextW(ui->hEditSeq, bufSeq, 512);
                    ::GetWindowTextW(ui->hEditTerm, bufTerm, 64);
                    ::GetWindowTextW(ui->hEditParam, bufParam, 64);

                    std::string utf8Seq = wstring_to_utf8(bufSeq);
                    int term = 0;
                    int param = 0;
                    try {
                        term = std::stoi(wstring_to_utf8(bufTerm));
                        param = std::stoi(wstring_to_utf8(bufParam));
                    }
                    catch (...) {
                        ::MessageBoxW(hwnd,
                            L"项数与参数都必须是整数，\n所以说你为什么要这样输入", L"错误",
                            MB_OK | MB_ICONERROR);
                        break;
                    }
                    if (term < 1) {
                        ::MessageBoxW(hwnd, L"项数必须为正整数，\n你难道要尝试向内存反方向写入序列吗",
                            L"错误", MB_OK | MB_ICONERROR);
                        break;
                    }
                    if (param < entry.param_min || param > entry.param_max) {
                        std::wstring paramMsg = L"参数必须是 ";
                        paramMsg += std::to_wstring(entry.param_min);
                        paramMsg += L" 到 ";
                        paramMsg += std::to_wstring(entry.param_max);
                        paramMsg += L" 之间的整数";
                        ::MessageBoxW(hwnd, paramMsg.c_str(), L"错误",
                            MB_OK | MB_ICONERROR);
                        break;
                    }
                    if (utf8Seq.empty()) {
                        ::MessageBoxW(hwnd, L"序列不能为空", L"错误",
                            MB_OK | MB_ICONERROR);
                        break;
                    }

                    // 参数化族的展开已包含完整 LNZ 语义，统一入口内部完成
                    // FS_alter 分支选择与 suffix 拼接，不做通用截断。
                    omegay::notation::ExpandResult r =
                        entry.notation->expand(mode, utf8Seq, term, param);
                    if (!r.ok) {
                        ::MessageBoxW(hwnd, utf8_to_wstring(r.err).c_str(), L"错误",
                            MB_OK | MB_ICONERROR);
                        break;
                    }
                    text = std::move(r.text);
                }
                else {
                    // 序列型记号
                    std::vector<int> seq;
                    int term = 0;
                    if (!readSeqAndTerm(hwnd, ui, seq, term)) break;

                    // 原始文本交给统一入口（内部解析、展开、FS pop_back、suffix）
                    wchar_t bufSeq[256]{};
                    ::GetWindowTextW(ui->hEditSeq, bufSeq, 256);
                    omegay::notation::ExpandResult r = entry.notation->expand(
                        mode, wstring_to_utf8(bufSeq), term, 0);
                    if (!r.ok) {
                        ::MessageBoxW(hwnd, utf8_to_wstring(r.err).c_str(), L"错误",
                            MB_OK | MB_ICONERROR);
                        break;
                    }
                    text = std::move(r.text);
                }

                // 展开文本已含 suffix（统一入口内部拼接）
                std::wstring wtext = utf8_to_wstring(text);
                HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
                    ::GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
                std::wstring resultTitle = L"展开结果 - ";
                resultTitle += entry.display_name;
                showResultWindow(hInst, hwnd, std::move(wtext),
                    std::move(resultTitle));
                break;
            }

            default:
                break;
            }
            break;
        }
        case WM_DESTROY: {
            UiState* ui = getUi(hwnd);
            if (ui) destroyIconStrip(ui->icons);
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

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                    _In_ PWSTR lpCmdLine, _In_ int nCmdShow) {
    ::LoadLibraryW(L"Msftedit.dll");

    // GDI+：用于加载 Assets/namePic.jpg 等 JPG 资源；失败时相关控件静默跳过。
    Gdiplus::GdiplusStartupInput gdiplusInput;
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok)
        gdiplusToken = 0;

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
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 280,   // 600：为顶部 namePic 名字图让位
        nullptr, nullptr, hInstance, nullptr);
    if (!hwnd) return 0;

    ::ShowWindow(hwnd, nCmdShow);
    ::UpdateWindow(hwnd);

    MSG msg;
    while (::GetMessageW(&msg, nullptr, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
    if (gdiplusToken != 0)
        Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}

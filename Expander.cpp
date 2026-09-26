#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <sal.h>
#include <richedit.h>

#include "Header/common/utf8.hpp"
#include "Header/common/sequence.hpp"
#include "Header/notation/empty.hpp"
#include "Header/notation/prss.hpp"
#include "Header/notation/pps_family.hpp"
#include "Header/notation/omega_y.hpp"
#include <memory>
#include <string>
#include <vector>

using namespace omegay;
using namespace omegay::common;
using namespace omegay::notation;

namespace {

    constexpr int IDM_ABOUT = 1001;
    constexpr int IDM_EXIT = 1002;
    constexpr int IDM_HELP = 1003;
    constexpr int IDM_LEGAL = 1004;
    constexpr int IDM_FRDLNK = 1005; // 友情链接
    constexpr int IDM_NOTE_PRSS = 1101; // 记号定义 - PrSS
    constexpr int IDM_NOTE_PPS1 = 1102; // 记号定义 - PPS1
    constexpr int IDM_FS = 101;
    constexpr int IDM_FSALTER = 102;

    constexpr wchar_t kNotationWndClass[] = L"NotationDefWnd";

    struct MenuDeleter { void operator()(HMENU  m) const noexcept { if (m) ::DestroyMenu(m); } };
    struct BrushDeleter { void operator()(HBRUSH b) const noexcept { if (b) ::DeleteObject(b); } };
    using MenuPtr = std::unique_ptr<std::remove_pointer_t<HMENU>, MenuDeleter>;
    using BrushPtr = std::unique_ptr<std::remove_pointer_t<HBRUSH>, BrushDeleter>;

    struct UiState {
        HWND hEditSeq{};
        HWND hEditTerm{};
        HWND hComboNotation{};
        HWND hBtnFS{};
        HWND hBtnFSalter{};
        BrushPtr background;
    };

    // 下拉框每一项对应的记号接口
    struct NotationEntry {
        const wchar_t* display_name;
        std::vector<int>(*expand)(const std::vector<int>&, int);
        std::string(*suffix)();
    };

    // 只保留实际实现了规则的记号
    constexpr NotationEntry kNotations[] = {
        { L"空记号",       &EmptyNotation::expand,  &EmptyNotation::suffix  },
        { L"PrSS",         &PrSSNotation::expand,   &PrSSNotation::suffix   },
        { L"PPS",          &PPSNotation::expand,    &PPSNotation::suffix    },
        { L"PPS4",         &PPS4Notation::expand,   &PPS4Notation::suffix   },
        { L"ω-Y sequence", &OmegaYNotation::expand, &OmegaYNotation::suffix },
    };

    [[nodiscard]] UiState* getUi(HWND h) {
        return reinterpret_cast<UiState*>(::GetWindowLongPtrW(h, GWLP_USERDATA));
    }

    [[nodiscard]] HMENU createMenuBar() {
        HMENU hMenu = ::CreateMenu();

        HMENU hFile = ::CreatePopupMenu();
        ::AppendMenuW(hFile, MF_STRING, IDM_EXIT, L"退出(&X)");
        ::AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hFile), L"文件(&F)");

        HMENU hNotationDef = ::CreatePopupMenu();
        ::AppendMenuW(hNotationDef, MF_STRING, IDM_NOTE_PRSS, L"初等序列 (PrSS)...");
        ::AppendMenuW(hNotationDef, MF_STRING, IDM_NOTE_PPS1, L"PPS1...");
        ::AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hNotationDef), L"记号定义(&D)");

        HMENU hHelp = ::CreatePopupMenu();
        ::AppendMenuW(hHelp, MF_STRING, IDM_HELP, L"帮助(&H)...");
        ::AppendMenuW(hHelp, MF_STRING, IDM_ABOUT, L"关于(&A)...");
        ::AppendMenuW(hHelp, MF_STRING, IDM_LEGAL, L"法律声明(&L)...");
        ::AppendMenuW(hHelp, MF_SEPARATOR, 0, nullptr);
        ::AppendMenuW(hHelp, MF_STRING, IDM_FRDLNK, L"友情链接(&L)...");
        ::AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hHelp), L"帮助(&H)");

        return hMenu;
    }

    struct NotationDefWindow {
        const wchar_t* title;
        const wchar_t* text;
    };

    constexpr wchar_t kTextPrSS[] =
        L"定义 6.1  初等序列 (a_0, a_1, ..., a_{m-1}, a_m) 定义如下：\r\n"
        L"\r\n"
        L"  (1)  ( ) = 0\r\n"
        L"\r\n"
        L"  (2)  (#, 0) = (#) + 1，式中 # 为任意合法序列。\r\n"
        L"\r\n"
        L"  (3)  (#_1, a_i, #_2, a_k) = (#_1, a_i, #_2, a_i, #_2, ...)，\r\n"
        L"       式中 #_1, #_2 为任意两段合法序列，a_k > 0，\r\n"
        L"       a_i = a_k - 1 为 a_k 前首个小于 a_k 的数，\r\n"
        L"       省略号代表任意有限次循环的极限。\r\n";

    constexpr wchar_t kTextPPS1[] =
        L"PPS1\r\n"
        L"Parented Predecessor Sequence 1\r\n"
        L"\r\n"
        L"PPS 是形如 0,1,0,3 这样用逗号分隔的序列（序列首项是第 1 项）\r\n"
        L"极限表达式：0,1,2,3,4,5,......\r\n"
        L"\r\n"
        L"记末项的值为 x，坏根为第 x 项，坏根的值为 b，末项是序列中的第 y 项，并令 L=y-x\r\n"
        L"\r\n"
        L"展开：\r\n"
        L"  1. 如果末项是 0，则它是后继序数\r\n"
        L"  2. 末项之前的部分保持不变\r\n"
        L"  3. 替换末项：如果末项和坏根之间（两边都不含）存在一项，它的值等于 b，那么将末项的值换成 b；否则\r\n"
        L"     将末项的值减 1\r\n"
        L"  4. 递归生成其他项（第 i+L 项的值由第 i 项确定）：对任意的 i>x，如果第 i 项的值大于等于 x，那么第 i+L\r\n"
        L"     项的值等于第 i 项的值 +L，否则第 i+L 项的值等于第 i 项的值\r\n"
        L"  5. 基本列 [n] 为展开到第 y+n*L-1 项。\r\n";

    LRESULT CALLBACK NotationDefWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            auto* def = reinterpret_cast<const NotationDefWindow*>(cs->lpCreateParams);
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(def));

            HINSTANCE hInst = cs->hInstance;

            HWND hEdit = ::CreateWindowExW(
                0, MSFTEDIT_CLASS, nullptr,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE |
                ES_READONLY | ES_AUTOVSCROLL | ES_NOHIDESEL,
                0, 0, 0, 0,
                hwnd, nullptr, hInst, nullptr);

            if (!hEdit) {
                ::MessageBoxW(hwnd, L"RichEdit 控件创建失败", L"错误",
                    MB_OK | MB_ICONERROR);
                return -1;
            }

            if (def && def->text) {
                ::SetWindowTextW(hEdit, def->text);
            }

            ::SendMessageW(hEdit, EM_SETBKGNDCOLOR, 0,
                static_cast<LPARAM>(::GetSysColor(COLOR_WINDOW)));

            CHARFORMATW cf{};
            cf.cbSize = sizeof(cf);
            cf.dwMask = CFM_FACE | CFM_SIZE;
            cf.yHeight = 200;
            ::wcscpy_s(cf.szFaceName, L"Microsoft YaHei UI");
            ::SendMessageW(hEdit, EM_SETCHARFORMAT,
                SCF_ALL, reinterpret_cast<LPARAM>(&cf));

            ::SendMessageW(hEdit, EM_SETREADONLY, TRUE, 0);
            ::SendMessageW(hEdit, EM_SETMARGINS,
                EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(8, 8));

            return 0;
        }
        case WM_SIZE: {
            HWND hEdit = ::GetWindow(hwnd, GW_CHILD);
            if (hEdit) {
                RECT r; ::GetClientRect(hwnd, &r);
                ::MoveWindow(hEdit, 0, 0, r.right, r.bottom, TRUE);
            }
            return 0;
        }
        case WM_SETFOCUS: {
            HWND hEdit = ::GetWindow(hwnd, GW_CHILD);
            if (hEdit) ::SetFocus(hEdit);
            return 0;
        }
        case WM_CLOSE:
            ::DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            return 0;
        default:
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }
    }

    void showNotationDef(HWND owner, const NotationDefWindow* def) {
        HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
            ::GetWindowLongPtrW(owner, GWLP_HINSTANCE));

        HWND hwnd = ::CreateWindowExW(
            WS_EX_TOOLWINDOW,
            kNotationWndClass, def->title,
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
            WS_MINIMIZEBOX | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 480, 360,
            owner, nullptr, hInst, const_cast<NotationDefWindow*>(def));
        if (hwnd) {
            ::ShowWindow(hwnd, SW_SHOW);
            ::UpdateWindow(hwnd);
            RECT r; ::GetClientRect(hwnd, &r);
            ::SendMessageW(hwnd, WM_SIZE, SIZE_RESTORED,
                MAKELPARAM(r.right, r.bottom));
        }
    }

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE: {
            auto* ui = new UiState{};
            ui->background.reset(::CreateSolidBrush(::GetSysColor(COLOR_WINDOW)));
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ui));

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

            ui->hComboNotation = ::CreateWindowExW(0, L"COMBOBOX", nullptr,
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                100, mh + 85, 220, 200,
                hwnd, nullptr, hInst, nullptr);

            for (const auto& entry : kNotations) {
                ::SendMessageW(ui->hComboNotation, CB_ADDSTRING, 0,
                    reinterpret_cast<LPARAM>(entry.display_name));
            }
            ::SendMessageW(ui->hComboNotation, CB_SETCURSEL, 1, 0); // 默认 PrSS

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
        case WM_CTLCOLORSTATIC: case WM_CTLCOLOREDIT: case WM_CTLCOLORBTN: {
            auto* ui = getUi(hwnd);
            if (!ui) return ::DefWindowProcW(hwnd, msg, wParam, lParam);
            HDC hdc = reinterpret_cast<HDC>(wParam);
            ::SetBkMode(hdc, TRANSPARENT);
            ::SetTextColor(hdc, ::GetSysColor(COLOR_WINDOWTEXT));
            return reinterpret_cast<LRESULT>(ui->background.get());
        }
        case WM_COMMAND: {
            auto* ui = getUi(hwnd); if (!ui) break;
            switch (LOWORD(wParam)) {
            case IDM_HELP:
                ::MessageBoxW(hwnd,
                    L"鸣谢:Hyp Cos,naruyoko,test_alpha-0\n"
                    L"请注意 代码系利用人工智能技术生成",
                    L"帮助", MB_OK | MB_ICONINFORMATION);
                break;
            case IDM_ABOUT:
                ::MessageBoxW(hwnd,
                    L"ω-Y 展开器 v1.3\n\n"
                    L"当前支持：空记号、PrSS、PPS、PPS4、ω-Y sequence。\n",
                    L"关于", MB_OK | MB_ICONINFORMATION);
                break;
            case IDM_LEGAL:
                ::MessageBoxW(hwnd,
                    L"Unlicense 授权\n\n"
                    L"This is free and unencumbered software released into the public domain.\n"
                    L"\n"
                    L"Anyone is free to copy, modify, publish, use, compile, sell, or\n"
                    L"distribute this software, either in source code form or as a compiled\n"
                    L"binary, for any purpose, commercial or non-commercial, and by any\n"
                    L"means.\n"
                    L"\n"
                    L"In jurisdictions that recognize copyright laws, the author or authors\n"
                    L"of this software dedicate any and all copyright interest in the\n"
                    L"software to the public domain. We make this dedication for the benefit\n"
                    L"of the public at large and to the detriment of our heirs and\n"
                    L"successors. We intend this dedication to be an overt act of\n"
                    L"relinquishment in perpetuity of all present and future rights to this\n"
                    L"software under copyright law.\n"
                    L"\n"
                    L"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n"
                    L"EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\n"
                    L"MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.\n"
                    L"IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR\n"
                    L"OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,\n"
                    L"ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
                    L"OTHER DEALINGS IN THE SOFTWARE.\n"
                    L"\n"
                    L"For more information, please refer to <https://unlicense.org/>\n",
                    L"法律声明",
                    MB_OK | MB_ICONINFORMATION);
                break;
            case IDM_FRDLNK:
                ::MessageBoxW(hwnd,
                    L"友情链接\n\n"
                    L"SmileLee-lyx NER :\n https://github.com/SmileLee-lyx/ne-rewritten\n"
                    L"Hypcos NE：\n https://github.com/hypcos/notation-explorer\n"
                    L"《大数理论》:\n https://github.com/zhiqiucao/googology\n"
                    L"Googology Wiki:\n https://wiki.googology.top\n",
                    L"友情链接",
                    MB_OK | MB_ICONINFORMATION);
                break;
            case IDM_NOTE_PRSS: {
                static const NotationDefWindow def{
                    L"记号定义 - 初等序列 (PrSS)",
                    kTextPrSS
                };
                showNotationDef(hwnd, &def);
                break;
            }
            case IDM_NOTE_PPS1: {
                static const NotationDefWindow def{
                    L"记号定义 - PPS1",
                    kTextPPS1
                };
                showNotationDef(hwnd, &def);
                break;
            }
            case IDM_EXIT:
                ::PostQuitMessage(0);
                break;

            case IDM_FS:
            case IDM_FSALTER: {
                wchar_t bufSeq[256]{}, bufTerm[64]{};
                ::GetWindowTextW(ui->hEditSeq, bufSeq, 256);
                ::GetWindowTextW(ui->hEditTerm, bufTerm, 64);

                auto seq = parse_sequence(wstring_to_utf8(bufSeq));
                int term = 0;
                try { term = std::stoi(wstring_to_utf8(bufTerm)); }
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
                if (seq.empty()) {
                    ::MessageBoxW(hwnd, L"序列不能为空", L"错误",
                        MB_OK | MB_ICONERROR);
                    break;
                }

                int sel = static_cast<int>(::SendMessageW(ui->hComboNotation, CB_GETCURSEL, 0, 0));
                if (sel < 0 || sel >= static_cast<int>(std::size(kNotations))) {
                    sel = 0;
                }

                std::vector<int> result = kNotations[sel].expand(seq, term);
                std::string suffix_text = kNotations[sel].suffix();

                if (LOWORD(wParam) == IDM_FS && result.size() > 1)
                    result.pop_back();

                std::string text = seq_to_string(result) + suffix_text;
                std::wstring wtext = utf8_to_wstring(text);
                ::MessageBoxW(hwnd, wtext.c_str(), L"展开结果",
                    MB_OK | MB_ICONINFORMATION);
                break;
            }
            default: break;
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

int WINAPI wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     PWSTR     lpCmdLine,
    _In_     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (!::LoadLibraryW(L"Msftedit.dll")) {
        ::MessageBoxW(nullptr, L"无法加载 Msftedit.dll", L"错误",
            MB_OK | MB_ICONERROR);
        return 0;
    }

    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OmegaY";
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!::RegisterClassW(&wc)) return 0;

    WNDCLASS wcDef{};
    wcDef.lpfnWndProc = NotationDefWndProc;
    wcDef.hInstance = hInstance;
    wcDef.lpszClassName = kNotationWndClass;
    wcDef.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcDef.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    ::RegisterClassW(&wcDef);

    HWND hwnd = ::CreateWindowExW(
        0, L"OmegaY", L"展开器 (重构中)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 380, 240,
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
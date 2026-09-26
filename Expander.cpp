#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "Header/common/utf8.hpp"
#include "Header/common/sequence.hpp"
#include "Header/notation/empty.hpp"
#include "Header/notation/prss.hpp"
#include "Header/notation/pps_family.hpp" // PPS 家族：PPS / PPS4 / Weak PPS4 / Third PPS4 / ...
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
    constexpr int IDM_FS = 101;
    constexpr int IDM_FSALTER = 102;

    struct MenuDeleter { void operator()(HMENU m)  const noexcept { if (m) ::DestroyMenu(m); } };
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
        std::vector<int> (*expand)(const std::vector<int>&, int);
        std::string (*suffix)();
    };

    constexpr NotationEntry kNotations[] = {
        { L"空记号",                 &EmptyNotation::expand,      &EmptyNotation::suffix },
        { L"PrSS",                   &PrSSNotation::expand,       &PrSSNotation::suffix },
        { L"PPS",                    &PPSNotation::expand,        &PPSNotation::suffix },
        { L"PPS4",                   &PPS4Notation::expand,       &PPS4Notation::suffix },
        { L"Weak PPS4",              &WPPS4Notation::expand,      &WPPS4Notation::suffix },
        { L"Third PPS4",             &TPPS4Notation::expand,      &TPPS4Notation::suffix },
        { L"Extremely Weak PPS4",    &EWPPS4Notation::expand,     &EWPPS4Notation::suffix },
        { L"Second PPS4",            &SecondPPS4Notation::expand, &SecondPPS4Notation::suffix },
        { L"2-pps4",                 &PPS2Notation::expand,       &PPS2Notation::suffix },
    };

    [[nodiscard]] UiState* getUi(HWND h) {
        return reinterpret_cast<UiState*>(::GetWindowLongPtrW(h, GWLP_USERDATA));
    }

    [[nodiscard]] HMENU createMenuBar() {
        HMENU hMenu = ::CreateMenu();
        HMENU hFile = ::CreatePopupMenu();
        ::AppendMenuW(hFile, MF_STRING, IDM_EXIT, L"退出(&X)");
        ::AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hFile), L"文件(&F)");
        HMENU hHelp = ::CreatePopupMenu();
        ::AppendMenuW(hHelp, MF_STRING, IDM_HELP, L"帮助(&H)...");
        ::AppendMenuW(hHelp, MF_STRING, IDM_ABOUT, L"关于(&A)...");
        ::AppendMenuW(hHelp, MF_STRING, IDM_LEGAL, L"法律声明(&L)...");
        ::AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hHelp), L"帮助(&H)");
        return hMenu;
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
                    L"当前支持：空记号、PrSS、PPS、PPS4、Weak PPS4、\n"
                    L"Third PPS4、Extremely Weak PPS4、Second PPS4、2-pps4。\n",
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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OmegaY";
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!::RegisterClassW(&wc)) return 0;

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
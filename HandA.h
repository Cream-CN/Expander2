
#pragma once

#include <windows.h>
#include <richedit.h>   // MSFTEDIT_CLASS、CHARFORMAT2W、EM_* / EC_*
#include <string>

namespace expander_help_about {

    inline constexpr wchar_t kSourceWindowClass[] = L"OmegaYSource";
    inline constexpr int    IDC_SOURCE_EDIT     = 6002;
    inline HWND             g_hSourceWindow     = nullptr;
    inline constexpr wchar_t kSourceText[] =
        L"Expander\r\n"
        L"\r\n"
        L"本软件是可自由分发的自由软件，使用 GNU 通用公共许可证 第三版本 授权\r\n"
        L"By Cream-CN\r\n"
        L"\r\n"
        L"\r\n"
        L"部分记号的代码修改自 hypcos/notation-explorer\r\n"
        L"部分记号的代码修改自 smilelee-lyx/ne-weritten（DEN，TON，MN）\r\n"
        L"记号的定义和部分记号的 AI 提示词选自曹知秋《大数理论》（zhiqiucao/googology）\r\n"
        L"BMS系列的定义来自kyodaisuu/basmat"
        L"MrSS 的定义来自 AAA滚木批发 (QQ3682911373)\r\n"
        L"ε-Y,X-Y 的代码修改自 Go men 的代码 gomen520/gomen520.github.io\r\n"
        L"\r\n"
        L"请注意：代码系利用人工智能技术生成";

    // 将 RichEdit 字号设为 10pt（供本窗体与定义弹窗共用）。
    inline void applyRichEdit10pt(HWND hRich, const wchar_t* face = L"Microsoft YaHei UI") {
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

    inline LRESULT CALLBACK SourceWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_SOURCE_EDIT)),
                cs->hInstance, nullptr);

            if (hRich && text) {
                ::SetWindowTextW(hRich, text->c_str());
                applyRichEdit10pt(hRich);
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
            g_hSourceWindow = nullptr;
            break;
        default:
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    inline void registerSourceClass(HINSTANCE hInst) {
        static bool registered = false;
        if (registered) return;

        WNDCLASSW wc{};
        wc.lpfnWndProc = SourceWndProc;
        wc.hInstance = hInst;
        wc.lpszClassName = kSourceWindowClass;
        wc.hbrBackground = ::GetSysColorBrush(COLOR_WINDOW);
        wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        ::RegisterClassW(&wc);
        registered = true;
    }

    inline void showSourceWindow(HINSTANCE hInst, HWND hParent) {
        if (::IsWindow(g_hSourceWindow)) {
            ::ShowWindow(g_hSourceWindow, SW_RESTORE);
            ::SetForegroundWindow(g_hSourceWindow);
            return;
        }

        registerSourceClass(hInst);
        auto* text = new std::wstring(kSourceText);

        HWND hwnd = ::CreateWindowExW(
            0, kSourceWindowClass, L"帮助 / 关于",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 640, 520,
            hParent, nullptr, hInst, text);

        if (!hwnd) {
            delete text;
            return;
        }
        g_hSourceWindow = hwnd;
        ::ShowWindow(hwnd, SW_SHOW);
        ::UpdateWindow(hwnd);
    }

} // namespace expander_help_about

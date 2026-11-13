
#pragma once

#include <windows.h>
#include <richedit.h>   // MSFTEDIT_CLASS、CHARFORMAT2W、EM_* / EC_*
#include <objidl.h>    // IStream / CreateStreamOnHGlobal（从 RCDATA 内存流解码图片）
#include <string>
#include <gdiplus.h>
#include "Resource.h"   // IDI_EXPANDER、IDI_MFC、IDB_AUTHOR_AVATAR、IDB_ORG_AVATAR

#pragma comment(lib, "gdiplus.lib")

namespace expander_help_about {

    inline constexpr wchar_t kSourceWindowClass[] = L"OmegaYSource";
    inline constexpr int    IDC_SOURCE_EDIT     = 6002;
    inline HWND             g_hSourceWindow     = nullptr;
    inline HFONT            g_hSourceFont       = nullptr; // “关于”窗体文本框字体
    inline constexpr wchar_t kSourceText[] =
        L"Expander\r\n"
        L"\r\n"
        L"本软件是可自由分发的自由软件，使用 GNU 通用公共许可证 3 授权\r\n"
        L"By Cream-CN\r\n"
        L"\r\n"
        L"\r\n"
        L"鸣谢"
        L"部分记号的代码修改自 hypcos/notation-explorer\r\n"
        L"部分记号的代码修改自 smilelee-lyx/ne-weritten（DEN，TON，MN）\r\n"
        L"记号的定义和部分记号的 AI 提示词定义选自曹知秋《大数理论》（zhiqiucao/googology）\r\n"
        L"BMS系列的定义来自kyodaisuu/basmat"
        L"MrSS 的定义来自 AAA滚木批发 (QQ3682911373)\r\n"
        L"ε-Y,X-Y 的代码修改自 Go men 的代码 gomen520/gomen520.github.io\r\n"
        L"\r\n"
        L"请注意：代码系利用人工智能技术生成\r\n\r\n"
        L"Zen Of C++20:\r\n"
        L"Clear is better than clever.\r\n清晰胜于取巧。\r\n\r\nExplicit is better than implicit.\r\n显式胜于隐式。\r\n\r\nValue is better than pointer.\r\n值语义胜于裸指针。\r\n\r\nSafe is better than fast, unless proven slow.\r\n安全优先于速度，除非实测瓶颈。\r\n\r\nFlat is better than tangled.\r\n扁平胜于纠缠。\r\n\r\nComposable is better than monolithic.\r\n可组合胜于大一统。\r\n\r\nReadability counts.\r\n可读性至关重要。\r\n\r\nTypes aren’t rigid enough to block reason.\r\n类型约束不必僵化到抛弃常理。\r\n\r\nAlthough correctness beats brevity.\r\n但正确性优先于简洁。\r\n\r\nUndefined behavior must not be ignored.\r\n未定义行为绝不能视而不见。\r\n\r\nUnless you know exactly what you trade.\r\n除非你清楚自己取舍的代价。\r\n\r\nIn the face of ambiguity, prefer compile-time checks.\r\n遇到歧义，优先交给编译期校验。\r\n\r\nThere should be a straightforward way to express intent.\r\n应当有一种直抒意图的写法。\r\n\r\nThough it may demand learning new idioms.\r\n只是它往往需要你习得新范式。\r\n\r\nNow is better than later.\r\n尽早落地胜于无限拖延。\r\n\r\nBut half-baked abstractions are worse than none.\r\n不成熟的抽象，不如不要抽象。\r\n\r\nIf the template is hard to constrain, it’s a bad design.\r\n模板约束难以说清，便是糟糕设计。\r\n\r\nIf the concept reads like plain prose, it may be a good design.\r\n概念约束清晰易懂，多半是好设计。\r\n\r\nModules are one honking great idea — let’s use more of those!\r\n模块是超棒的特性——多多善用！\r\n";

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

    // “关于”窗体的子窗口句柄
    struct AboutChildWindows {
        HWND  hEdit;
    };

    inline LRESULT CALLBACK SourceWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            auto* text = reinterpret_cast<std::wstring*>(cs->lpCreateParams);

            auto* ch = new AboutChildWindows{};

            // “关于”窗体：只读多行编辑框，从窗体顶部铺满，向下滚动展示文字
            HWND hEdit = ::CreateWindowExW(
                0, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
                ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                2, 2, 10, 10,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_SOURCE_EDIT)),
                cs->hInstance, nullptr);

            if (hEdit) {
                if (!g_hSourceFont) {
                    g_hSourceFont = ::CreateFontW(-13, 0, 0, 0, FW_NORMAL,
                        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                        L"Microsoft YaHei UI");
                }
                ::SendMessageW(hEdit, WM_SETFONT,
                    reinterpret_cast<WPARAM>(g_hSourceFont), TRUE);
                if (text) {
                    ::SetWindowTextW(hEdit, text->c_str());
                    ::SendMessageW(hEdit, EM_SETSEL, 0, 0);
                }
            }
            ch->hEdit = hEdit;
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(ch));
            delete text;
            break;
        }
        case WM_SIZE: {
            auto* ch = reinterpret_cast<AboutChildWindows*>(
                ::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (!ch) break;
            const int w = LOWORD(lParam), h = HIWORD(lParam);
            if (ch->hEdit) {
                ::MoveWindow(ch->hEdit, 2, 2, w - 4, h - 4, TRUE);
            }
            break;
        }
        case WM_CLOSE:
            ::DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            if (g_hSourceFont) {
                ::DeleteObject(g_hSourceFont);
                g_hSourceFont = nullptr;
            }
            {
                auto* ch = reinterpret_cast<AboutChildWindows*>(
                    ::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
                if (ch) {
                    delete ch;
                    ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                }
            }
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

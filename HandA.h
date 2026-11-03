
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
    inline constexpr int    IDC_ABOUT_ICON1     = 6003;   // “关于”窗体：Expander.ico
    inline constexpr int    IDC_ABOUT_ICON2     = 6004;   // “关于”窗体：MFC.ico
    inline constexpr int    kAboutIconSize      = 48;     // 图标显示尺寸（两枚统一 48×48）
    inline constexpr int    kAboutIconMargin    = 12;     // 图标距左边距 / 两枚图标间距
    inline constexpr int    kAboutTextTop       = kAboutIconMargin + kAboutIconSize + 10; // 文本区顶边
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

    // “关于”窗体的子窗口与图标句柄
    struct AboutChildWindows {
        HWND  hEdit;
        HWND  hIcon1;
        HWND  hIcon2;
        HICON hIcoExpander;
        HICON hIcoMfc;
    };

    inline LRESULT CALLBACK SourceWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            auto* text = reinterpret_cast<std::wstring*>(cs->lpCreateParams);

            auto* ch = new AboutChildWindows{};

            // 两枚图标统一按 48×48 显示：Expander.ico（软件图标）与 MFC.ico
            ch->hIcoExpander = static_cast<HICON>(::LoadImageW(
                cs->hInstance, MAKEINTRESOURCEW(IDI_EXPANDER),
                IMAGE_ICON, kAboutIconSize, kAboutIconSize, LR_DEFAULTCOLOR));
            ch->hIcoMfc = static_cast<HICON>(::LoadImageW(
                cs->hInstance, MAKEINTRESOURCEW(IDI_MFC),
                IMAGE_ICON, kAboutIconSize, kAboutIconSize, LR_DEFAULTCOLOR));

            ch->hIcon1 = ::CreateWindowExW(0, L"STATIC", L"",
                WS_CHILD | WS_VISIBLE | SS_ICON,
                kAboutIconMargin, kAboutIconMargin, kAboutIconSize, kAboutIconSize,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_ABOUT_ICON1)),
                cs->hInstance, nullptr);
            if (ch->hIcon1 && ch->hIcoExpander) {
                ::SendMessageW(ch->hIcon1, STM_SETICON,
                    reinterpret_cast<WPARAM>(ch->hIcoExpander), 0);
            }

            ch->hIcon2 = ::CreateWindowExW(0, L"STATIC", L"",
                WS_CHILD | WS_VISIBLE | SS_ICON,
                kAboutIconMargin + kAboutIconSize + kAboutIconMargin,
                kAboutIconMargin, kAboutIconSize, kAboutIconSize,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_ABOUT_ICON2)),
                cs->hInstance, nullptr);
            if (ch->hIcon2 && ch->hIcoMfc) {
                ::SendMessageW(ch->hIcon2, STM_SETICON,
                    reinterpret_cast<WPARAM>(ch->hIcoMfc), 0);
            }

            // “关于”窗体：普通多行编辑框（尺寸与旧 RichEdit 相同），向下滚动展示文字
            HWND hEdit = ::CreateWindowExW(
                0, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
                ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                2, kAboutTextTop, 10, 10,
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
            if (ch->hIcon1) {
                ::MoveWindow(ch->hIcon1, kAboutIconMargin, kAboutIconMargin,
                    kAboutIconSize, kAboutIconSize, TRUE);
            }
            if (ch->hIcon2) {
                ::MoveWindow(ch->hIcon2,
                    kAboutIconMargin + kAboutIconSize + kAboutIconMargin,
                    kAboutIconMargin, kAboutIconSize, kAboutIconSize, TRUE);
            }
            if (ch->hEdit) {
                ::MoveWindow(ch->hEdit, 2, kAboutTextTop, w - 4, h - kAboutTextTop - 2, TRUE);
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
                    if (ch->hIcoExpander) ::DestroyIcon(ch->hIcoExpander);
                    if (ch->hIcoMfc)     ::DestroyIcon(ch->hIcoMfc);
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

    // ==================== “关于作者”窗体 ====================
    // 顶部并排显示两张 256×256 头像（全部内嵌为 RCDATA 资源，不再读外部文件）：
    //   左：作者头像（IDB_AUTHOR_AVATAR，源 Assets/pic1.jpg）
    //   右：组织头像（IDB_ORG_AVATAR，源 Assets/pic2.png）
    // 头像下方分别标注“作者 / 组织”，再下方为只读多行简介文本。
    // 图片由 GDI+ 从 IStream 解码，再等比缩放到 256×256；资源缺失时 STATIC 留空。
    inline constexpr wchar_t kAuthorWindowClass[]     = L"OmegaYAuthor";
    inline constexpr int    IDC_AUTHOR_AVATAR_AUTHOR  = 6101;   // 作者头像 STATIC
    inline constexpr int    IDC_AUTHOR_AVATAR_ORG     = 6102;   // 组织头像 STATIC
    inline constexpr int    IDC_AUTHOR_NAME_LABEL     = 6103;   // 作者名标签
    inline constexpr int    IDC_AUTHOR_ORG_LABEL      = 6104;   // 组织名标签
    inline constexpr int    IDC_AUTHOR_EDIT           = 6105;   // 简介只读 EDIT
    inline constexpr int    kAvatarSize               = 256;      // 头像显示尺寸（正方形 256×256）
    inline constexpr int    kAvatarMargin            = 16;      // 头像距窗口上边 / 左边
    inline constexpr int    kAvatarGap                = 32;      // 两头像之间的水平间距
    inline constexpr int    kAvatarLabelHeight        = 24;      // 头像下方标签高度
    inline HWND             g_hAuthorWindow          = nullptr;

    // 简介文本：基于原“关于”窗里的 By Cream-CN 字段整理，可按需修改。
    inline constexpr wchar_t kAuthorText[] =
        L"作者：Cream-CN\r\n"
        L"组织：（见右侧组织头像）\r\n"
        L"\r\n"
        L"Expander 是一个大数数学（Googology）记号展开器，\r\n"
        L"用 C++20 与 Win32 原生 API 编写，支持约 90 种主流大数记号的展开与互译。\r\n"
        L"\r\n"
        L"如果对本项目有任何问题、建议或记号修正，欢迎通过软件内的友情链接联系。\r\n";

    // 从 RCDATA 资源读出字节，包成 IStream，交给 GDI+ 解码，再等比缩放到正方形 dstSize。
    // 透明像素合成到白底；失败返回 nullptr。
    [[nodiscard]] inline HBITMAP loadResourceImageSquare(HINSTANCE hInst, int resourceId, int dstSize) {
        HRSRC hrsrc = ::FindResourceW(hInst, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
        if (!hrsrc) return nullptr;
        HGLOBAL hRes = ::LoadResource(hInst, hrsrc);
        if (!hRes) return nullptr;
        DWORD size = ::SizeofResource(hInst, hrsrc);
        const void* pData = ::LockResource(hRes);
        if (!pData || size == 0) return nullptr;

        // 拷贝一份到 HGLOBAL（GDI+ 会读流，资源锁在我们这边不能长期占用）
        HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, size);
        if (!hMem) return nullptr;
        void* pWrite = ::GlobalLock(hMem);
        if (!pWrite) { ::GlobalFree(hMem); return nullptr; }
        ::memcpy(pWrite, pData, size);
        ::GlobalUnlock(hMem);

        IStream* pStream = nullptr;
        HRESULT hr = ::CreateStreamOnHGlobal(hMem, TRUE, &pStream);  // TRUE：流销毁时自动 free hMem
        if (FAILED(hr) || !pStream) { ::GlobalFree(hMem); return nullptr; }

        HBITMAP hbmp = nullptr;
        {
            Gdiplus::Bitmap src(pStream);
            pStream->Release();   // GDI+ 构造时已读完流，立刻释放
            if (src.GetLastStatus() == Gdiplus::Ok) {
                const int sw = src.GetWidth();
                const int sh = src.GetHeight();
                if (sw > 0 && sh > 0) {
                    Gdiplus::Bitmap dst(dstSize, dstSize, PixelFormat32bppARGB);
                    Gdiplus::Graphics g(&dst);
                    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
                    g.Clear(Gdiplus::Color(255, 255, 255, 255));
                    int drawW = dstSize, drawH = dstSize, offX = 0, offY = 0;
                    if (sw > sh) {
                        drawH = dstSize * sh / sw;
                        offY = (dstSize - drawH) / 2;
                    } else {
                        drawW = dstSize * sw / sh;
                        offX = (dstSize - drawW) / 2;
                    }
                    g.DrawImage(&src, Gdiplus::Rect(offX, offY, drawW, drawH),
                                0, 0, sw, sh, Gdiplus::UnitPixel);
                    dst.GetHBITMAP(Gdiplus::Color(255, 255, 255), &hbmp);
                }
            }
        }
        return hbmp;
    }

    struct AuthorChildWindows {
        HWND   hAvatarAuthor{};
        HWND   hAvatarOrg{};
        HWND   hNameLabel{};
        HWND   hOrgLabel{};
        HWND   hEdit{};
        HBITMAP hBmpAuthor{};
        HBITMAP hBmpOrg{};
        HFONT  hFont{};
    };

    inline LRESULT CALLBACK AuthorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            auto* ch = new AuthorChildWindows{};

#ifndef EXPANDER_CLEAN_UI
            ch->hBmpAuthor = loadResourceImageSquare(cs->hInstance, IDB_AUTHOR_AVATAR, kAvatarSize);
            ch->hBmpOrg    = loadResourceImageSquare(cs->hInstance, IDB_ORG_AVATAR,    kAvatarSize);

            ch->hAvatarAuthor = ::CreateWindowExW(0, L"STATIC", L"",
                WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE,
                kAvatarMargin, kAvatarMargin, kAvatarSize, kAvatarSize,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_AUTHOR_AVATAR_AUTHOR)),
                cs->hInstance, nullptr);
            if (ch->hAvatarAuthor && ch->hBmpAuthor) {
                ::SendMessageW(ch->hAvatarAuthor, STM_SETIMAGE,
                    IMAGE_BITMAP, reinterpret_cast<LPARAM>(ch->hBmpAuthor));
            }

            ch->hAvatarOrg = ::CreateWindowExW(0, L"STATIC", L"",
                WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE,
                kAvatarMargin + kAvatarSize + kAvatarGap, kAvatarMargin,
                kAvatarSize, kAvatarSize,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_AUTHOR_AVATAR_ORG)),
                cs->hInstance, nullptr);
            if (ch->hAvatarOrg && ch->hBmpOrg) {
                ::SendMessageW(ch->hAvatarOrg, STM_SETIMAGE,
                    IMAGE_BITMAP, reinterpret_cast<LPARAM>(ch->hBmpOrg));
            }

            ch->hNameLabel = ::CreateWindowExW(0, L"STATIC", L"作者：Cream-CN",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                kAvatarMargin, kAvatarMargin + kAvatarSize + 4,
                kAvatarSize, kAvatarLabelHeight,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_AUTHOR_NAME_LABEL)),
                cs->hInstance, nullptr);

            ch->hOrgLabel = ::CreateWindowExW(0, L"STATIC", L"组织",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                kAvatarMargin + kAvatarSize + kAvatarGap,
                kAvatarMargin + kAvatarSize + 4,
                kAvatarSize, kAvatarLabelHeight,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_AUTHOR_ORG_LABEL)),
                cs->hInstance, nullptr);

            const int editTop = kAvatarMargin + kAvatarSize + kAvatarLabelHeight + 8;
#else
            // 清爽版：无头像，只留作者名标签 + 简介文本，布局紧凑
            ch->hNameLabel = ::CreateWindowExW(0, L"STATIC", L"作者：Cream-CN",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                12, 12, 300, 24,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_AUTHOR_NAME_LABEL)),
                cs->hInstance, nullptr);
            const int editTop = 44;
#endif

            ch->hEdit = ::CreateWindowExW(
                0, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
                ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                2, editTop, 10, 10,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_AUTHOR_EDIT)),
                cs->hInstance, nullptr);

            ch->hFont = ::CreateFontW(-13, 0, 0, 0, FW_NORMAL,
                FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                L"Microsoft YaHei UI");
            if (ch->hFont) {
                ::SendMessageW(ch->hEdit, WM_SETFONT,
                    reinterpret_cast<WPARAM>(ch->hFont), TRUE);
                ::SendMessageW(ch->hNameLabel, WM_SETFONT,
                    reinterpret_cast<WPARAM>(ch->hFont), TRUE);
                ::SendMessageW(ch->hOrgLabel, WM_SETFONT,
                    reinterpret_cast<WPARAM>(ch->hFont), TRUE);
            }
            ::SetWindowTextW(ch->hEdit, kAuthorText);
            ::SendMessageW(ch->hEdit, EM_SETSEL, 0, 0);

            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(ch));
            break;
        }
        case WM_SIZE: {
            auto* ch = reinterpret_cast<AuthorChildWindows*>(
                ::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (!ch) break;
            const int w = LOWORD(lParam), h = HIWORD(lParam);
#ifndef EXPANDER_CLEAN_UI
            const int editTop = kAvatarMargin + kAvatarSize + kAvatarLabelHeight + 8;
#else
            const int editTop = 44;
#endif
            if (ch->hEdit) {
                ::MoveWindow(ch->hEdit, 2, editTop, w - 4, h - editTop - 2, TRUE);
            }
            break;
        }
        case WM_CLOSE:
            ::DestroyWindow(hwnd);
            break;
        case WM_DESTROY: {
            auto* ch = reinterpret_cast<AuthorChildWindows*>(
                ::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (ch) {
                if (ch->hBmpAuthor) ::DeleteObject(ch->hBmpAuthor);
                if (ch->hBmpOrg)    ::DeleteObject(ch->hBmpOrg);
                if (ch->hFont)      ::DeleteObject(ch->hFont);
                delete ch;
                ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            }
            g_hAuthorWindow = nullptr;
            break;
        }
        default:
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    inline void registerAuthorClass(HINSTANCE hInst) {
        static bool registered = false;
        if (registered) return;
        WNDCLASSW wc{};
        wc.lpfnWndProc = AuthorWndProc;
        wc.hInstance = hInst;
        wc.lpszClassName = kAuthorWindowClass;
        wc.hbrBackground = ::GetSysColorBrush(COLOR_WINDOW);
        wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        ::RegisterClassW(&wc);
        registered = true;
    }

    inline void showAuthorWindow(HINSTANCE hInst, HWND hParent) {
        if (::IsWindow(g_hAuthorWindow)) {
            ::ShowWindow(g_hAuthorWindow, SW_RESTORE);
            ::SetForegroundWindow(g_hAuthorWindow);
            return;
        }
        registerAuthorClass(hInst);
        // 默认尺寸：能容纳两张 256 头像（宽 = 16+256+32+256+16 = 576）与下方文本区；
        // 清爽版无头像，整体收紧。
#ifndef EXPANDER_CLEAN_UI
        constexpr int kDefaultW = 620;
        constexpr int kDefaultH = 640;
#else
        constexpr int kDefaultW = 480;
        constexpr int kDefaultH = 320;
#endif
        HWND hwnd = ::CreateWindowExW(
            0, kAuthorWindowClass, L"关于作者",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, kDefaultW, kDefaultH,
            hParent, nullptr, hInst, nullptr);
        if (!hwnd) return;
        g_hAuthorWindow = hwnd;
        ::ShowWindow(hwnd, SW_SHOW);
        ::UpdateWindow(hwnd);
    }

} // namespace expander_help_about

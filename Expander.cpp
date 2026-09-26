#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "Header/common/utf8.hpp"
#include "Header/common/sequence.hpp"
#include "Header/core/arena.hpp"
#include "Header/core/entry.hpp"
#include "Header/notation/empty.hpp"
#include "Header/notation/PPS-family.hpp"
#include "Header/notation/Omega-YMagma.hpp"

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

using namespace omegay;
using namespace omegay::common;
using namespace omegay::notation;

namespace {

    // ------------------------------------------------------------------
    // 控件 ID
    // ------------------------------------------------------------------
    constexpr int IDM_ABOUT = 1001;
    constexpr int IDM_EXIT = 1002;
    constexpr int IDM_HELP = 1003;
    constexpr int IDM_LEGAL = 1004;
    constexpr int IDM_FS = 101;
    constexpr int IDM_FSALTER = 102;
    constexpr int IDM_ENTRY_DEMO = 103;
    constexpr int IDC_NOTATION_COMBO = 2001;

    // ------------------------------------------------------------------
    // 记号注册表
    // ------------------------------------------------------------------
    struct NotationEntry {
        const wchar_t* display_name;
        const char* id;
        std::vector<int>(*expand)(const std::vector<int>&, int);
        std::string(*suffix)();
    };

    [[nodiscard]] const std::vector<NotationEntry>& notationTable() {
        static const std::vector<NotationEntry> table = {
            { L"空记号",        "empty",  &notation::EmptyNotation::expand,       &notation::EmptyNotation::suffix },
            { L"PPS",           "pps",    &notation::PPSNotation::expand,         &notation::PPSNotation::suffix },
            { L"PPS4",          "pps4",   &notation::PPS4Notation::expand,        &notation::PPS4Notation::suffix },
            { L"Weak PPS4",     "wpps4",  &notation::WPPS4Notation::expand,       &notation::WPPS4Notation::suffix },
            { L"Third PPS4",    "tpps4",  &notation::TPPS4Notation::expand,       &notation::TPPS4Notation::suffix },
            { L"Ex. Weak PPS4", "ewpps4", &notation::EWPPS4Notation::expand,      &notation::EWPPS4Notation::suffix },
            { L"Second PPS4",   "spps4",  &notation::SecondPPS4Notation::expand,  &notation::SecondPPS4Notation::suffix },
            { L"2-pps4",        "2-pps4", &notation::PPS2Notation::expand,        &notation::PPS2Notation::suffix },
            { L"ω-Y (medium)",  "omega-y-medium", &notation::OmegaYMediumNotation::expand, &notation::OmegaYMediumNotation::suffix },
            { L"ω-Y (strong)",  "omega-y-strong", &notation::OmegaYStrongNotation::expand, &notation::OmegaYStrongNotation::suffix },
        };
        return table;
    }

    // ------------------------------------------------------------------
    // GDI 资源 RAII
    // ------------------------------------------------------------------
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
        BrushPtr background;
    };

    [[nodiscard]] UiState* getUi(HWND h) {
        return reinterpret_cast<UiState*>(::GetWindowLongPtrW(h, GWLP_USERDATA));
    }

    // ------------------------------------------------------------------
    // 菜单
    // ------------------------------------------------------------------
    [[nodiscard]] HMENU createMenuBar() {
        HMENU hMenu = ::CreateMenu();

        HMENU hFile = ::CreatePopupMenu();
        ::AppendMenuW(hFile, MF_STRING, IDM_EXIT, L"退出(&X)");
        ::AppendMenuW(hMenu, MF_POPUP,
            reinterpret_cast<UINT_PTR>(hFile), L"文件(&F)");

        HMENU hTool = ::CreatePopupMenu();
        ::AppendMenuW(hTool, MF_STRING, IDM_ENTRY_DEMO, L"Entry 演示(&E)...");
        ::AppendMenuW(hMenu, MF_POPUP,
            reinterpret_cast<UINT_PTR>(hTool), L"工具(&T)");

        HMENU hHelp = ::CreatePopupMenu();
        ::AppendMenuW(hHelp, MF_STRING, IDM_HELP, L"帮助(&H)...");
        ::AppendMenuW(hHelp, MF_STRING, IDM_ABOUT, L"关于(&A)...");
        ::AppendMenuW(hHelp, MF_STRING, IDM_LEGAL, L"法律声明(&L)...");
        ::AppendMenuW(hMenu, MF_POPUP,
            reinterpret_cast<UINT_PTR>(hHelp), L"帮助(&H)");

        return hMenu;
    }

    // ------------------------------------------------------------------
    // 通用：读取序列与项数，失败时弹窗并返回 false
    // 符合 CONTRIBUTING.txt：UI 层负责 term >= 1、seq 非空校验
    // ------------------------------------------------------------------
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

    // ------------------------------------------------------------------
    // Entry 演示：构造一棵小 Entry 图，刷新 ykey，显示结果
    // 对应 api.txt 中 arena.hpp / entry.hpp / make_ykey 的调用示例
    // ------------------------------------------------------------------
    void showEntryDemo(HWND hwnd, UiState* ui) {
        std::vector<int> seq;
        int term = 0;
        if (!readSeqAndTerm(hwnd, ui, seq, term)) return;

        omegay::core::EntryArena arena;

        // 根节点：value = seq[0]，x = term，y = seq
        auto* root = arena.make(
            seq.empty() ? 0 : seq[0],
            term,
            seq);

        // 子节点：value = seq.back()，x = 1，y = 截断后的序列
        std::vector<int> childY = seq;
        if (childY.size() > 1) childY.pop_back();
        auto* child = arena.make(
            seq.back(),
            1,
            childY);

        // 建立 leg 关系，演示 api.txt 中的指针链接用法
        root->rightleg_down = child;
        child->leftleg_up.push_back(root);

        // 修改 child 的 y，并刷新 ykey
        child->y = childY;
        child->refresh_key();

        // 单独计算 ykey，验证与 Entry::ykey 一致
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

        // arena 析构时自动释放 root / child
    }

    // ------------------------------------------------------------------
    // 窗口过程
    // ------------------------------------------------------------------
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

            switch (LOWORD(wParam)) {
            case IDM_HELP:
                ::MessageBoxW(hwnd,
                    L"鸣谢:Hyp Cos,naruyoko,test_alpha-0\n"
                    L"请注意 代码系利用人工智能技术生成",
                    L"帮助", MB_OK | MB_ICONINFORMATION);
                break;

            case IDM_ABOUT:
                ::MessageBoxW(hwnd,
                    L"ω-Y 展开器 v1.6\n\n"
                    L"已加入 PPS 家族记号、ω-Y magma 展开与 Entry 演示。\n",
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
                    L"法律声明", MB_OK | MB_ICONINFORMATION);
                break;

            case IDM_EXIT:
                ::PostQuitMessage(0);
                break;

            case IDM_ENTRY_DEMO:
                showEntryDemo(hwnd, ui);
                break;

            case IDM_FS:
            case IDM_FSALTER: {
                std::vector<int> seq;
                int term = 0;
                if (!readSeqAndTerm(hwnd, ui, seq, term)) break;

                // 读取当前选中的记号
                int sel = static_cast<int>(
                    ::SendMessageW(ui->hComboNotation, CB_GETCURSEL, 0, 0));
                const auto& table = notationTable();
                if (sel < 0 || sel >= static_cast<int>(table.size())) sel = 0;
                const NotationEntry& entry = table[sel];

                // 调用对应记号的 expand（纯函数，不抛异常）
                std::vector<int> result = entry.expand(seq, term);

                // FS / FSalter 后处理属于 UI 层，记号层不应感知
                if (LOWORD(wParam) == IDM_FS && result.size() > 1)
                    result.pop_back();

                std::string text = seq_to_string(result) + entry.suffix();
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
    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OmegaY";
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!::RegisterClassW(&wc)) return 0;

    HWND hwnd = ::CreateWindowExW(
        0, L"OmegaY", L"ω-Y 展开器 (重构中)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 280,
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
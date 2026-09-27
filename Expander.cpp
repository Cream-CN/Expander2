#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <richedit.h>

#include "Header/common/utf8.hpp"
#include "Header/common/sequence.hpp"
#include "Header/core/arena.hpp"
#include "Header/core/entry.hpp"
#include "Header/notation/empty.hpp"
#include "Header/notation/PPS-family.hpp"
#include "Header/notation/Omega-YMagma.hpp"
#include "Header/notation/mrss121.hpp"
#include "Header/notation/omega_y.hpp"

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

    // 定义子项 ID 基址
    constexpr int IDM_DEFINITION_BASE = 3000;
    constexpr int IDC_DEFINITION_EDIT = 4001;

    // 独立定义弹窗相关
    constexpr wchar_t kDefWindowClass[] = L"OmegaYDefPopup";
    constexpr int IDC_POPUP_DEF_EDIT = 5001;
    constexpr int IDC_POPUP_DEF_CLOSE = 5002;

    // ------------------------------------------------------------------
    // 记号注册表
    // ------------------------------------------------------------------
    struct NotationEntry {
        const wchar_t* display_name;
        const char* id;
        std::vector<int>(*expand)(const std::vector<int>&, int);
        std::string(*suffix)();
        const wchar_t* definition;   // 记号定义（纯文本，可为 nullptr）

        // 可选的文本展开入口。若不为空，UI 会优先用它处理输入文本，
        // 以支持 MrSS 这类递归嵌套表达式。签名约定：
        //   std::string expand_string(std::string_view, int)
        std::string(*expand_text)(std::string_view, int) = nullptr;
    };

    [[nodiscard]] const std::vector<NotationEntry>& notationTable() {
        static const std::vector<NotationEntry> table = {
            { L"空记号", "empty", &notation::EmptyNotation::expand, &notation::EmptyNotation::suffix,
              L"空记号（empty）\n\n返回输入序列" },

            { L"PPS", "pps", &notation::PPSNotation::expand, &notation::PPSNotation::suffix,
              L"PPS🎄\n\n定义：PPS1\nParented Predecessor Sequence 1\n\n极限表达式：0,1,2,3,4,5,......\n记末项的值为 x，坏根为第 x 项，坏根的值为 b，末项是序列中的第 y 项，并令 L = y - x\n展开：\n1.如果末项是 0，则它是后继序数\n2.末项之前的部分保持不变\n3.替换末项：如果末项和坏根之间(两边都不含) 存在一项，它的值等于 b，那么将末项的值换成 b；否则\n将末项的值减 1\n4.递归生成其他项(第 i + L 项的值由第 i 项确定)：对任意的 i > x，如果第 i 项的值大于等于 x，那么第 i + L\n项的值等于第 i 项的值 + L，否则第 i + L 项的值等于第 i 项的值\n5.基本列[n] 为展开到第 y + n * L - 1 项。" },

            { L"PPS4", "pps4", &notation::PPS4Notation::expand, &notation::PPS4Notation::suffix,
              L"PPS4\n\nPPS 的第四版本\n定义：PPS 4\nParented Predecessor Sequence 4\n极限表达式：0,1,2,3,....\n坏根：列标是 (末项的值) 的项（首项的列标是 1）；如果末项是 0，则表示后继序数 \n记此时末项的列标减末项的值为 L，坏根的值为 b，末项的值为 x、列标为 y\n末项展开：\n> 如果末项和坏根之间 (两边都不含) 存在一项，它的值等于 b，那么是弱展开，否则是强展开；弱展开：将末项的值换成 b；\n> 强展开：在第 b 列和第 x 列 (都不含) 之间找到最右侧的值小于等于 b 的项，将末项的值换为这个项的列标；如果找不到，则等同弱展开\n其他项展开：对任意的 i>y-L，如果第 i 项的值大于等于 x，那么第 i+L 项的值等于第 i 项的值 +L，否则第 i+L 项的值等于第 i 项的值\n基本列 [n] 为展开到第 y+nL-1 项" },

            { L"Weak PPS4", "wpps4", &notation::WPPS4Notation::expand, &notation::WPPS4Notation::suffix,
              L"Weak PPS4\n\nPPS4 的弱化版本。" },

            { L"Third PPS4", "tpps4", &notation::TPPS4Notation::expand, &notation::TPPS4Notation::suffix,
              L"Third PPS4\n\nPPS4 的第三型变体。" },

            { L"Ex. Weak PPS4", "ewpps4", &notation::EWPPS4Notation::expand, &notation::EWPPS4Notation::suffix,
              L"Ex. Weak PPS4\n\n扩展弱 PPS4。" },

            { L"Second PPS4", "spps4", &notation::SecondPPS4Notation::expand, &notation::SecondPPS4Notation::suffix,
              L"Second PPS4\n\nPPS4 的第二型变体。" },

            { L"2-pps4", "2-pps4", &notation::PPS2Notation::expand, &notation::PPS2Notation::suffix,
              L"2-pps4\n\n2-PPS4 。" },

            { L"ω-Y (medium)", "omega-y-medium", &notation::OmegaYMediumNotation::expand, &notation::OmegaYMediumNotation::suffix,
              L"ω-Y (medium)\n\nω-Y Medium Magma Style" },

            { L"ω-Y (strong)", "omega-y-strong", &notation::OmegaYStrongNotation::expand, &notation::OmegaYStrongNotation::suffix,
              L"ω-Y (strong)\n\nω-Y Strong Magam Style" },

            { L"MrSS1.2.1", "mrss121", &notation::Mrss121Notation::expand, &notation::Mrss121Notation::suffix,
              L"MrSS1.2.1（山脉结构序列）\n\n本版本只利用 1 层山脉结构。\n合法表达式形如 S = a1, a2, a3, ...，其中 a1 = 1，\nan 为 MrSS 表达式或有限非零序数。\n\n支持嵌套写法，例如：\n  1,(1,2),(1,2,3)", &notation::Mrss121Notation::expand_string },
            { L"ω-Y sequence", "omega-y-sequence", &notation::OmegaYNotation::expand, &notation::OmegaYNotation::suffix,
                L"ω-Y sequence\n\nω-Y sequence\n\n一个 ω − Y 序列是形如 ω − Y(a1, a2, . . . , an) 的序列。\nω − Y 序列山脉图的绘制方法如下：\n(1) 为每一行赋予一个行标，原序列的行标为 0。\n(2) 第 0 行中元素的父项为从该元素起，在该元素左边且小于该元素的第一个项。\n(3) 元素所对应的阶差项为该元素与其父项的差值，所有元素的阶差项构成阶差序列。特别地，如果某一项\n不存在父项，则其阶差项为空。\n(4) 各阶阶差序列中某元素的父项定义为阶差序列中第一个在它左边、小于它，并且其正下方的项是该元素正下方元素祖先项的项。\n(5) 逐阶计算阶差序列，直到某一阶阶差序列的所有项均为空为止。\n(6) 将各阶阶差序列从下到上写在原序列对应元素的正上方，并将各阶阶差序列中的每一项与其所对应的正 \n下方的项以及正下方项的父项相连。特别地，如果某一项不存在父项，则不将该项与其他项相连。连接阶差项与 \n其正下方元素的线称为右腿，而连接阶差项与其左下方的父项的连线称为左腿。每计算一次阶差序列，则其行 \n标增加 1。这样可以得到山脉图的前 n 行。\n(7) 对于山脉图某列顶端的元素来说，元素的父项关系为：从一个顶端元素出发，如果沿着它的左腿向下一步，再沿着右腿向正上方走到不超过 A 的行标（如果无路可走则不走），不断地重复这一过程，直到达到了另一个顶端元素。接下来从新得到的顶端元素出发重复上述操作，又得到另一个顶端元素。这样得到的所有顶端元 \n素，称为该元素的待定父项。\n(8) 每作一条分隔线之前，都要检查山脉图全部列的元素是否全为 1。如果不是的话，就从最低阶的分隔线开始作起，然后找到该条分隔线与其下方最近的同阶分隔线的山脉图（如果不包含其他的同阶分隔线，则考虑 全部的山脉图），对这些山脉图中包含的所有列的顶端元素计算阶差序列。如果这样的阶差序列不能够计算的话，就提高分隔线的阶次，重复计算阶差序列，直到能够计算阶差序列为止。每穿过一条 n 阶分隔线，则行标右加 ω^n。\n(9) 对新的山脉图不断重复上述操作，直到山脉图中所有的顶端项全为 1，则山脉图绘制结束。ω − Y 序列 的山脉图的行标总小于 ω^ω。\n在 ω − Y 序列的山脉图中定义如下概念：\n(1) 末列最上方的 1 左腿所指的元素称为根元素。\n(2) 根元素所在列称为根列。\n(3) 根列包含的所有元素称为根列元素。\n(4) 根列元素的作用区域为从这一根列元素出发（包含这一列），到在它正上方的根列元素（包含这一列，如果没有的话就到山脉图的顶端）之间的部分。如果某个根列元素的行标为 α，在它正上方的根列元素的行标为 β，那么它的作用范围为所有行标 γ 满足 α ≤ γ < β 的行。\n(5) 第 α 行和第 β 行 (α ≤ β) 的行差为满足 α + δ = β 的序数 δ 。\n(6) 轮廓边定义为：从一个根列元素出发，沿左腿向上一步（但不能超出这个根列元素的作用区域）之后， 沿右腿向下若干步（可以不向下，但同样不能超出这个根列元素的作用区域），随后重复这个过程直到无路可走。 能通过这样的操作经过的边，都是这个根列元素对应的（或者这个作用区域内的）轮廓边。\n(7) 非轮廓边定义为：经过了某个作用区域，但不符合这个作用区域内轮廓边的概念的边，称为这个作用区 域内的非轮廓边。\n\n(8) 填充边定义为：从一个根列元素出发，沿左腿向上走一行后，沿右腿向下走一行，随后重复这个过程直到无路可走。能通过这样的操作经过的边，称为这个根列元素对应的填充边。\nω − Y 序列山脉图的展开方法如下：\n(1) 将末列的各项减一。如果最上方的元素被减为零，则删去它相关的左腿和右腿。\n\n(2) 从最上方的根列元素开始，找到其作用区域。\n\n(3) 在这一根列元素作用区域内找到所有的轮廓边，并将轮廓边的端点及其所指的元素向右上方进行复制。 \n具体地，向右平移（末列位置 − 根列位置）的列数，然后向上平移至满足以下条件的位置：若平移前端点所指的元素与根列元素行差为 δ0 ，那么平移后对应端点所指的元素与末列最上方的元素行差也要为 δ0 。像这样，至所有的轮廊边和它们所指的元素都被复制到了新的位置上。\n\n(4) 如果末列最上方元素与根列元素的行差为 0，则在这一根列元素作用区域内找到所有的填充边，然后将它不断复制，并填补到轮廓边提升所产生的所有缝隙之中。特别地，当填充边被复制到跨过了 n 阶分隔线的位置上时，其左腿和右腿的行差需要为 ω^n−1 。\n(5) 在这一根列元素作用区域内找到所有的非轮廓边，并对这些边进行复制。非轮廓边左腿指向的元素都保持不动，而右腿向右或右上平移，向右平移的列数为（末列位置 − 根列位置）。如果这条非轮廓边右腿所指元素没有被这个作用区域内的轮廊边所指到，那么无需向上平移；如果有，那么向上平移到与末列最上方元素行差为 δ0 的位置，其中 δ0 为平移之前该元素与根列元素的行差。\n(6) 自上而下不断地对各个根列元素所对应的山脉图重复 (3) − (5) 的操作，直到山脉图的所有部分都完成复制。\n\n(7) 按照从上到下、从左到右的顺序将山脉图中的各个元素计算出来。如果该元素不是山脉图某一部分的顶端元素，则其取值等于该元素正上方的元素与其父项之和。\nω − Y 序列的取值定义如下： \n(1) ω − Y(∅) = 0。\n(2) 如果原序列的末项为 1，则它对应的序数为删去末尾的 1 之后余下的部分所对应的序数加 1。\n(3) 否则按照前述山脉图的展开方式对序列进行展开，展开后山脉图最下方的序列就是展开后的 ω − Y 序列。" },
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
        HWND hRichDef{};
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

        // —— 定义菜单：按记号表动态生成子项 ——
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

    // ------------------------------------------------------------------
    // 通用：读取序列与项数
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
    // 定义展示（主窗口内的 RichEdit，纯文本，与下拉框联动）
    // ------------------------------------------------------------------
    void showDefinition(UiState* ui, int index) {
        const auto& table = notationTable();
        if (!ui->hRichDef) return;
        if (index < 0 || index >= static_cast<int>(table.size())) return;

        std::wstring text;
        text += L"【";
        text += table[index].display_name;
        text += L"】\r\n\r\n";
        text += table[index].definition ? table[index].definition : L"（暂无定义）";

        ::SetWindowTextW(ui->hRichDef, text.c_str());

        ::SendMessageW(ui->hRichDef, EM_SETSEL, 0, 0);
        ::SendMessageW(ui->hRichDef, EM_SCROLLCARET, 0, 0);
    }

    // ------------------------------------------------------------------
    // 独立定义弹窗
    //   字号 10pt，用 RichEdit 只读展示定义正文
    // ------------------------------------------------------------------
    void applyRichEdit10pt(HWND hRich) {
        // 10pt 转 twips：1pt = 20 twips（RichEdit 使用 twips）
        const LONG sizeTwips = 10 * 20;

        CHARFORMAT2W cf{};
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_SIZE | CFM_FACE;
        cf.yHeight = sizeTwips;
        wcscpy_s(cf.szFaceName, L"Microsoft YaHei UI");

        ::SendMessageW(hRich, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&cf));
    }

    void fillPopupDefinition(HWND hRich, const NotationEntry& entry) {
        std::wstring text;
        text += L"【";
        text += entry.display_name;
        text += L"】\r\n\r\n";
        text += entry.definition ? entry.definition : L"（暂无定义）";

        // 先设置字符格式为 10pt，再写入文本，避免空文档时设置无效
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

    // ------------------------------------------------------------------
    // Entry 演示
    // ------------------------------------------------------------------
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

            // RichEdit：主窗口内的定义展示区（与下拉框联动）
            ui->hRichDef = ::CreateWindowExW(
                0, MSFTEDIT_CLASS, L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
                ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                10, mh + 150, 360, 110,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDC_DEFINITION_EDIT)),
                hInst, nullptr);

            // 主窗口内的 RichEdit 也设为 10pt，保持一致观感
            applyRichEdit10pt(ui->hRichDef);

            // 启动即展示当前选中记号（第 0 项）的定义
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

            // 下拉框选择变化 → 只同步主窗口内的 RichEdit，不影响弹窗
            if (id == IDC_NOTATION_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
                int sel = static_cast<int>(
                    ::SendMessageW(ui->hComboNotation, CB_GETCURSEL, 0, 0));
                showDefinition(ui, sel);
                break;
            }

            // 菜单“定义”子项 → 单独弹窗展示，不再切换下拉框
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
                    L"请注意 代码系利用人工智能技术生成，我（和所有贡献者）不保证展开结果正确",
                    L"帮助", MB_OK | MB_ICONINFORMATION);
                break;

            case IDM_ABOUT:
                ::MessageBoxW(hwnd,
                    L"ω-Y 展开器\n\n"
                    L"By Cream-CN\n",
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
                int sel = static_cast<int>(
                    ::SendMessageW(ui->hComboNotation, CB_GETCURSEL, 0, 0));
                if (sel < 0 || sel >= static_cast<int>(table.size())) sel = 0;
                const NotationEntry& entry = table[sel];

                std::string text;

                if (entry.expand_text) {
                    // 文本展开路径：支持 MrSS 这类嵌套表达式。
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

                    // FS / FSalter 的“移除末项 / 保留末项”后处理。
                    // 文本路径下，先解析成表达式序列再决定是否移除。
                    if (id == IDM_FS) {
                        auto parsed = notation::Mrss121Notation::parse(text);
                        if (parsed && parsed->size() > 1) {
                            parsed->pop_back();
                            text = notation::Mrss121Notation::to_string(*parsed);
                        }
                    }
                }
                else {
                    // 原整数序列路径。
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
    // 运行时加载 RichEdit 5.0（MSFTEDIT_CLASS）
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
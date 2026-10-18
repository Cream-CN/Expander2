// 运行时验证：Definitions.h 查表 -> utf8_to_wstring 解码是否正确。
#include <iostream>
#include <string>
#include "Definitions.h"
#include "Header/common/utf8.hpp"

int main() {
    using omegay::common::utf8_to_wstring;
    bool ok = true;

    const auto pps = utf8_to_wstring(expander_defs::lookup(IDR_DEF_PPS));
    if (pps.empty()) { ok = false; std::cout << "EMPTY PPS\n"; }
    if (pps.find(L"Parented") == std::wstring::npos) { ok = false; std::cout << "NO Parented\n"; }
    if (pps.find(L'\n') == std::wstring::npos) { ok = false; std::cout << "NO real newline\n"; }
    if (pps.find(L"\\n") != std::wstring::npos) { ok = false; std::cout << "LITERAL backslash-n\n"; }
    if (pps.find(L"\U0001F384") == std::wstring::npos) { ok = false; std::cout << "NO emoji\n"; }
    if (!pps.empty() && static_cast<unsigned>(pps[0]) == 0xFEFF) { ok = false; std::cout << "BOM leaked into payload\n"; }

    const auto oy = utf8_to_wstring(expander_defs::lookup(IDR_DEF_OMEGA_Y_SEQUENCE));
    if (oy.find(L"山脉图") == std::wstring::npos) { ok = false; std::cout << "NO 山脉图\n"; }
    if (oy.find(L'ω') == std::wstring::npos) { ok = false; std::cout << "NO omega\n"; }

    // 未收录的记号应返回空（G-Y 无 def 文件）
    if (!expander_defs::lookup(IDR_DEF_GY).empty()) { ok = false; std::cout << "GY should be empty\n"; }

    // 打印 PPS 前 60 个码元，供人工确认中文/换行正常
    std::cout << "PPS[:60] = ";
    for (size_t i = 0; i < pps.size() && i < 60; ++i) {
        std::wcout << static_cast<wchar_t>(pps[i]);
    }
    std::wcout << L"\n";

    std::cout << (ok ? "ALL_OK" : "FAIL") << "\n";
    return ok ? 0 : 1;
}

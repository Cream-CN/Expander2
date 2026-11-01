#pragma once
// ============================================================
// 互译器 (Translator) —— 接口契约
//
// 定义：一个"互译器"是一个纯函数，把某一种记号（源记号 source）的合法表达式
// 文本，映射为另一种记号（目标记号 target）中表示同一序数的表达式文本。
// 它不负责展开（那是记号的事），只负责"翻译"——在两种记号的写法之间转换，
// 并保证所表达的序数不变。
//
// 规范（新增互译器时必须遵守）详见：Docs/互译器规范.md
// ============================================================
#include <string>
#include <string_view>

namespace omegay::translator {

// ---- 互译函数签名 ----
// 输入：源记号的表达式文本（UTF-8）。
// 输出：目标记号的表达式文本（UTF-8）。
// 约定（与记号 expand_text 一脉相承）：
//   * 必须是纯函数：无副作用、无全局可变状态，同一输入必然得到同一输出；
//   * 解析失败 / 非标准 / 超出适用范围时，必须返回本头文件定义的错误文本常量
//     （kParseError / kRangeError / kNotSupported）之一，或带有明确"互译失败"
//     前缀的可读信息，不允许抛异常、不允许未定义行为；
//   * 返回的普通文本不会被当作合法表达式提交给展开器。
using TranslateFn = std::string (*)(std::string_view);

// ---- 互译器条目 ----
// 登记在互译器表中的一项，对应一个"源记号 → 目标记号"的互译方向。
// 互译是"有方向的"：A→B 与 B→A 是两个不同的互译器（多数互译不可逆，或两个
// 方向需要分别实现）。source 与 target 相同表示"不改写"，仅作占位 / 调试。
struct Translator {
    const wchar_t* display_name;   // 展示名（如 L"恒等互译器"），用于 UI
    const char*    id;             // 唯一英文 id（如 "identity"），稳定引用 / 调试
    const wchar_t* source;         // 源记号名（人类可读）
    const wchar_t* target;         // 目标记号名（人类可读）
    const wchar_t* description;    // 一句话说明（展示在互译器页面）
    TranslateFn    translate;      // 互译函数（必填，不得为空）
};

// ---- 错误文本约定 ----
// 互译函数失败时应返回下列常量之一；UI 会原样展示并据此提示"互译失败"，
// 不会把它当作有效表达式。各互译器可按需补充带 "互译失败：" 前缀的说明文本。
inline constexpr const char* kParseError   = "互译失败：无法解析输入";
inline constexpr const char* kRangeError   = "互译失败：输入超出该互译器的适用范围";
inline constexpr const char* kNotSupported = "互译失败：该表达式暂不支持互译";

// ---- 内置互译器 ----
// 恒等互译器：把输入原样返回（占位 / 调试 / 框架基线）。
// source 与 target 相同（"任意 → 相同"），不做任何改写，也不校验输入。
// 它用于验证互译器框架本身：选中它、输入任意文本、点击"互译"，
// 输出应与输入逐字节一致。
[[nodiscard]] inline std::string identityTranslate(std::string_view input) {
    return std::string(input);
}

} // namespace omegay::translator

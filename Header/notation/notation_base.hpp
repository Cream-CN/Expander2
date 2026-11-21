// Header/notation/notation_base.hpp
// ---------------------------------------------------------------------------
// 记号实现的公共基类（NotationBase）
//
// 背景：Header/notation 下 100+ 个记号实现都写成
//     struct XxxNotation {
//         static constexpr const char* kName = "...";
//         [[nodiscard]] static std::string expand_text(std::string_view, int);
//         [[nodiscard]] static std::string suffix() { return {}; }   // ← 逐字重复
//     };
// 注册中心（registry.hpp）按静态多态消费（T::expand_text / T::suffix / ...），
// 因此“共同处”只能收在基类里，而不能改成虚函数。
//
// 本类只收敛**与记号语义无关**的样板：
//   · suffix()      —— 历史实现里除 EmptyNotation 外一律返回空串；
//   · clamp_term()  —— 展开入口的项数归一（term<1 → 1，TON 家族用 min_term=0）。
//
// 刻意**不**收敛的东西：
//   · 文本解析 —— 顶层 "Limit" / "∞" 解析已按用户要求整体拆除，各记号只处理
//     普通表达式；表达式内部的极限/后继判定仍留在各族 detail 层，本类不参与；
//   · 展开算法本体（expand_text / expand_text_n / expand）—— 留在各记号内。
//
// 约定：记号实现可直接继承本类并从其获得 suffix()；若某个记号确有非空后缀
// （如 EmptyNotation），在自己的结构体里重新定义 suffix() 即可遮蔽基类版本。
// ---------------------------------------------------------------------------
#pragma once

#include <string>

namespace omegay::notation {

struct NotationBase {
    // 展开结果后缀：绝大多数记号不需要，统一返回空串。
    [[nodiscard]] static std::string suffix() { return {}; }

    // 项数归一：term 小于下限时取该下限。多数记号的下限是 1；
    // TON 家族按 0 起算，调用时显式传 min_term = 0。
    [[nodiscard]] static int clamp_term(int term, int min_term = 1) noexcept {
        return term < min_term ? min_term : term;
    }
};

} // namespace omegay::notation

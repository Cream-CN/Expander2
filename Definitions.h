// GENERATED FILE —— 请勿手工编辑。由 tools/gen_definitions.py 从 defs/*.txt 生成。
// 重新生成: python tools/gen_definitions.py
//
// 记号定义文本以 UTF-8 原始字符串字面量直接编译进程序，完全绕过 rc.exe，
// 从而避免 rc.exe 对 UTF-8 源文件的编码误读(首尾乱码)与 \n 不转义的问题。
#pragma once
#include <string_view>
#include "Resource.h"

namespace expander_defs {

constexpr std::string_view kDef_IDR_DEF_2PPS4 = R"DEFTXT(
2-pps4

2-PPS4 。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V1 = R"DEFTXT(
Bashicu矩阵 版本1
定义暂缺
良定义性：证伪
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V2 = R"DEFTXT(
Bashicu矩阵 版本2
定义暂缺
良定义性：证伪
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V21 = R"DEFTXT(
Bashicu矩阵 版本2.1
定义暂缺
良定义性：未知
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V22 = R"DEFTXT(
Bashicu矩阵 版本2.2
定义暂缺
良定义性：未知
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V23 = R"DEFTXT(
Bashicu矩阵 版本2.3
定义暂缺
良定义性：未知
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V3 = R"DEFTXT(
Bashicu矩阵 版本3
定义暂缺
良定义性：证伪
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V31 = R"DEFTXT(
Bashicu矩阵 版本3.1
定义暂缺
良定义性：未知
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V32 = R"DEFTXT(
Bashicu矩阵 版本3.2
定义暂缺
良定义性：未知
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V33 = R"DEFTXT(
Bashicu矩阵 版本3.3
IBMS 较为理想的无提升BMS
定义暂缺
良定义性：有争议
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V4 = R"DEFTXT(
Bashicu矩阵 版本4
良定义性：证明
    论文：arXiv:2307.04606v3 [math.LO] 11 Oct 2024
    名字：Well-Orderedness of the Bashicu Matrix System
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_EMPTY = R"DEFTXT(
空记号（empty）

返回输入序列
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_EPSILON_Y = R"DEFTXT(
ε-Y
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_EWPPS4 = R"DEFTXT(
Ex. Weak PPS4

扩展弱 PPS4。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_MRSS121 = R"DEFTXT(
MrSS1.2.1
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_OMEGA_Y_MEDIUM = R"DEFTXT(
ω-Y (medium)

ω-Y Medium Magma Style
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_OMEGA_Y_SEQUENCE = R"DEFTXT(
ω-Y sequence

ω-Y sequence

一个 ω − Y 序列是形如 ω − Y(a1, a2, . . . , an) 的序列。
ω − Y 序列山脉图的绘制方法如下：
(1) 为每一行赋予一个行标，原序列的行标为 0。
(2) 第 0 行中元素的父项为从该元素起，在该元素左边且小于该元素的第一个项。
(3) 元素所对应的阶差项为该元素与其父项的差值，所有元素的阶差项构成阶差序列。特别地，如果某一项
不存在父项，则其阶差项为空。
(4) 各阶阶差序列中某元素的父项定义为阶差序列中第一个在它左边、小于它，并且其正下方的项是该元素正下方元素祖先项的项。
(5) 逐阶计算阶差序列，直到某一阶阶差序列的所有项均为空为止。
(6) 将各阶阶差序列从下到上写在原序列对应元素的正上方，并将各阶阶差序列中的每一项与其所对应的正 
下方的项以及正下方项的父项相连。特别地，如果某一项不存在父项，则不将该项与其他项相连。连接阶差项与 
其正下方元素的线称为右腿，而连接阶差项与其左下方的父项的连线称为左腿。每计算一次阶差序列，则其行 
标增加 1。这样可以得到山脉图的前 n 行。
(7) 对于山脉图某列顶端的元素来说，元素的父项关系为：从一个顶端元素出发，如果沿着它的左腿向下一步，再沿着右腿向正上方走到不超过 A 的行标（如果无路可走则不走），不断地重复这一过程，直到达到了另一个顶端元素。接下来从新得到的顶端元素出发重复上述操作，又得到另一个顶端元素。这样得到的所有顶端元 
素，称为该元素的待定父项。
(8) 每作一条分隔线之前，都要检查山脉图全部列的元素是否全为 1。如果不是的话，就从最低阶的分隔线开始作起，然后找到该条分隔线与其下方最近的同阶分隔线的山脉图（如果不包含其他的同阶分隔线，则考虑 全部的山脉图），对这些山脉图中包含的所有列的顶端元素计算阶差序列。如果这样的阶差序列不能够计算的话，就提高分隔线的阶次，重复计算阶差序列，直到能够计算阶差序列为止。每穿过一条 n 阶分隔线，则行标右加 ω^n。
(9) 对新的山脉图不断重复上述操作，直到山脉图中所有的顶端项全为 1，则山脉图绘制结束。ω − Y 序列 的山脉图的行标总小于 ω^ω。
在 ω − Y 序列的山脉图中定义如下概念：
(1) 末列最上方的 1 左腿所指的元素称为根元素。
(2) 根元素所在列称为根列。
(3) 根列包含的所有元素称为根列元素。
(4) 根列元素的作用区域为从这一根列元素出发（包含这一列），到在它正上方的根列元素（包含这一列，如果没有的话就到山脉图的顶端）之间的部分。如果某个根列元素的行标为 α，在它正上方的根列元素的行标为 β，那么它的作用范围为所有行标 γ 满足 α ≤ γ < β 的行。
(5) 第 α 行和第 β 行 (α ≤ β) 的行差为满足 α + δ = β 的序数 δ 。
(6) 轮廓边定义为：从一个根列元素出发，沿左腿向上一步（但不能超出这个根列元素的作用区域）之后， 沿右腿向下若干步（可以不向下，但同样不能超出这个根列元素的作用区域），随后重复这个过程直到无路可走。 能通过这样的操作经过的边，都是这个根列元素对应的（或者这个作用区域内的）轮廓边。
(7) 非轮廓边定义为：经过了某个作用区域，但不符合这个作用区域内轮廓边的概念的边，称为这个作用区 域内的非轮廓边。

(8) 填充边定义为：从一个根列元素出发，沿左腿向上走一行后，沿右腿向下走一行，随后重复这个过程直到无路可走。能通过这样的操作经过的边，称为这个根列元素对应的填充边。
ω − Y 序列山脉图的展开方法如下：
(1) 将末列的各项减一。如果最上方的元素被减为零，则删去它相关的左腿和右腿。

(2) 从最上方的根列元素开始，找到其作用区域。

(3) 在这一根列元素作用区域内找到所有的轮廓边，并将轮廓边的端点及其所指的元素向右上方进行复制。 
具体地，向右平移（末列位置 − 根列位置）的列数，然后向上平移至满足以下条件的位置：若平移前端点所指的元素与根列元素行差为 δ0 ，那么平移后对应端点所指的元素与末列最上方的元素行差也要为 δ0 。像这样，至所有的轮廊边和它们所指的元素都被复制到了新的位置上。

(4) 如果末列最上方元素与根列元素的行差为 0，则在这一根列元素作用区域内找到所有的填充边，然后将它不断复制，并填补到轮廓边提升所产生的所有缝隙之中。特别地，当填充边被复制到跨过了 n 阶分隔线的位置上时，其左腿和右腿的行差需要为 ω^n−1 。
(5) 在这一根列元素作用区域内找到所有的非轮廓边，并对这些边进行复制。非轮廓边左腿指向的元素都保持不动，而右腿向右或右上平移，向右平移的列数为（末列位置 − 根列位置）。如果这条非轮廓边右腿所指元素没有被这个作用区域内的轮廊边所指到，那么无需向上平移；如果有，那么向上平移到与末列最上方元素行差为 δ0 的位置，其中 δ0 为平移之前该元素与根列元素的行差。
(6) 自上而下不断地对各个根列元素所对应的山脉图重复 (3) − (5) 的操作，直到山脉图的所有部分都完成复制。

(7) 按照从上到下、从左到右的顺序将山脉图中的各个元素计算出来。如果该元素不是山脉图某一部分的顶端元素，则其取值等于该元素正上方的元素与其父项之和。
ω − Y 序列的取值定义如下： 
(1) ω − Y(∅) = 0。
(2) 如果原序列的末项为 1，则它对应的序数为删去末尾的 1 之后余下的部分所对应的序数加 1。
(3) 否则按照前述山脉图的展开方式对序列进行展开，展开后山脉图最下方的序列就是展开后的 ω − Y 序列。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_OMEGA_Y_STRONG = R"DEFTXT(
ω-Y (strong)

ω-Y Strong Magam Style
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_PPS = R"DEFTXT(
PPS🎄

定义：PPS1
Parented Predecessor Sequence 1

极限表达式：0,1,2,3,4,5,......
记末项的值为 x，坏根为第 x 项，坏根的值为 b，末项是序列中的第 y 项，并令 L = y - x
展开：
1.如果末项是 0，则它是后继序数
2.末项之前的部分保持不变
3.替换末项：如果末项和坏根之间(两边都不含) 存在一项，它的值等于 b，那么将末项的值换成 b；否则
将末项的值减 1
4.递归生成其他项(第 i + L 项的值由第 i 项确定)：对任意的 i > x，如果第 i 项的值大于等于 x，那么第 i + L
项的值等于第 i 项的值 + L，否则第 i + L 项的值等于第 i 项的值
5.基本列[n] 为展开到第 y + n * L - 1 项。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_PPS4 = R"DEFTXT(
PPS4

PPS 的第四版本
定义：PPS 4
Parented Predecessor Sequence 4
极限表达式：0,1,2,3,....
坏根：列标是 (末项的值) 的项（首项的列标是 1）；如果末项是 0，则表示后继序数 
记此时末项的列标减末项的值为 L，坏根的值为 b，末项的值为 x、列标为 y
末项展开：
> 如果末项和坏根之间 (两边都不含) 存在一项，它的值等于 b，那么是弱展开，否则是强展开；弱展开：将末项的值换成 b；
> 强展开：在第 b 列和第 x 列 (都不含) 之间找到最右侧的值小于等于 b 的项，将末项的值换为这个项的列标；如果找不到，则等同弱展开
其他项展开：对任意的 i>y-L，如果第 i 项的值大于等于 x，那么第 i+L 项的值等于第 i 项的值 +L，否则第 i+L 项的值等于第 i 项的值
基本列 [n] 为展开到第 y+nL-1 项
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_SPPS4 = R"DEFTXT(
Second PPS4

PPS4 的第二型变体。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TPPS4 = R"DEFTXT(
Third PPS4

PPS4 的第三型变体。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_UPMS = R"DEFTXT(
UPMS
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_WPPS4 = R"DEFTXT(
Weak PPS4

PPS4 的弱化版本。
)DEFTXT";

struct Entry { int id; std::string_view utf8; };
constexpr Entry kTable[] = {
    { IDR_DEF_2PPS4, kDef_IDR_DEF_2PPS4 },
    { IDR_DEF_BMS_V1, kDef_IDR_DEF_BMS_V1 },
    { IDR_DEF_BMS_V2, kDef_IDR_DEF_BMS_V2 },
    { IDR_DEF_BMS_V21, kDef_IDR_DEF_BMS_V21 },
    { IDR_DEF_BMS_V22, kDef_IDR_DEF_BMS_V22 },
    { IDR_DEF_BMS_V23, kDef_IDR_DEF_BMS_V23 },
    { IDR_DEF_BMS_V3, kDef_IDR_DEF_BMS_V3 },
    { IDR_DEF_BMS_V31, kDef_IDR_DEF_BMS_V31 },
    { IDR_DEF_BMS_V32, kDef_IDR_DEF_BMS_V32 },
    { IDR_DEF_BMS_V33, kDef_IDR_DEF_BMS_V33 },
    { IDR_DEF_BMS_V4, kDef_IDR_DEF_BMS_V4 },
    { IDR_DEF_EMPTY, kDef_IDR_DEF_EMPTY },
    { IDR_DEF_EPSILON_Y, kDef_IDR_DEF_EPSILON_Y },
    { IDR_DEF_EWPPS4, kDef_IDR_DEF_EWPPS4 },
    { IDR_DEF_MRSS121, kDef_IDR_DEF_MRSS121 },
    { IDR_DEF_OMEGA_Y_MEDIUM, kDef_IDR_DEF_OMEGA_Y_MEDIUM },
    { IDR_DEF_OMEGA_Y_SEQUENCE, kDef_IDR_DEF_OMEGA_Y_SEQUENCE },
    { IDR_DEF_OMEGA_Y_STRONG, kDef_IDR_DEF_OMEGA_Y_STRONG },
    { IDR_DEF_PPS, kDef_IDR_DEF_PPS },
    { IDR_DEF_PPS4, kDef_IDR_DEF_PPS4 },
    { IDR_DEF_SPPS4, kDef_IDR_DEF_SPPS4 },
    { IDR_DEF_TPPS4, kDef_IDR_DEF_TPPS4 },
    { IDR_DEF_UPMS, kDef_IDR_DEF_UPMS },
    { IDR_DEF_WPPS4, kDef_IDR_DEF_WPPS4 },
};

// 按资源 ID 取定义文本；未收录的记号返回空(由调用方显示“暂缺”)。
[[nodiscard]] inline std::string_view lookup(int id) {
    for (const Entry& e : kTable) {
        if (e.id == id) return e.utf8;
    }
    return {};
}

} // namespace expander_defs

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

constexpr std::string_view kDef_IDR_DEF_BLP = R"DEFTXT(
blp（即 DEN2 / IBLP）
输入格式：行序列，每行形如 (条目,条目,...)步数，条目可用 * 前缀表示标记，例如
(1,0)1(2,1,0)1(*3,2,1,0)2
记号极限输入 Limit。
逻辑移植自 hypcos 的 notation-explorer（src/notations/DEN/DEN2.ts）。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V1 = R"DEFTXT(
BMS v1.0（Bashicu Matrix System, Version 1）

输入格式：BMS 文本，如 (0,0)(1,1)(2,2) 或 (0,0)(1,1)[3]。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V2 = R"DEFTXT(
BMS v2.0（Version 2.0）

输入格式同 v1.0。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V21 = R"DEFTXT(
BMS v2.1（Version 2.1）

输入格式同 v1.0。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V22 = R"DEFTXT(
BMS v2.2（Version 2.2）

输入格式同 v1.0。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V23 = R"DEFTXT(
BMS v2.3（Version 2.3）

输入格式同 v1.0。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V3 = R"DEFTXT(
BMS v3.0（Version 3.0）

输入格式同 v1.0。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V31 = R"DEFTXT(
BMS v3.1（Version 3.1）

输入格式同 v1.0。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V32 = R"DEFTXT(
BMS v3.2（Version 3.2）

输入格式同 v1.0。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V33 = R"DEFTXT(
BMS v3.3（Version 3.3）

输入格式同 v1.0。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V4 = R"DEFTXT(
BMS v4.0（Version 4.0，当前默认）

输入格式同 v1.0。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_DEN1 = R"DEFTXT(
DEN1（Defective embedding notation）
输入格式：主体行序列与父标号列表，用分号分隔，例如
(1,0)1(2,1,0)1;0
每一行形如 (值,值,...)步数，分号后是父标号列表（可为空）。
记号极限输入 Limit。
逻辑移植自 hypcos 的 notation-explorer（src/notations/DEN/DEN.ts）。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_EMPTY = R"DEFTXT(
空记号（empty）

返回输入序列
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_EPSILON_Y = R"DEFTXT(
ε-Y（1-Y）

ε-Y 即 1-Y 记号，等价于维度序列为 {1} 的 ω-Y 变体。

极限表达式：
  1,2,3,4,5,...

展开规则（1-Y / 维度 1）：
  与 ω-Y 的山脉图机制相同，但所有阶差分量的维度固定为 1。
  对末列最上方的 1 求根元素，将其作用区域内的轮廓边、填充边、
  非轮廓边按 ω-Y 的规则复制，再自上而下计算各元素取值。
  由于维度被限制为 1，展开不会引入新的高阶分量，
  因此 ε-Y 强于 0-Y，是 1-Y 的标准实现。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_EWPPS4 = R"DEFTXT(
Ex. Weak PPS4

扩展弱 PPS4。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_IBLP = R"DEFTXT(
iblp

HypCos 基于 BLP 定义了 Defective Embeeding Notation (DEN)。DEN 绝大部分定义都照搬 blp，但在 “M 操作” 之中的 “Copied 操作” 中，复制的不是从第 s₍₋₁,₋₁₋₂₎ 行到第 s₍₋₁,₋₁₋₁₎₋₁ 行，而是从第 s₍₋₁,₋₁₋₂₎ 行开始的所有内容（包括末行），这算一轮延伸，延伸多轮之后再去掉末行。基本上如果把 DEN 比作 bms，那么 blp 就可以比作只有最后一列长度为 1 时展开 n 次，其他时候展开两次的 bms。HypCos 制作了 DEN 的展开器，详见。后续 test_alpha0 和 HypCos 制作了第二版的 blp 和 DEN，其代码详见。

test_alpha0 则将 DEN 称为无限 blp (iblp)。他给出了 iblp 的形式化文字定义如下。

（IBLP）。一个 IBLP 是一个四元组
A = (n, (A₁, …, Aₙ), (l₁, …, lₙ), (M₁, …, Mₙ)),
其中：
(1) n ∈ ℕ（允许 n = 0）。
(2) 对每个 i ∈ {1, …, n}, Aᵢ 是严格递增的非负整数序列，满足 len (Aᵢ) ≥ 2 且 max (Aᵢ) = i。
(3) 对每个 i ∈ {1, …, n}，步长 lᵢ ∈ ℕ。
(4) 对每个 i ∈ {1, …, n}，标记集合 Mᵢ ⊆ Aᵢ。

约定所有编号从 1 开始；Aᵢ[k] 表示第 k 个元素，Aᵢ[−j] := Aᵢ[len (Aᵢ) − j + 1] 表示倒数第 j 个元素。

（长行 / 中等行 / 短行）。行 i 称为长行若 len (Aᵢ) > 2lᵢ，称为中等行若 len (Aᵢ) = 2lᵢ，称为短行若 len (Aᵢ) < 2lᵢ。

（零图案 / 后继图案 / 极限图案）。若 n = 0，称 A 为零图案。若 n > 0 且 len (Aₙ) = 2 且 min (Aₙ) = 0，称 A 为后继图案。否则称 A 为极限图案。

（行比较键）。令 Aᵢ = (a₁ < a₂ < … < aₘ) 且 L = lᵢ。定义保留序列
Kᵢ :=
⎧ (a₁,a₂,…,aₘ),  L ≤ 1;
⎨ (a₁,a_{L₊₁},a_{L₊₂},…,aₘ), L ≥ 2 且 m ≥ L;
⎩ (a₁,a₂,…,aₘ),  L ≥ 2 且 m < L.
定义行键为 K̅ᵢ（即 Kᵢ 逆序，从大到小）。

（IBLP 的大小比较）。给定两个 IBLP A,B，从 i = 1 起逐行比较其行键 K̅ᵢ(A) 与 K̅ᵢ(B) 的字典序；第一处不同决定大小。若前 min (nₐ,nᵦ) 行都相等，则行数较小者更小。

(p (i))。对 i ∈ {1, …, n}，定义
p (i) := Aᵢ[−(lᵢ + 1)].

（传递序列 tᵢ(j)）。固定行 i。若 j = Aᵢ[k] (1 ≤ k ≤ len (Aᵢ)) 且 k > lᵢ，定义阈值 θ := Aᵢ[k − lᵢ]。令 t₀ := j，并在 tₘ > θ 且 p (tₘ) 有定义时令 tₘ₊₁ := p (tₘ)。若存在最小 m ≥ 0 使 tₘ = θ，则定义
trᵢ(j) := (t₀,t₁,…,tₘ),
否则称 tᵢ(j) 无定义。若 k ≤ lᵢ，亦称无定义。

（部分函数 fₐ）。设 A 非零且有 n ≥ 1 行。令 a₁ := Aₙ[1] = min (Aₙ), L := lₙ, p := p (n) = Aₙ[−(L + 1)]。定义部分函数 fₐ:ℕ→ℕ：
(1) 若 x < a₁，则 fₐ(x) = x；
(2) 若 x = Aₙ[k] 且 x < p 且 k + L ≤ len (Aₙ)，则 fₐ(x) = Aₙ[k + L]；
(3) 若 x ≥ p，则 fₐ(x) = x + (n − p)；
(4) 其余情形 fₐ(x) 无定义。

基本 Laver 图案

（copy 的行与步长）。设 A 行数为 n ≥ 1，令 p := p (n)。若 copy 过程中出现 fₐ(x) 无定义，则 copy (A) 无定义。否则定义 A′ = copy (A) 行数为 n′ := 2n − p，满足：
(1) 对 1 ≤ i ≤ n − 1 − p + 1：A′ᵢ = Aᵢ, l′ᵢ = lᵢ, M′ᵢ = Mᵢ；
(2) 对每个 i ∈ {1,…,n − p + 1}，令源行 σ := p + i − 1，目标行 τ := n − 1 + i，定义
A′τ := sort ({ fₐ(x) | x ∈ Aσ }), l′τ := lσ,
要求 x ∈ Aσ 时 fₐ(x) 全部有定义。

（copy 的标记过滤）。延续上一定义。对目标行 τ 中元素 x ∈ A′τ，令 y ∈ Aσ 为其唯一来源（即 fₐ(y) = x）。则 x ∈ M′τ 当且仅当：
(1) y ∈ Mσ，并且满足下列之一：
(a) trτ(x) 有定义且 trτ(x)[−2] ≥ p；
(b) 令 y₀ 为 trτ(x) 中首次出现的且 < p 的元素（等价于 “最大的 < p 的元素”）。要么 y₀ < a₁；要么存在 k 使 y₀ = Aₙ[k] 且 k + L ≤ len (Aₙ) 且 Aₙ[k + L] ∈ Mₙ，并且若 x 在 A′τ 中的位置为 q (从 1 开始)，则当 q + 1 − lσ ≥ 1 时需满足
A′τ[q + 1 − lσ] ≤ a₁

(E (A,0))。若 A 非零且 n ≥ 1，则 E (A,0) 为删除最后一行后的图案（行数变为 n − 1，其余行、步长、标记保持不变）。
(E (A,m) 的 copy 阶段 (m ≥ 1))。设 A 既非零图案也非后继图案。令 A⁽⁰⁾ := A。若第一次 copy (A⁽⁰⁾) 无定义，则称 A 为坏图案并定义 E (A,m) := E (A,0)。否则定义
A⁽¹⁾ := copy (A⁽⁰⁾), A⁽²⁾ := copy (A⁽¹⁾), …, A⁽ᵐ⁾ := copy (A⁽ᵐ⁻¹⁾),
并令
B := E (A⁽ᵐ⁾,0)

接下来对 B 做补全，初始行 r := n（这里 n 为原始 A 的行数）。补全过程允许使用临时记录映射 Rec（行号 ↦ 有限序列），结束后丢弃。

（标记补全（快照语义））。固定当前行 r。令
Bᵣ := {b ∈ Mᵣ | b ∈ Aᵣ},
并按升序枚举其元素 b₁ < … < bₖ(该集合在本轮开始时冻结，新产生标记不加入本轮)。对每个 bᵢ 依次：
(1) 若 trᵣ(bᵢ) 无定义则跳过；
(2) 令 t := trᵣ(bᵢ)[−2]。若 Rec (t) 不存在或为空则跳过，否则 s := Rec (t)；
(3) 检查对所有 j = 3,4,…,len (trᵣ(bᵢ))，
trᵣ(bᵢ)[−j + 1] + 1 ∈ A_{trᵣ(bᵢ)[−j]}
若失败则跳过；
(4) 若成功，则把 s 及 bᵢ₊₁,…,bᵢ₊len (s) 加入 Aᵣ，并重新排序去重使之严格递增。更新标记：s 中元素不标记，bᵢ₊₁,…,bᵢ₊len (s) 全标记，其余标记保留（按上述规则修正）。更新步长：lᵣ := lᵣ + len (s)。

（原生补全）。固定当前行 r。若 len (Aᵣ) > 2lᵣ(长行)，则原生补全不执行并返回 0。否则令
pᵣ := p (r) = Aᵣ[−(lᵣ + 1)], a := Aᵣ[−lᵣ].

构造序列 s：令 u₀ := a，并在 A_{uₘ}[−2] 存在且 > pᵣ 时令 uₘ₊₁ := A_{uₘ}[−2] 并把 uₘ₊₁ 追加到 s，直至无法继续。令 t := len (s)。若 t = 0 返回 0；若 t > 0，令 Rec (r) := s 并继续：
(1) 在 r 后插入 t 行，使原本行号 > r 的行号整体加 t；
(2) 仅对原本行号 > r 的那些行，在其中把所有 > r 的元素加 t，并同步标记集合，步长不变；
(3) 令旧行及旧步长为 Aᵣᵒˡᵈ, lᵣᵒˡᵈ。将旧行替换为 t + 1 行：
A_{r₊t} := sort ( Aᵣᵒˡᵈ ∪ s ∪ {r + 1, …, r + t} ), l_{r₊t} := lᵣᵒˡᵈ + t.
标记规则：旧标记保留；r,r + 1,…,r + t − 1 标记；s 不标记；r + t 不标记。
(4) 对 i = t,t − 1,…,1，由 A_{r₊i} 构造 A_{r₊i₋₁}：删除元素 r + i，并删除元素 A_{r₊i}[−l_{r₊i}]，去除 r + i − 1 的标记，并令 l_{r₊i₋₁} := l_{r₊i} − 1。
(5) 中等行例外：若 i = t 且 Aᵣᵒˡᵈ 为中等行（len (Aᵣᵒˡᵈ) = 2lᵣᵒˡᵈ），则在构造 A_{r₊t₋₁} 时不删除 A_{r₊t}[−l_{r₊t}] 且不减步长（令 l_{r₊t₋₁} := l_{r₊t}），但仍删除 r + t 并去除 r + t − 1 的标记。

原生补全返回 t。

（补全主循环（用于 E (A,m)））。令初始 r := n（n 为原始 A 的行数）。当 r 不超过当前 B 的行数时循环：先对行 r 执行标记补全，再执行原生补全得到 t，然后更新 r := r + t + 1。当 r 越界时补全结束，丢弃 Rec，所得 B 即为 E (A,m)。
iblp的定义目前还有些问题，但是其在MHO下的停机是公认证实的。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_MN = R"DEFTXT(
MN 系列记号（源自 hypcos 的 Notation Explorer）
=================================================

本组记号均为大数记号（googology）系统中的展开器实现。输入为对应记号的
标准表达式文本，输出为其第 n 项基本列展开（Fundamental Sequence）。

包含家族：
- e0MN / strong e0MN / ωMN / PCF's ωMN / TωMN：ε0 之上构造的 MN 基础系列；
- 1MN / 2MN / 3MN（nMN 系列）：以 n 为幂基的 MN 系列；
- Sω2MN / SAω2MN / Sω^ωMN：S 系列 MN 型记号；
- S1DBMS / S2DBMS / SωDBMS v1/v2/v3 / Sω+1DBMS / Sω2DBMS / Sω^2DBMS：
  S 系列 DBMS 型记号；
- UP1MN / UP2MN v1A / v1B / v1B+：UP 系列 MN 型记号；
- UP1DBMS / UP2DBMS v1 / v1B+ / v1C / v2：UP 系列 DBMS 型记号；
- Aω2MN2 / Aω2MN3 / wAω2MN2 / wAω2MN3：Aw2MN 系列。

用法：在输入框内输入该记号的表达式，选择展开次数（指数），点击展开。
极限序数输入 "Limit"。各记号的具体语法见各自参考实现（Notation Explorer）。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_MRSS121 = R"DEFTXT(
MrSS1.2.1（山脉结构序列）

本版本只利用 1 层山脉结构。
合法表达式形如 S = a1, a2, a3, ...，其中 a1 = 1，
an 为 MrSS 表达式或有限非零序数。

支持嵌套写法，例如：
  1,(1,2),(1,2,3)
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

constexpr std::string_view kDef_IDR_DEF_TON_DR = R"DEFTXT(
Degrees of Reflection（TON_DoR）
Taranovsky 序数记号的“反射度”变体。noraise 格式。
输入格式：后缀式文本。叶子记号："0" 表示 -1，"Ω" 表示 0，"Limit" 表示极限；两个叶子或子树并排后再跟一个 "C" 表示组合。
记号极限输入 Limit（等价于反射极限项）。
逻辑移植自 ne-rewritten（src/notations/TON/TON_DoR.ts）。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_DRC = R"DEFTXT(
Degrees of Reflection (reflection configuration)（TON_DRC）
Taranovsky 序数记号的“反射度”变体，反射配置。noraise 格式。
输入格式：后缀式文本。叶子记号："0" 表示 -1，"Ω" 表示 0，"Limit" 表示极限；两个叶子或子树并排后再跟一个 "C" 表示组合。
记号极限输入 Limit。
逻辑移植自 ne-rewritten（src/notations/TON/TON_DRC.ts）。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_DRP = R"DEFTXT(
Degrees of Reflection with Passthrough（TON_DRP）
Taranovsky 序数记号的“反射度”变体，含 passthrough。noraise 格式。
输入格式：后缀式文本。叶子记号："0" 表示 -1，"Ω" 表示 0，"Limit" 表示极限；两个叶子或子树并排后再跟一个 "C" 表示组合。
记号极限输入 Limit。
逻辑移植自 ne-rewritten（src/notations/TON/TON_DRP.ts）。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_DRPC = R"DEFTXT(
Degrees of Reflection with Passthrough (reflection configuration)（TON_DRPC）
Taranovsky 序数记号的“反射度”变体，含 passthrough，反射配置。noraise 格式。
输入格式：后缀式文本。叶子记号："0" 表示 -1，"Ω" 表示 0，"Limit" 表示极限；两个叶子或子树并排后再跟一个 "C" 表示组合。
记号极限输入 Limit。
逻辑移植自 ne-rewritten（src/notations/TON/TON_DRPC.ts）。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_I = R"DEFTXT(
Iteration of n-built from below (no passthrough)（TON_I）
Taranovsky 序数记号的“n-built from below 迭代”变体，不含 passthrough。noraise 格式。
输入格式：后缀式文本。叶子记号："0" 表示 -1，"Ω" 表示 0，"Limit" 表示极限；两个叶子或子树并排后再跟一个 "C" 表示组合，例如 0ΩC。
记号极限输入 Limit（等价于迭代极限项）。
逻辑移植自 ne-rewritten（src/notations/TON/TON_I.ts）。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_IBP = R"DEFTXT(
Iteration of n-built from below（TON_IBP）
Taranovsky 序数记号的“n-built from below 迭代”变体（含 passthrough）。noraise 格式。
输入格式：后缀式文本。叶子记号："0" 表示 -1，"Ω" 表示 0，"Limit" 表示极限；两个叶子或子树并排后再跟一个 "C" 表示组合。
记号极限输入 Limit。
逻辑移植自 ne-rewritten（src/notations/TON/TON_IBP.ts）。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_M = R"DEFTXT(
TON（Taranovsky 序数记号，主家族）
输入格式：后缀式文本。叶子记号："0" 表示 -1，"Ω<n>" 表示非负序数（角括号定界，如 Ω<0>、Ω<2>），"Limit" 表示极限；两个叶子或子树并排后再跟一个 "C" 表示组合（l,r)ₙ，例如 Ω<0>0C 表示 Ω<0> 与 0 的组合。
记号极限输入 Limit（等价于第 ω 个可数容许序数）。
逻辑移植自 ne-rewritten（src/notations/TON/TON_main.ts）。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_MC = R"DEFTXT(
TON（reflection configuration）without passthrough（TON_MC）
Taranovsky 序数记号的“反射配置”变体，不含 passthrough。主家族格式。
输入格式：后缀式文本。叶子记号："0" 表示 -1，"Ω<n>" 表示非负序数（角括号定界），"Limit" 表示极限；两个叶子或子树并排后再跟一个 "C" 表示组合。
记号极限输入 Limit。
逻辑移植自 ne-rewritten（src/notations/TON/TON_MC.ts）。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_MPC = R"DEFTXT(
TON with passthrough (reflection configuration)（TON_MPC）
Taranovsky 序数记号的“反射配置”变体，含 passthrough。主家族格式。
输入格式：后缀式文本。叶子记号："0" 表示 -1，"Ω<n>" 表示非负序数（角括号定界），"Limit" 表示极限；两个叶子或子树并排后再跟一个 "C" 表示组合。
记号极限输入 Limit。
逻辑移植自 ne-rewritten（src/notations/TON/TON_MPC.ts）。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TPPS4 = R"DEFTXT(
Third PPS4

PPS4 的第三型变体。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_UPMS = R"DEFTXT(
UPMS（User-defined Primitive Matrix System）

输入格式：省略尾随零的矩阵文本，例如 (0)(1)(2,1)
或显式补零的 (0,0)(1,0)(2,1)。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_WPPS4 = R"DEFTXT(
Weak PPS4

PPS4 的弱化版本。
)DEFTXT";

struct Entry { int id; std::string_view utf8; };
constexpr Entry kTable[] = {
    { IDR_DEF_2PPS4, kDef_IDR_DEF_2PPS4 },
    { IDR_DEF_BLP, kDef_IDR_DEF_BLP },
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
    { IDR_DEF_DEN1, kDef_IDR_DEF_DEN1 },
    { IDR_DEF_EMPTY, kDef_IDR_DEF_EMPTY },
    { IDR_DEF_EPSILON_Y, kDef_IDR_DEF_EPSILON_Y },
    { IDR_DEF_EWPPS4, kDef_IDR_DEF_EWPPS4 },
    { IDR_DEF_IBLP, kDef_IDR_DEF_IBLP },
    { IDR_DEF_MN, kDef_IDR_DEF_MN },
    { IDR_DEF_MRSS121, kDef_IDR_DEF_MRSS121 },
    { IDR_DEF_OMEGA_Y_MEDIUM, kDef_IDR_DEF_OMEGA_Y_MEDIUM },
    { IDR_DEF_OMEGA_Y_SEQUENCE, kDef_IDR_DEF_OMEGA_Y_SEQUENCE },
    { IDR_DEF_OMEGA_Y_STRONG, kDef_IDR_DEF_OMEGA_Y_STRONG },
    { IDR_DEF_PPS, kDef_IDR_DEF_PPS },
    { IDR_DEF_PPS4, kDef_IDR_DEF_PPS4 },
    { IDR_DEF_SPPS4, kDef_IDR_DEF_SPPS4 },
    { IDR_DEF_TON_DR, kDef_IDR_DEF_TON_DR },
    { IDR_DEF_TON_DRC, kDef_IDR_DEF_TON_DRC },
    { IDR_DEF_TON_DRP, kDef_IDR_DEF_TON_DRP },
    { IDR_DEF_TON_DRPC, kDef_IDR_DEF_TON_DRPC },
    { IDR_DEF_TON_I, kDef_IDR_DEF_TON_I },
    { IDR_DEF_TON_IBP, kDef_IDR_DEF_TON_IBP },
    { IDR_DEF_TON_M, kDef_IDR_DEF_TON_M },
    { IDR_DEF_TON_MC, kDef_IDR_DEF_TON_MC },
    { IDR_DEF_TON_MPC, kDef_IDR_DEF_TON_MPC },
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

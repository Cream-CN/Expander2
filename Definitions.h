// GENERATED FILE —— 请勿手工编辑。由 build.ps1 的 Invoke-GenDefinitions 从 defs/*.txt 生成。
// 重新生成: .\build.ps1 -Mode gen （或菜单 1 / 4 / 5）
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
blp
基本Laver图案（Basic Laver Pattern）是Renrui Qi（test_alpha0）在文献《Notes On Laver Table》（arXiv:2501.06733v4 [math.LO]）中提出的记号。

我们称 3 元组 p = (s, l, r) 为基本 Laver 图（Basic Laver Pattern，或简称为 blp），如果对于某个整数 n ≥ 2（称为 p 的长度）有：

1. s = (s₁, s₂, …, sₙ) 是一个序列，使得对于任意的 i，sᵢ 均为严格递增的自然数序列，|sᵢ| ≥ 3，并且 sᵢ 以 i, i+1 结尾。此外，s₁ = (0,1,2)，s₂ = (0,1,2,3)。sᵢ 称为 p 的第 i 行。我们用 sᵢ,₁ 表示 sᵢ 的第一个元素。

2. l = (l₁, l₂, …, lₙ)，其中 lᵢ ∈ ω，2·lᵢ ≤ |sᵢ| ≤ 2·lᵢ + 2。若 2·lᵢ = |sᵢ|，则 lᵢ ≥ 3。lᵢ 称为 sᵢ 的步长。
若 lᵢ > 1, |sᵢ| = 2·lᵢ + 1，则我们称 sᵢ 是恰当的。若 lᵢ > 1, |sᵢ| = 2·lᵢ + 2，则我们称 sᵢ 是长的。

3. r 是一个映射，使得若 i ∈ dom(r)，则 sᵢ 是长的，r(i) < i，s_{r(i)} 是恰当的，并且 sᵢ 与 s_{r(i)} 在各自步长上的前 lᵢ + 1 个元素相互重合。

令 m 为正整数，θ₁ < θ₂ < θ₃ < λ。我们称 θ₁, θ₂, θ₃ 是 m-线性的，如果存在 θ₂ = θ₀' < … < θₘ' = θ₃
使得对于任意的 0 ≤ i < m，都存在 jᵢ ∈ Iter(j) 使得 jᵢ : θ₁ ↦ θᵢ' ↦ θ_{i+1}'。

如果 j₀, j₁, …, jₘ 见证 θ₁, θ₂, θ₃ 是 m + 1-线性的，则 j₁, j₂, …, j_{m−2}, jₘ(jₘ)(j_{m−1}) 见证 θ₁, θ₂, θ₃ 是 m-线性的。

一个基本 Laver 系统（basic Laver system，简称为 bls）包含一个 blp p、一个正整数 m、一个序数序列 θ₀', …, θₘ'、一个序数序列 θ₀, θ₁, …, θ_{n+1}、嵌入 j₁, …, jₙ ∈ Iter(j)，使得：

1. θ₀', …, θₘ' 见证 θ₀, θ₁, θ₂ 是 m-线性的，
2. 对于任意 i ≥ 1 有 crit(jᵢ) = θ_{s_{i,1}}，且 jᵢ : (lᵢ)θ_{s_{i,1}} ↦ … ↦ θ_{s_{i,|sᵢ|}}。
3. 若 r(i) = i'，则 jᵢ ^{θ_{s_{i',-1}}} ≡ j_{i'}。

若 q 为 bls，则我们称 q 是 q.p 的实现，此时它继承了从 q.p 到 q 的每个属性。例如，q.l₂ = q.p.l₂。如果 blp 或 bls 在上下文中是清楚的，则我们可以省略 p. 部分，使用 l₂ 来表示 p.l₂。
注意到通过对 n 进行归纳，对于任何 bls q 以及 0 ≤ i < j ≤ q.n+1，都有 l ∈ Iter(j) 使得 l : (1)q.θᵢ ↦ q.θⱼ。
现在我们定义 blp 的复制操作。

令 p 为 blp，p.n ≥ 3。我们定义 p.del 为 p 删除掉最后一行。如果 q 为 bls 且 q.n ≥ 3，则 q.del 定义为从 q 中删除 q.θ_{q.n+1}、q.j_{q.n} 和 q.p 的最后一行。当 n' < n 时，q|n' 定义为对 q 应用 n − n' 次 .del。

令 p 为 blp。我们定义 p 的可复制性，以及当 p 可复制时的 blp copy(p) 如下。如果 n = 2，则 p 不可复制。

令 a = s_{n,-l_n-2}，b = s_{n,-l_n-1} − 1。如果 a = 0，则 p 不可复制。如果对于某些 i，a ≤ i ≤ b，sᵢ 有一个元素 x，使得 s_{n,1} < x < a，并且 x 不是 sₙ 的元素，则 p 不可复制。

我们定义 q = p.Copied 为：令 q|n−1 = p.del，且 q.n = n + b − a，q.l_{n+i} = l_{a+i}。q.s_{n+i} = m''(s_{a+i})，其中对于 k < s_{n,1}，m(k) = k；对于某个 i，若 k ≤ a，则 m(k) = s_{n,i+l_n}；若 k > a，则 m(k) = n − a + k。否则 m 未定义，且在计算 p.Copied 时，若对未定义的输入调用 m，则 p 不可复制。若 r(i) = i'，a ≤ i < b，则当 i' < s_{n,1} 时，令 q.r(i + n − a) = i + n − a，当 i' ≥ a 时，令 q.r(i + n − a) = i' + n − a。

若 q 是 bls，且 q.p 是可复制的，则我们定义 q.Copied 为唯一的 bls，并且 q.Copied.p = q.p.Copied，并且 q.Copied 的嵌入序列为 j₁, j₂, …, j_{n−1}, jₙ(jₐ), jₙ(j_{a+1}), …, jₙ(j_{b−1})，其中 a,b 如前所述，并且 m 线性的见证保持不变。注意最后一个嵌入 jₙ(j_{b−1}) 为 q.j_{−1}(q.j_{b−1})。

对于一个 blp p，若 sₙ 的前三个元素为 0,1,2，且 lₙ = 3，且 |sₙ| = 6，则我们称 p 为极限类型；若 sₙ 的前三个元素为 0,1,2，且 lₙ = 2，且 |sₙ| = 5，则称 p 为后继类型；若 p.n > 2，且 p 既不是极限类型也不是后继类型，则称 p 为暂时类型。

当 m 为正整数，p 为极限类型或后继类型时，我们定义 p.E(m) 如下。令 a = s_{n,-3}，我们取 p.E(0) = p.del，且 p.E'(k) 是唯一的 blp，满足 p.E'(k).del = p.E(k)，p.E'(k).s_{-1,1} = a，p.E'(k).l_{-1} = 1，p.E(k+1) = p.E(k).Copied，其中 k = 1,2,3,…。

若 q 为 bls，则我们取 q.E 使得 q.E.p = q.p.E(m)，并且通过假设 m 线性的见证嵌入为 j₁',…,jₘ'，我们将关于 p.E'(k) 的 bls 的 j_{−1} 取为 jₙ(j_k')。
注意到 q.E.j_{−1} = q.j_{−1}(q.j₁')(q.E(m − 1).j_{−1})，以及 crit(q.j_{−1}(q.j₁')) = q.θ_{s_{-1,-3}} < q.θ_{−1}。
现在我们定义 blp 中恰当行的完成操作。

令 sᵢ 为 p 的恰当行，S 为严格在 s_{i,l_i} 与 s_{i,l_i+1} 之间的整数构成的集合。令 T ⊆ S，T = {t₁ < … < t_u}。

我们定义 q = comp(p,i,T) 如下：q.n = n + u，并且对于任意的 i' < i 都有 q.s_{i'} = s_{i'}, q.l_{i'} = l_{i'}。对于任意的 i ≤ i' ≤ n，若 i' > i 且不满足 r(i') = i，则 q.s_{i'+u} = m''(s_{i'}), q.l_{i'+u} = l_{i'}+u，其中 m(x) = x, x ≤ i, m(x) = x+u, x > i，否则 q.s_{i'+u} 定义为将 i+1,i+2,…,i+u 和 T 插入 m''(s_{i'})，q.l_{i'+u} = l_{i'} + u。对于任意 x,y 且 r(x) = y，令 q.r(m(x)) = m(y)。对于任意的 0 < i ≤ u，q.s_{i−1+i'} 定义为将 t₁,t₂,…,t_{i'},i+1,i+2,…,i+i' 插入 sᵢ。

我们定义 fullcomp(p,i) = comp(p,i,T)，其中 T = {x₁,x₂,…,x_{k−1}}，x₀ = i，x_l = s_{x_{l−1},-3}，x_{k−1} > s_{i,-3} ≥ x_k。

注意到若 T = ∅，则 comp(p,i,T) = p。

对于 bls q，若 sᵢ 为 p 的恰当行，则我们令 q' = comp(q,i,T) 为唯一的 bls，使得 q'.p = comp(p,i,T)，
q'.j_i = q'.j_{i+1} = … = q'.j_{i+|T|}，并且 m-线性保持不变。

若 p 是可复制的，m > 0 为整数，则我们通过假设 i₁ < i₂ < … < i_u 为 p.Copied.r 的值域，并且对 p.Copied
依次执行 fullcomp(−,i_v) 来定义 p.M，其中 v = u, u − 1, …, 1。类似地，如果 q 是 bls，并且 q.p 是可复制
的，则我们以同样的方式通过 q.Copied 定义 q.M。我们有 q.M.p = q.p.M，并且 q.M.j_{−1} = q.Copied.j_{−1}。
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
BMS v3.3
BM3.3被认为是比较理想的IBMS(Idealized BMS, IBMS)，它的提升次数被大幅削弱了
我们令 t 为末列中最后一个非零元素的行指标。在 BM3.3 和 BM4 之中，如果末列中 t 行元素的祖先项经过坏根，
那么对于坏部中所有的 t − 1 行元素来说，提升矩阵的对应项均为 1，
这一点上两个版本的 BMS 是相同的。
但是如果末列中 t 行元素的祖先项不经过坏根，那么二者的规则就会产生区别。
对于 BM4 来说，当且仅当其父项提升矩阵的取值为 1 ，且父项是坏根或位于 BM4 中的坏根的右侧时，该项提升矩阵的取值为 1。
但在BM3.3 中，当且仅当其父项提升矩阵的取值为 1 ，且父项位于坏根的右侧时，该项提升矩阵的取值为 1。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BMS_V4 = R"DEFTXT(
BMS v4.0
(1) 第一行元素的父项为从该元素起，在该元素左边且小于该元素的第一个项。
(2) 元素的祖先项为元素的父项、父项的父项等元素。
(3) 其余行元素的父项为从该元素起，在该元素左边，小于该元素，且其正上方的项是该元素正上方的项的祖先项的第一个项。
(4) 坏根为最后一列中从下往上数的第一个非零项的父项所在的列。
(5) 坏部为坏根和末列之间的部分，包含坏根，但是不包含末列。
(6) 好部为坏根之前的部分，不包含坏根。
(7) 阶差向量为末列和坏根的差值，但是其最后一项始终为零。特别地，如果末列中的第 i + 1 行元素为零，则阶差向量的第 i 行元素也为零。
展开：
(1) 空矩阵所对应的序数为 0，即 (∅) = 0。
(2) 如果 BMS 的最后一列全为零，则它所对应的序数为删去最后一列之后余下部分所对应的序数 +1。
(3) 否则保留好部的所有内容不动，将坏部不断复制加在好部后面，每复制一次就将坏部加
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_DEN1 = R"DEFTXT(
DEN1（Defective embedding notation）。
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
MN Notations
包含家族：
- e0MN / strong e0MN / ωMN / PCF's ωMN / TωMN：ε0 之上构造的 MN 基础系列；
- 1MN / 2MN / 3MN（nMN 系列）：以 n 为幂基的 MN 系列；
- Sω2MN / SAω2MN / Sω^ωMN：S 系列 MN 型记号；
- S1DBMS / S2DBMS / SωDBMS v1/v2/v3 / Sω+1DBMS / Sω2DBMS / Sω^2DBMS：
  S 系列 DBMS 型记号；
- UP1MN / UP2MN v1A / v1B / v1B+：UP 系列 MN 型记号；
- UP1DBMS / UP2DBMS v1 / v1B+ / v1C / v2：UP 系列 DBMS 型记号；
- Aω2MN2 / Aω2MN3 / wAω2MN2 / wAω2MN3：Aw2MN 系列。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_MRSS121 = R"DEFTXT(
MrSS1.2.1（山脉结构序列）
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

PPS4治好了无穷降链的问题，但是其地府段的特色减弱，并且极限只有ζ0
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_SPPS4 = R"DEFTXT(
Second PPS4
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_DR = R"DEFTXT(
Degrees of Reflection（TON_DoR）。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_DRC = R"DEFTXT(
Degrees of Reflection (reflection configuration)（TON_DRC）
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_DRP = R"DEFTXT(
Degrees of Reflection with Passthrough（TON_DRP）
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_DRPC = R"DEFTXT(
Degrees of Reflection with Passthrough (reflection configuration)（TON_DRPC）
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_I = R"DEFTXT(
Iteration of n-built from below (no passthrough)（TON_I）
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_IBP = R"DEFTXT(
Iteration of n-built from below（TON_IBP）
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_M = R"DEFTXT(
TON（Taranovsky 序数记号，主家族）
最正宗的TON
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_MC = R"DEFTXT(
TON（reflection configuration）without passthrough
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TON_MPC = R"DEFTXT(
TON with passthrough (reflection configuration)（TON_MPC）
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

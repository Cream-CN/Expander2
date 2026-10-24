// GENERATED FILE —— 请勿手工编辑。由 build.ps1 的 Invoke-GenDefinitions 从 defs/*.txt 生成。
// 重新生成: .\build.ps1 -Mode gen （或菜单 1 / 4 / 5）
//
// 记号定义文本以 UTF-8 原始字符串字面量直接编译进程序，完全绕过 rc.exe，
// 从而避免 rc.exe 对 UTF-8 源文件的编码误读(首尾乱码)与 \n 不转义的问题。
#pragma once
#include <string_view>
#include "Resource.h"

namespace expander_defs {

constexpr std::string_view kDef_IDR_DEF_1BM_BHM = R"DEFTXT(
1BM-BHM（BMS(1 行) + BHM）

nBM-BHM 系列（n=1）：在 BHM 的基础上，当末列顶行（最大非零行号）小于 n 时
退化为普通 BMS(BM4) 展开，否则按 BHM 展开。

输入格式：矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0)(1,1)(2,1)。
记号极限输入 Limit；最后一列全为零时为后继型，删去最后一列。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_1MN = R"DEFTXT(
1MN

1MN 是 n-MN 家族（TS 类别 category-n-mn，credit.n_mn）中模板参数 N=1 的实例，逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/SMN/n_MN.ts（TS id "1-MN"）；C++ 实现为模板 NMnNotation<N>，见 Header/notation/n_mn.hpp，展开核心与共享机制见 Header/notation/smnshare.hpp。2MN、3MN 是同一模板 N=2、N=3 的实例，表达式结构、竖向机制与展开流程完全相同，唯一差异在 ∞ 基本列的分隔符参数（见各自定义文件）。

表达式结构：一个表达式 Expr 是若干列 Column 的序列（Mountain）；每列是若干条目 Entry=(value, sep) 的序列；value 是从 1 起的列标引用（决定父位置；0 仅作为展开过程中补出的幽灵条目哨兵），sep 是非负整数分隔符，显示为 sep+1 个逗号。空表达式表示 0；另外用 ∞ 标志表示记号的极限，显示为 "Limit"。解析时逗号个数减 1 得到 sep（零个逗号对应 sep=0），列可带括号也可不带括号，单独的 "0" 表示空列；每个列在显示时都包裹在一对括号中。

竖向机制：一列中每个条目对应一个竖向（Vertical，分隔符序列）：按条目顺序做 vertical_increase 得到，vertical_increase(v, s) 从 v 末尾向前删除所有小于 s 的分隔符再追加 s。条目 (i,j) 的父位置为 (pi, pj)：pi = value−1；当 pi=−1 时 pj=0，否则 pj = find_index_below(V[pi], V[i][j])，即在父列（列标 value）的竖向表中二分查找第一个严格小于该条目竖向的位置。

极限与后继：空表达式表示 0；末列为空的表达式是后继表达式，它的展开（前驱）就是删去末列；其余非空表达式为极限表达式。

∞ 的基本列：Limit[n]（n 为正整数）展开为第一列空列、第二列含 n 个条目 (value=1, sep=N−1) 的表达式。本记号 N=1，分隔符为 0，每个条目显示为一个逗号加列标，即 Limit[1] = ()(,1)，Limit[2] = ()(,1,1)，余类推。解析 "Limit" 得到 ∞ 标志。

展开（参数 m≥1；FS 采用较短展开 shorter=true，FS_alter 采用较长展开 shorter=false）：对极限表达式依次执行 fill_ghost 补幽灵行，随后连续 m 次应用 extend；较短展开在最后删去末列，较长展开再做一次 subtract_1；最后 clear_ghost 删去所有 value=0 的幽灵条目。

fill_ghost（补幽灵行）：先计算各列竖向表；对每个条目求父位置，若父位置恰为父列末尾，比较 vertical_increase(v_parent, sep) 与当前条目竖向：若前者更小，则在父列末尾追加一个幽灵条目 Entry{0, undef_sep}（undef_sep 是大于任意值的哨兵，仅在取不到父列前一项竖向时产生），其竖向为当前竖向删去末元素。

extend（一次反射式扩展）：设 right 为末列列标、top 为末列最右上条目下标；求该条目父位置 p=(Ri, Rj)；用 magma_indices 计算岩浆检查表 MI0；先调用 subtract_1 得到中间山形 m，重算末列竖向，取 MI0 的前 right 列部分继续计算 magma_indices 得到 MI；offset = right − Ri；随后对列 Ri+1 到 right 依次调用 copy_column，把源列复制到列标加 offset 的新位置。

magma_indices（岩浆检查表）：对坏根列 Ri 右侧每一列的每个条目沿父链上溯：若父列标 pi < Ri 则中断；若 pi == Ri，记录 min(pj, Rj)；否则转记父条目对应的检查值（父位置等于父列末尾时退到前一项，若超出结果表则中断）。

subtract_1（取前驱并拼接坏根列）：删去末列末项；若末项分隔符 top_right_sep > 0，令 new_sep = top_right_sep−1，比较 vertical_increase(v_parent, new_sep) 与末列倒数第二项竖向 v_bottom：若前者更大，则在末列追加条目 Entry{Ri+1, new_sep}；随后把坏根列从 Rj 起的全部条目拼接到末列末尾。

copy_column（复制单列）：遍历源列每个条目；若该条目的岩浆检查值在表内，新列标为 value+offset，mi 切换时按末列 mr 中同 mi 的记录做行提升：当 current_mi == Rj，或 mr 中下一项的 mi 也等于 current_mi 时，复制 mr[ref_j] 的分隔符到新条目；检查表之外的条目保持原分隔符，仅当 value ≥ Ri+1 时列标加 offset。

记号极限：在本 C++ 移植中，模板参数 N 仅出现在 ∞ 基本列的分隔符 N−1 上，展开算法本身与 N 无关；记号对应的序数阶位以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_2BM_BHM = R"DEFTXT(
2BM-BHM（BMS(2 行) + BHM）

nBM-BHM 系列（n=2）：在 BHM 的基础上，当末列顶行（最大非零行号）小于 2 时
退化为普通 BMS(BM4) 展开，否则按 BHM 展开。

输入格式：矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0)(1,1)(2,1)。
记号极限输入 Limit；最后一列全为零时为后继型，删去最后一列。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_2MN = R"DEFTXT(
2MN

2MN 是 n-MN 家族（TS 类别 category-n-mn，credit.n_mn）中模板参数 N=2 的实例，逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/SMN/n_MN.ts（TS id "2-MN"）；C++ 实现为模板 NMnNotation<N>，见 Header/notation/n_mn.hpp，共享核心见 Header/notation/smnshare.hpp。表达式结构、竖向机制、极限与后继判定、fill_ghost/extend/subtract_1/copy_column 展开流程均与 1MN 完全相同（同一模板、同一份 smnshare 核心），本文件只列出口径与 N=2 的差异。

表达式结构：Expr 是若干列 Column 的序列；每列是若干条目 Entry=(value, sep) 的序列；value 是从 1 起的列标引用（0 仅为幽灵条目哨兵），sep 是非负整数分隔符，显示为 sep+1 个逗号；空表达式表示 0；∞ 标志显示为 "Limit"。竖向由 vertical_increase 逐条目生成；父位置 pi = value−1，pi=−1 时 pj=0，否则在父列竖向表中二分查找第一个严格小于当前竖向的位置。

极限与后继：末列为空的表达式是后继表达式，展开就是删去末列；其余非空表达式为极限表达式。

∞ 的基本列：Limit[n] 展开为第一列空列、第二列含 n 个条目 (value=1, sep=N−1) 的表达式。本记号 N=2，分隔符为 1，每个条目显示为两个逗号加列标，即 Limit[1] = ()(,,1)，Limit[2] = ()(,,1,,1)，余类推。解析 "Limit" 得到 ∞ 标志。

展开（参数 m≥1；FS 较短展开、FS_alter 较长展开）：先 fill_ghost 补幽灵行，连续 m 次 extend；较短展开删去末列，较长展开再做一次 subtract_1；最后 clear_ghost 删去 value=0 的幽灵条目。extend 中先求末列最右上条目的父位置 (Ri,Rj) 与岩浆检查表，subtract_1 删去末项后按竖向比较决定是否追加 (Ri+1, new_sep) 条目，再把坏根列从 Rj 起的条目拼接到末列，随后把坏根列右侧各列按 offset = right−Ri 复制到新位置；copy_column 中按岩浆检查表做行提升复制分隔符，表外条目仅在 value ≥ Ri+1 时列标加 offset。

记号极限：模板参数 N 在本移植中仅作用于 ∞ 基本列分隔符 N−1；记号对应的序数阶位以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_2PPS4 = R"DEFTXT(
2-pps4

2-PPS4 。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_3BM_BHM = R"DEFTXT(
3BM-BHM（BMS(3 行) + BHM）

nBM-BHM 系列（n=3）：在 BHM 的基础上，当末列顶行（最大非零行号）小于 3 时
退化为普通 BMS(BM4) 展开，否则按 BHM 展开。

输入格式：矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0)(1,1)(2,1)。
记号极限输入 Limit；最后一列全为零时为后继型，删去最后一列。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_3MN = R"DEFTXT(
3MN

3MN 是 n-MN 家族（TS 类别 category-n-mn，credit.n_mn）中模板参数 N=3 的实例，逻辑移植自 notation-explorer（hypcos）的 src/notations/MN/SMN/n_MN.ts（TS id "3-MN"）；C++ 实现为模板 NMnNotation<N>，见 Header/notation/n_mn.hpp，共享核心见 Header/notation/smnshare.hpp。表达式结构、竖向机制、极限与后继判定、fill_ghost/extend/subtract_1/copy_column 展开流程均与 1MN、2MN 完全相同（同一模板、同一份 smnshare 核心），本文件只列出口径与 N=3 的差异。

表达式结构：Expr 是若干列 Column 的序列；每列是若干条目 Entry=(value, sep) 的序列；value 是从 1 起的列标引用（0 仅为幽灵条目哨兵），sep 是非负整数分隔符，显示为 sep+1 个逗号；空表达式表示 0；∞ 标志显示为 "Limit"。竖向由 vertical_increase 逐条目生成；父位置 pi = value−1，pi=−1 时 pj=0，否则在父列竖向表中二分查找第一个严格小于当前竖向的位置。

极限与后继：末列为空的表达式是后继表达式，展开就是删去末列；其余非空表达式为极限表达式。

∞ 的基本列：Limit[n] 展开为第一列空列、第二列含 n 个条目 (value=1, sep=N−1) 的表达式。本记号 N=3，分隔符为 2，每个条目显示为三个逗号加列标，即 Limit[1] = ()(,,,1)，Limit[2] = ()(,,,1,,,1)，余类推。解析 "Limit" 得到 ∞ 标志。

展开（参数 m≥1；FS 较短展开、FS_alter 较长展开）：先 fill_ghost 补幽灵行，连续 m 次 extend；较短展开删去末列，较长展开再做一次 subtract_1；最后 clear_ghost 删去 value=0 的幽灵条目。extend 中先求末列最右上条目的父位置 (Ri,Rj) 与岩浆检查表，subtract_1 删去末项后按竖向比较决定是否追加 (Ri+1, new_sep) 条目，再把坏根列从 Rj 起的条目拼接到末列，随后把坏根列右侧各列按 offset = right−Ri 复制到新位置；copy_column 中按岩浆检查表做行提升复制分隔符，表外条目仅在 value ≥ Ri+1 时列标加 offset。

记号极限：模板参数 N 在本移植中仅作用于 ∞ 基本列分隔符 N−1；记号对应的序数阶位以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_A_OMEGA2_MN_2 = R"DEFTXT(
Aω2MN2

Aω2MN2 是 hypcos_mn 在 notation-explorer 中提出的 MN 型记号（家族 category-hypcos-w2mn，TS 源：src/notations/MN/Aw2MN2.ts），C++ 移植见 Header/notation/aw2mn2.hpp，与弱版 wAω2MN2 及版本3 Aω2MN3、wAω2MN3 共享核心 Header/notation/aw2mn_share.hpp。本记号采用 subtract1 版本2 规则，非弱版。

表达式结构：一个表达式 Expr 是若干列 Column 的序列；每列是若干条目 Entry 的序列；条目由三元组 (value, sep, mark) 组成：value 是从 1 起的列标引用，sep 是分隔符，递归地也是一个 Expr（分隔符本身仍是表达式），mark 是可选的星号标志。竖向 Vertical 是分隔符 Sep 的序列。空表达式表示 0；另外用 ∞ 标志表示记号的极限，显示为 "Limit"。

显示规则：一列用圆括号包裹，条目按顺序先显示分隔符 sep_display，再在 mark 为真时显示 "*"，最后显示 value。分隔符若所有列皆空，显示为与列数等长的逗号串；若为 ω 分隔符（结构 [[], [[1, [[]]]] 再接若干空列），显示为 ";" 加上剩余空列数目的逗号；否则按普通表达式显示。

合法性：展开前对表达式做值域校验，每个条目的 value 必须满足 1 ≤ value ≤ 列数（即父列标 value−1 落在现有列范围内），否则返回解析失败；括号、逗号、分号不匹配的输入同样解析失败。

极限与后继：非空且末列非空的表达式是极限表达式；末列为空的表达式是后继表达式，它的展开（前驱）就是删去末列；空表达式展开为空。

∞ 的基本列：Limit[n]（n 为正整数）展开为 [[], [[1, sep_n]]]，其中 sep_n = [[], [[1, [[]]]]] 再接 n 个空列；显示为 ()(; 后接 n 个逗号再接 1。解析 "Limit" 得到 ∞ 标志；对 ∞ 调用展开时直接返回 infinity_fs(term) 作为第 term 个基本列。

展开（参数 m≥1，FS 用较短展开 shorter=true，FS_alter 用较长展开 shorter=false）：两者都先对极限表达式反复调用 extend 共 m 次；较短展开随后删去末列；较长展开则再调用一次 extend（is_small=true，不新增列）。

extend 的步骤：
(1) 计算各列竖向 V0：一列从空竖向起，逐条目应用 vertical_increase——从竖向末尾向前删去所有小于该条目分隔符的竖向，再追加该分隔符。
(2) 取末列最右上条目，经 Parent 得到坏根位置 BRij=(br, bj)：父列标为 value−1；父列内位置 bj 由 find_index_below_row 在父列竖向表中二分查找第一个小于该条目竖向的位置得到。
(3) top_separators 与 top_verticals：坏根列前 bj 个条目的分隔符与竖向，再分别加上末列最右上条目的分隔符与竖向。width = 末列列标 − br。
(4) 岩浆检查 magma_checks_list：对坏根列右侧每一列的每个条目，沿父链上溯直到列标不大于 br；若最终落在坏根列、位置不超过 bj，且该位置上一项的竖向与该条目左侧竖向相等，则记录该坏根列内位置，否则记 −1；坏根本列全部记 −1。
(5) 调用 subtract1（版本2，见下）修改表达式。
(6) get_references：对 subtract1 后的末列竖向（前置一个空竖向）与 top_verticals 做双指针匹配，记录每个 top 竖向在末列中对应的位置引用 refs。
(7) stretch 拉伸阈值与拉伸量：对每个为极限表达式的 top 分隔符，取 S(A0, br, i−1) 作为参照（mark 条目取自身分隔符，否则沿本列向上回溯到上一个 mark 条目，越界为空分隔符）；阈值 = 若参照 ≥ 该 top 分隔符则为单空列，否则参照后接一个空列；在 refs 给出的末列引用区间内自上而下寻找第一个落在阈值与该 top 分隔符之间的分隔符 M，拉伸量 stretch_value = M 的列数 − 阈值列数 + 1；非极限的 top 分隔符拉伸量为 0。
(8) stretched 拉伸判定（本记号非弱版）：若拉伸量为 0，或条目分隔符 ≥ top 分隔符，或条目分隔符 < 阈值，保持原分隔符；否则在条目分隔符后追加 stretch_value 个空列。
(9) 复制：dx 从 0 到 width（is_small 时仅 dx=0），目标列标 = x + width；dx>0 时新建一个空列；dx=0 时跳过坏根列前 bj 个条目。岩浆检查命中（magma ≠ −1）的条目按引用区间 [ref(BR_index−1)+1, ref(BR_index)] 展开为多个条目：区间末项使用 stretched 处理后的自身分隔符，其余项复制目标列对应位置条目的分隔符与 mark；未命中条目复制单个条目，value 加 width（value ≤ br 时不加），分隔符经 stretched 处理，mark 保持不变。

subtract1 版本2（V2）：先删去末列最右上条目；alpha = 坏根列第 bj−1 项的竖向（bj=0 时为空竖向）；working_vertical = 末列最右上条目前一项的竖向（无前一项时为空竖向）。
copy_entries_loop：若 alpha ≤ working_vertical 直接返回；否则在坏根列竖向表中定位 working_vertical 下方位置，把该位置之后、bj 之前的条目依次复制到末列，value 取被删条目的 value，working_vertical 随复制逐条 vertical_increase。
随后按被删条目分隔符分三种情况：
(a) 分隔符为极限表达式：BR_separator = S(A, br, bj−1)；J = 若 BR_separator ≥ 被删分隔符则为单空列，否则 BR_separator 后接一个空列；若 vertical_increase(alpha, J) > working_vertical，则当 alpha > working_vertical 时先执行 copy_entries_loop，再向末列追加带 mark 的条目 (value, J, mark=true)。
(b) 分隔符不是极限且不是 one（one 指单空列表达式 [[]]）：truncated = 被删分隔符删去末列；先执行 copy_entries_loop；若 alpha == working_vertical，向末列追加不带 mark 的条目 (value, truncated, mark=false)。
(c) 分隔符为 one：本分支不追加条目。

记号极限：∞ 的第 n 个基本列 ()(; 后接 n 个逗号再接 1，随 n 增长的是 ω 分隔符后追加的空列数目；本记号达到的极限序数以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_A_OMEGA2_MN_3 = R"DEFTXT(
Aω2MN3

Aω2MN3 是 hypcos_mn 在 notation-explorer 中提出的 Aw2MN 版本3 记号（家族 category-hypcos-w2mn，TS 源：src/notations/MN/Aw2MN3.ts），C++ 移植见 Header/notation/aw2mn3.hpp，与版本2 Aω2MN2、wAω2MN2 及弱版 wAω2MN3 共享核心 Header/notation/aw2mn_share.hpp。表达式结构、合法性、极限与后继判定、∞ 基本列、extend 的总体流程与拉伸、复制机制均与 Aω2MN2 相同，区别仅在 subtract1 采用版本3 规则。

表达式结构（与 Aω2MN2 相同）：Expr = Column[]，Column = Entry[]，Entry=(value, sep, mark)：value 是从 1 起的列标引用，sep 是递归地也是 Expr 的分隔符，mark 是星号标志；竖向 Vertical = Sep[]。空表达式表示 0；∞ 用标志表示，显示为 "Limit"。展开前校验每个条目 1 ≤ value ≤ 列数，括号、逗号、分号不匹配则解析失败。

极限与后继：非空且末列非空为极限表达式；末列为空为后继表达式，删去末列即得前驱；空表达式展开为空。

∞ 的基本列：Limit[n] = [[], [[1, sep_n]]]，其中 sep_n = [[], [[1, [[]]]]] 再接 n 个空列，显示为 ()(; 后接 n 个逗号再接 1；对 ∞ 展开时直接返回 infinity_fs(term)。

展开（参数 m≥1，FS 较短展开 shorter=true，FS_alter 较长展开 shorter=false）：先对极限表达式反复调用 extend 共 m 次，较短展开随后删去末列，较长展开再调用一次 extend（is_small=true，不新增列）。extend 中依次完成：计算各列竖向 V0；由末列最右上条目经 Parent 求得坏根位置 BRij=(br, bj) 与宽度 width；记录坏根列前 bj 项及末列最右上项的分隔符 top_separators 与竖向 top_verticals；做岩浆检查；调用 subtract1 版本3；用 get_references 匹配末列竖向与 top_verticals；计算拉伸阈值与拉伸量；按 stretched 规则处理条目分隔符（本记号非弱版，无高位豁免）；最后按岩浆检查结果把坏根列右侧 width 列复制到列标加 width 的新位置（is_small 时只处理 dx=0）。

subtract1 版本3（V3）：先删去末列最右上条目；alpha = 坏根列第 bj−1 项的竖向（bj=0 时为空竖向）；working_vertical = 末列最右上条目前一项的竖向（无前一项时为空竖向）。copy_entries_loop：若 alpha ≤ working_vertical 直接返回；否则在坏根列竖向表中定位 working_vertical 下方位置，把该位置之后、bj 之前的条目依次复制到末列，value 取被删条目的 value，working_vertical 随复制逐条 vertical_increase。

版本3 与版本2 的实现差异（以代码为准）：
(1) one 提前返回：若被删条目分隔符是 one（单空列表达式 [[]]），subtract1 直接返回删去末项后的表达式，不执行复制循环，也不追加任何条目；版本2 在此情况下同样不追加条目，但不提前返回。
(2) 复制循环位置：版本3 在分支判断之前先执行一次 copy_entries_loop；版本2 的复制循环位于各分支内部——极限分支按 alpha > working_vertical 的条件执行，非极限分支先执行复制循环再判定是否追加。
(3) 非极限分支的追加判据：truncated = 被删分隔符删去末列；版本3 在 vertical_increase(alpha, truncated) > working_vertical 时追加不带 mark 的条目 (value, truncated, mark=false)；版本2 的判据是 alpha == working_vertical。
(4) 极限分支的 J 构造：BR_separator = S(A, br, bj−1)；J = 若 BR_separator ≥ 被删分隔符则为单空列，否则 BR_separator 后接一个空列。版本3 随后进入 while 循环：只要 vertical_increase(alpha, J) 与 vertical_increase(working_vertical, J) 不相等，就向 J 追加一个空列，直到两侧竖向相等，最后向末列追加带 mark 的条目 (value, J, mark=true)；版本2 不循环加长 J，只在 vertical_increase(alpha, J) > working_vertical 时追加，且复制循环按 alpha > working_vertical 的条件嵌套在该极限分支内。

记号极限：∞ 基本列与 Aω2MN2 相同（共用 infinity_fs）；版本3 的规则差异集中在 one 提前返回、复制循环前置、非极限分支的追加判据以及极限分支 J 的循环加长。本记号达到的极限序数以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BHM = R"DEFTXT(
BHM（Bashicu Hyper Matrix，Bashicu 高阶矩阵系统）

输入格式：与 BMS 相同的矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0)(1,1)(2,1)。
记号极限输入 Limit，其第 n 个基本列为 (0)(1,1,...,1)。
最后一列全为零时为后继型，删去最后一列。

展开规则：取末列顶行 top，沿 top 行父链确定特殊根 special_root，
再沿 top-1 行父链找出所有 top 行父项等于 special_root 的候选根；
对每个候选根计算阶差向量 V 与上升阈值向量 A，比较各自“一次展开加末列”
的阈值，取第一个严格更小的候选根；随后从该根起按 w·V（仅在前 A 行）
复制坏部指定项数。FS 少复制一段，FS_alter 的末段只复制根列一列。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_BLM = R"DEFTXT(
BLM（Bashicu Large Matrix，Bashicu 大矩阵系统）

输入格式：与 BMS 相同的矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0)(1,1)(2,2)(3,2)。
记号极限输入 Limit，其第 n 个基本列为 (0)(1,1,...,1)。
最后一列全为零时为后继型，删去最后一列。该记号只有 FS，没有 FS_alter。

展开规则：BLM 的经典大函数式实现：自末列向上确定每一行的有效高度，
逐候选根计算阶差向量 c，并维护一张根编号矩阵 b2（含其历史的共享传播语义）；
确定最终坏根 d7 后，保留好部，将坏部连同根编号标记一起复制 a*d7 列，
仅在对应根编号落在激活范围内的位置施加阶差；最后在顶行全零时裁去该尾行。
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

constexpr std::string_view kDef_IDR_DEF_BSM = R"DEFTXT(
BSM（Bashicu Sudden Matrix，Bashicu 突变矩阵系统）

输入格式：与 BMS 相同的矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0)(1,1)(2,1)。
记号极限输入 Limit，其第 n 个基本列为 (0)(1,1,...,1)（第 2 列含 n+1 个 1）。
最后一列全为零时为后继型，删去最后一列。

展开规则：取末列最后一个非零项所在行 LNZ，沿 LNZ 行父链确定特殊根与候选根，
比较各候选根的“一次展开加末列”阈值，取第一个严格小于参照阈值的候选根；
随后将坏部按根的阶差向量复制指定项数，阶差在 LNZ 行额外减 1，
并且仅对相对根“上升”的行施加增量。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_CMS = R"DEFTXT(
CMS（Crane Matrix System，Crane 矩阵系统）

输入格式：与 BMS 相同的矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0)(1,1)(2,1)。
记号极限输入 Limit，其第 n 个基本列为 (0)(1,1,...,1)。
最后一列全为零时为后继型，删去最后一列。该记号只有 FS，没有 FS_alter。

展开规则：取末列最后一个非零行 LNZ 与坏根 BR（末列在 LNZ 行的父项），
计算阶差并仅在相对坏根“上升”的位置施加，先将坏部复制指定项数；
再在坏部中寻找列 col 使层级指标 L(BR,col) 超过 LNZ：若不存在则结束；
否则追加提升后的末列，并对复制区间逐列、逐行沿父链重算其值（层级判定后改为基准值加上跳数 u）。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_DEN1 = R"DEFTXT(
DEN1（Defective embedding notation）。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_DSM = R"DEFTXT(
DSM（Diagonal Sudden Matrix，对角突变矩阵系统）

输入格式：与 BMS 相同的矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0)(1,1)(2,1)。
记号极限输入 Limit，其第 n 个基本列为三角矩阵 (0)(1)(2,1)(3,2,1)...。
最后一列全为零时为后继型，删去最后一列。

展开规则：自行计算严格父项矩阵（高层父项沿上一行父链寻找），沿前驱
（predecessor）链枚举目标项的候选祖先与候选项；以“一次强展开”为参照，
取首个产生更小结果段的候选项，按其确定坏项。随后在父项矩阵上进行
小展开（目标行等于坏行）或完整展开（复制上部、上升项斜向平移、
基底项在新增行间竖连），最后由父项矩阵还原数值。FS 比 FS_alter 再多删一列。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_E0MN = R"DEFTXT(
e0MN

e0 Mountain Notation（e0MN）是 test_alpha0 提出的基于"山"（Mountain）结构的记号，由 hypcos 的 notation-explorer 收录（TS 源：src/notations/MN/e0MN.ts），C++ 移植见 Header/notation/e0mn.hpp 与共享核心 Header/notation/mnordshare.hpp。它属于 ta0-MN 家族，与 strong e0MN 共享同一套核心实现。

表达式结构：一个表达式 Expr 是若干列 Column 的序列；每列是若干条目 Entry=(a,x) 的序列；a 是左腿所指向的列标（从 1 起），x 是行高序数。行高序数 Ord 是若干项 Term=(exp,coeff) 的序列，exp 递归地也是 Ord（即指数），coeff 是正整数系数，空 Ord 表示 0。空表达式表示 0；另外用 ∞ 标志表示记号的极限，显示为 "Limit"。

合法性：第 i 列（列标从 1 起）中每个条目的左腿列标 a 必须满足 0 < a < i，行高 x 必须为正；同一列中的条目自上而下必须满足 a 严格递减、x 严格递增。

极限与后继：非空且末列非空的表达式是极限表达式；末列为空的表达式是后继表达式，它的展开（前驱）就是删去末列。

∞ 的基本列：Limit[n]（n 为正整数）展开为 ()(1:ω^^n)，即第一列为空列、第二列为单个条目 (a=1, x=ω^^n) 的表达式；ω^^n 表示 n 层 ω 的指数塔。解析 "Limit" 得到 ∞ 标志，解析 "Limit[n]" 直接得到上述基本列。

展开（FS 与 FS_alter，参数 m≥1）：FS 采用较短展开（shorter=true），FS_alter 采用较长展开（shorter=false）。二者都先对极限表达式反复应用反射 rfl 共 m 次；较短展开随后删去末列（即取前驱）；较长展开则再应用一次 rfl，并把列数截回前一步的列数。

反射 rfl 对极限表达式 e 的步骤（设列数 l，末列末项 (a_n, x_n)，d = l − a_n，d 必须为正）：
(1) 下降 down：取第 a_n 列（源列）中所有行高小于 x_n 的条目，拼接到删去末项后的末列末尾；若省略条件不成立且 ordFS(x_n, l−1) 非零，则在末列末尾追加条目 (a_n, ordFS(x_n, l−1))。省略条件为：x_n = 1，或末列不止一项且 ordFS(x_n, l−1) 小于等于倒数第二项的行高。
(2) 逐列处理：对列 a_n+1 到 l 的每一列，左腿列标不小于 cutoff = a_n 的条目的列标加 d；所有条目的行高按 shiftOrdNumbers(cutoff, d) 处理。
shiftOrdNumbers 的移位规则：仅当系数大于 cutoff 且系数不为 1 时系数加 d；指数为 1 的项不递归移位。

行高序数的基本列 ordFS（e0 基本列）：取序数末项；若其指数为 0（即纯数字项），则系数减 1（系数为 1 时直接删去）；否则删去末项（若系数大于 1，先留下一项指数不变、系数减 1 的项），再对指数递归求 ordFS 得到 next_exp：若原指数是后继序数，则追加项 (next_exp, m)；否则追加项 (next_exp, 1)。

记号极限：∞ 的第 n 个基本列 ()(1:ω^^n) 的塔高随 n 增长，sup_n ω^^n = ε0，故记号极限为 ε0（与记号名 e0MN 一致）。
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

constexpr std::string_view kDef_IDR_DEF_GMS_GBMS_FULL = R"DEFTXT(
GMS（General Matrix System，通用矩阵系统）— GBMS / Full

输入格式：BM 矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0,0,0)(1,1,0)(2,2,1)。
极限 Limit 的基本列按层级逐段增长（Full 满矩阵）。
最后一列全为零时为后继型，基本列退化为删去该末列（继续取更长项无定义）。

展开规则：三者共用同一套多级父项（1-父列向左扫描，更高层父列按后缀字典序选取）。
GBMS 用“祖先即施加阶差”的 GBMS 阶差矩阵 D：坏部中凡在第 row 层以根列为祖先的列，第 row-1 行都加上阶差 d[row-1]。
每次基本列结果都会删除整列全为零的行（行间裁剪）；Weirdly Full 还按行数优先比较。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_GMS_GBMS_OMEGA_P = R"DEFTXT(
GMS（General Matrix System，通用矩阵系统）— GBMS / ω-P

输入格式：BM 矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0,0,0)(1,1,0)(2,2,1)。
极限 Limit 的第 n 个基本列为 (0,0,0)(1,0,1)(2,1,1)...，列 (n, ⌊n/2⌋, ⌈n/2⌉)。
最后一列全为零时为后继型，基本列退化为删去该末列（继续取更长项无定义）。

展开规则：三者共用同一套多级父项（1-父列向左扫描，更高层父列按后缀字典序选取）。
GBMS 用“祖先即施加阶差”的 GBMS 阶差矩阵 D：坏部中凡在第 row 层以根列为祖先的列，第 row-1 行都加上阶差 d[row-1]。
保留全部行（零填充），不做整行删除。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_GMS_GBMS_PQSS = R"DEFTXT(
GMS（General Matrix System，通用矩阵系统）— GBMS / pQSS

输入格式：BM 矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0,0,0)(1,1,0)(2,2,1)。
极限 Limit 的第 n 个基本列在四行上构造，列 (n, ⌊n/2⌋, ⌊(n-1)/2⌋, 1)。
最后一列全为零时为后继型，基本列退化为删去该末列（继续取更长项无定义）。

展开规则：三者共用同一套多级父项（1-父列向左扫描，更高层父列按后缀字典序选取）。
GBMS 用“祖先即施加阶差”的 GBMS 阶差矩阵 D：坏部中凡在第 row 层以根列为祖先的列，第 row-1 行都加上阶差 d[row-1]。
保留全部行（零填充），不做整行删除。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_GMS_GBMS_QSS = R"DEFTXT(
GMS（General Matrix System，通用矩阵系统）— GBMS / QSS

输入格式：BM 矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0,0,0)(1,1,0)(2,2,1)。
极限 Limit 的第 n 个基本列在四行上构造，列 (n, ⌊(n+1)/3⌋, ⌊n/3⌋, ⌈n/3⌉)。
最后一列全为零时为后继型，基本列退化为删去该末列（继续取更长项无定义）。

展开规则：三者共用同一套多级父项（1-父列向左扫描，更高层父列按后缀字典序选取）。
GBMS 用“祖先即施加阶差”的 GBMS 阶差矩阵 D：坏部中凡在第 row 层以根列为祖先的列，第 row-1 行都加上阶差 d[row-1]。
保留全部行（零填充），不做整行删除。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_GMS_GBMS_WEIRD = R"DEFTXT(
GMS（General Matrix System，通用矩阵系统）— GBMS / Weirdly Full

输入格式：BM 矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0,0,0)(1,1,0)(2,2,1)。
极限 Limit 的基本列为逐维增大的方阵（Weirdly Full）。
最后一列全为零时为后继型，基本列退化为删去该末列（继续取更长项无定义）。

展开规则：三者共用同一套多级父项（1-父列向左扫描，更高层父列按后缀字典序选取）。
GBMS 用“祖先即施加阶差”的 GBMS 阶差矩阵 D：坏部中凡在第 row 层以根列为祖先的列，第 row-1 行都加上阶差 d[row-1]。
每次基本列结果都会删除整列全为零的行（行间裁剪）；Weirdly Full 还按行数优先比较。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_GMS_LPMS2_FULL = R"DEFTXT(
GMS（General Matrix System，通用矩阵系统）— LPMS2 / Full

输入格式：BM 矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0,0,0)(1,1,0)(2,2,1)。
极限 Limit 的基本列按层级逐段增长（Full 满矩阵）。
最后一列全为零时为后继型，基本列退化为删去该末列（继续取更长项无定义）。

展开规则：三者共用同一套多级父项（1-父列向左扫描，更高层父列按后缀字典序选取）。
LPMS2 在 UPMS 的 v 标记基础上，用一次性基本列扫描确定 s 标记，构造修正列 X，再以 fs1+X 递归一次得到下一项。
每次基本列结果都会删除整列全为零的行（行间裁剪）；Weirdly Full 还按行数优先比较。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_GMS_LPMS2_OMEGA_P = R"DEFTXT(
GMS（General Matrix System，通用矩阵系统）— LPMS2 / ω-P

输入格式：BM 矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0,0,0)(1,1,0)(2,2,1)。
极限 Limit 的第 n 个基本列为 (0,0,0)(1,0,1)(2,1,1)...，列 (n, ⌊n/2⌋, ⌈n/2⌉)。
最后一列全为零时为后继型，基本列退化为删去该末列（继续取更长项无定义）。

展开规则：三者共用同一套多级父项（1-父列向左扫描，更高层父列按后缀字典序选取）。
LPMS2 在 UPMS 的 v 标记基础上，用一次性基本列扫描确定 s 标记，构造修正列 X，再以 fs1+X 递归一次得到下一项。
保留全部行（零填充），不做整行删除。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_GMS_LPMS2_PQSS = R"DEFTXT(
GMS（General Matrix System，通用矩阵系统）— LPMS2 / pQSS

输入格式：BM 矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0,0,0)(1,1,0)(2,2,1)。
极限 Limit 的第 n 个基本列在四行上构造，列 (n, ⌊n/2⌋, ⌊(n-1)/2⌋, 1)。
最后一列全为零时为后继型，基本列退化为删去该末列（继续取更长项无定义）。

展开规则：三者共用同一套多级父项（1-父列向左扫描，更高层父列按后缀字典序选取）。
LPMS2 在 UPMS 的 v 标记基础上，用一次性基本列扫描确定 s 标记，构造修正列 X，再以 fs1+X 递归一次得到下一项。
保留全部行（零填充），不做整行删除。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_GMS_LPMS2_QSS = R"DEFTXT(
GMS（General Matrix System，通用矩阵系统）— LPMS2 / QSS

输入格式：BM 矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0,0,0)(1,1,0)(2,2,1)。
极限 Limit 的第 n 个基本列在四行上构造，列 (n, ⌊(n+1)/3⌋, ⌊n/3⌋, ⌈n/3⌉)。
最后一列全为零时为后继型，基本列退化为删去该末列（继续取更长项无定义）。

展开规则：三者共用同一套多级父项（1-父列向左扫描，更高层父列按后缀字典序选取）。
LPMS2 在 UPMS 的 v 标记基础上，用一次性基本列扫描确定 s 标记，构造修正列 X，再以 fs1+X 递归一次得到下一项。
保留全部行（零填充），不做整行删除。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_GMS_LPMS2_WEIRD = R"DEFTXT(
GMS（General Matrix System，通用矩阵系统）— LPMS2 / Weirdly Full

输入格式：BM 矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0,0,0)(1,1,0)(2,2,1)。
极限 Limit 的基本列为逐维增大的方阵（Weirdly Full）。
最后一列全为零时为后继型，基本列退化为删去该末列（继续取更长项无定义）。

展开规则：三者共用同一套多级父项（1-父列向左扫描，更高层父列按后缀字典序选取）。
LPMS2 在 UPMS 的 v 标记基础上，用一次性基本列扫描确定 s 标记，构造修正列 X，再以 fs1+X 递归一次得到下一项。
每次基本列结果都会删除整列全为零的行（行间裁剪）；Weirdly Full 还按行数优先比较。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_GMS_UPMS_FULL = R"DEFTXT(
GMS（General Matrix System，通用矩阵系统）— UPMS / Full

输入格式：BM 矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0,0,0)(1,1,0)(2,2,1)。
极限 Limit 的基本列按层级逐段增长（Full 满矩阵）。
最后一列全为零时为后继型，基本列退化为删去该末列（继续取更长项无定义）。

展开规则：三者共用同一套多级父项（1-父列向左扫描，更高层父列按后缀字典序选取）。
UPMS 先构造各层 Y_i 与标记向量 v_i，只有被 v 标记（经修正后缀与 Y_i 比较后仍不小于）的位置才施加阶差，从而抑制 GBMS 中部分过大的增量。
每次基本列结果都会删除整列全为零的行（行间裁剪）；Weirdly Full 还按行数优先比较。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_GMS_UPMS_OMEGA_P = R"DEFTXT(
GMS（General Matrix System，通用矩阵系统）— UPMS / ω-P

输入格式：BM 矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0,0,0)(1,1,0)(2,2,1)。
极限 Limit 的第 n 个基本列为 (0,0,0)(1,0,1)(2,1,1)...，列 (n, ⌊n/2⌋, ⌈n/2⌉)。
最后一列全为零时为后继型，基本列退化为删去该末列（继续取更长项无定义）。

展开规则：三者共用同一套多级父项（1-父列向左扫描，更高层父列按后缀字典序选取）。
UPMS 先构造各层 Y_i 与标记向量 v_i，只有被 v 标记（经修正后缀与 Y_i 比较后仍不小于）的位置才施加阶差，从而抑制 GBMS 中部分过大的增量。
保留全部行（零填充），不做整行删除。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_GMS_UPMS_PQSS = R"DEFTXT(
GMS（General Matrix System，通用矩阵系统）— UPMS / pQSS

输入格式：BM 矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0,0,0)(1,1,0)(2,2,1)。
极限 Limit 的第 n 个基本列在四行上构造，列 (n, ⌊n/2⌋, ⌊(n-1)/2⌋, 1)。
最后一列全为零时为后继型，基本列退化为删去该末列（继续取更长项无定义）。

展开规则：三者共用同一套多级父项（1-父列向左扫描，更高层父列按后缀字典序选取）。
UPMS 先构造各层 Y_i 与标记向量 v_i，只有被 v 标记（经修正后缀与 Y_i 比较后仍不小于）的位置才施加阶差，从而抑制 GBMS 中部分过大的增量。
保留全部行（零填充），不做整行删除。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_GMS_UPMS_QSS = R"DEFTXT(
GMS（General Matrix System，通用矩阵系统）— UPMS / QSS

输入格式：BM 矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0,0,0)(1,1,0)(2,2,1)。
极限 Limit 的第 n 个基本列在四行上构造，列 (n, ⌊(n+1)/3⌋, ⌊n/3⌋, ⌈n/3⌉)。
最后一列全为零时为后继型，基本列退化为删去该末列（继续取更长项无定义）。

展开规则：三者共用同一套多级父项（1-父列向左扫描，更高层父列按后缀字典序选取）。
UPMS 先构造各层 Y_i 与标记向量 v_i，只有被 v 标记（经修正后缀与 Y_i 比较后仍不小于）的位置才施加阶差，从而抑制 GBMS 中部分过大的增量。
保留全部行（零填充），不做整行删除。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_GMS_UPMS_WEIRD = R"DEFTXT(
GMS（General Matrix System，通用矩阵系统）— UPMS / Weirdly Full

输入格式：BM 矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0,0,0)(1,1,0)(2,2,1)。
极限 Limit 的基本列为逐维增大的方阵（Weirdly Full）。
最后一列全为零时为后继型，基本列退化为删去该末列（继续取更长项无定义）。

展开规则：三者共用同一套多级父项（1-父列向左扫描，更高层父列按后缀字典序选取）。
UPMS 先构造各层 Y_i 与标记向量 v_i，只有被 v 标记（经修正后缀与 Y_i 比较后仍不小于）的位置才施加阶差，从而抑制 GBMS 中部分过大的增量。
每次基本列结果都会删除整列全为零的行（行间裁剪）；Weirdly Full 还按行数优先比较。
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

constexpr std::string_view kDef_IDR_DEF_LPMS = R"DEFTXT(
LPMS（Lifting Projection Matrix System，提升投影矩阵系统）

输入格式：合法矩阵的每一列自上而下不增，首列全为 0，例如 (0)(1,1)(2,1,1)。
极限 Limit 的第 n 个基本列为 (0)(1,1,...,1)（n 个 1，n=0 时为 (0)(0)）。
末列全为 0 时为后继型，删去末列；列不满足合法性时基本列同样退化为删末列。

展开规则：计算末列的多级父列根与阶差；不足三行或末列第三行为 0 时直接按
祖先是否含根逐份复制坏部（投影）；否则执行“提升”——用阶段列（缺失列视为
全 -1）划分若干情形，必要时递归调用一次投影并插入后继常数列，把表达式
提升到可投影形态后再投影。特殊地 (0^n)(1^n)（n>1）先收缩一行。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_LPTSS = R"DEFTXT(
LPTSS（Lifting Projection Triple Sequence System，提升投影三列序列系统）

有限矩阵的展开、合法性与提升规则与 LPMS 完全相同；唯一区别在极限 Limit 的
基本列：第 n 个基本列为 (0)(1,1,1)(2,2,2)...(n,n,n)（n+1 列三元组）。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_MRSS121 = R"DEFTXT(
MrSS1.2.1（山脉结构序列）
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_OMEGA_MN = R"DEFTXT(
ωMN

ωMN（TS 源：src/notations/MN/Omega_MN.ts，家族 category-mn，原作者 hypcos_mn；C++ 实现见 Header/notation/omega_mn.hpp，竖向机制复用 Header/notation/smnshare.hpp）。ωMN 与 n-MN 系列共用 SMN 家族的表达式骨架，但展开算法不同：ωMN 使用"坏根复制"（get_references + magma_checks_list），而非 n-MN 的 fill_ghost/extend。

表达式结构：表达式（Mountain）是若干列 Column 的序列；每列是若干条目 Entry=(value, sep) 的序列；value 是从 1 起的列标引用（决定父位置），sep 是分隔符（非负整数，显示为 sep+1 个逗号，如条目 (1,2) 显示为 ",,1"）。空表达式表示 0；另外用 ∞ 标志表示极限，显示为 "Limit"。

竖向机制：一列中每个条目对应一个竖向（Vertical，分隔符序列）：按条目顺序做 verticalIncrease 得到，verticalIncrease(v, s) 从 v 末尾向前删除所有小于 s 的分隔符再追加 s。条目 (i,j) 的父位置为 (value−1, findIndexBelow(V[value−1], V[i][j]))，即在父列（列标 value）的竖向表中二分查找第一个小于该条目竖向的项位置。

极限与后继：非空且末列非空的表达式是极限表达式；末列为空的表达式是后继表达式，它的展开就是删去末列。

∞ 的基本列：Limit[n] 展开为 [[], [[1, n]]]：第一列为空列，第二列为单个条目 (value=1, sep=n)。

展开（参数 index=m，FS 用较短展开、FS_alter 用较长展开）：对极限表达式，以末列最右上条目为起点：
(1) 计算最右上条目的父位置 BRij（坏根列 BRij.i 与坏根列内位置 BRij.j），宽度 width = 末列列标 − BRij.i。
(2) 处理末项分隔符：若 sep = 0，删去末项；否则令新分隔符为 sep−1，比较 verticalIncrease(坏根列中 BRij.j 上一项处的竖向, sep−1) 与末列倒数第二项处的竖向：若前者小于等于后者则删去末项，否则把末项分隔符改为 sep−1。
(3) 把坏根列从 BRij.j 起的所有条目拼接到末列末尾。
(4) 计算岩浆检查表 magma_checks_list：对坏根列右侧每一列的每个条目，沿父链上溯直到列标不大于 BRij.i；若最终落在坏根列（列标恰为 BRij.i）、位置不超过 BRij.j，且该位置上一项的竖向与该条目左侧竖向相同，则记录该坏根列内位置，否则记 −1。
(5) 对 n = 1..m 逐次把坏根列右侧 width 列复制到列标加 width·n 的新位置：对每个条目，若岩浆检查值为 sm ≥ 0，则按引用区间 [ref(sm−1)+1, ref(sm)] 展开为多个条目（值均为 value + width·n）；否则复制单个条目（值为 value + width·n，若 value 不大于坏根列标则保持 value）。
(6) 较短展开（FS）在复制完成后删去末列。
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

constexpr std::string_view kDef_IDR_DEF_PCF_OMEGA_MN = R"DEFTXT(
PCF's ωMN

PCF's ωMN（TS 源：src/notations/MN/pcf-omega_MN.ts，家族 category-mn，credit pcf-omega-mn；C++ 实现见 Header/notation/pcf_omega_mn.hpp）属于 MN 家族，展开算法是"坏根复制"的一个变体，核心运算包括 base_vertical、stretch、normalize_col 与 magma_verticals。

表达式结构：一个表达式 Expr 是若干列 Column 的序列；每列是若干条目 Entry=(p,v) 的序列；p 是 0 起列标（显示时加 1），v 是竖向 Vertical，即 Cantor 系数数组（小端存储：v[j] 为 ω^j 的系数，空数组表示 0）。空表达式表示 0；另外用 ∞ 标志表示记号极限，显示为 "Limit"。竖向按反字典序比较：长度优先，长度相同则从末元素向前逐项比较；条目先比较 p 再比较竖向；列按条目字典序比较；表达式按列字典序比较。

显示规则：条目显示为 p+1 后接冒号与竖向的 Cantor 形式（ω^j·c，j=0 时为纯数字 c，系数 1 省略乘号，高次项之间用加号连接）；一列用括号括起，条目间以逗号分隔；表达式为各列直接拼接。

合法性与标准形：解析时条目列标显示值必须不小于 1；竖向由 ω^j·c 项与纯数字项经加号组合而成。展开过程中每列都要经过 normalize_col 规整：自上而下只保留竖向严格大于此前最大竖向的条目，即规整后同一列内竖向严格递增，每个保留条目都是新的最大值。

极限与后继：非空且末列非空的表达式是极限表达式；末列为空的表达式是后继表达式，后继表达式的展开（前驱）就是删去末列；空表达式展开结果仍为空。

∞ 的基本列：Limit[n]（n 为正整数）展开为 ()(1:ω^n)，即第一列为空列、第二列为单个条目（p=0，竖向 v 的前 n 项为 0、第 n 项为 1，对应 ω^n）；解析 "Limit" 直接得到 ∞ 标志，展开时按上述基本列规则给出。

展开（参数 m≥1；FS 采用较短展开 shorter=true，FS_alter 采用较长展开 shorter=false；expand 的前置条件是输入为极限表达式）：
(1) 设末列列标为 right，末列末项为 top_entry=(r, top)，令 s 为 top_sep(top)，即 top 中首个正系数的下标（空竖向为 −1）。
(2) threshold = base_vertical(expr, top, r)：先令 bottom = vertical_decrease(top)，从第 r 列开始循环：在当前列用 find_index_below_row 查找竖向小于 top 的最大位置 j（即列中最大的 j 使第 j−1 条目的竖向小于 top）；把 bottom 与第 j−1 条目的竖向取较大者；若 j 已到列尾则返回 bottom，否则跳到第 j 条目的 p 所指列继续循环。
(3) target_candidate：当 s=0 时取 threshold；否则取 vertical_increase(threshold, s−1)。
(4) 构造新末列：把原末列去掉末项，追加条目 (r, target_candidate)，再拼上第 r 列的全部条目，然后用 normalize_col 规整；prev_vert 为原末列倒数第二项的竖向（无则为空），target 取 target_candidate 与 prev_vert 的较大者。
(5) magma = magma_verticals(expr, r)：对第 r 列右侧的每一列，自上而下扫描条目：若条目 p<r 则中断；若 p=r 则记录该条目竖向并中断；否则取 p 列的岩浆竖向 mv_p 与当前条竖向比较，当 mv_p 不大于当前竖向时把该列岩浆竖向取为 mv_p 与已有值的较大者后中断，否则记录当前条竖向。
(6) offset = right − r；对 w=1,2,…,m，把第 r+1 列到第 right 列逐列复制为新列。copy_column 中：新列标 p1 = e.p ≥ r 时为 e.p + offset·w，否则保持 e.p；若 e.v ≤ magma，竖向取 stretch(e.v, threshold, target, w)；若 e.p < r，竖向原样保留；其余情形竖向取 e.v 与 stretched_magma 的较大者，其中 stretched_magma = stretch(magma, threshold, target, w)；复制结果再经 normalize_col 规整。
(7) stretch(v, threshold, target, w)：若 v ≤ threshold 则原样返回；否则重复 w 次 result = vertical_add(target, vertical_sub(result, threshold))。
(8) 较短展开（FS）在复制完成后删去末列；较长展开（FS_alter）保留末列。

竖向辅助运算：vertical_increase(v,s)：当 size(v) ≤ s 时返回 s 个 0 后接 1；否则把 v[s] 加 1，并把下标 0 到 s−1 全部清零。vertical_decrease(v)：取 top_sep 处系数减 1；若该系数为 1 且位于数组末位，则返回空数组。vertical_add(v,a) 按 Cantor 系数合并：低位项取自 a，顶部系数相加，更高位取自 v；vertical_sub(v,base) 从高位向下逐项相减，遇不足返回空数组，两项系数全部相等也返回空数组。

记号极限：∞ 第 n 个基本列的竖向为 ω^n，其上确界为 ω^ω；记号最终极限以原始 TS 定义为准。
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

constexpr std::string_view kDef_IDR_DEF_S_OMEGA_DBMS = R"DEFTXT(
SωDBMS v1

SωDBMS v1 是 SDBMS 家族中第一个变长行高版本，TS 源为 src/notations/MN/SDBMS/S_omega_DBMS-v1.ts（TS id s-omega-dbms），C++ 移植见 Header/notation/s_omega_dbms_v1.hpp，展开骨架与共享核心 Header/notation/sdbms_core.hpp 一致。与 v2、v3 的区别：行高是纯整数序列，位置信息由序列前缀与长度编码，没有独立的 (值, 位置) 二元条目。

表达式结构：Expr 是若干 Column 的序列；Column 是若干 Entry 的序列；Entry=(value, height)，value 为 0 基列标引用（显示 value+1）；Height 为 0 基整数序列，内部存储顺序与显示顺序相反，整条显示为 value+1^(反序后各分量 +1)，例如内部 [0,2] 显示为 (3,1)。空表达式表示 0；空列显示为 "(0)"；省略上标即空行高，列尾 value=−1 的条目被删去。另外用 ∞ 标志表示记号极限，显示为 "Limit"。

合法性：每个条目的 value 与行高首分量 h[0] 必须落在 [0, 列数) 内，行高不允许为空；越界输入返回解析失败提示。

极限与后继：非空且末列非空为极限表达式；末列为空为后继表达式，前驱即删去末列。

∞ 的基本列：Limit[n] 展开为 (0)(1^(1,…,1))，括号内共有 n+1 个 1：第一列为空列，第二列为单个条目 value=0、行高为 n+1 个 0 的序列。

行高比较 compare_height（v1 特有）：先比长度，短者小；等长时从末项起向前逐分量比较（反字典序），数值大者大。height(col) 为末项行高，空列返回未定义（视为空序列）。合并 merge_column：col1 非空时追加 col2 中行高严格大于 height(col1) 的条目；col1 为空时整列拼接（空序列在比较中长度最小，等价于全保留）。复制规则：value ≥ r 时加 offset；行高每个分量 ≥ r 时加 offset，否则不变。

compute_new_height(h, expr, r)（v1 特有）：设 rh=h[0]，取第 rh 列 col_rh；s = top_separator(h)，即 h[0] 开头连续等于 h[0] 的前缀长度；hj = col_rh 中行高严格小于 h 的条目个数（二分查找）。
(1) 若 hj 等于 col_rh 长度（整列都小于 h）：取 col_rh 末项行高 h_candidate；若其长度小于 s，则把 new_h 末项弹出；否则把 new_h 的第 s−1 分量改为 h_candidate 的第 s−1 分量；col_rh 为空时，若 s>0 则弹出末项。
(2) 否则：基项 base_ref 为 col_rh[hj−1] 的行高（hj=0 时无）；若基项与 h 等长且从第 s 分量起逐项相同，则 new_h[s−1] 取 base_ref[s−1]，否则取 col_rh[hj].value。
最后把 new_h 的前 s−1 个分量全部置为 r，返回 new_h。

展开骨架：right 为末列下标，末列末项为 (r, h)；以 rh=h[0] 所指列作为 col_rh 求出 new_h；末列删去末项后并入中项 (r, new_h) 与第 r 列；复制循环 w=1..m，i=r+1..right，偏移量 (right−r)·w；FS 删去末列，FS_alter 保留末列。

记号极限：Limit[n] 的行高长度随 n 增长；实现只给出基本列与展开，其上确界序数以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_S_OMEGA_DBMS_V2 = R"DEFTXT(
SωDBMS v2

SωDBMS v2 是 SDBMS 家族中带位置 p 与值 v 分量、且允许 ω 的行高版本，TS 源为 src/notations/MN/SDBMS/S_omega_DBMS-v2.ts（TS id s-omega-dbms-v2），C++ 移植见 Header/notation/s_omega_dbms_v2.hpp。与 v1 的区别：行高每个位置是 (p, v) 二元条目，p 或 v 均可为未定义（显示 ω）；与 v3 的区别：v2 保留 undefined-last 语义，p、v 都可缺省，行高比较为较长者大。

表达式结构：Expr 是若干 Column 的序列；Column 是若干 Entry 的序列；Entry=(value, height)；Height 为 HEntry 序列，每个 HEntry=(p, v)，分量未定义时表示 ω。整条显示为 value+1^((v 项),…)：v 定义显示 v+1，未定义显示 ω；p 定义时附 "@p"，省略 "@p" 只允许在序列首位（表示 p 未定义）。空表达式表示 0；空列显示为 "(0)"；省略上标即空行高，列尾 value=−1 的条目被删去。另外用 ∞ 标志表示记号极限，显示为 "Limit"。

合法性：value 落在 [0, 列数)；行高非空；每个定义的 v 落在 [0, 列数) 内；未定义的 v 必须 p 有定义（否则对应 TS 的 Illegal state，按解析失败处理）；越界输入返回解析失败提示。

极限与后继：非空且末列非空为极限表达式；末列为空为后继表达式，前驱即删去末列。

∞ 的基本列：Limit[n] 展开为 (0)(1^(1, ω@n))：第一列为空列；第二列为单个条目 value=0，行高两项：首项 p 未定义、v=0（显示 1），次项 p=n、v 未定义（显示 ω@n）。

分量比较：未定义（ω）视为最大；HEntry 先比 p 再比 v。行高比较 compare_height 为字典序：前缀相同后较长者大。height(col) 为末项行高，空列返回未定义（空序列）；合并规则与 v1 相同（col1 空时整列拼接）。复制规则：value ≥ r 加 offset；p 保持原样，定义的 v ≥ r 时加 offset，未定义 v 保持 ω。

top_separator(h)（v2 特有）：取末项；若其 v 未定义且 p 有定义，返回 p+1；否则返回 0。height_fill(h, p, vDef, v)：从末项起，只要末项 p 有定义且 p ≤ 当前 p 就弹出；再比较末项的 (vDef, v)，若与新值不同则压入 HEntry(p 有定义, p, vDef, v)。

compute_new_height(h, expr, r)（v2 特有）：new_h 从 h 复制，分三种情况处理末项：
(1) 末项 v 未定义：先 height_fill(new_h, 末项.p, 定义, r)；若末项 p > 0，再 height_fill(new_h, p−1, 未定义, 0)。（p 也未定义属非法状态。）
(2) 末项 p 未定义：取第末项.v 列的末项行高替换 new_h；该列为空时清空 new_h。
(3) p、v 都有定义：取第 v 列 col_rh，找第一个 compare_height(h, 该行高度) ≤ 0 的下标 hj；若整列都大于 h，new_h 取 col_rh 末项行高（空列清空）；否则先 height_fill(new_h, 末项.p, 定义, col_rh[hj].value)，p>0 时再 height_fill(new_h, p−1, 未定义, 0)；若 hj>0 且 col_rh[hj−1] 行高严格大于 new_h，则改用该行高。

展开骨架与其余版本一致：right 为末列下标，末列末项为 (r, h)；求出 new_h 后，末列删去末项并入中项 (r, new_h) 与第 r 列；复制循环 w=1..m，i=r+1..right，偏移量 (right−r)·w；FS 删去末列，FS_alter 保留末列。

记号极限：Limit[n]=(0)(1^(1, ω@n))；实现只给出基本列与展开，其上确界序数以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_S_OMEGA_DBMS_V3 = R"DEFTXT(
SωDBMS v3

SωDBMS v3 是 SDBMS 家族的纯整数 (v, p) 行高版本，TS 源为 src/notations/MN/SDBMS/S_omega_DBMS-v3.ts（TS id s-omega-dbms-v3），C++ 移植见 Header/notation/s_omega_dbms_v3.hpp。与 v2 的区别：p、v 均为定义整数、没有 ω 分量，行高比较为较短者大；与 v1 的区别：行高不是整数序列，而是 (值, 位置) 二元条目序列。

表达式结构：Expr 是若干 Column 的序列；Column 是若干 Entry 的序列；Entry=(value, height)；Height 为 HEntry 序列，HEntry=(v, p)，v 为 0 基列标、p 为位置数；整条显示为 value+1^((v+1@p),…)，p 原样显示不减一。空表达式表示 0；空列显示为 "(0)"；省略上标即空行高，列尾 value=−1 的条目被删去。另外用 ∞ 标志表示记号极限，显示为 "Limit"。

合法性：value 落在 [0, 列数)；行高非空；每个 v 落在 [0, 列数) 内；越界输入返回解析失败提示。

极限与后继：非空且末列非空为极限表达式；末列为空为后继表达式，前驱即删去末列。

∞ 的基本列：Limit[n] 展开为 (0)(1^(1@n))：第一列为空列；第二列为单个条目 value=0，行高单项 (v=0, p=n)。

比较规则 compare_hentry：先比 v 再比 p，数值大者大。行高比较 compare_height：前缀相同后较短者大。height(col) 为末项行高，空列返回未定义；合并 merge_column：col1 非空时追加 col2 中行高严格大于 height(col1) 的条目，col1 为空时整列拼接。复制规则：value ≥ r 加 offset；v ≥ r 加 offset，p 保持不变。

height_fill_dec(h, p, v)（v3 特有）：从末项起，末项 p ≤ 给定 p 就弹出；若弹出后末项 v 与给定 v 相同再弹出；最后压入 (v, p)。

compute_new_height(h, expr, r)（v3 特有）：末项为 (v, p)。
(1) 若 p > 0：直接返回 height_fill_dec(h, p−1, r)。
(2) 若行高只有一项：取第 v 列末项行高返回；该列为空则返回未定义（不插入中项）。
(3) 否则：p_bound 取倒数第二项的 p；取第 v 列 col_rh，找第一个 compare_height(h, col_rh[k].height) ≤ 0 的下标 hj；若整列都大于 h，返回 col_rh 末项行高（空列未定义）；否则 new_h = height_fill_dec(h, p_bound−1, col_rh[hj].value)；若 hj>0 且 col_rh[hj−1] 行高严格大于 new_h，则改用该行高返回。

展开骨架：right 为末列下标，末列末项为 (r, h)；new_h 求出后并入中项 (r, new_h) 与第 r 列；复制循环 w=1..m，i=r+1..right，偏移量 (right−r)·w；FS 删去末列，FS_alter 保留末列。

记号极限：Limit[n]=(0)(1^(1@n))；实现只给出基本列与展开，其上确界序数以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_S_OMEGA_P1_DBMS = R"DEFTXT(
Sω+1DBMS

Sω+1DBMS 是 SDBMS 家族中位置可取 ω 的版本，TS 源为 src/notations/MN/SDBMS/S_omega_p1_DBMS.ts（TS id s-omega+1-dbms），C++ 移植见 Header/notation/s_omega_p1_dbms.hpp。与 v3 的区别：位置 p 除整数外可取 ω；求新高由三个互相递归的函数 descend_height_to_value、find_maximal_entry_below、find_lower_height_pos 驱动。

表达式结构：Expr 是若干 Column 的序列；Column 是若干 Entry 的序列；Entry=(value, height)；Height 为 HEntry 序列，HEntry=(v, p)，v 为 0 基列标，p 为数字位置（0 基）或 ω。整条显示为 value+1^((v+1@p),…)：p 为 ω 时显示 @ω，否则显示 @(p+1)。空表达式表示 0；空列显示为 "(0)"；省略上标即空行高，列尾 value=−1 的条目被删去。另外用 ∞ 标志表示记号极限，显示为 "Limit"。

合法性：value 落在 [0, 列数)；行高非空；每个 v 落在 [0, 列数)；非 ω 的 p 也落在 [0, 列数)；越界输入返回解析失败提示。

极限与后继：非空且末列非空为极限表达式；末列为空为后继表达式，前驱即删去末列。

∞ 的基本列：Limit[n] 展开为 (0)(1^(1@ω))(2^(2@ω))…(n^(n@ω))：第一列为空列；其后第 i 列（i=0..n−1）为单个条目 value=i、行高单项 (v=i, p=ω)。

比较规则：compare_hpos 中 ω 最大，同为数字时数值大者大；compare_hentry 先比 v 再比 p；行高比较为字典序、前缀相同后较短者大。height(col) 为末项行高，空列未定义；合并规则同 v3 家族（col1 空时整列拼接）。复制规则：value ≥ r 加 offset；v ≥ r 加 offset；ω 位置保持不变，数字 p ≥ r 时加 offset。

height_fill_dec(h, p, v)：末项 v ≥ 给定 v 就弹出，然后压入 (v, p)。

compute_new_height(h, expr, r)：末项为 (v, p)。先调 find_lower_height_pos(expr, h, p, r) 求新位置 new_p；若找到，返回 height_fill_dec(h, new_p, r)；否则调 find_maximal_entry_below(expr, v, h, r) 取条目，找不到返回未定义（不插入中项），找到则返回该条目的行高。

三个递归函数（Sω+1 特有机制）：
(1) descend_height_to_value(expr, h, v)：找第一个 v 大于给定 v 的项位置 j；找不到原样返回 h。j=0 时取首项 v 所指列的末项行高，递归下降到给定 v；否则取倒数第二项位置 p_bound，调 find_lower_height_pos 求新位置后 height_fill_dec 到给定 v。
(2) find_maximal_entry_below(expr, i, h, r)：若 r ≠ i，先以 r 为索引递归找到条目，再以其行高、其 value 递归到 i；r=i 时在第 i 列找第一个行高严格大于 h 的下标 j；j 越界则返回列末项（空列未定义）；否则用 descend_height_to_value 把 h 下降到 col[j].value，若结果行高比上一项更大（j=0 视为更大）就返回 col[j] 连同下降后的行高，否则返回上一项。
(3) find_lower_height_pos(expr, h, p, r)：p 为 ω 时直接返回位置 r；数字 p 时调 find_maximal_entry_below 取条目 (r1, h1)；取 h1 末项 (v1, p1)，p1 为 ω 时失败返回未定义；v1=p1.num 时返回位置 v1；否则以位置 v1 递归到 h1 与 r1。

展开骨架：right 为末列下标，末列末项为 (r, h)；new_h 求出后并入中项 (r, new_h) 与第 r 列；复制循环 w=1..m，i=r+1..right，偏移量 (right−r)·w；FS 删去末列，FS_alter 保留末列。

记号极限：Limit[n] 的每列位置均为 ω；实现只给出基本列与展开，其上确界序数以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_S_OMEGA_POW_OMEGA_MN = R"DEFTXT(
Sω^ωMN

Sω^ωMN（TS id "S-omega^omega-MN"，逻辑移植自 notation-explorer 的 src/notations/MN/SMN/S_omega_pow_omega_MN.ts，家族 category-smile-mn，credit.n_mn；C++ 实现见 Header/notation/s_omega_pow_omega_mn.hpp）与 Sω2MN、SAω2MN 同属 SMN 系列，山形骨架同构；本记号的分隔符可以是任意维度的整数向量，显示与解析在二维分号逗号语法之外另支持方括号数组语法，拉伸判定也按维度分层处理。

表达式结构：Expr 是若干列 Column 的序列；每列是若干条目 Entry=(value, sep) 的序列；value 是从 1 起的列标引用，sep 是整数向量（解析时去掉尾部 0 归一）；竖向 Vertical 是 sep 的序列。空表达式表示 0；∞ 标志显示为 "Limit"。显示时 sep 长度不超过 2：先显示 s[1] 个分号再显示 s[0] 个逗号；长度大于 2 时显示为方括号内倒序排列、逗号分隔的整数序列。解析支持同样的两种语法，方括号内给出的序列在内部反转存放；解析接受带括号列与不带括号列，展开前校验每个条目列标须落在 0 到列数范围内，非法输入返回解析失败提示。

比较规则：sep 按反字典序比较——长度不等时短者小，等长时从末元素向前逐位比较；竖向按字典序比较，元素相同则较长者大。竖向由 vertical_increase 逐条目生成；父位置 pi = value−1，pi=−1 时 pj=0，否则在父列竖向表中二分查找第一个严格小于当前竖向的位置。

极限与后继：空表达式表示 0；末列为空的表达式是后继表达式，展开就是删去末列；其余非空表达式为极限表达式。

∞ 的基本列：Limit[n] 展开为空列后接单条目 (value=1, sep)，其中 sep 由 n 个 0 后接 1 构成。n=1 时 sep={0,1} 显示为 ";1"，即 Limit[1] = ()(;1)；n=2 时 sep={0,0,1} 长度为 3，显示为 [1,0,0]，即 Limit[2] = ()([1,0,0]1)，余类推。解析 "Limit"、"limit"、"LIMIT" 均得到 ∞ 标志。

展开（参数 m≥1；FS 较短展开 shorter=true，FS_alter 较长展开 shorter=false）：不做 fill_ghost；连续 m 次应用 extend；较短展开删去末列，较长展开再做一次 subtract_1。extend 总体流程与 Sω2MN 相同：求末列最右上条目的父位置 p=(Ri,Rj)，计算岩浆检查表 MI0 与拉伸数据表 SD0，subtract_1 后重算竖向与检查表，再把坏根列右侧各列按 offset = right−Ri 复制到新位置，拉伸竖向上限取原末列最右上条目的竖向。

magma_indices：坏根列右侧每个条目沿父链上溯，pi < Ri 中断，pi == Ri 记录 min(pj,Rj)，否则转记父条目检查值（父位置等于父列末尾时退到前一项，超出结果表则中断）。

辅助函数 S_default(bound)：令 d = sep_dimension(bound)；若 d == bound 长度−1 且 bound[d] == 1，返回空向量；否则返回 bound 的副本并把第 d 位减 1。S(c, j, bound)：j 越上界递归 S(c, j−1, bound)；j<0 返回 S_default(bound)；若 c[j].sep ≥ bound 也返回 S_default(bound)；否则 previous = S(c, j−1, bound)，返回当前 sep 与 previous 中较小者。sep_dimension(s) 为 s 前导 0 的个数；sep_is_one(s) 当且仅当 s 长度为 1 且首元素为 1；sep_increase(a, d) 把 a 补零到 d+1 位，第 d 位加 1，前 d 位置零。

stretch_data_top：threshold = S(Ri 列, Rj−1, t)，stretch_to = S(末列, top−1, t)，force 初值为假，t 为末列最右上条目的分隔符。(1) 若 sep_is_one(t)，不做任何动作；(2) 若 sep_dimension(t) > 0，令 dim = sep_dimension(t)，当 stretch_to < sep_increase(threshold, dim−1) 时，stretch_to = sep_increase(stretch_to, dim−1) 并置 force=true；(3) 其余首元素非零情形，比较 vertical_increase(v_parent, stretch_to) 与末列倒数第二项竖向 v_bottom，前者更大时置 force=true（v_parent、v_bottom 在对应位置为空时取空竖向）。

stretch_data_list：对 j=0 到 Rj−1，先按末列岩浆检查值推进 ref_j；threshold = S(Ri 列, j−1, m[Ri][j].sep)，stretch_to = S(末列, ref_j−1, m[Ri][j].sep)，force=false；末位数据取 stretch_data_top。

subtract_1：删去末列末项；若 SD_top.force 为真，追加条目 Entry{Ri+1, SD_top.stretch_to}；随后把坏根列从 Rj 起的全部条目拼接到末列末尾。

compute_stretch(sep, data)：若 sep ≤ threshold 保持不变；否则返回 sep_add(stretch_to, sep_sub(sep, threshold))。sep_add(a,b)：b 为空时返回 a；先把 a 补到 b 的长度，末位加上 b 的末位，前 b.size−1 位取 b 的对应位。sep_sub(a,b)：a 更长时返回 a，a 更短时返回空向量；否则找到最后一位不同的位置 d，若 d=0 或 a[d−1] < b[d−1] 返回空向量，否则把结果第 d−1 位减去 b[d−1]。

copy_column：表外条目仅在 value ≥ Ri+1 时列标加 offset；表内条目新列标为 value+offset，mi 切换时按末列同 mi 的记录做行提升并复制对应分隔符；新分隔符在源竖向大于竖向上限时保持原 sep，否则按 SD[current_mi] 调用 compute_stretch。

记号极限：本移植未内建序数阶位结论；记号对应的序数极限以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_S_OMEGA_SQUARE_DBMS = R"DEFTXT(
Sω^2DBMS

Sω^2DBMS 是 SDBMS 家族中位置为二元 (a, b) 段序（b 可取 ω）的版本，TS 源为 src/notations/MN/SDBMS/S_omega^2_DBMS.ts（TS id s-omega^2-dbms），C++ 移植见 Header/notation/s_omega_square_dbms.hpp。与 Sω2DBMS 的区别：位置不是一阶高阶标记，而是 (p1=a, p0=b) 二元段序，b 未定义表示 ω；find_lower_height_pos 按段序的降阶与匹配规则运行。

表达式结构：Expr 是若干 Column 的序列；Column 是若干 Entry 的序列；Entry=(value, height)；Height 为 HEntry 序列，HEntry=(v, p)；HeightPos p=(a, b, bDef)，bDef 为 false 表示 b=ω。整条显示为 value+1^((v+1@p),…)：b 未定义时显示 ω（a≠0 时附 a+1）；b 定义且 a=0 显示 b+1；其余显示 ω（a≠1 时附 a）+（b+1）。空表达式表示 0；空列显示为 "(0)"；省略上标即空行高，列尾 value=−1 的条目被删去。另外用 ∞ 标志表示记号极限，显示为 "Limit"。

合法性：value 落在 [0, 列数)；行高非空；v 落在 [0, 列数)；b 定义时 b 落在 [0, 列数)；越界输入返回解析失败提示。

极限与后继：非空且末列非空为极限表达式；末列为空为后继表达式，前驱即删去末列。

∞ 的基本列：Limit[n] 展开为 (0)(1^(1@ω))(2^(2@ω2))(3^(3@ω3))…(n^(n@ωn))：第一列为空列；其后第 i 列（i=0..n−1）为单个条目 value=i、行高单项 (v=i, p=(a=i, b=未定义))。

位置比较 compare_hpos：先比 a，再比 b，b 未定义（ω）视为最大。compare_hentry 先比 v 再比 p；行高字典序前缀相同后较短者大。height(col) 为末项行高，空列未定义；合并规则同家族。复制规则：value、v ≥ r 时加 offset；a 保持不变，定义的 b ≥ r 时加 offset。

height_fill_dec(h, p, v)：末项 v ≥ 给定 v 弹出后压入 (v, p)。

compute_new_height(h, expr, r)：末项为 (v, p)；先 find_lower_height_pos 求 new_p，找到则 height_fill_dec(h, new_p, r)；否则 find_maximal_entry_below 取条目行高，找不到返回未定义。

递归机制：descend_height_to_value 与 find_maximal_entry_below 与 Sω2DBMS 相同；find_lower_height_pos 的 Sω^2 特有规则：
(1) b 未定义时返回 (a, r, 定义)。
(2) b 已定义时：默认值为 a=0 时未定义，否则 (a−1, b 未定义)；调 find_maximal_entry_below(expr, b, h, r) 取条目；取不到返回默认值。
(3) 取到条目 (r1, h1)，其末项位置 p1 若严格大于给定 p，则返回默认值；若 p1=(a, b) 且 b=v1，返回 p1；否则以 (a, v1, 定义) 递归到 h1 与 r1。

展开骨架：right 为末列下标，末列末项为 (r, h)；new_h 并入中项 (r, new_h) 与第 r 列；复制循环 w=1..m，i=r+1..right，偏移量 (right−r)·w；FS 删去末列，FS_alter 保留末列。

记号极限：Limit[n] 的段序 a 随列递增；实现只给出基本列与展开，其上确界序数以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_S_OMEGA2_DBMS = R"DEFTXT(
Sω2DBMS

Sω2DBMS 是 SDBMS 家族中位置可取 ω+n 高阶位置的版本，TS 源为 src/notations/MN/SDBMS/S_omega2_DBMS.ts（TS id s-omega2-dbms），C++ 移植见 Header/notation/s_omega2_dbms.hpp。与 Sω+1DBMS 的区别：位置除整数外是高阶位置 ω+n（而非单独的 ω），find_lower_height_pos 的高阶分支会逐级退阶；其余递归机制相同。

表达式结构：Expr 是若干 Column 的序列；Column 是若干 Entry 的序列；Entry=(value, height)；Height 为 HEntry 序列，HEntry=(v, p)；HeightPos p 为数字位置（0 基）或高阶位置 (higher, num)。整条显示为 value+1^((v+1@p),…)：高阶位置显示 ω（num=0）或 ω+num，数字位置显示 p+1。空表达式表示 0；空列显示为 "(0)"；省略上标即空行高，列尾 value=−1 的条目被删去。另外用 ∞ 标志表示记号极限，显示为 "Limit"。

合法性：value 落在 [0, 列数)；行高非空；v 落在 [0, 列数)；非高阶的数字 p 落在 [0, 列数)；越界输入返回解析失败提示。

极限与后继：非空且末列非空为极限表达式；末列为空为后继表达式，前驱即删去末列。

∞ 的基本列：Limit[n] 展开为 (0)(1^(1@ω))(2^(2@ω+1))…(n^(n@ω+n−1))：第一列为空列；其后第 i 列（i=0..n−1）为单个条目 value=i、行高单项 (v=i, 高阶位置 num=i)。

比较规则：compare_hpos 中高阶位置大于数字位置；同为高阶比 num，同为数字比 num。compare_hentry 先比 v 再比 p；行高字典序前缀相同后较短者大。height(col) 为末项行高，空列未定义；合并规则同家族。复制规则：value、v ≥ r 时加 offset；高阶位置保持不变，数字 p ≥ r 时加 offset。

height_fill_dec(h, p, v)：末项 v ≥ 给定 v 弹出后压入 (v, p)。

compute_new_height(h, expr, r)：末项为 (v, p)；先 find_lower_height_pos 求 new_p，找到则 height_fill_dec(h, new_p, r)；否则 find_maximal_entry_below 取条目行高，找不到返回未定义。

递归机制：descend_height_to_value 与 find_maximal_entry_below 与 Sω+1DBMS 相同；find_lower_height_pos 的差异在于高阶位置分支：p 为高阶位置时，若 num>0 返回高阶位置 num−1，若 num=0 返回数字位置 r；数字位置分支与 Sω+1 相同（先取最大条目，末项位置为高阶时失败，v1=p1.num 时返回 v1，否则递归）。

展开骨架：right 为末列下标，末列末项为 (r, h)；new_h 并入中项 (r, new_h) 与第 r 列；复制循环 w=1..m，i=r+1..right，偏移量 (right−r)·w；FS 删去末列，FS_alter 保留末列。

记号极限：Limit[n] 高阶位置的 num 随列递增；实现只给出基本列与展开，其上确界序数以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_S_OMEGA2_MN = R"DEFTXT(
Sω2MN

Sω2MN（TS id "S-omega2-MN"，逻辑移植自 notation-explorer 的 src/notations/MN/SMN/S_omega2_MN.ts，家族 category-smile-mn，credit.n_mn；C++ 实现见 Header/notation/s_omega2_mn.hpp）属于 Smile's MN（SMN）系列，与 SAω2MN、Sω^ωMN 共用同一套山形骨架，区别在于分隔符比较、拉伸数据与前驱分支的具体规则。

表达式结构：一个表达式 Expr 是若干列 Column 的序列；每列是若干条目 Entry=(value, sep) 的序列；value 是从 1 起的列标引用（决定父位置），sep 是整数向量（解析时去掉尾部 0 归一）；竖向 Vertical 是 sep 的序列。空表达式表示 0；另外用 ∞ 标志表示极限，显示为 "Limit"。

显示：sep = (c0, c1) 时先显示 c1 个分号再显示 c0 个逗号；条目显示为分隔符加列标；每列包裹在一对括号中。解析接受带括号列与不带括号列，单独的 "0" 或紧跟分隔符、空格、括号的 "0" 表示空列；解析时若发现条目列标超出 0 到列数的范围，直接返回解析失败提示。

比较规则：sep 按反字典序比较——长度不等时短者小，等长时从末元素向前逐位比较；竖向按字典序比较，元素相同则较长者大。

竖向机制：按条目顺序做 vertical_increase 生成竖向表，vertical_increase(v, s) 从 v 末尾向前删除所有小于 s 的分隔符再追加 s。父位置 (pi, pj)：pi = value−1；pi=−1 时 pj=0，否则 pj = find_index_below(V[pi], V[i][j])，即在父列竖向表中二分查找第一个严格小于当前竖向的位置。

极限与后继：空表达式表示 0；末列为空的表达式是后继表达式，展开就是删去末列；其余非空表达式为极限表达式。

∞ 的基本列：Limit[n] 展开为空列后接单条目 (value=1, sep={n, 1})，即 Limit[1] = ()(;,1)，Limit[2] = ()(;,,1)，余类推。解析 "Limit"、"limit"、"LIMIT" 均得到 ∞ 标志。

展开（参数 m≥1；FS 采用较短展开 shorter=true，FS_alter 采用较长展开 shorter=false）：本记号不做 fill_ghost；对极限表达式连续 m 次应用 extend；较短展开在最后删去末列，较长展开再做一次 subtract_1。

extend（一次反射式扩展）：设 right 为末列列标、top 为末列最右上条目下标；计算竖向表 V0；求末列最右上条目的父位置 p=(Ri, Rj)；magma_indices 得岩浆检查表 MI0；stretch_data_list 得拉伸数据表 SD0；先调用 subtract_1(m0, V0, SD0[Rj]) 得到中间山形 m，重算末列竖向，取 MI0 的前 right 列部分继续计算 magma_indices 得 MI；offset = right−Ri；随后对列 Ri+1 到 right 依次调用 copy_column，把源列复制到列标加 offset 的新位置，拉伸竖向上限取 V0[right][top]。

magma_indices（岩浆检查表）：对坏根列 Ri 右侧每一列的每个条目沿父链上溯：父列标 pi < Ri 则中断；pi == Ri 记录 min(pj, Rj)；否则转记父条目对应的检查值（父位置等于父列末尾时退到前一项，超出结果表则中断）。

辅助函数 S(c, j, bound)：j 越上界则递归 S(c, j−1, bound)；j<0 返回空向量；若 c[j] 的 sep ≥ bound 返回空向量；否则 previous = S(c, j−1, bound)，返回当前 sep 与 previous 中较小者。其他辅助：sep_dimension(s) 为 s 前导 0 的个数；sep_is_one(s) 当且仅当 s 长度为 1 且首元素为 1；sep_increase(a, d) 把 a 补零到 d+1 位，第 d 位加 1，前 d 位置零。

stretch_data_top（末项拉伸数据）：取末列最右上条目的分隔符 t。若 sep_dimension(t) > 0（t 含前导零、二阶及以上激活）：threshold = S(Ri 列, Rj−1, t)，stretch_to = S(末列, top−1, t)；当 stretch_to ≤ threshold 时，stretch_to = sep_increase(stretch_to, 0)，并置 force=true。否则：threshold 为 t 的副本、首元素减 1（t 为空则保持空向量），stretch_to = threshold，force=false；若 t 不是 sep_is_one，比较 vertical_increase(v_parent, stretch_to) 与末列倒数第二项竖向 v_bottom，前者更大时置 force=true（v_parent 取父列 Rj−1 处竖向，Rj=0 时为空竖向；v_bottom 在 top=0 时为空竖向）。

stretch_data_list：对 j=0 到 Rj−1，先按末列岩浆检查值 MI[right] 推进 ref_j 使 MI[right][ref_j+1] ≤ j；threshold = S(Ri 列, j−1, m[Ri][j].sep)，stretch_to = S(末列, ref_j−1, m[Ri][j].sep)，force=false；末位数据取 stretch_data_top。

subtract_1：删去末列末项；若 SD_top.force 为真，追加条目 Entry{Ri+1, SD_top.stretch_to}；随后把坏根列从 Rj 起的全部条目拼接到末列末尾。

compute_stretch(sep, data)：若 sep ≤ threshold 保持不变；否则返回 sep_add(stretch_to, sep_sub(sep, threshold))。sep_add(a,b)：b 为空时返回 a；先把 a 补到 b 的长度，末位加上 b 的末位，前 b.size−1 位取 b 的对应位。sep_sub(a,b)：a 更长时返回 a，a 更短时返回空向量；否则找到最后一位不同的位置 d，若 d=0 或 a[d−1] < b[d−1] 返回空向量，否则把结果第 d−1 位减去 b[d−1]。

copy_column：遍历源列条目；表外条目仅在 value ≥ Ri+1 时列标加 offset 后直接复制；表内条目新列标为 value+offset，mi 切换时按末列同 mi 的记录做行提升：current_mi == Rj，或下一项 mi 相同，则复制 mr[ref_j] 的分隔符；新分隔符若源竖向大于竖向上限 stretch_v_max 则保持原 sep，否则对该条目调用 compute_stretch，所用拉伸数据为 SD[current_mi]。

记号极限：本移植未内建序数阶位结论；记号对应的序数极限以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_S1DBMS = R"DEFTXT(
S1DBMS

S1DBMS 是 hypcos 的 notation-explorer 中 SDBMS 家族（category-sdbms）的最简成员，TS 源为 src/notations/MN/SDBMS/S1DBMS.ts，C++ 移植见 Header/notation/s1dbms.hpp；展开骨架与共享核心 Header/notation/sdbms_core.hpp 一致，区别仅在于行高是单个 0 基整数。

表达式结构：表达式 Expr 是若干列 Column 的序列；每列是若干条目 Entry 的序列；条目由行值 value（0 基列标引用，显示为 value+1）与行高 height（0 基整数，显示为 height+1，整条写成 value+1^height+1）组成。空表达式表示 0；空列显示为 "(0)"；省略上标的条目作为哨兵（行高 -1），列尾若出现 value=-1 的条目会被删去。另外用 ∞ 标志表示记号极限，显示为 "Limit"。

合法性：解析后每个条目的 value 必须落在 [0, 列数) 内；末列非空时末项行高也必须落在 [0, 列数) 内；越界输入返回解析失败提示。

极限与后继：非空且末列非空的表达式是极限表达式；末列为空的表达式是后继表达式，其前驱就是删去末列。

∞ 的基本列：Limit[n]（n 为正整数）展开为 ()(1^1)(2^2)…(n^n)：第一列为空列，其后第 i 列为单个条目 (value=i, height=i)，i 从 0 到 n−1。解析 "Limit" 得到 ∞ 标志。

展开骨架（参数 m≥1；FS 用较短展开 shorter=true，FS_alter 用较长展开 shorter=false）：设 right 为末列下标，末列末项为 (r, h)。先按本记号的 compute_new_height 求出新行高 new_h；末列删去末项得到新列前缀，先把条目 (r, new_h) 按合并规则并入，再把第 r 列按合并规则并入；保留第 0 到 right−1 列与新列。随后复制循环 w=1..m：对每个 i=r+1..right，把当前第 i 列按偏移量 (right−r)·w 复制追加；复制规则为 value ≥ r 时 value 加 offset，height ≥ r 时 height 加 offset，否则不变。较短展开（FS）在复制完成后删去末列，较长展开（FS_alter）保留末列。

合并规则 merge_column(col1, col2)：先取 col1，再拼接 col2 中行高严格大于 height(col1) 的条目；height(col) 取末项行高，空列取 −1。条目比较先比 value 再比 height；列与表达式均为字典序（较长者大）。

compute_new_height（S1 特有）：new_h = height(expr[h])，即取当前行高 h 所指列的末项行高；该列为空时取 −1。

记号极限：基本列 ()(1^1)(2^2)…(n^n) 的列数随 n 增长；实现只给出基本列与展开，其上确界序数以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_S2DBMS = R"DEFTXT(
S2DBMS

S2DBMS 是 SDBMS 家族的二元行高成员，TS 源为 src/notations/MN/SDBMS/S2DBMS.ts，C++ 移植见 Header/notation/s2dbms.hpp，展开骨架与共享核心 Header/notation/sdbms_core.hpp 一致，区别仅在行高为二元组及其比较、求新高规则。

表达式结构：Expr 是若干 Column 的序列；Column 是若干 Entry 的序列；Entry=(value, height)，value 为 0 基列标引用（显示 value+1）；Height=(h0, h1) 两个 0 基整数，整条显示为 value+1^(h1, h0+1)，即上标先写 h1、后写 h0+1。空表达式表示 0；空列显示为 "(0)"；省略上标的条目用哨兵行高 (0, −1)，列尾 value=−1 的条目被删去。另外用 ∞ 标志表示记号极限，显示为 "Limit"。

合法性：每个条目的 value 与 h0 都必须落在 [0, 列数) 内；h1 是层级数，不作列标校验；越界输入返回解析失败提示。

极限与后继：非空且末列非空为极限表达式；末列为空为后继表达式，前驱即删去末列。

∞ 的基本列：Limit[n] 展开为 (0)(1^(n,1))：第一列为空列，第二列为单个条目 value=0、Height=(h0=0, h1=n)。

行高比较 compare_height：先比 h1，相同再比 h0（反字典序），数值大者大；height(col) 为末项行高，空列返回未定义。合并 merge_column：col1 非空时追加 col2 中行高严格大于 height(col1) 的条目；col1 为空时整列拼接 col2。复制规则：value ≥ r 时 value 加 offset；h0 ≥ r 时 h0 加 offset，h1 保持不变。

compute_new_height(h, expr, r)（S2 特有）：设 v=h.h0、h1=h.h1，取第 v 列 col_rh；从首项起找第一个满足 compare_height(h, 该行高度) ≤ 0 的下标 hj。
(1) 若整列都大于 h（hj 不存在）：当 col_rh 的 h1（height1）小于 h1 时返回 Height=(r, h1−1)；否则返回 col_rh 末项行高，col_rh 为空时返回未定义（不插入中项）。
(2) 否则：新值 new_value = col_rh[hj].value，新行高 = (new_value, h1)；若 hj>0 且 col_rh[hj−1] 的行高严格大于新行高，则改用该行高；返回之。

展开骨架：right 为末列下标，末列末项为 (r, h)；用 compute_new_height 求出 new_h；末列删去末项后，若 new_h 有定义先并入中项 (r, new_h)，再并入第 r 列；复制循环 w=1..m，i=r+1..right，偏移量 (right−r)·w，按上述复制规则复制；FS（shorter=true）复制后删去末列，FS_alter（shorter=false）保留末列。

记号极限：Limit[n]=(0)(1^(n,1))，实现只给出基本列与展开，其上确界序数以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_SA_OMEGA2_MN = R"DEFTXT(
SAω2MN

SAω2MN（TS id "SA-omega2-MN"，逻辑移植自 notation-explorer 的 src/notations/MN/SMN/SA_omega2_MN.ts，家族 category-smile-mn，credit.n_mn；C++ 实现见 Header/notation/sa_omega2_mn.hpp）与 Sω2MN 同属 SMN 系列、山形骨架同构，主要区别在于条目带星号标记、S 函数与拉伸数据按标记和首元素归零判定，且前驱 subtract_1 分三条分支处理末项。

表达式结构：Expr 是若干列 Column 的序列；每列是若干条目 Entry=(value, sep, mark) 的序列；value 是从 1 起的列标引用，sep 是整数向量（解析时去掉尾部 0 归一），mark 是布尔标记；竖向 Vertical 是 sep 的序列。空表达式表示 0；∞ 标志显示为 "Limit"。显示时 mark=true 的条目在列标前加 "*"；sep=(c0,c1) 先显示 c1 个分号再显示 c0 个逗号；每列包裹在一对括号中。解析只接受连续带括号的列，分隔符后可选一个 "*" 再写列标；展开前同样校验每个条目列标须落在 0 到列数范围内，非法输入返回解析失败提示。条目比较依次比较列标、sep（反字典序）、mark。

比较规则：sep 按反字典序比较——长度不等时短者小，等长时从末元素向前逐位比较；竖向按字典序比较，元素相同则较长者大。竖向由 vertical_increase 逐条目生成；父位置 pi = value−1，pi=−1 时 pj=0，否则在父列竖向表中二分查找第一个严格小于当前竖向的位置。

极限与后继：空表达式表示 0；末列为空的表达式是后继表达式，展开就是删去末列；其余非空表达式为极限表达式。

∞ 的基本列：Limit[n] 展开为空列后接单条目 (value=1, sep={n, 1}, mark=false)，显示与 Sω2MN 相同：Limit[1] = ()(;,1)，Limit[2] = ()(;,,1)，余类推。解析 "Limit"、"limit"、"LIMIT" 均得到 ∞ 标志。

展开（参数 m≥1；FS 较短展开 shorter=true，FS_alter 较长展开 shorter=false）：不做 fill_ghost；连续 m 次应用 extend；较短展开删去末列，较长展开再做一次 subtract_1。extend 的总体流程与 Sω2MN 相同：先求末列最右上条目的父位置 p=(Ri,Rj)，计算岩浆检查表 MI0 与拉伸数据表 SD0，subtract_1 后重算竖向与检查表，再把坏根列右侧各列按 offset = right−Ri 复制到新位置。

magma_indices 与 Sω2MN 相同：坏根列右侧每个条目沿父链上溯，pi < Ri 中断，pi == Ri 记录 min(pj,Rj)，否则转记父条目检查值。

本记号的 S(c, j)（不带 bound）：j 越上界递归 j−1；j<0 返回空向量；若条目的 sep 长度大于 1 且 sep[1] ≠ 0 返回空向量；若该条目 mark=true，直接返回其 sep；否则递归 S(c, j−1)。

stretch_data_top：取末列最右上条目的分隔符 t；若 t 非空且 t[0] ≠ 0，直接返回无拉伸数据（nullopt）。否则 threshold = S(Ri 列, Rj−1)，stretch_to = S(末列, top−1)；当 stretch_to ≤ threshold 时，stretch_to = sep_increase(stretch_to, 0)，并置 force=true。

stretch_data_list：对 j=0 到 Rj−1，先按末列岩浆检查值推进 ref_j；若 m[Ri][j] 的 sep 非空且首元素不为 0，该位置记为无拉伸数据；否则 threshold = S(Ri 列, j−1)，stretch_to = S(末列, ref_j−1)，force=false；末位数据取 stretch_data_top。

subtract_1：删去末列末项，随后按末项分隔符 t 分三条分支：(1) 若 sep_is_one(t)（长度 1 且首元素为 1），不追加新条目；(2) 若 sep_dimension(t) == 0（首元素非零），令 new_sep 为 t 首元素减 1 的副本，比较 vertical_increase(v_parent, new_sep) 与末列倒数第二项竖向 v_bottom，前者更大时追加 Entry{Ri+1, new_sep, mark = t 非空且 t[0]==0}；(3) 其余维度情形，仅当 SD_top 有效且 force 为真时追加 Entry{Ri+1, stretch_to, mark=true}。最后把坏根列从 Rj 起的全部条目（连同原 mark）拼接到末列末尾。

copy_column：表外条目仅在 value ≥ Ri+1 时列标加 offset；表内条目新列标为 value+offset，行提升时复制 mr 对应条目的 sep 与 mark；新条目保留源条目的 mark；分隔符在源竖向大于竖向上限时保持原 sep，否则按 SD[current_mi] 做 compute_stretch。

compute_stretch、sep_add、sep_sub、sep_increase、sep_dimension、sep_is_one 等辅助函数的规则与 Sω2MN 相同。

记号极限：本移植未内建序数阶位结论；记号对应的序数极限以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_SPPS4 = R"DEFTXT(
Second PPS4
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_STRONG_E0MN = R"DEFTXT(
strong e0MN

strong e0MN 是 e0MN 的强版本（TS 源：src/notations/MN/strong_e0MN.ts，家族 category-ta0-mn，原作者 test_alpha0），C++ 实现见 Header/notation/strong_e0mn.hpp，与 e0MN 共享核心 Header/notation/mnordshare.hpp。表达式结构、合法性、极限与后继判定、∞ 的基本列均与 e0MN 相同，区别仅在展开算法中的三处判定（以代码为准）。

与 e0MN 的三处差异：
(1) shiftOrdNumbers 的移位判据：strong 版对指数项也递归移位，且判据为系数 ≥ cutoff（含省略的 1，例如 cutoff=1、d=2 时 ω = ω^1·1 变为 ω^3·3）；而 e0MN 标准版仅当系数 > cutoff 且系数 ≠ 1 时移位。
(2) down 的省略判据：strong 版要求 ordFS(x_n, l−1) 等于末列倒数第二项的行高才省略（相等判定）；标准版是小于等于。
(3) shortExpansion 的省略判据：strong 版同样用相等判定，标准版用小于等于。

其余展开流程（rfl / down / processRflCol / FS / FS_alter 的较短与较长分支）与 e0MN 相同，此处不再重复：FS 先对表达式反复应用 rfl 共 m 次后删去末列；FS_alter 应用 rfl 共 m 次后再应用一次并把列数截回。

记号极限：∞ 的基本列与 e0MN 相同（共用的 infinity_fs），故极限同为 ε0；"strong" 体现在展开省略与移位的判据更激进。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_T_OMEGA_MN = R"DEFTXT(
TωMN

TωMN（TS 源：src/notations/MN/T_omega_MN.ts，家族 category-mn，credit hypcos_mn；C++ 实现见 Header/notation/t_omega_mn.hpp）与 ωMN 同属 MN 家族的坏根复制机制。与 ωMN 的关键差异是：条目的分隔符本身就是一个递归的表达式 Expr；当末项分隔符为极限表达式时，要先递归展开该分隔符，threshold 取使 low、high 两个竖向在该基本列下汇合的最小指标。

表达式结构：表达式（Mountain）是若干列 Column 的序列；每列是若干条目 Entry=(value, sep) 的序列；value 是整数，sep（分隔符）递归地也是一个 Expr。空表达式表示 0；另外用 ∞ 标志表示记号极限，显示为 "Limit"。比较规则：条目先比较 value，相等再比较分隔符的 mountain_compare；列按条目字典序比较，长度长者在后；表达式按列字典序比较，长度长者在后。

显示规则：若分隔符的所有列皆空（纯逗号分隔符），则显示为与列数相同个数的逗号（simple 模式下单个逗号省略）；否则显示分隔符自身的 mountain_display（simple 模式外加方括号）。条目显示为分隔符显示后接 value（simple 模式下 value 为两位数以上时加括号）；非 simple 模式下列整体用括号括起。

竖向机制：一列中每个条目对应一个竖向 TVertical，即分隔符序列。column_verticals 按条目顺序累积：vertical_increase(v, m) 从 v 尾部向前删去所有小于 m 的分隔符，再追加 m。条目 (value, sep) 的父位置：令 target_i = value − 1；若 target_i < 0 记为位置 (−1, 0)；否则父位置为 (target_i, find_index_below_row(V[target_i], V[pos.i][pos.j]))，即在父列的竖向表中二分查找第一个小于该条目竖向的位置。

极限与后继：is_limit 为表达式非空且末列非空；末列为空的表达式是后继表达式，后继展开就是删去末列，空表达式展开结果仍为空。is_one 指分隔符恰为一列且该列为空，即单个纯逗号分隔符。

∞ 的基本列：infinity_fs(n) 递归定义：n ≤ 0 时为单个空列；n > 0 时为 ()(1: infinity_fs(n−1))，即第一列为空列，第二列为单个条目 value=1、分隔符为 infinity_fs(n−1)。解析 "Limit" 直接得到 ∞ 标志，展开时按上述递归基本列给出。

展开（参数 m≥1；FS 采用较短展开 shorter=true，FS_alter 采用较长展开 shorter=false；输入为极限表达式）：设末列列标为 rightmost，末项位置为 topmost，末项分隔符为 sep；先计算各列竖向 V0，求末项的父位置 BRij，宽度 width = rightmost − BRij.i。

情形一：sep 是极限表达式。取 v_low = BRij.j − 1 处坏根列的竖向（越界取空），v_high = topmost − 1 处末列的竖向（越界取空）；threshold(A, shorter, low, high) 从 n=0 起递增，返回使 vertical_increase(low, expand(A, n, shorter)) 不小于 vertical_increase(high, expand(A, n, shorter)) 成立的最小 n。把末项分隔符替换为 expand(sep, threshold + m, shorter)，随后直接返回表达式。

情形二：sep 非极限表达式。
(1) 先构造 top_verticals：取坏根列前 BRij.j 个竖向（BRij.j 越界时取空），再接上末项自身的竖向。
(2) 若 sep 为 is_one，直接删去末项；否则令 new_sep 为 sep 去掉末列：v_parent 取 BRij.j − 1 处坏根列竖向（越界为空），v_bottom 取 topmost − 1 处末列竖向（越界为空）；若 vertical_increase(v_parent, new_sep) 不大于 v_bottom 则删去末项，否则把末项分隔符改为 new_sep。
(3) 把坏根列从 BRij.j 起的条目拼接到末列末尾；BRij.j 越界时不拼接。
(4) 重新计算竖向 V，并计算岩浆检查表 magma_checks_list：对坏根列右侧每一列的每个条目，沿父链上溯直到列标不大于 BRij.i（上溯中条目位置越界时先回退位置）；若最终落在坏根列（wi = BRij.i）、位置 wj ≤ BRij.j，且该位置上一项的竖向 v_work 与该条目左侧竖向 v_src 相同，则该检查值记为 wj，否则记为 −1。
(5) 对 n=1,2,…,m：先用 get_references(A, top_verticals) 把末列竖向表与 top_verticals 双指针匹配，得到每个坏根位置对应的引用区间；再把坏根列右侧 width 列复制到列标加 width·n 的新位置。岩浆检查 sm ≥ 0 的条目按区间 [ref(sm−1)+1, ref(sm)] 展开为多个条目：区间末项沿用原条目分隔符，其余各项取目标坏根列块对应条目的分隔符，value 均为 value + width·n；检查值为 −1 的条目复制为单个条目，value > BRij.i 时取 value + width·n，否则保持原 value，分隔符原样保留。列数不足 target_end = BRij.i + width·n + width + 1 时先扩容。
(6) 较短展开（FS）在复制完成后删去末列；较长展开（FS_alter）保留末列。

记号极限：TωMN 的极限由递归嵌套的分隔符基本列承载，分隔符自身为极限时进入递归展开；记号最终上确界以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_TBM = R"DEFTXT(
TBM（Transfinite Bashicu matrix，超限巴斯丘矩阵，又称 TBMS）

树形矩阵记号。与平面巴斯丘矩阵不同，每个元素写成“数值^高度”，其中高度本身
又是一个（可嵌套的）TBM 矩阵；高度为 1 时省略上标，只写数值。列用圆括号，
列内元素用逗号，例如 (0)(1,1^ω)(2)。空列写成 ()，ω 也可用 w 代替，极限写 Limit。

基本列：Limit 的第 n 个基本列为
  n=0：()
  n>0：()(1^<上一项>)
即 ()、()(1)、()(1^ω)、()(1^()(1^ω))、……

展开：末列为空是后继型，删去末列；末元素高度为 1（即上式中的后继）时按树形
巴斯丘规则复制提升——计算各列的竖直序列、父列、阶差与提升阈值，从根列起按
term 复制并加上阶差；末元素高度为更高极限时，先对该高度递归取第 term 个基本
序列，按空列切成若干段，替换末元素的高度。
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

constexpr std::string_view kDef_IDR_DEF_UP1DBMS = R"DEFTXT(
UP1DBMS

UP1DBMS 是 notation-explorer（hypcos）中 category-upmn-test 家族的 UP 型矩阵记号（TS 源：src/notations/MN/UPMN/UP1DBMS.ts），C++ 实现见 Header/notation/up1dbms.hpp。它在 UP1MN 的列标引用之外为每个条目附加一个整数行高，行高同时作为父列索引参与坏根判定。

表达式结构：一个表达式 Expr 是若干列 Column 的序列；每列是若干条目 Entry=(value, height) 的序列；value 是从 0 起的列标引用，height 是从 0 起的行高，显示为 (value+1)^(height+1)；空列显示为 "(0)"。解析时条目先读列标减 1，可选上标 ^ 数字再减 1 作为行高；无上标时行高置为哨兵 −1，列尾 value=−1 的条目在解析时删去。空表达式表示 0；另外用 ∞ 标志表示极限，显示为 "Limit"。

合法性：展开前校验每个 value 落在 [0, 表达式列数) 内；末列非空时末项行高也须落在 [0, 列数) 内，越界按解析失败处理。

极限与后继：非空且末列非空的表达式是极限表达式；末列为空的表达式是后继表达式，它的展开就是删去末列。

∞ 的基本列：Limit[n]（n 为正整数）展开为第一列空列，随后第 i=0..n−1 列各含单个条目 (value=i, height=i)，显示依次为 (1)^(1)、(2)^(2)、……、(n)^(n)。

展开（参数 m≥1；FS 较短 shorter=true，FS_alter 较长 shorter=false）：空表达式原样返回；末列为空时删去末列。否则取末列末项 (r, h)，新行高 new_h 取第 h 列末项的行高（空列记 −1）。构造新末列：末列删去末项，先并入条目 (r, new_h)，再并入第 r 列；merge_column 规则为保留第一列全部条目，再追加第二列中行高严格大于第一列末项行高的条目（空列行高视为 −1）。新表达式由其余各列与新末列组成。随后由 compute_up(expr, r, h) 算出每列 up 标志，对 w=1..m 逐轮把第 r+1 到末列按 copy_column 复制，列标偏移 (right−r)·w；较短展开删去最后一列，较长展开保留全部复制列。

copy_column(col, r, offset, up)：条目 value 大于 r，或等于 r 且 up 为真时 value 加 offset，否则保持；行高按同一 copy_value 规则同步平移。

compute_up(expr, r, h) 返回每列布尔标志：列标小于 r 为 0，第 r 列为 1。对 i=r+1 到末列逐列判定：h_root 为该列自后向前第一个 value 不小于 r 的条目行高，h_root 小于 h 记 0；h_proj 为第一个 value 不小于 r+1 的条目行高，若 h_proj 不小于 h，取 parent_at(col, h)（第一个行高不小于 h 的条目 value，找不到记 −1），标志继承该引用列，引用不到记 0；若 h_proj 不小于 0，再沿 h_proj 行高上溯到父列等于 r 的列 p_proj，p_proj 引用不到或其标志为 0 则本列记 0。随后进入坏根比较：X_start=i，Y_start=末列，沿 h_proj+1 行高上溯到父列等于 r 的列 Y_start，引用不到记 0；Y_start 不大于 X_start 时相等记 1、否则记 0；否则先比较 X_start 列与 Y_start 列中 value 不大于 r 的条目（discard_after 截断），逐项先比 value 再比 height，前缀相同则长者大，不一致即按方向决定；再对 k=1,2,... 比较相对列，每个条目展开为四元组（value 是否不小于起点、相对 value、height 是否不小于起点、相对 height），先比 value 标记、再比相对 value、再比 height 标记、再比相对 height，前缀相同则长者大，首次不一致决定；全部一致记 1。

记号极限：实现未给出该记号所对应序数上界，具体极限以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_UP1MN = R"DEFTXT(
UP1MN

UP1MN 是 notation-explorer（hypcos）中 category-upmn 家族的 UP 型山记号（credit.up1mn，TS 源：src/notations/MN/UPMN/UP1MN.ts），C++ 移植见 Header/notation/up1mn.hpp。它只有单列标引用、不带分隔符层号，显示与解析骨架复用 n-MN 的 smnshare.hpp。

表达式结构：一个表达式 Expr 是若干列 Column 的序列；每列是若干条目值 v 的序列，v 是从 0 起的列标引用（显示时加 1，经 n-MN 骨架显示为不带逗号的单值）。空表达式表示 0；另外用 ∞ 标志表示记号的极限，显示为 "Limit"。

合法性：解析先经 n-MN 文本骨架读入，凡条目分隔符不为 0 的文本解析失败；展开前校验每个条目值 v 落在 [0, 表达式列数) 内，越界即按解析失败处理。

极限与后继：非空且末列非空的表达式是极限表达式；末列为空的表达式是后继表达式，它的展开（前驱）就是删去末列。

∞ 的基本列：Limit[n]（n 为正整数）展开为 [[], [0, 0, ..., 0]]，即第一列为空列，第二列含 n+1 个均引用第 0 列的条目。解析 "Limit" 直接得到 ∞ 标志，展开时套用上述基本列。

展开（参数 m≥1；FS 采用较短展开 shorter=true，FS_alter 采用较长展开 shorter=false）：空表达式原样返回；末列为空时删去末列。否则取末列末项的值 r，top 为末列项数减 1，先由 compute_up(expr, r, top) 算出每列的 up 标志。随后构造新表达式：保留除末列外的各列，新末列由末列删去末项后，追加第 r 列自第 top 项起的所有条目；再对 w=1..m 逐轮把第 r+1 列到末列的各列按 copy_column 复制到列标偏移 (right−r)·w 的新位置。较短展开在复制完成后删去最后一列，较长展开保留全部复制列。

compute_up(expr, r, b) 返回每列的布尔标志：第 r 列左侧（列标小于 r）一律为 0，第 r 列为 1。对 i=r+1 到末列逐列判定：(1) 第 i 列项数不超过 b 时记 0；(2) 若该列第 b 项的值不等于 r，则该列标志继承它所引用列的标志；(3) 在第 i 列中找到第一个值等于 r 的位置 j，找不到记 0；若 j>0，沿第 j−1 项的值上溯到值等于 r 的列 p，p 的标志为 0 则本列记 0；(4) 进入坏根比较：X_start=i，Y_start=末列，沿第 j 项的值上溯直到值等于 r 的列 Y_start；若 Y_start 不大于 X_start，相等记 1、否则记 0；否则先比较 X_start 列与 Y_start 列自 j 项起的后缀（逐项比列标，前缀相同则长者更大），首次不一致即按方向记 1 或 0；若后缀一致，再对 k=1,2,... 逐对比较 X_start+k 列与 Y_start+k 列的相对列：值不小于本列起点的条目标记为相对项，先比标记（标记为真是更大），再比相对值，前缀相同则长者更大，首次不一致决定本列标志，全部一致则记 1。

copy_column(col, r, offset, up)：条目值大于 r 的加 offset；小于 r 的保持不变；等于 r 时仅当 up 为真才加 offset，否则保持原值。

记号极限：实现未给出该记号所对应序数上界，具体极限以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_UP2DBMS_V1 = R"DEFTXT(
UP2DBMS v1

UP2DBMS v1 是 notation-explorer（hypcos）中 category-upmn 家族的 UP 型 DBMS 山记号（credit.upmn，TS 源：src/notations/MN/UPMN/UP2DBMS-v1.ts），C++ 实现见 Header/notation/up2dbms_v1.hpp，共享核心 Header/notation/mnup2share.hpp。表达式结构与 UP2MN v1B 同属 mnup2 骨架，但展开时行高偏移恒为 top−Rj，且有限情形的 up 判定采用恒真简化规则。

表达式结构：一个表达式 Expr 是若干列 Column 的序列；每列是若干条目 Entry=(value, sep) 的序列；value 是从 0 起的列标引用，sep 是分隔符层号（非负整数），显示时列标加 1 交给 n-MN 骨架。空表达式表示 0；另外用 ∞ 标志表示极限，显示为 "Limit"。竖向表与父位置表 P 的生成规则与 mnup2share 相同：vertical_increase 生成竖向，父位置 pi=value、pj 为父列竖向表中第一个小于本条目竖向的位置。

合法性：展开前校验所有 value 落在 [0, 表达式列数) 内，越界按解析失败处理。

极限与后继：非空且末列非空的表达式是极限表达式；末列为空的表达式是后继表达式，它的展开就是删去末列。

∞ 的基本列：Limit[n]（n 为正整数）展开为第一列空列；第 i=1..n 列先含 i 个引用第 i−1 列、sep=0 的条目，末项为引用第 i−1 列、sep=1 的条目。

展开（参数 m≥1；FS 较短 shorter=true，FS_alter 较长 shorter=false）：空表达式原样返回；末列为空时删去末列。否则取末列列标 right 与末项位置 top，计算竖向表 V 与父位置表 P；rp=P[right][top] 记为 (Ri, Rj)。末项 sep=0 为有限情形 is_finite=true，否则为无限情形。有限情形调用 v1 专用的 compute_up_1mn：列标小于 Ri 记 0，第 Ri 列及其右侧全部记 1；无限情形调用 compute_up_2mn（v1c=false 分支）。构造新表达式：保留除末列外各列，新末列由末列删去末项后，拼回第 Ri 列自第 Rj 项起的所有条目（本记号不在新末列补入 (Ri, 0) 条目）。y_offset 恒为 top−Rj。对 w=1..m 逐轮把第 Ri+1 到 right 列按 copy_column 复制，列标偏移 (right−Ri)·w，高度偏移 y_offset·w；较短展开删去最后一列，较长展开保留全部复制列。

compute_up_2mn（v1 与 v1B+ 相同，v1C 的差异另见其定义）：列标小于 Ri 为 0，第 Ri 列为 1。对 i=Ri+1 到末列：列项数不超过 Rj+1 记 0；项数不少于 Rj+3 时继承 P[i][Rj+1].i 列的标志；末项 sep=0 的有限列同样继承 P[i][Rj+1].i 列的标志；其余取 p=P[i][Rj+1].i，p 不等于 Ri 时继承 p 列标志。p 等于 Ri 时进入坏根比较：X_start=i，Y_start=末列，沿父位置上溯直到找到项数恰为 Rj+2 的列 Y_start；Y_start 不大于 X_start 时相等记 1、否则记 0；否则先比较两列自对应位置起的后缀列，再对 k=1,2,... 比较相对列（value 不小于起点的条目标记为相对项、值改为减起点），先比标记再比 value 与 sep，首次不一致决定；全部一致记 1。

copy_column(col, rp, offset, y_offset, up, v1_flag=true)：value 大于 Ri 加 offset，小于 Ri 保持不变；value 等于 Ri 时，只要 up 为真或位置 j 不超过 Rj 即加 offset，否则保持（j≤Rj 判据是 v1 相对 v1B+ 的特有规则）。若 up 为真且 y_offset 大于 0、Rj 未超出结果列范围，在第 Rj 项位置前补入 y_offset 个填充条目（value 取第 Rj 项平移后的值，sep=0），头部 Rj 项与尾部其余条目依次保留。

记号极限：实现未给出该记号所对应序数上界，具体极限以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_UP2DBMS_V1B_PLUS = R"DEFTXT(
UP2DBMS v1B+

UP2DBMS v1B+ 是 notation-explorer（hypcos）中 category-upmn 家族的 UP 型 DBMS 山记号（credit.upmn，TS 源：src/notations/MN/UPMN/UP2DBMS-v1b-plus.ts），C++ 实现见 Header/notation/up2dbms_v1b_plus.hpp，共享核心 Header/notation/mnup2share.hpp。表达式结构、合法性、极限与后继判定、∞ 基本列、新末列构造与 y_offset 规则均与 UP2DBMS v1 相同，仅在有限情形 up 判定与复制值平移两处不同（以代码为准）。

共同部分：Expr 为列序列，条目 (value 0 起列标, sep 层号)，空表达式表示 0，∞ 标志显示 "Limit"；非空且末列非空为极限表达式，末列为空为后继表达式，后继展开即删去末列；value 须落在 [0, 列数) 内。Limit[n] 为第一列空列，第 i=1..n 列先含 i 个引用第 i−1 列、sep=0 的条目，末项为引用第 i−1 列、sep=1 的条目。展开时末项 sep=0 为有限情形，否则为无限情形；新末列由末列删去末项后拼回第 Ri 列自 Rj 项起的尾部，y_offset 恒为 top−Rj，随后对 w=1..m 复制第 Ri+1 到 right 列，FS 较短删末列、FS_alter 较长保留。

与 v1 的两处差异：

(1) 有限情形 up 判定：v1 把第 Ri 列及其右侧全部记为 up=1；v1B+ 改用真实的 up2mn_compute_up_1mn 算法（与 UP2MN v1B 的有限情形同一套坏根判定：列项数分档、父列继承、j>0 前置列检查、Y_start 上溯到父列等于 Ri 的列、后缀列与相对列比较，未决记 1）。无限情形的 compute_up_2mn 与 v1 完全相同：项数 ≤Rj+1 记 0，≥Rj+3 继承 P[i][Rj+1].i 列，有限末项同样继承父列，p≠Ri 继承 p 列，p=Ri 时上溯到项数恰为 Rj+2 的列再做后缀与相对列比较。

(2) copy_column 的值平移：value 等于 Ri 时，v1B+ 仅当 up 为真才加 offset，不再含 v1 的 j≤Rj 位置判据；value 大于 Ri 加 offset、小于 Ri 保持不变的规则，以及 up 为真时在第 Rj 项前补 y_offset 个填充条目（sep=0）的规则，均与 v1 相同。

记号极限：实现未给出该记号所对应序数上界，具体极限以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_UP2DBMS_V1C = R"DEFTXT(
UP2DBMS v1C

UP2DBMS v1C 是 notation-explorer（hypcos）中 category-upmn 家族的 UP 型 DBMS 山记号（credit.upmn，TS 源：src/notations/MN/UPMN/UP2DBMS-v1c.ts），C++ 实现见 Header/notation/up2dbms_v1c.hpp，共享核心 Header/notation/mnup2share.hpp。表达式结构、合法性、极限与后继判定、∞ 基本列、新末列构造、有限情形 up 判定与复制值平移规则均与 UP2DBMS v1B+ 相同，唯一差异在无限情形 compute_up_2mn 的坏根 Y_start 上溯循环（以代码为准）。

共同部分：Expr 为列序列，条目 (value 0 起列标, sep 层号)，空表达式表示 0，∞ 标志显示 "Limit"；非空且末列非空为极限表达式，末列为空为后继表达式，后继展开即删去末列；value 须落在 [0, 列数) 内。Limit[n] 为第一列空列，第 i=1..n 列先含 i 个引用第 i−1 列、sep=0 的条目，末项为引用第 i−1 列、sep=1 的条目。展开时末项 sep=0 为有限情形，否则为无限情形；新末列由末列删去末项后拼回第 Ri 列自 Rj 项起的尾部，y_offset 恒为 top−Rj，随后对 w=1..m 复制第 Ri+1 到 right 列，FS 较短删末列、FS_alter 较长保留。有限情形使用真实的 up2mn_compute_up_1mn 算法；copy_column 中 value 等于 Ri 时仅当 up 为真才加 offset。

与 v1B+ 的唯一差异：compute_up_2mn 中定位坏根列 Y_start 的上溯循环。v1B+ 沿 P[Y_start][Rj+1].i 上溯，终止条件为列项数恰等于 Rj+2；v1C 沿 P[Y_start][Rj].i 上溯，终止条件为列项数恰等于 Rj+2 且该位置父列恰为 Ri，两个条件同时满足才停止。循环之前的分档判定（项数 ≤Rj+1 记 0、≥Rj+3 继承 P[i][Rj+1].i 列、末项 sep=0 继承父列、p≠Ri 继承 p 列）、循环之后的 Y_start 位置比较（相等记 1、否则记 0）、后缀列与相对列比较规则及未决记 1，均与 v1B+ 相同。

记号极限：实现未给出该记号所对应序数上界，具体极限以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_UP2DBMS_V2 = R"DEFTXT(
UP2DBMS v2

UP2DBMS v2 是 notation-explorer（hypcos）中 category-upmn 家族的 UP 型 DBMS 矩阵记号（credit.up2dbms-v2，TS 源：src/notations/MN/UPMN/UP2DBMS-v2.ts），C++ 实现见 Header/notation/up2dbms_v2.hpp。它与 v1 系列不同：条目行高是一对高度 (h0, h1)，显示为高度对形式；新行高计算与列合并沿用 SDBMS 家族规则，坏根 up 判定与带 BMS 升级分支的复制列均为 v2 专属实现。UI 等价视图（dbms、RMN、Y、层化图等）未移植。

表达式结构：一个表达式 Expr 是若干列 Column 的序列；每列是若干条目 Entry=(value, height) 的序列；value 是从 0 起的列标引用；height=(h0, h1) 为高度对，h0 是父列列标（0 起），h1 是层级号，显示为 (value+1)^(h1, h0+1)；空列显示为 "(0)"。解析时条目先读列标减 1，上标 ^ 后接高度对 (h1, h0+1)（也允许列标后直接跟高度对括号）；无上标时置哨兵高度 (0, −1)，列尾 value=−1 的条目在解析时删去。空表达式表示 0；另外用 ∞ 标志表示极限，显示为 "Limit"。

高度比较：先比 h1，h1 相同再比 h0（反字典序）；条目比较先比 value 再比高度；列比较为条目的字典序，前缀相同则长者更大。

合法性：展开前校验每个 value 与每个 h0 都落在 [0, 表达式列数) 内，越界按解析失败处理。

极限与后继：非空且末列非空的表达式是极限表达式；末列为空的表达式是后继表达式，它的展开就是删去末列。

∞ 的基本列：Limit[n]（n 为正整数）展开为第一列空列，第 i=0..n−1 列各含单个条目 (value=i, height=(h0=i, h1=i+1))，显示依次为 (1)^(1,1)、(2)^(2,2)、……、(n)^(n,n)。

展开（参数 m≥1；FS 较短 shorter=true，FS_alter 较长 shorter=false）：空表达式原样返回；末列为空时删去末列。否则取末列末项 (r, h)，由 compute_new_height(h, expr, r) 算出新行高 new_h。构造新末列：末列删去末项，若 new_h 存在先并入条目 (r, new_h)，再并入第 r 列；merge_column 规则为第一列空时直接取第二列，否则保留第一列全部条目，追加第二列中高度严格大于第一列末项高度的条目。新表达式由其余各列与新末列组成。随后由 compute_up(expr, r, h) 算出每列 up 标志，对 w=1..m 逐轮把第 r+1 到末列按 copy_column 复制，列标偏移 (right−r)·w；较短展开删去最后一列，较长展开保留全部复制列。

compute_new_height(h=(v, h1), expr, r)：在第 v 列中找到第一个高度不小于 h 的位置 hj；若不存在：当该列末项 h1 小于 h.h1 时返回高度 (r, h1−1)，否则返回该列末项高度；若存在：new_h 取为 (第 hj 项 value, h1)，若 hj>0 且前一项高度大于 new_h，则把 new_h 抬升为前一项高度。

compute_up(expr, r, h) 返回每列布尔标志：列标小于 r 为 false，第 r 列为 true；若 h.h1=0，第 r 列右侧全部记 true 并直接返回。对 i=r+1 到末列：h_root 为该列自后向前第一个 value 不小于 r 的条目高度，不存在或其 h1 小于 h.h1 记 false；h_proj 为第一个 value 不小于 r+1 的条目高度，阈值高度 threshold=(r+1, min(h_proj.h1, h.h1−1))（h_proj 不存在时 h1 取 0）；若 h_proj 不小于 threshold，取 parent_at(col, threshold)（第一个高度不小于 threshold 的条目 value，找不到记 −1），标志继承该引用列，引用不到记 false；否则取 p_proj=parent_at(col, h_proj)，引用不到或其标志为 false 记 false。随后进入坏根比较：X_start=i，Y_start=末列，沿 threshold 高度上溯到父列等于 r 的列 Y_start，上溯途中引用不到记 false；Y_start 不大于 X_start 时相等记 true、否则记 false；否则先比较 X_start 列与 Y_start 列中 value 不大于 r 的条目（discard_after 截断）的列字典序，不一致即按方向决定；再对 k=1,2,... 比较相对列：每个条目展开为（value 是否不小于起点、相对 value、h0 是否不小于起点、相对 h0、h1），依次比较标记、相对 value、h0 标记、相对 h0、h1，前缀相同则长者大，首次不一致决定；全部一致记 true。

copy_column(col, r, offset, up)：常规条目经 copy_entry 平移——value 大于 r，或等于 r 且 up 为真时 value 加 offset，否则保持；h0 按同一规则平移，h1 不变。特殊 BMS 升级分支：当条目 value 等于 r 且 up 为假时，若该项 h1=0，或 h1 与前一项 h1 相同，则直接平移为 (r+offset, 平移后高度)；否则先补入条目 (r+offset, (r, 前一项 h1，无前项时 h1=0))，再把原条目按 up 为假规则平移后追加。

记号极限：实现未给出该记号所对应序数上界，具体极限以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_UP2MN_V1A = R"DEFTXT(
UP2MN v1A

UP2MN v1A 是 notation-explorer（hypcos）中 category-upmn-test 家族的早期 UP 型山记号（credit.upmn，TS 源：src/notations/MN/UPMN/UP2MN-v1a.ts），C++ 实现见 Header/notation/up2mn_v1a.hpp。表达式类型、显示解析、竖向表、父位置与标准 n-MN 展开全部复用 smnshare.hpp（n-MN 家族），仅在末列含无限条目时走自定义坏根复制分支。

表达式结构：一个表达式 Mountain 是若干列 Column 的序列；每列是若干条目 Entry=(value, sep) 的序列；value 是从 1 起的列标引用，sep 是分隔符层号（非负整数，显示为 sep+1 个逗号）。空表达式表示 0；另外用 ∞ 标志表示极限，显示为 "Limit"。竖向表由各列条目按顺序经 vertical_increase 生成；父位置为父列 value−1 中竖向小于本条目竖向的第一个位置（二分查找）。

合法性：展开前校验每个 value 落在 [1, 表达式列数] 内，越界即按解析失败处理。

极限与后继：非空且末列非空的表达式是极限表达式；末列为空的表达式是后继表达式，它的展开就是删去末列。

∞ 的特殊判定与基本列：本记号把固定表达式 INFINITY = [[], [[1,1]], [[2,1]]]（第一列为空列，第二列为单条目 (1,1)，第三列为单条目 (2,1)）整体视为 ∞；当前表达式与该固定表达式按列字典序比较相等即识别为 ∞。Limit[n] 的基本列是对 INFINITY 应用标准 n-MN 展开（smnshare::expand，shorter=false）所得的 Mountain。

展开分流（参数 m≥1；FS 较短 shorter=true，FS_alter 较长 shorter=false）：表达式为 ∞ 时直接返回其第 m 个基本列；空表达式原样返回；末列为空时删去末列。若末列不含无限条目（末项 sep=0），直接调用标准 n-MN 展开：fill_ghost 补幽灵行后反复 extend 共 m 次，较短展开删去末列，较长展开再做一次 subtract_1，最后 clear_ghost 清掉幽灵行。若末列含无限条目（末项 sep>0），进入自定义分支 expand_infinite。

expand_infinite 步骤：取末列列标 right 与末项位置 top，计算竖向表 V 与父位置 pr=(Ri, Rj)；offset=right−Ri；y_offset 在 top=Rj 时取 1，否则取 top−Rj。up 标志数组初值全 0，末列标记为 up=1。对 i=Ri+1 到 right−1 逐列判定：该列不含无限条目时，有限高度 finite_height=项数；若 finite_height 不超过 Rj+1 则保持 0，否则取该列第 Rj 项的父位置，up[i] 继承父列标志。该列含无限条目时，finite_height=项数−1；小于 Rj 保持 0；大于 Rj 同样取第 Rj 项的父位置继承标志；恰等于 Rj 时进入阈值坏根比较：构造阈值列，前 Rj+1 个条目为 (列标 i, 0)，末项为 (列标 Ri, 1)；从 i+1 向右找到第一列与阈值列比较结果小于 0 的列标 X_end。若 X_end 已到末列之后，本列记 1；否则自末列起沿第 Rj 项父位置上溯到首个 finite_height 不超过 Rj 的列 Y_start，再对 k=1,2,... 比较 X_start+k 列（值不小于 X_start 的条目整体平移 Y_start−X_start）与 Y_start+k 列，按列字典序首次不一致决定 up_i 为 1 或 0；未决时若 X_end−X_start 不小于 Y_end−Y_start 记 1，否则记 0。随后先对原表达式做 subtract_1（末列末项减一层分隔符，条件成立时补回父列对应条目，再拼回父列尾部）；再对 w=1..m 把第 Ri+1 到 right 列按 copy_column 复制，列标偏移 offset·w，高度填充 y_offset·w；较短展开删去最后一列。

copy_column(col, Ri, Rj, offset, up, y_offset)：若列为空、Rj=0 且 up 为真，先补 y_offset 个 (Ri+1+offset, sep=0) 条目；逐项处理时，若位置 j=Rj 且 up 为真，先在该条目前补 y_offset 个 (e.value+offset, 0) 条目；值平移规则为 e.value 大于 Ri+1，或 e.value 等于 Ri+1 且（up 为真或 j 小于 Rj）时 value 加 offset，否则原样保留。

记号极限：实现未给出该记号所对应序数上界，具体极限以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_UP2MN_V1B = R"DEFTXT(
UP2MN v1B

UP2MN v1B 是 notation-explorer（hypcos）中 category-upmn-test 家族的 UP 型山记号（credit.upmn，TS 源：src/notations/MN/UPMN/UP2MN-v1b.ts），C++ 实现见 Header/notation/up2mn_v1b.hpp，共享核心为 Header/notation/mnup2share.hpp。表达式为 0 起列标、整数分隔符层号的双参数山结构，显示时列标加 1 交给 n-MN 骨架。

表达式结构：一个表达式 Expr 是若干列 Column 的序列；每列是若干条目 Entry=(value, sep) 的序列；value 是从 0 起的列标引用，sep 是分隔符层号（非负整数）。空表达式表示 0；另外用 ∞ 标志表示极限，显示为 "Limit"。竖向表：一列的竖向序列按条目顺序经 vertical_increase 生成——当前竖向长度不超过 sep 时得到 [0]*sep+[1]，否则把第 sep 项加 1、前 sep 项置 0；条目父位置 pi 取自身 value，pj 为父列竖向表中第一个小于本条目竖向的位置（find_index_below_row，二分查找）。

合法性：展开前校验所有 value 落在 [0, 表达式列数) 内，越界按解析失败处理。

极限与后继：非空且末列非空的表达式是极限表达式；末列为空的表达式是后继表达式，它的展开就是删去末列。

∞ 的基本列：Limit[n]（n 为正整数）展开为：第一列为空列；对第 i=1..n 列，列内含 i−1 个引用第 i−1 列、sep=0 的条目，末项为引用第 0 列、sep=1 的条目（plus=false 分支）。解析 "Limit" 直接得到 ∞ 标志。

展开（参数 m≥1；FS 较短 shorter=true，FS_alter 较长 shorter=false）：空表达式原样返回；末列为空时删去末列。否则取末列列标 right 与末项位置 top，计算竖向表 V 与父位置表 P；rp=P[right][top] 记为 (Ri, Rj)。末项 sep=0 记为有限情形 is_finite=true，否则为无限情形；有限情形用 compute_up_1mn 算 up 标志，无限情形用 compute_up_2mn（plus=false）算 up 标志。构造新表达式：保留除末列外各列；新末列由末列删去末项组成，若不是有限情形且 top=Rj，补入条目 (Ri, 0)，再拼回第 Ri 列自第 Rj 项起的所有条目。高度偏移 y_offset 在有限情形为 0；无限情形取 top−Rj 大于 1 时的 top−Rj，否则取 1。对 w=1..m 逐轮把第 Ri+1 到 right 列按 copy_column 复制，列标偏移 (right−Ri)·w，高度偏移 y_offset·w；较短展开删去最后一列。

compute_up_1mn（有限情形，v1B 与 v1B+ 相同）：列标小于 Ri 的标志为 0，第 Ri 列为 1；若 Rj=0，则第 Ri 列及其右侧全部记 1。对 i=Ri+1 到末列：该列项数不超过 Rj 记 0；若 P[i][Rj].i 不等于 Ri，继承 P[i][Rj].i 列的标志；否则在列中找到第一个 value=Ri 的位置 j，找不到记 0；若 j>0，沿第 j−1 项父位置上溯到父列等于 Ri 的列 p，p 标志为 0 则本列记 0。随后进入坏根比较：X_start=i，Y_start=末列，沿第 j 项父位置上溯到父列等于 Ri 的列 Y_start；Y_start 不大于 X_start 时相等记 1、否则记 0；否则先比较两列自 j 项起的后缀列（逐项先比 value 再比 sep，前缀相同则长者大），不一致即按方向决定；再对 k=1,2,... 比较相对列 to_rel_column（value 不小于起点的条目标记为相对项、值改为减起点），相对列先比标记（标记为真是更大），再比 value 与 sep，首次不一致决定；全部一致记 1。

compute_up_2mn（无限情形，v1B 即 plus=false 分支）：列标小于 Ri 为 0，第 Ri 列为 1。对 i=Ri+1 到末列：列项数不超过 Rj 记 0；项数不少于 Rj+2 时继承 P[i][Rj].i 列的标志；末项 sep=0 的有限列记 0；其余取 p=P[i][Rj].i，p 不等于 Ri 时本列记 0（这是 v1B 与 v1B+ 的关键差异，v1B+ 此处继承 p 列标志）。p 等于 Ri 时进入坏根比较：X_start=i，Y_start=末列，沿父位置上溯直到找到项数恰为 Rj+1 的列 Y_start；Y_start 不大于 X_start 时相等记 1、否则记 0；之后的相对列比较与 compute_up_1mn 相同，未决记 1。

copy_column(col, rp, offset, y_offset, up)：value 大于 Ri 加 offset，小于 Ri 保持不变，等于 Ri 时仅当 up 为真才加 offset；若 up 为真且 y_offset 大于 0、Rj 未超出结果列范围，则在第 Rj 项位置前补入 y_offset 个填充条目（value 取第 Rj 项平移后的值，sep=0），原头部 Rj 项与尾部其余条目依次保留。

记号极限：实现未给出该记号所对应序数上界，具体极限以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_UP2MN_V1B_PLUS = R"DEFTXT(
UP2MN v1B+

UP2MN v1B+ 是 notation-explorer（hypcos）中 category-upmn 家族的 UP 型山记号（credit.upmn，TS 源：src/notations/MN/UPMN/UP2MN-v1b-plus.ts），C++ 实现见 Header/notation/up2mn_v1b_plus.hpp，共享核心 Header/notation/mnup2share.hpp。它与 UP2MN v1B 共用同一套表达式结构、竖向表、父位置、极限与后继判定、扩展骨架与复制函数，仅在两处判定上不同（以代码为准）。

表达式结构、合法性、极限与后继判定均与 UP2MN v1B 相同：Expr 为列序列，条目 (value 0 起列标, sep 层号)，空表达式表示 0，∞ 标志显示 "Limit"；非空且末列非空为极限表达式，末列为空为后继表达式，后继展开即删去末列；value 须落在 [0, 列数) 内。

与 v1B 的两处差异：

(1) ∞ 的基本列：Limit[n]（n 为正整数）为第一列空列，第 i=1..n 列只含单个条目 (value=i−1, sep=1)；v1B 的第 i 列则先有 i−1 个 sep=0 条目再加一个 sep=1 末项。

(2) compute_up_2mn（无限情形）中 p≠Ri 分支：当 P[i][Rj].i 不等于 Ri 时，v1B+ 继承该父列 p 的标志 result[p]，而 v1B 直接记 0。其余判定——列项数分档（≤Rj 记 0、≥Rj+2 继承父列、末项 sep=0 记 0）、坏根 Y_start 上溯到项数恰为 Rj+1 的列、后缀列与相对列比较规则、未决记 1——均与 v1B 相同。

展开流程与 v1B 相同：先算 up 标志，构造新末列（非有限且 top=Rj 时补条目 (Ri, 0)，再拼回第 Ri 列自 Rj 项起的尾部），y_offset 无限情形取 top−Rj 与 1 的较大者，随后对 w=1..m 复制第 Ri+1 到 right 列，列标偏移 (right−Ri)·w、高度偏移 y_offset·w；FS 较短展开删去最后一列，FS_alter 较长展开保留全部复制列。copy_column 的值平移与 Rj 位置补条目规则亦与 v1B 相同。

记号极限：实现未给出该记号所对应序数上界，具体极限以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_UPMS = R"DEFTXT(
UPMS（User-defined Primitive Matrix System）

输入格式：省略尾随零的矩阵文本，例如 (0)(1)(2,1)
或显式补零的 (0,0)(1,0)(2,1)。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_WEAK_A_OMEGA2_MN_2 = R"DEFTXT(
wAω2MN2

wAω2MN2 是 Aω2MN2 的弱版本（TS 源：src/notations/MN/Aw2MN2.ts，家族 category-hypcos-w2mn，credit.hypcos_mn），C++ 实现见 Header/notation/waw2mn2.hpp，与普通版 Aω2MN2 共享核心 Header/notation/aw2mn_share.hpp。表达式结构、合法性、极限与后继判定、∞ 的基本列、extend 的总体流程以及 subtract1 版本2 规则均与 Aω2MN2 相同，区别仅在 stretched 拉伸判定中的弱版豁免。

表达式结构（与 Aω2MN2 相同）：Expr = Column[]，Column = Entry[]，Entry=(value, sep, mark)：value 是从 1 起的列标引用，sep 是递归地也是 Expr 的分隔符，mark 是星号标志；竖向 Vertical = Sep[]。空表达式表示 0；∞ 用标志表示，显示为 "Limit"。展开前校验每个条目 1 ≤ value ≤ 列数，括号、逗号、分号不匹配则解析失败。

极限与后继：非空且末列非空为极限表达式；末列为空为后继表达式，删去末列即得前驱；空表达式展开为空。

∞ 的基本列：Limit[n] = [[], [[1, sep_n]]]，其中 sep_n = [[], [[1, [[]]]]] 再接 n 个空列，显示为 ()(; 后接 n 个逗号再接 1；对 ∞ 展开时直接返回 infinity_fs(term)。

展开（参数 m≥1）：FS 为较短展开 shorter=true，FS_alter 为较长展开 shorter=false；两者先对极限表达式反复调用 extend 共 m 次，较短展开随后删去末列，较长展开再调用一次 extend（is_small=true，不新增列）。extend 中依次完成：计算各列竖向 V0；由末列最右上条目经 Parent 求得坏根位置 BRij=(br, bj) 与宽度 width；记录坏根列前 bj 项及末列最右上项的分隔符 top_separators 与竖向 top_verticals；做岩浆检查；调用 subtract1 版本2；用 get_references 匹配末列竖向与 top_verticals；计算各 top 极限分隔符的拉伸阈值与拉伸量；按 stretched 规则处理条目分隔符；最后按岩浆检查结果把坏根列右侧 width 列复制到列标加 width 的新位置（is_small 时只处理 dx=0）。

与 Aω2MN2 的唯一差异在 stretched 判定：弱版（weak=true）在施加拉伸之前先检查该条目在原表达式中的竖向 V0[x][y] 是否不低于对应的 top 竖向 top_verticals[br]；若是，条目保持原分隔符，不追加任何空列；普通版没有这一豁免。其余保持原分隔符的条件与普通版相同：拉伸量为 0，条目分隔符 ≥ 对应的 top 分隔符，或条目分隔符 < 拉伸阈值；不满足这些条件时，在条目分隔符后追加 stretch_value 个空列。

subtract1 仍为版本2：先删去末列最右上条目，取 alpha 为坏根列 bj−1 项的竖向（无则空竖向），working_vertical 为末列最右上条目前一项的竖向（无则空竖向）；copy_entries_loop 在 alpha > working_vertical 时把坏根列中定位位置之后、bj 之前的条目复制到末列。被删分隔符为极限表达式时，构造 J（BR_separator ≥ 被删分隔符则 J 为单空列，否则 BR_separator 后接一空列），在 vertical_increase(alpha, J) > working_vertical 时先按需执行复制循环，再追加带 mark 的条目 (value, J, mark=true)；被删分隔符非极限且非 one（one 为单空列表达式 [[]]）时，先复制循环，alpha == working_vertical 时追加不带 mark 的截断条目 (value, truncated, mark=false)；分隔符为 one 时不追加条目。

记号极限：与 Aω2MN2 共用 infinity_fs，基本列相同；"w"（弱）体现在拉伸时保留高位条目的原分隔符。本记号达到的极限序数以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_WEAK_A_OMEGA2_MN_3 = R"DEFTXT(
wAω2MN3

wAω2MN3 是 Aω2MN3 的弱版本（TS 源：src/notations/MN/Aw2MN3.ts，家族 category-hypcos-w2mn，credit.hypcos_mn），C++ 实现见 Header/notation/waw2mn3.hpp，与其余三个记号共享核心 Header/notation/aw2mn_share.hpp。表达式结构、合法性、极限与后继判定、∞ 基本列、extend 总体流程与 Aω2MN3 相同；本记号同时携带版本3 的 subtract1 规则与弱版拉伸豁免。

表达式结构（与其余三个记号相同）：Expr = Column[]，Column = Entry[]，Entry=(value, sep, mark)：value 是从 1 起的列标引用，sep 是递归地也是 Expr 的分隔符，mark 是星号标志；竖向 Vertical = Sep[]。空表达式表示 0；∞ 用标志表示，显示为 "Limit"。展开前校验每个条目 1 ≤ value ≤ 列数，括号、逗号、分号不匹配则解析失败。

极限与后继：非空且末列非空为极限表达式；末列为空为后继表达式，删去末列即得前驱；空表达式展开为空。

∞ 的基本列：Limit[n] = [[], [[1, sep_n]]]，其中 sep_n = [[], [[1, [[]]]]] 再接 n 个空列，显示为 ()(; 后接 n 个逗号再接 1；对 ∞ 展开时直接返回 infinity_fs(term)。

展开（参数 m≥1，FS 较短展开 shorter=true，FS_alter 较长展开 shorter=false）：先对极限表达式反复调用 extend 共 m 次，较短展开随后删去末列，较长展开再调用一次 extend（is_small=true，不新增列）。extend 中依次完成：计算各列竖向 V0；由末列最右上条目经 Parent 求得坏根位置 BRij=(br, bj) 与宽度 width；记录坏根列前 bj 项及末列最右上项的分隔符 top_separators 与竖向 top_verticals；做岩浆检查；调用 subtract1 版本3；用 get_references 匹配末列竖向与 top_verticals；计算拉伸阈值与拉伸量；按 stretched 弱版规则处理条目分隔符；最后按岩浆检查结果把坏根列右侧 width 列复制到列标加 width 的新位置（is_small 时只处理 dx=0）。

弱版拉伸豁免（与 wAω2MN2 相同，区别于普通版）：stretched 判定中，若 weak=true 且该条目在原表达式中的竖向 V0[x][y] 不低于对应的 top 竖向 top_verticals[br]，条目保持原分隔符，不追加空列；其余保持原分隔符的条件为拉伸量为 0、条目分隔符 ≥ 对应的 top 分隔符、条目分隔符 < 拉伸阈值；不满足这些条件时在条目分隔符后追加 stretch_value 个空列。普通版没有高位豁免。

subtract1 版本3（与 Aω2MN3 相同，区别于版本2）：先删去末列最右上条目；alpha = 坏根列第 bj−1 项的竖向（bj=0 时为空竖向）；working_vertical = 末列最右上条目前一项的竖向（无前一项时为空竖向）。若被删条目分隔符是 one（单空列表达式 [[]]），直接返回删去末项后的表达式，不执行复制循环，也不追加条目。否则先执行一次 copy_entries_loop：若 alpha > working_vertical，在坏根列竖向表中定位 working_vertical 下方位置，把该位置之后、bj 之前的条目依次复制到末列，value 取被删条目的 value，working_vertical 随复制逐条 vertical_increase。随后按分隔符类型分支：
(1) 极限分支：BR_separator = S(A, br, bj−1)；J = 若 BR_separator ≥ 被删分隔符则为单空列，否则 BR_separator 后接一个空列；while 循环把空列追加到 J，直到 vertical_increase(alpha, J) 与 vertical_increase(working_vertical, J) 相等；最后向末列追加带 mark 的条目 (value, J, mark=true)。
(2) 非极限分支：truncated = 被删分隔符删去末列；若 vertical_increase(alpha, truncated) > working_vertical，向末列追加不带 mark 的条目 (value, truncated, mark=false)。

版本3 相对版本2 的差异在此记号中同样成立：one 提前返回；复制循环前置；非极限分支以 vertical_increase(alpha, truncated) > working_vertical 为追加判据（版本2 为 alpha == working_vertical）；极限分支循环加长 J 至两侧竖向相等后总是追加（版本2 不循环加长，仅在 vertical_increase(alpha, J) > working_vertical 时追加）。

记号极限：与普通版共用 infinity_fs，基本列相同；"w"（弱）体现在拉伸时保留高位条目的原分隔符。本记号达到的极限序数以原始 TS 定义为准。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_WMM = R"DEFTXT(
wMM（Weak Mutant Matrix，弱突变矩阵系统）

输入格式：与 BMS 相同的矩阵文本，列为 (v0,v1,...)，省略尾随零，例如 (0,0)(1,1)(2,2)(3,2)。
记号极限输入 Limit，其第 n 个基本列为 (0)(1,1,...,1)。
最后一列全为零时为后继型，删去最后一列。

展开规则：每个单元格除数值外还对应一个“vertical 序列”（先比序列长度，
再从高位到低位逐项比较）；父项由数值恰好小 1 且 vertical 不更大的祖先
确定。展开时沿父链收集根的位置，把末列数值减 1 并覆写成根列后缀，再按
根宽与根高逐段复制坏部，通过 reference 行号映射为每列重建数值。

提供两个展开：
- FS（shorter）：在核心结果上再删去最后一列。
- FS_alter：保留核心结果的全部列。
)DEFTXT";

constexpr std::string_view kDef_IDR_DEF_WPPS4 = R"DEFTXT(
Weak PPS4

PPS4 的弱化版本。
)DEFTXT";

struct Entry { int id; std::string_view utf8; };
constexpr Entry kTable[] = {
    { IDR_DEF_1BM_BHM, kDef_IDR_DEF_1BM_BHM },
    { IDR_DEF_1MN, kDef_IDR_DEF_1MN },
    { IDR_DEF_2BM_BHM, kDef_IDR_DEF_2BM_BHM },
    { IDR_DEF_2MN, kDef_IDR_DEF_2MN },
    { IDR_DEF_2PPS4, kDef_IDR_DEF_2PPS4 },
    { IDR_DEF_3BM_BHM, kDef_IDR_DEF_3BM_BHM },
    { IDR_DEF_3MN, kDef_IDR_DEF_3MN },
    { IDR_DEF_A_OMEGA2_MN_2, kDef_IDR_DEF_A_OMEGA2_MN_2 },
    { IDR_DEF_A_OMEGA2_MN_3, kDef_IDR_DEF_A_OMEGA2_MN_3 },
    { IDR_DEF_BHM, kDef_IDR_DEF_BHM },
    { IDR_DEF_BLM, kDef_IDR_DEF_BLM },
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
    { IDR_DEF_BSM, kDef_IDR_DEF_BSM },
    { IDR_DEF_CMS, kDef_IDR_DEF_CMS },
    { IDR_DEF_DEN1, kDef_IDR_DEF_DEN1 },
    { IDR_DEF_DSM, kDef_IDR_DEF_DSM },
    { IDR_DEF_E0MN, kDef_IDR_DEF_E0MN },
    { IDR_DEF_EMPTY, kDef_IDR_DEF_EMPTY },
    { IDR_DEF_EPSILON_Y, kDef_IDR_DEF_EPSILON_Y },
    { IDR_DEF_EWPPS4, kDef_IDR_DEF_EWPPS4 },
    { IDR_DEF_GMS_GBMS_FULL, kDef_IDR_DEF_GMS_GBMS_FULL },
    { IDR_DEF_GMS_GBMS_OMEGA_P, kDef_IDR_DEF_GMS_GBMS_OMEGA_P },
    { IDR_DEF_GMS_GBMS_PQSS, kDef_IDR_DEF_GMS_GBMS_PQSS },
    { IDR_DEF_GMS_GBMS_QSS, kDef_IDR_DEF_GMS_GBMS_QSS },
    { IDR_DEF_GMS_GBMS_WEIRD, kDef_IDR_DEF_GMS_GBMS_WEIRD },
    { IDR_DEF_GMS_LPMS2_FULL, kDef_IDR_DEF_GMS_LPMS2_FULL },
    { IDR_DEF_GMS_LPMS2_OMEGA_P, kDef_IDR_DEF_GMS_LPMS2_OMEGA_P },
    { IDR_DEF_GMS_LPMS2_PQSS, kDef_IDR_DEF_GMS_LPMS2_PQSS },
    { IDR_DEF_GMS_LPMS2_QSS, kDef_IDR_DEF_GMS_LPMS2_QSS },
    { IDR_DEF_GMS_LPMS2_WEIRD, kDef_IDR_DEF_GMS_LPMS2_WEIRD },
    { IDR_DEF_GMS_UPMS_FULL, kDef_IDR_DEF_GMS_UPMS_FULL },
    { IDR_DEF_GMS_UPMS_OMEGA_P, kDef_IDR_DEF_GMS_UPMS_OMEGA_P },
    { IDR_DEF_GMS_UPMS_PQSS, kDef_IDR_DEF_GMS_UPMS_PQSS },
    { IDR_DEF_GMS_UPMS_QSS, kDef_IDR_DEF_GMS_UPMS_QSS },
    { IDR_DEF_GMS_UPMS_WEIRD, kDef_IDR_DEF_GMS_UPMS_WEIRD },
    { IDR_DEF_IBLP, kDef_IDR_DEF_IBLP },
    { IDR_DEF_LPMS, kDef_IDR_DEF_LPMS },
    { IDR_DEF_LPTSS, kDef_IDR_DEF_LPTSS },
    { IDR_DEF_MRSS121, kDef_IDR_DEF_MRSS121 },
    { IDR_DEF_OMEGA_MN, kDef_IDR_DEF_OMEGA_MN },
    { IDR_DEF_OMEGA_Y_MEDIUM, kDef_IDR_DEF_OMEGA_Y_MEDIUM },
    { IDR_DEF_OMEGA_Y_SEQUENCE, kDef_IDR_DEF_OMEGA_Y_SEQUENCE },
    { IDR_DEF_OMEGA_Y_STRONG, kDef_IDR_DEF_OMEGA_Y_STRONG },
    { IDR_DEF_PCF_OMEGA_MN, kDef_IDR_DEF_PCF_OMEGA_MN },
    { IDR_DEF_PPS, kDef_IDR_DEF_PPS },
    { IDR_DEF_PPS4, kDef_IDR_DEF_PPS4 },
    { IDR_DEF_S_OMEGA_DBMS, kDef_IDR_DEF_S_OMEGA_DBMS },
    { IDR_DEF_S_OMEGA_DBMS_V2, kDef_IDR_DEF_S_OMEGA_DBMS_V2 },
    { IDR_DEF_S_OMEGA_DBMS_V3, kDef_IDR_DEF_S_OMEGA_DBMS_V3 },
    { IDR_DEF_S_OMEGA_P1_DBMS, kDef_IDR_DEF_S_OMEGA_P1_DBMS },
    { IDR_DEF_S_OMEGA_POW_OMEGA_MN, kDef_IDR_DEF_S_OMEGA_POW_OMEGA_MN },
    { IDR_DEF_S_OMEGA_SQUARE_DBMS, kDef_IDR_DEF_S_OMEGA_SQUARE_DBMS },
    { IDR_DEF_S_OMEGA2_DBMS, kDef_IDR_DEF_S_OMEGA2_DBMS },
    { IDR_DEF_S_OMEGA2_MN, kDef_IDR_DEF_S_OMEGA2_MN },
    { IDR_DEF_S1DBMS, kDef_IDR_DEF_S1DBMS },
    { IDR_DEF_S2DBMS, kDef_IDR_DEF_S2DBMS },
    { IDR_DEF_SA_OMEGA2_MN, kDef_IDR_DEF_SA_OMEGA2_MN },
    { IDR_DEF_SPPS4, kDef_IDR_DEF_SPPS4 },
    { IDR_DEF_STRONG_E0MN, kDef_IDR_DEF_STRONG_E0MN },
    { IDR_DEF_T_OMEGA_MN, kDef_IDR_DEF_T_OMEGA_MN },
    { IDR_DEF_TBM, kDef_IDR_DEF_TBM },
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
    { IDR_DEF_UP1DBMS, kDef_IDR_DEF_UP1DBMS },
    { IDR_DEF_UP1MN, kDef_IDR_DEF_UP1MN },
    { IDR_DEF_UP2DBMS_V1, kDef_IDR_DEF_UP2DBMS_V1 },
    { IDR_DEF_UP2DBMS_V1B_PLUS, kDef_IDR_DEF_UP2DBMS_V1B_PLUS },
    { IDR_DEF_UP2DBMS_V1C, kDef_IDR_DEF_UP2DBMS_V1C },
    { IDR_DEF_UP2DBMS_V2, kDef_IDR_DEF_UP2DBMS_V2 },
    { IDR_DEF_UP2MN_V1A, kDef_IDR_DEF_UP2MN_V1A },
    { IDR_DEF_UP2MN_V1B, kDef_IDR_DEF_UP2MN_V1B },
    { IDR_DEF_UP2MN_V1B_PLUS, kDef_IDR_DEF_UP2MN_V1B_PLUS },
    { IDR_DEF_UPMS, kDef_IDR_DEF_UPMS },
    { IDR_DEF_WEAK_A_OMEGA2_MN_2, kDef_IDR_DEF_WEAK_A_OMEGA2_MN_2 },
    { IDR_DEF_WEAK_A_OMEGA2_MN_3, kDef_IDR_DEF_WEAK_A_OMEGA2_MN_3 },
    { IDR_DEF_WMM, kDef_IDR_DEF_WMM },
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

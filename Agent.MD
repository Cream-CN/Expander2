# Agent 工作交接说明（Agent.MD）

本文件记录 Expander-Dev（Googology 大数记号展开器）将 TypeScript 记号库
`ne-rewritten/src/notations/BM-like` 移植到 C++ 这一长期任务中**尚未完成**的部分、
当前阻塞点与续作约定。仅记录未竟事项与必要的恢复信息；已完成并提交的内容只在需要
界定边界时简述。

最后更新：2026-10-25。

---

## 1. 总体任务

- 来源（只读，工作区之外）：
  `C:\Users\Administrator\Desktop\ne-rewritten\src\notations\BM-like`
- 目标（本仓库唯一可写产物根）：`D:\expander\Expander-Dev`
- 要求：阅读 `Docs/`；按依赖链逐层移植，重新制造 C++ 接口；**只保留核心 Expand
  展开逻辑**（舍弃山脉图 / diagram / 0Y / simple / OCN / BM_converter / 记忆化缓存
  UI / layer 与 marked 等价显示 / html / latex；Minus1 系列另舍弃 BOCF 等价显示）。
- 每完成一个独立拆分任务，在 `dev` 分支创建一个 commit（**不 push**）。
- 已存在、不重做：BM4(BMS) 在 `Header/notation/BMSFamily.hpp`；UPMS 在
  `Header/notation/upms.hpp`。

### 参数化族的 UI 约定（本系列新增，已落地）

Minus1_Y_nSS 系列在 TS 侧是“参数为 n 的工厂”，网页可无限生成实例。经确认的方案是
**运行时 UI 输入**而非编译期枚举多个 n：

- 每族只注册**一个**记号；
- 选中该类记号时，主窗口显示一个额外整数输入框，接受 `n <= 25`；
  - `start=0` 的族允许 `n` 取 0（界面显示为 1SS 起）；
  - `start=1` 的族 `n` 从 1 起（2SS 起）；
- `n` 在展开时传入核心 Expand。
- 已在 `Expander.cpp` 落地：
  - `NotationEntry` 新增
    `std::string(*expand_text_n)(std::string_view,int,int)` 以及
    `param_min / param_max / param_default / param_label`；
  - `UiState` 新增 `hStaticParam / hEditParam`，在 WM_CREATE 创建（默认隐藏），
    `updateNotationLabel()` 在选中带 `expand_text_n` 的记号时显示并填默认值；
  - `IDM_FS / IDM_FSALTER` 路径新增分支：读取并 stoi 校验“项数”和“参数”，
    参数范围按该记号 `[param_min, param_max]` 校验，再调 `expand_text_n`；
    参数化族结果不做通用截断。

---

## 2. 进度总览

任务在内部任务表中编号。#1–#12、#15、#16、#17 已完成并提交；#13、#18、#14 未完成。

### 已完成（均已 commit 到 dev，仅供界定边界）

- 共享设施 `bmlike_share.hpp`；BSM、BHM 及 nBM-BHM×3、CMS、DSM、BLM、wMM、
  GMS 族（15 个）、LPMS/LPTSS、TBM、TUPMS、BTBM、BTBM-weak、BBM。
- **Minus1 基础族** `Minus1_Y_nSS`（commit `ba4abfe`，IDR `20104`）：
  平面元组结构 `Column=[arr, step:int]`；跨 n=0..3 共 600 例 golden 全过。
- **UI 参数管线**（同 commit `ba4abfe`，见上节）。
- **T 族** `T_Minus1_Y_nSS`（IDR `20105`）：
  递归嵌套 `Column=[arr, step:子表达式]`，子表达式支持裸数字 / ω(w) 简写；
  跨 n=0..3 共 723 例 golden 全过。
- **Abs_B / Rel_B_Minus1_Y_nSS**（#17，IDR `20106` / `20107`）：
  共享模板 `BranchNotation<bool kRel>`（`Header/notation/bm1ynss.hpp`），
  父计算为活动栈语义（绝对 step 比较 + forbidden_stack / 相对差分 key +
  outer_stack）；跨 n=0..2 共 1518 例 golden 全过，T 族 723 例无回归。
  （注：提交记录见 git log；若工作区有尚未提交的登记变更，见第 5 节。）

### 未完成

| 编号 | 内容 | 状态 |
|------|------|------|
| #13  | Minus1_Y_nSS 全系列（共 8 族）的总任务 | 进行中（已完成基础族与 T 族，余 6 族） |
| #17  | Abs_B / Rel_B_Minus1_Y_nSS | **已完成**（golden 1518 例全过） |
| #18  | BT_star Minus1 v1 / v2 / v3（v3 含 FS_short）、BTL | 未开始 |
| #14  | 全部记号完成后的全量 rebuild + Docs 收尾 | 未开始 |

需要移植的族共 8 个，TS 文件均位于
`...\BM-like\Minus1_Y_nSS-series\`：

1. `Minus1_Y_nSS.ts`（基础族，**已完成**）
2. `T_Minus1_Y_nSS.ts`（**已完成**；Abs/Rel 依赖其 `from_display`）
3. `Abs_B_Minus1_Y_nSS.ts`（**已完成**）
4. `Rel_B_Minus1_Y_nSS.ts`（**已完成**）
5. `BT_star_Minus1_Y_nSS.ts`（v1，start=1，#18）
6. `BT_star_...-v2.ts`（v2 / strong，start=1，#18）
7. `BT_star_...-v3.ts`（v3 / weak，start=1，**同时提供 FS 与 FS_short**，#18；
   v3 额外 import `BM.ts` 的 `column_display`）
8. `BTL_Minus1_Y_nSS.ts`（start=1，使用 `ExprCompact` +
   `compactify/decompactify`，FS 为 `compactify(FS(decompactify(e),i,n))`，#18）

建议续作顺序：先收尾 #17（Abs/Rel）→ 再做 #18 的 v1→v2→v3→BTL → 最后 #14。
依赖关系：Abs_B、Rel_B 复用 T 族的解析；v3 依赖 `BM.ts`；其余 v1/v2/BTL 仅依赖
`@/utils` 与 notation-definition。

---

## 3. #17 Abs/Rel 移植记录（已完成）

### 3.1 已就绪的资产

- node 逐字真值引擎：`tools/bml_golden/branch_m1ynss_engine.mjs`
  - 复用 `t_m1ynss_engine.mjs` 的 `from_display`；
  - 导出 `fsString(mode, s, index, n)`，`mode ∈ {"abs","rel"}`；
  - 两族共用下层父 / 阈值 / 尾层 / 根 / `ascend_replace`，差异仅在第 n 个父：
    - abs：`compute_parents_abs`（同趟 DFS，按绝对 step 子表达式比较，
      排除 `forbidden_stack`）；
    - rel：`compute_lower_parents` + `fill_top_parents`（先 `to_relative`
      生成 `(bool,int)` 差分 key，按 `rel_compare` 比较，排除 `outer_stack`）。
- golden：`tools/bml_golden/branch_m1ynss_out.txt`
  格式 `mode \t input \t term \t n \t output`（UTF-8 无 BOM）。生成器
  `branch_m1ynss_ref.mjs` 会显式搜索 abs≠rel 的反例并纳入；最近一次生成
  1518 例，n=1 含 12 个确认分歧输入，n=2/3 仅普通输入（其分歧更难触发）。
- C++ 头（**有缺陷，正在调试**）：`Header/notation/bm1ynss.hpp`
  - 复用 `Header/notation/tm1ynss.hpp` 的 `Node/Column/Parser/render/compare_node`；
  - 模板 `BranchNotation<bool kRel>`，别名
    `AbsBM1Notation=BranchNotation<false>`、
    `RelBM1Notation=BranchNotation<true>`；
  - 均暴露 `expand_text_n(text, term, n)`，`kParamMin=0 / kParamMax=25`。
- smoke：`tools/_branch_m1ynss_smoke.cpp`（含临时 stderr 逐例打印，**收尾时删除**）。

### 3.2 完成过程要点（勿重蹈）

1. **崩溃根因**：旧版 `Ctx{gcol,gparr}` 用全局 preorder 下标当父下标；TS 的
   `stack / parent_stack` 是“活动深度路径”，随子树返回 splice。C++ 已改为
   `ColStack` / `ParStack` 活栈 + 每层 for 后 `resize(lS0)`，每个 PCol 自带
   `parr`（shared_ptr 与活栈共享同一对象，fill_top 回填第 n 个父时同步可见）。
2. **显示层分歧（216 例失败的真凶）**：`tm1_detail::is_one_column` / `render`
   是 T 族的**宽松**语义（n==0 → step 恰 1 列即 one-column；ω 简写只要求
   e[1] 为 one-column），branch 引擎是**严格**语义（one-column 还要求该列
   为零列；ω 简写还要求 e[0] 为零列）。修复 = 在 `bm1_detail` 内独立实现
   `br_is_one_column` / `br_render` / `br_column_display`（严格版），
   `expand_text_n` 输出走 `br_render`。**不要改 `tm1_detail`**（T 族 723 例
   依赖宽松语义；改共享头会回归 41 例）。
3. `br_thresholds` 的 r=undefined 分支以 fresh 空栈递归；iS>r 分支 while
   循环加边界守卫（TS 越界读 undefined 比较恒 false）。
4. `br_root` 返回 {-1,-1} 表示 b<0（r=undefined 路径：r_layer=0、ri=0、
   A 走 nullopt、V=[]、b===n 永不触发）；`br_a_pop_last` 加 nullptr 守卫。
5. 修复后 abs/rel 1518 例全过、T 族 723 例无回归；随后登记（IDR 20106/20107）、
   gen、rebuild（18437 函数）、按 §5 提交。

### 3.3 收尾（已完成）

- 已移除 `_branch_m1ynss_smoke.cpp` 的 br_fail.txt 转储（还原为仅输出
  通过/失败统计），删除一次性调试产物（`br_dbg*.mjs`、`br_crash.txt`、
  `br_fail.txt`、`br_parse_test.mjs`、`_branch_m1ynss_dbg.*`）；

---

## 4. #18 与 #14 的具体待办

### #18 BT_star v1/v2/v3 与 BTL

- 续作前**重新 Read 对应 TS 源当下内容**（源可能被外部更新）：
  `BT_star_Minus1_Y_nSS.ts`、`...-v2.ts`、`...-v3.ts`、
  `BTL_Minus1_Y_nSS.ts`；`utils.ts` / `notation_utils.ts` 语义以
  `C:\Users\Administrator\Desktop\ne-rewritten\src` 下源文件为准。
- 四族 `start=1`，注册记号时 `param_min=1`（不是 0）。
- v3 同时提供 `FS` 与 `FS_short`：
  - 评估是否映射到现有 `expand_text`（FS 按钮）与 `expand_text_alter`
    （FS_alter 按钮）；由于参数化族目前只接了 `expand_text_n`，可能需要再增加
    一个 `expand_text_n_alter(text,term,n)` 并在参数化分派分支中按
    `id==IDM_FSALTER` 选择。落地前先回看 `FS_default_LNZ_variant` 中
    `FS_short` 的 LNZ 截断语义（`notation_utils.ts`）。
  - v3 的显示依赖 `BM.ts` 的 `column_display`，移植时一并核对，不要臆造格式。
- BTL 使用 `ExprCompact` + `compactify/decompactify`：核心 Expand 外层包一层
  压缩 / 解压缩，C++ 需新增对应的紧凑表示与双向转换，并用 golden 覆盖往返。
- 每族独立：node 引擎 → golden（跨 n，建议至少覆盖 n=1..4）→ C++ 头 → smoke
  → 登记 → gen → rebuild → 一个 commit。

### #14 整体构建验证与文档收尾

- 全部记号并入后执行一次完整 `build.ps1 -Mode rebuild`，确认链接生成
  `x64/Release/Expander.exe` 且无新增警告（既有 `wWinMain` SAL 批注 C28251
  警告为历史遗留）。
- 更新 `Docs/`：
  - 增补新记号接口说明（尤其“参数化记号 + n 输入框”的
    `expand_text_n / param_*` 约定）；
  - 核对 `CONTRIBUTING` / API 类文档（以 `Docs/` 下实际文件名为准）；
  - `Docs/` 下存在一个 0 字节空文件“关于记号的贡献特殊说明.txt”，按需补写或保留。

---

## 5. 构建、登记与提交约定（务必遵守）

- 编译工具链：`D:\Microsoft_Visual_Studio\VC\Auxiliary\Build\vcvars64.bat`。
  独立 `cl` 必须带 `/utf-8`（否则中文头注释触发 C2001）；工程本身
  `/std:c++20 /utf-8 /analyze`，头文件**无 BOM**。
  smoke 模板：
  ```
  cmd /c "call vcvars64.bat >nul 2>&1 && cl /nologo /std:c++20 /utf-8 /EHsc \
    /I <root> /Fe:...exe /Fo:...obj ...cpp && ...exe <golden.txt>"
  ```
- 全量构建：`pwsh -NoProfile -File .\build.ps1 -Mode gen` 生成 `Definitions.h`，
  再 `-Mode rebuild`（约 1.7–1.8 万函数，2–4 分钟，建议后台运行）。
- 登记一个新参数化记号需要同步修改：
  1. `Header/notation/<name>.hpp`（命名空间 `omegay::notation`，
     `suffix()` 返回空 `std::string`，解析失败 / 内部异常 `catch(...)` 返回
     `bmlike::kParseError`，不抛异常）；
  2. `Expander.cpp`：`#include` + `notationTable` 条目（指定初始化，
     填 `expand_text_n / param_min / param_max / param_default / param_label`）；
  3. `Resource.h`：下一可用 `IDR_DEF_*` 号；
  4. `build.ps1` 的 `$defFiles`：`"def_<id>" = "IDR_DEF_<NAME>"`；
  5. `defs/def_<id>.txt` 定义文本（**须 `git add -f`**，因 `.gitignore`
     末尾 `*.txt` 忽略所有 txt）；
  6. 运行 `-Mode gen` 再 `-Mode rebuild`。
- git：分支 `dev`；本目录是 worktree，`.git` 元数据在
  `D:\expander\Expander\.git\worktrees\Expander-Dev\`（可写根之外），
  沙箱内 `git add/commit` 会因 `index.lock Permission denied` 失败，
  **必须在沙箱外（提权）执行 `git add` / `git commit`**（已多次获授权），不 push。
  每个独立族一个 commit，commit message 写明记号名、IDR、golden 例数。
- `tools/bml_golden/` 整个目录与 `tools/_*.cpp/.exe/.obj`、`*_out.txt`
  均为本地验证资产，按既往惯例**不入库**（`*.mjs`/`*.json` 会显示为 untracked，
  保持不提交即可）。

### 命名 / 集成注意

- 接口签名：`std::string expand_text_n(std::string_view text, int term, int n)`。
- 命名空间内同名自由函数经 ADL 与 `using namespace` 易触发 C2668 歧义；
  后引入的本地函数一律加族前缀（既往：tupms 用 `tu_`、weak 用 `w_`、
  bbm 用 `b_`、T 族 detail 用 `tm1_`、Abs/Rel 用 `br_`）。
- 树形 / 嵌套结构**不要**复用 `bmlike_share.hpp` 的平面 Matrix parse/to_string；
  Minus1 基础族的平面元组、T/Abs/Rel 的递归 Node 各自独立。
- 已知历史教训：长 node 进程对共享可变树有别名突变风险，golden 生成尽量
  “一次构造 / 每例独立”；Minus1 系列为纯数据结构、深拷贝隔离，风险较低。
- 空层 / 空列访问必须守卫（既往 BBM 曾因 `cols.back()` 在空层越界导致
  `0xC0000005`），递归树切片同理。

---

## 6. 恢复本任务时的第一步

1. 读本文件；再 Read `Header/notation/bm1ynss.hpp` 与
   `tools/bml_golden/branch_m1ynss_engine.mjs` 当前内容。
2. 重读 TS 源 `Abs_B_Minus1_Y_nSS.ts` / `Rel_B_Minus1_Y_nSS.ts` 与
   `T_Minus1_Y_nSS.ts` 的 `from_display`，以源文件为准。
3. 按第 3.3 节对单例 `(1)(2)`, n=0 做 node 与 C++ 双边埋点，修正 `br_fs`
   的层 / 根 / 切片坐标系问题（而非加空守卫掩盖）。
4. 跑通 1518 例后按第 5 节登记、gen、rebuild、commit，再继续 #18、#14。

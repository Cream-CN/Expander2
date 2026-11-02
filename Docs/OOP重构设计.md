# Expander 面向对象重构设计（OOP Refactoring Design）

> 目标：对 Expander 的核心调度层做深度面向对象改革（抽象接口、多态、注册中心、
> 策略化展开语义），同时**包一层原壳子**——GUI 的 `NotationEntry` 表与 CLI 的
> `CliEntry` 表、命令行为、展开结果保持完全不变。
>
> 日期：2026-11-02　状态：实施中

## 1. 现状与问题

### 1.1 现有架构

```
Expander.cpp (GUI)                 ExpanderCLI.cpp (CLI)
┌──────────────────────────────┐   ┌──────────────────────────────┐
│ NotationEntry 函数指针表      │   │ CliEntry 函数指针表           │
│ kNotationTable[114]          │   │ kCliTable[114]               │
│  └ expand / suffix /         │   │  └ (同构，窄字符版)            │
│     expand_text /            │   │                               │
│     expand_text_alter /      │   │                               │
│     expand_text_n / ...      │   │                               │
├──────────────────────────────┤   ├──────────────────────────────┤
│ IDM_FS 分派逻辑               │   │ expandFs() 分派逻辑           │
│  · 文本/参数化/序列三分支      │   │  · 文本/参数化/序列三分支      │
│  · FS 截断：BMS/UPMS/MrSS    │   │  · FS 截断：BMS/UPMS/MrSS    │
│  · isBmsTextEntry 地址比较    │   │  · isBmsTextEntry 地址比较    │
└──────────────┬───────────────┘   └──────────────┬───────────────┘
               │  static struct 记号（Header/notation/*.hpp，130+ 文件）
               ▼
        EmptyNotation / PrSSNotation / BMSFamilyNotation / ...
        （每个记号 = 无状态静态方法集合，如 struct + static expand/expand_text/suffix）
```

### 1.2 主要问题

| # | 问题 | 具体表现 |
|---|------|---------|
| 1 | **双表重复维护** | GUI 的 `kNotationTable` 与 CLI 的 `kCliTable` 是同一份 114 项注册表的宽/窄字符双份拷贝，人为同步，极易失步 |
| 2 | **无接口契约** | 记号是"静态方法集合"（鸭子类型），没有任何抽象接口约束；能力靠调用方检查函数指针是否为 null 推断 |
| 3 | **分派逻辑散落重复** | FS/FS_alter/参数化/序列 的四路分派在 GUI（IDM_FS 分支）与 CLI（expandFs）各写一遍，且两处 FS 截断三路（BMS/UPMS/MrSS）逐字重复 |
| 4 | **脆弱类型判定** | `isBmsTextEntry` 用"函数指针地址是否等于 bmsExpandV1..V4"判断记号类型，新增 BMS 版本必须同步修改两处 |
| 5 | **游离包装函数** | BMS 10 个版本各配一个 `bmsExpandV1..V4` 游离函数；UPMS 的 parse/format 在 GUI 与 CLI 各复制一份 |
| 6 | **难以扩展** | 新增记号需要同时改 GUI 表、CLI 表、分派特判；新增 BMS 版本需要改 10 个包装函数 + 两处 isBmsTextEntry |

## 2. 重构目标架构

```
                    ┌──────────────────────────────────────────┐
                    │   OOP 内核  Header/notation/registry.hpp │
                    │                                          │
                    │   INotation（抽象接口）                   │
                    │    ├ 元数据（id/名称/定义/参数/tags）       │
                    │    ├ 能力虚函数（text/alter/n/seq/suffix）│
                    │    ├ 统一展开入口 expand(mode,...)        │
                    │    └ FS 截断策略 truncate_fs()（虚函数）  │
                    │                                          │
                    │   适配器模板（包住现有 static struct）：    │
                    │   SequenceNotation<T> / TextNotation<T> / │
                    │   TextAlterNotation<T> /                  │
                    │   ParametricNotation<T> /                 │
                    │   ParametricAlterNotation<T>              │
                    │   特殊实现：BmsNotation / UpmsNotation /  │
                    │   MrssNotation                            │
                    │                                          │
                    │   NotationRegistry（单例注册中心）         │
                    │    └ register_all_notations()：114 项     │
                    │       唯一事实来源                        │
                    └─────────────────┬────────────────────────┘
                                      │
        ┌─────────────────────────────┴─────────────────────────────┐
        │ 壳层（对外接口不变）                                        │
        ▼                                                           ▼
┌───────────────────────────┐                          ┌───────────────────────────┐
│ Expander.cpp              │                          │ ExpanderCLI.cpp           │
│  NotationEntry（原字段不变 │                          │  CliEntry（原字段不变）    │
│  + 新增 notation 指针）    │                          │  + 新增 notation 指针）    │
│  notationTable() 由注册   │                          │  kCliTable 由注册中心构建  │
│  中心构建（宽字符转换）     │                          │  expandFs → INotation     │
│  IDM_FS 分派 → INotation  │                          │  （窄字符直通）            │
└───────────────────────────┘                          └───────────────────────────┘
```

### 2.1 设计原则

1. **单一事实来源**：114 个记号只在一个地方注册（`register_all_notations()`），
   GUI/CLI 壳层从注册中心**派生**，杜绝双表失步。
2. **接口不变**：`NotationEntry` / `CliEntry` 的全部原字段保留、语义不变；
   GUI/CLI 的命令与 UI 行为、展开结果逐字节一致。壳层新增一个只读指针字段
   `const INotation* notation`（供新架构消费），不删除任何旧字段。
3. **实现不动**：130+ 个记号头文件的内部实现**零改动**——通过模板适配器
   （静态多态）接入 OOP 内核（运行时多态），把"深度 OOP"施加在调度/契约层，
   不冒破坏记号正确性的风险。
4. **策略化 FS 语义**：FS 通用截断（BMS 删末列 / UPMS 删末列 / MrSS pop_back /
   默认 MrSS 尝试）从两处重复代码收敛为 `truncate_fs()` 虚函数，按记号覆盖。
5. **header-only**：内核放在单个头文件 `Header/notation/registry.hpp`，
   兼容 GUI（MSBuild）与 CLI（cl.exe 单文件 / g++ 单文件）两种构建方式。

### 2.2 内核接口定义

```cpp
namespace omegay::notation {

enum class ExpandMode { Fs, FsAlter };   // FS = 展开按钮；FsAlter = FS_alter 按钮

// 展开结果：成功返回文本，失败置 ok=false 并给出人可读错误
struct ExpandResult {
    std::string text;
    bool ok = true;
    std::string err;
};

// 元数据（UTF-8 窄字符，跨平台；GUI 壳层转换宽字符）
struct NotationMeta {
    std::string_view id;
    std::string_view display_name;
    int definition_resource_id = 0;
    std::array<std::string_view, 16> tags{};   // 空槽 nullptr 结尾
    int param_min = 0, param_max = 25, param_default = 0;
    std::string_view param_label = "n:";
};

// 能力函数指针（旧壳层字段的来源；由适配器从 T 的静态方法提取）
struct NotationCaps {
    std::vector<int> (*expand_seq)(const std::vector<int>&, int) = nullptr;
    std::string (*text)(std::string_view, int) = nullptr;
    std::string (*text_alter)(std::string_view, int) = nullptr;
    std::string (*text_n)(std::string_view, int, int) = nullptr;
    std::string (*text_n_alter)(std::string_view, int, int) = nullptr;
    std::string (*suffix)() = nullptr;
};

class INotation {
public:
    explicit INotation(NotationMeta meta) noexcept;
    virtual ~INotation() = default;

    // —— 元数据 ——
    const NotationMeta& meta() const noexcept;
    std::string_view id() const noexcept;
    int definition_resource_id() const noexcept;
    bool is_parametric() const noexcept;      // 是否带 n 参数框
    const NotationCaps& caps() const noexcept; // 旧壳层字段来源

    // —— 能力虚函数（默认空实现）——
    virtual bool has_seq() const noexcept;
    virtual bool has_text() const noexcept;
    virtual bool has_text_alter() const noexcept;
    virtual bool has_text_n() const noexcept;
    virtual bool has_text_n_alter() const noexcept;
    virtual std::vector<int> expand_seq(const std::vector<int>&, int) const;
    virtual std::string expand_text(std::string_view, int) const;
    virtual std::string expand_text_alter(std::string_view, int) const;
    virtual std::string expand_text_n(std::string_view, int, int) const;
    virtual std::string expand_text_n_alter(std::string_view, int, int) const;
    virtual std::string suffix() const;

    // —— 统一展开（分派 + 截断 + suffix，取代 GUI/CLI 两处重复）——
    ExpandResult expand(ExpandMode mode, std::string_view text,
                        int term, int param) const;

protected:
    // FS 截断策略：BMS / UPMS 覆盖为删末列，其余沿用默认（MrSS 尝试）
    virtual std::string truncate_fs(std::string out, int term) const;
    NotationMeta meta_;
    NotationCaps caps_;
};

// 适配器模板：包住既有 static struct（零改动接入）
template<class T> class SequenceNotation      : public INotation { /* T::expand, T::suffix */ };
template<class T> class TextNotation          : public INotation { /* T::expand_text, T::suffix */ };
template<class T> class TextAlterNotation     : public INotation { /* + T::expand_text_alter */ };
template<class T> class ParametricNotation    : public INotation { /* T::expand_text_n + 参数 */ };
template<class T> class ParametricAlterNotation: public INotation { /* + T::expand_text_n_alter */ };

// 特殊实现（含 FS 截断策略与文本⇄Matrix 转换，取代 GUI/CLI 游离函数）
class MrssNotation  : public INotation;   // text + seq，FS 截断 = MrSS pop_back
class BmsNotation   : public INotation;   // 构造传 BMSFamilyNotation::Version，FS 截断 = 删末列
class UpmsNotation  : public INotation;   // UPMS 文本 ⇄ Matrix，FS 截断 = 删末列

// 注册中心
class NotationRegistry {
public:
    static NotationRegistry& instance();
    void add(std::unique_ptr<INotation> n);
    const INotation* by_id(std::string_view id) const;
    const INotation* by_index(std::size_t i) const;
    std::size_t size() const noexcept;
    const std::vector<std::unique_ptr<INotation>>& all() const noexcept;
};

// 唯一事实来源：注册全部 114 个记号（生成自旧表，id/顺序与旧表一致）
void register_all_notations();
const std::vector<std::unique_ptr<INotation>>& registered_notations();

}
```

### 2.3 统一展开语义（原 GUI/CLI 分派的收敛）

`INotation::expand()` 的非虚默认实现复刻原 IDM_FS / expandFs 语义，逐字节等价：

| 分支 | 原逻辑（GUI 2757–2856 / CLI expandFs） | 新逻辑 |
|------|----------------------------------------|--------|
| 文本型 + FS_alter | `expand_text_alter(text, term)` | `expand_text_alter(text, term)` |
| 文本型 + FS（无 alter） | `expand_text` → 截断 | `expand_text` → `truncate_fs` |
| 文本型 + FS_alter（无 alter） | `expand_text`（不截断） | `expand_text`（不截断） |
| 参数化 | 参数越界报错；FS_alter 优先 `n_alter` | 同左 |
| 序列型 | `parse_sequence` → `expand` → FS 时 pop_back | 同左 |
| suffix | 调用方拼接 | `expand()` 内部拼接 |

FS 截断（`truncate_fs`）：
- `BmsNotation`：`parse_matrix` → 删末列 → `matrix_to_string`
- `UpmsNotation`：`parseUpmsText` → 删末列 → `formatUpmsText`
- 其余文本记号：MrSS 尝试解析（成功且 >1 项则 pop_back；失败原样返回）——复刻现状
  对 cms/gms/blm 等记号的既有行为。

### 2.4 壳层（对外接口）改造

**Expander.cpp（GUI）**
- `NotationEntry` 结构**字段全保留**，新增 `const INotation* notation = nullptr;`。
- `kNotationTable` 静态数组 → `notationTable()` 由注册中心构建的
  `std::vector<NotationEntry>`（返回类型不变，UI 消费点零改动）。
  元数据窄→宽转换一次，wstring 存静态存储。
- `IDM_FS / IDM_FSALTER` 分支：输入读取、校验、错误弹窗保留（UI 专属），
  展开调用改为 `entry.notation->expand(...)`。
- 删除：`bmsExpandTextWith`、`bmsExpandV1..V4`、`parseUpmsText`、
  `formatUpmsText`、`upmsExpandText`、`isBmsTextEntry`。

**ExpanderCLI.cpp（CLI）**
- `CliEntry` 结构**字段全保留**，新增 `const INotation* notation = nullptr;`。
- `kCliTable` 静态数组 → 由注册中心构建（窄字符直通）。
- `expandFs()` → 调用 `entry.notation->expand(...)`（含后缀与截断），
  错误消息与现状一致。
- 删除：同 GUI 的重复游离函数与 isBmsTextEntry。

**对外行为不变清单**
1. `ExpanderCLI list` 输出：114 项、顺序、类型标注、参数范围 —— 逐字符一致。
2. `expand / fsalter / chain / show-def / translate` 行为与输出一致。
3. GUI 记号选择（搜索打分、tags、参数框、定义查看、展开结果）一致。
4. 展开结果对同一输入逐字节一致。

## 3. 改动文件清单

| 文件 | 动作 |
|------|------|
| `Header/notation/registry.hpp` | **新增**：OOP 内核（INotation + 适配器 + 注册中心 + register_all_notations） |
| `Expander.cpp` | 改：include 内核；壳层从注册中心构建；IDM_FS 走 INotation；删游离函数 |
| `ExpanderCLI.cpp` | 改：include 内核；壳层从注册中心构建；expandFs 走 INotation；删游离函数 |
| `Expander.vcxproj` / `.filters` | 改：登记新头文件 |
| `tools/gen_registry.ps1` | **新增**：从旧表生成注册代码（维护辅助） |
| 130+ 记号头文件 | **不改**（适配器接入） |

## 4. 验证方案

1. **编译**：GUI 走 `build.ps1 -Mode rebuild`（MSBuild）；CLI 走 `CLIBuild.ps1`
   （cl.exe 单文件，/bigobj）。
2. **行为基准**：用重构前的旧 `ExpanderCLI.exe` 采集基线（list 全量输出 +
   每记号至少一条 expand/fsalter 样例 + chain 若干），重构后对比逐字节一致。
3. **GUI 冒烟**：启动 GUI 手动展开若干记号，核对结果窗体文本。
4. **静态核验**：确认 kCliTable / kNotationTable 条目数=114、id 顺序与旧表一致。

---

## 5. 实施完成状态（2026-11-02）

### 5.1 已落地改动

- **内核** `Header/notation/registry.hpp`（722 行）：INotation 抽象基类 + 五类适配器
  （Sequence/Text/TextAlter/Parametric/ParametricAlter，`if constexpr requires` 处理
  BSM 的 alter 复用情形）+ Bms/Upms/Mrss 特殊实现 + NotationRegistry 单例 +
  register_all_notations()（114 项，幂等）。BMS/UPMS 全部游离函数收敛为内核 inline。
  修复 mrss121.hpp 的硬编码绝对路径 include（`D:\expander\Expander\...` → `../common/mrssshare.hpp`）。
- **GUI** `Expander.cpp`（约 2040 行）：include 精简为 registry.hpp + core/arena + core/entry
  + translator；NotationEntry 末尾新增 `const INotation* notation`；删除游离函数块与
  kNotationTable 数组；notationTable() 从 registered_notations() 构建（宽字符静态驻留）；
  IDM_FS/IDM_FSALTER 分支改走 `entry.notation->expand(mode, ...)`，UI 输入校验与
  错误弹窗逐字保留，后缀由统一入口内部拼接。
- **CLI** `ExpanderCLI.cpp`（约 1975 行）：include 精简；CliEntry 末尾新增 notation 指针；
  删除 BMS/UPMS 游离函数与 kCliTable 数组；cliTable() 从注册中心构建（窄字符直通）；
  expandFs 改为 `entry.notation->expand((alter?FsAlter:Fs), text, term, param)`，
  错误消息原文保留；findById / runList / TUI 引用点全部改指 cliTable()。
- **构建登记**：Expander.vcxproj 已登记 registry.hpp。
- **生成脚本** tools/gen_registry.ps1：注册代码改全 designated-initializer
  （MSVC 不允许混用位置/指定初始化），支持 -UseSnapshot。

### 5.2 验证结果

| 验证项 | 结果 |
|--------|------|
| CLI 编译（cl.exe Release，/bigobj） | 通过 |
| GUI 全量重建（MSBuild Release x64） | 通过 |
| CLI `list` 与基线逐字节对比 | **0 差异**（4363 字节） |
| CLI 174 段样例（87 记号 × expand+fsalter）与基线逐字节对比 | **0 差异**（1,256,801 字节） |
| GUI 冒烟（启动 6 秒存活、无崩溃） | 通过 |
| 注册表条目数 / id 顺序与旧表一致 | 通过（快照机器对比差异 0） |

### 5.3 说明与遗留

- CLI TUI 交互模式未做自动验证（需终端交互），但其数据路径（cliTable/findById/
  expandFs）与非交互命令完全共享，且 list/expand 已逐字节验证。
- GUI 交互展开未做自动化（对话框流程），逻辑与 CLI 共享同一 INotation::expand 实现。
- 快照与生成产物（_gui_table.tsv / _cli_table.tsv / _gen_registry.inc 等）保留在仓库根，
  供后续维护参照；`_refactor_baseline/` 为行为基线，勿删。

### 5.4 编译期注册表排序改造（2026-11，追加诉求）

用户要求注册表在**编译期**按 `display_name` 首字符的 UTF-8 码位自动排序，避免手工乱排。
改动全部位于内核 `registry.hpp` 的注册段与生成脚本 `tools/gen_registry.ps1`，壳层（GUI/CLI）
未动。

**机制**：
- `AllNotations`：由 `tools/gen_registry.ps1` 生成的一个 `TypeList< Reg<工厂,索引>, ... >`，
  是"全部记号"的编译期类型清单（顺序 = 旧表 id 顺序，作稳定序基准）。
- `kMetaArray`：由同一脚本生成的一个 `constexpr std::array<NotationMeta,114>`，集中存放
  全部元数据（排序键 / 构造数据）。
- `Reg<Factory, I>`：工厂类型 + 索引 I，`make()` 运行时取 `kMetaArray[I]` 构造适配器。
- `first_utf8_codepoint(string_view)`：constexpr 取首字符完整码位（按字节序 = 码位序）。
- `sorted_order()` + `kSortedOrder`：constexpr `std::sort`，主键 = 首码位升序，同码位回退
  `i<j` 保证稳定（保持源代码顺序）。
- `RegStep<0..kNotationCount>`：按 `kSortedOrder` 递归实例化并注册，注册顺序编译期确定。

**踩过的坑**：
1. **MSVC C7658**：`NotationMeta`（含 `std::string_view` 成员）作为**非类型模板参数**
   （`Reg<Factory, NotationMeta M>`）时，MSVC 报"初始值设定项必须是变量或函数的地址"。
   规避：meta 改放普通 `constexpr` 数组 `kMetaArray`（字符串字面量指针在此合法），
   `Reg` 只带工厂类型 + 索引，运行时取 meta。
2. **模板实参尾逗号**：`TypeList< A, B, ..., N, >` 的尾逗号是**语法错误**（C2059），
   生成脚本须去掉最后一项的逗号。
3. 定位这类"报错位置不在真正源头"的模板元编程问题时，用**空壳隔离编译**
   （`#include registry.hpp + int main(){}` + cl /Zs）比直接在 CLI 上排错更快。

**验证（排序改造后）**：
- CLI 编译（cl.exe Release /bigobj）通过；GUI 全量重建通过。
- CLI `list` 输出顺序已按 display_name 首字符 UTF-8 码位稳定升序
  （`-`→数字→大写→小写→希腊/中文），同首字符保持原 id 顺序。
- CLI 174 段样例（87 记号 × expand+fsalter）与 `_refactor_baseline/samples.txt` 仍
  **逐字节 0 差异** —— 排序只改变注册/展示顺序，不改变每个记号的展开结果。
- GUI 冒烟（启动 6 秒存活、无崩溃）通过。

**说明**：排序会改变 GUI 选择表与 CLI `list` 的展示顺序（用户主动要求）。
`_refactor_baseline/list.txt` 是旧顺序基线，不再适用于顺序类输出；如需回归应重建新基线
或改为"排序规则校验 + 非顺序内容一致"式对比。展开结果类对比（samples.txt）仍适用。
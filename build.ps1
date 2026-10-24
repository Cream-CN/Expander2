<#
.SYNOPSIS
    一键构建 Expander：默认进入 TUI 交互菜单选择构建方式，也可用 -Mode 直接指定（适合脚本/CI）。

.DESCRIPTION
    脚本不硬编码 VS 安装路径，依次尝试：
      1) vswhere（Visual Studio Installer 官方定位工具，最可靠）；
      2) 环境变量 VSINSTALLDIR；
      3) 常见安装目录回退。
    找到 VS 安装根后，使用其 VC\Auxiliary\Build\vcvars64.bat 初始化编译环境，
    并在同一 cmd 会话内调用 MSBuild 构建 Expander.slnx。

    Definitions.h 的生成流程已合并进本脚本（原 tools/gen_definitions.py 的逻辑改为纯
    PowerShell 实现，不再依赖 python）：把 defs/*.txt 的记号定义文本以 UTF-8 原始字符串
    字面量写进标头，由 MSVC 直接编译进程序，绕过 rc.exe 的编码误读与 \n 不转义问题。
    仅重新生成头文件（-Mode gen / 菜单 1）不需要 Visual Studio。

    TUI 菜单选项：
      1) 只重新生成 Definitions.h（内置流程）
      2) 直接调用 VS 增量构建（MSBuild /t:Build）
      3) 调用 VS 并全量重建（MSBuild /t:Rebuild）
      4) 先 1 后 3：重新生成 + 全量重建
      5) 先 1 后 2：重新生成 + 增量构建
      0) 退出

    由于此前出现过 MSBuild 增量构建误判（改动了 Definitions.h 等头文件却未重编译），
    在修改任何 .hpp / Definitions.h / Resource.h 后使用全量重建（菜单 3 或 4）。

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File build.ps1

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File build.ps1 -Configuration Debug

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File build.ps1 -Mode gen+rebuild

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File build.ps1 -Rebuild
#>
[CmdletBinding()]
param(
    [string]$Configuration = "Release",
    [string]$Platform     = "x64",
    [switch]$Rebuild,                     # 兼容旧用法：等价于 -Mode rebuild
    [ValidateSet("gen", "build", "rebuild", "gen+build", "gen+rebuild")]
    [string]$Mode         = "",           # 非交互模式；留空则进入 TUI 菜单
    [string]$Project      = ""            # 默认使用脚本所在目录的 Expander.slnx
)

$ErrorActionPreference = "Stop"

# 脚本所在目录即项目根
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $Project) { $Project = Join-Path $root "Expander.slnx" }

function Resolve-VsInstallPath {
    # 1) vswhere：官方定位工具
    $vswhere = @(
        "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe",
        "C:\Program Files\Microsoft Visual Studio\Installer\vswhere.exe"
    ) | Where-Object { Test-Path $_ } | Select-Object -First 1
    if ($vswhere) {
        $p = (& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath) 2>$null
        if ($p -and $p.Trim()) {
            $p = $p.Trim()
            if (Test-Path (Join-Path $p "VC\Auxiliary\Build\vcvars64.bat")) { return $p }
        }
    }
    # 2) 环境变量
    if ($env:VSINSTALLDIR -and (Test-Path $env:VSINSTALLDIR)) {
        $p = $env:VSINSTALLDIR.TrimEnd('\')
        if (Test-Path (Join-Path $p "VC\Auxiliary\Build\vcvars64.bat")) { return $p }
    }
    # 3) 常见安装目录回退
    $candidates = @(
        "D:\Microsoft_Visual_Studio",
        "C:\Program Files\Microsoft Visual Studio\2026\Community",
        "C:\Program Files\Microsoft Visual Studio\2026\Professional",
        "C:\Program Files\Microsoft Visual Studio\2026\Enterprise",
        "C:\Program Files\Microsoft Visual Studio\2025\Community",
        "C:\Program Files\Microsoft Visual Studio\2025\Professional",
        "C:\Program Files\Microsoft Visual Studio\2025\Enterprise",
        "C:\Program Files\Microsoft Visual Studio\2022\Community",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
    )
    foreach ($c in $candidates) {
        if (Test-Path (Join-Path $c "VC\Auxiliary\Build\vcvars64.bat")) { return $c }
    }
    throw "未找到 Visual Studio 安装。请安装含使用C++桌面开发工作负载的VS，或设置 VSINSTALLDIR 环境变量。"
}

function Resolve-VcvarsAndMsbuild([string]$vs) {
    $vcvars = Join-Path $vs "VC\Auxiliary\Build\vcvars64.bat"
    if (-not (Test-Path $vcvars)) { throw "未找到 vcvars64.bat：$vcvars" }
    $msbuild = @(
        (Join-Path $vs "MSBuild\Current\Bin\amd64\MSBuild.exe"),
        (Join-Path $vs "MSBuild\Current\Bin\MSBuild.exe")
    ) | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $msbuild) { throw "未找到 MSBuild.exe（$vs 下）" }
    return $vcvars, $msbuild
}

# VS 环境仅在构建类步骤需要时解析；gen（重新生成 Definitions.h）为纯 PowerShell，不依赖 VS
$script:vs = $null
$script:vcvars = $null
$script:msbuild = $null

function Ensure-VsEnv {
    if ($script:vs -and $script:vcvars -and $script:msbuild) { return }
    $script:vs = Resolve-VsInstallPath
    $script:vcvars, $script:msbuild = Resolve-VcvarsAndMsbuild $script:vs
}

# ---------- 各构建步骤 ----------
function Invoke-GenDefinitions {
    # 原 tools/gen_definitions.py 的生成逻辑，现以纯 PowerShell 内嵌于此。
    # 记号定义文本原本作为 RCDATA 资源由 rc.exe 编译进 .res；rc.exe 对 UTF-8 无 BOM
    # 的源文件按 ANSI/本地代码页误读（中文首尾乱码），且资源脚本里的 "\n" 只是字面字符。
    # 本流程把定义文本以 C++ 原始字符串字面量 R"(...)" 写进标头 Definitions.h（字节原样
    # 保留、换行即真换行），由 MSVC 直接编译进程序，完全不经过 rc.exe。
    Write-Host ""
    Write-Host "==> [1] 重新生成 Definitions.h（内置流程，无需 python）..." -ForegroundColor Green
    $defsDir = Join-Path $root "defs"
    $out     = Join-Path $root "Definitions.h"
    if (-not (Test-Path -LiteralPath $defsDir)) { throw "错误: 找不到定义目录 $defsDir" }

    # 定义文本文件名(去 .txt 扩展名) -> 资源 ID 宏名(与 Resource.h 保持一致)
    $fileToId = @{
        "def_empty"            = "IDR_DEF_EMPTY"
        "def_pps"              = "IDR_DEF_PPS"
        "def_pps4"             = "IDR_DEF_PPS4"
        "def_wpps4"            = "IDR_DEF_WPPS4"
        "def_tpps4"            = "IDR_DEF_TPPS4"
        "def_ewpps4"           = "IDR_DEF_EWPPS4"
        "def_spps4"            = "IDR_DEF_SPPS4"
        "def_2-pps4"           = "IDR_DEF_2PPS4"
        "def_omega-y-medium"   = "IDR_DEF_OMEGA_Y_MEDIUM"
        "def_omega-y-strong"   = "IDR_DEF_OMEGA_Y_STRONG"
        "def_mrss121"          = "IDR_DEF_MRSS121"
        "def_bms-v1"           = "IDR_DEF_BMS_V1"
        "def_bms-v2"           = "IDR_DEF_BMS_V2"
        "def_bms-v21"          = "IDR_DEF_BMS_V21"
        "def_bms-v22"          = "IDR_DEF_BMS_V22"
        "def_bms-v23"          = "IDR_DEF_BMS_V23"
        "def_bms-v3"           = "IDR_DEF_BMS_V3"
        "def_bms-v31"          = "IDR_DEF_BMS_V31"
        "def_bms-v32"          = "IDR_DEF_BMS_V32"
        "def_bms-v33"          = "IDR_DEF_BMS_V33"
        "def_bms-v4"           = "IDR_DEF_BMS_V4"
        "def_omega-y-sequence" = "IDR_DEF_OMEGA_Y_SEQUENCE"
        "def_epsilon-y"        = "IDR_DEF_EPSILON_Y"
        "def_upms"             = "IDR_DEF_UPMS"
        "def_den1"             = "IDR_DEF_DEN1"
        "def_blp"              = "IDR_DEF_BLP"
        "def_iblp"             = "IDR_DEF_IBLP"
        "def_ton-m"            = "IDR_DEF_TON_M"
        "def_ton-i"            = "IDR_DEF_TON_I"
        "def_ton-dr"           = "IDR_DEF_TON_DR"
        "def_ton-drc"          = "IDR_DEF_TON_DRC"
        "def_ton-drp"          = "IDR_DEF_TON_DRP"
        "def_ton-drpc"         = "IDR_DEF_TON_DRPC"
        "def_ton-ibp"          = "IDR_DEF_TON_IBP"
        "def_ton-mc"           = "IDR_DEF_TON_MC"
        "def_ton-mpc"          = "IDR_DEF_TON_MPC"
        "def_e0mn"              = "IDR_DEF_E0MN"
        "def_strong-e0mn"       = "IDR_DEF_STRONG_E0MN"
        "def_omega-mn"          = "IDR_DEF_OMEGA_MN"
        "def_pcf-omega-mn"      = "IDR_DEF_PCF_OMEGA_MN"
        "def_t-omega-mn"        = "IDR_DEF_T_OMEGA_MN"
        "def_1-mn"              = "IDR_DEF_1MN"
        "def_2-mn"              = "IDR_DEF_2MN"
        "def_3-mn"              = "IDR_DEF_3MN"
        "def_s-omega2-mn"       = "IDR_DEF_S_OMEGA2_MN"
        "def_sa-omega2-mn"      = "IDR_DEF_SA_OMEGA2_MN"
        "def_s-omega-pow-omega-mn" = "IDR_DEF_S_OMEGA_POW_OMEGA_MN"
        "def_up1mn"             = "IDR_DEF_UP1MN"
        "def_up2mn-v1a"         = "IDR_DEF_UP2MN_V1A"
        "def_up2mn-v1b"         = "IDR_DEF_UP2MN_V1B"
        "def_up2mn-v1b-plus"    = "IDR_DEF_UP2MN_V1B_PLUS"
        "def_up1dbms"           = "IDR_DEF_UP1DBMS"
        "def_up2dbms-v1"        = "IDR_DEF_UP2DBMS_V1"
        "def_up2dbms-v1b-plus"  = "IDR_DEF_UP2DBMS_V1B_PLUS"
        "def_up2dbms-v1c"       = "IDR_DEF_UP2DBMS_V1C"
        "def_up2dbms-v2"        = "IDR_DEF_UP2DBMS_V2"
        "def_s1dbms"            = "IDR_DEF_S1DBMS"
        "def_s2dbms"            = "IDR_DEF_S2DBMS"
        "def_s-omega-dbms"      = "IDR_DEF_S_OMEGA_DBMS"
        "def_s-omega-dbms-v2"   = "IDR_DEF_S_OMEGA_DBMS_V2"
        "def_s-omega-dbms-v3"   = "IDR_DEF_S_OMEGA_DBMS_V3"
        "def_s-omega-p1-dbms"   = "IDR_DEF_S_OMEGA_P1_DBMS"
        "def_s-omega2-dbms"     = "IDR_DEF_S_OMEGA2_DBMS"
        "def_s-omega-square-dbms" = "IDR_DEF_S_OMEGA_SQUARE_DBMS"
        "def_a-omega2-mn-2"     = "IDR_DEF_A_OMEGA2_MN_2"
        "def_weak-a-omega2-mn-2" = "IDR_DEF_WEAK_A_OMEGA2_MN_2"
        "def_a-omega2-mn-3"     = "IDR_DEF_A_OMEGA2_MN_3"
        "def_weak-a-omega2-mn-3" = "IDR_DEF_WEAK_A_OMEGA2_MN_3"
        "def_bsm"               = "IDR_DEF_BSM"
        "def_bhm"               = "IDR_DEF_BHM"
        "def_1bm-bhm"           = "IDR_DEF_1BM_BHM"
        "def_2bm-bhm"           = "IDR_DEF_2BM_BHM"
        "def_3bm-bhm"           = "IDR_DEF_3BM_BHM"
        "def_cms"               = "IDR_DEF_CMS"
        "def_dsm"               = "IDR_DEF_DSM"
        "def_blm"               = "IDR_DEF_BLM"
        "def_wmm"               = "IDR_DEF_WMM"
    }

    $delim = "DEFTXT"  # 原始字符串字面量分隔符，须避开正文里的 ')'+$delim+'"'
    $early = ")" + $delim + '"'

    # 严格 UTF-8 解码器：非法字节序列直接抛异常
    $utf8Strict = New-Object Text.UTF8Encoding($false, $true)

    $entries = [System.Collections.Generic.List[object]]::new()
    foreach ($name in ($fileToId.Keys | Sort-Object)) {
        $id = $fileToId[$name]
        $p  = Join-Path $defsDir ($name + ".txt")
        if (-not (Test-Path -LiteralPath $p)) {
            Write-Host "警告: 缺 $name.txt —— $id 将回退为“暂缺”" -ForegroundColor Yellow
            continue
        }
        $bytes = [IO.File]::ReadAllBytes($p)
        if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
            $stripped = New-Object byte[] ($bytes.Length - 3)
            [Array]::Copy($bytes, 3, $stripped, 0, $stripped.Length)
            $bytes = $stripped
        }
        try { $text = $utf8Strict.GetString($bytes) }
        catch { throw "错误: $p 不是合法 UTF-8 ($($_.Exception.Message))" }
        # 校验原始字符串字面量不会被提前终止
        if ($text.Contains($early)) {
            throw "错误: $p 含有会终止原始字面量的序列 )$delim`""
        }
        $entries.Add([pscustomobject]@{ Id = $id; Body = $text.TrimEnd("`r", "`n") })
    }

    # 按 ID 宏名排序，保证输出稳定、便于 diff
    $sorted = @($entries | Sort-Object { $_.Id })

    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.Add('// GENERATED FILE —— 请勿手工编辑。由 build.ps1 的 Invoke-GenDefinitions 从 defs/*.txt 生成。')
    $lines.Add('// 重新生成: .\build.ps1 -Mode gen （或菜单 1 / 4 / 5）')
    $lines.Add('//')
    $lines.Add('// 记号定义文本以 UTF-8 原始字符串字面量直接编译进程序，完全绕过 rc.exe，')
    $lines.Add('// 从而避免 rc.exe 对 UTF-8 源文件的编码误读(首尾乱码)与 \n 不转义的问题。')
    $lines.Add('#pragma once')
    $lines.Add('#include <string_view>')
    $lines.Add('#include "Resource.h"')
    $lines.Add('')
    $lines.Add('namespace expander_defs {')
    $lines.Add('')
    foreach ($e in $sorted) {
        $lines.Add('constexpr std::string_view kDef_' + $e.Id + ' = R"' + $delim + '(')
        $lines.Add($e.Body)
        $lines.Add(')' + $delim + '";')
        $lines.Add('')
    }
    $lines.Add('struct Entry { int id; std::string_view utf8; };')
    $lines.Add('constexpr Entry kTable[] = {')
    foreach ($e in $sorted) {
        $lines.Add('    { ' + $e.Id + ', kDef_' + $e.Id + ' },')
    }
    $lines.Add('};')
    $lines.Add('')
    $lines.Add('// 按资源 ID 取定义文本；未收录的记号返回空(由调用方显示“暂缺”)。')
    $lines.Add('[[nodiscard]] inline std::string_view lookup(int id) {')
    $lines.Add('    for (const Entry& e : kTable) {')
    $lines.Add('        if (e.id == id) return e.utf8;')
    $lines.Add('    }')
    $lines.Add('    return {};')
    $lines.Add('}')
    $lines.Add('')
    $lines.Add('} // namespace expander_defs')
    $lines.Add('')

    # 输出为 UTF-8 with BOM：Debug|Win32 配置未开 /utf-8，BOM 让 MSVC 按 UTF-8 识别源文件
    $text = [string]::Join("`n", $lines)
    [IO.File]::WriteAllText($out, $text, (New-Object Text.UTF8Encoding($true)))
    Write-Host "生成完成: $out  ($($sorted.Count) 个记号)" -ForegroundColor Green
}

function Show-EnvInfo {
    Write-Host ""
    Write-Host "Visual Studio : $script:vs"     -ForegroundColor Cyan
    Write-Host "vcvars64.bat  : $script:vcvars" -ForegroundColor Cyan
    Write-Host "MSBuild       : $script:msbuild" -ForegroundColor Cyan
    Write-Host "项目          : $Project" -ForegroundColor Cyan
    Write-Host "配置          : $Configuration / $Platform" -ForegroundColor Cyan
    Write-Host ""
}

function Invoke-MsBuild {
    param([string]$Target)   # "" = 增量构建；" /t:Rebuild" = 全量重建
    Ensure-VsEnv
    $label = if ($Target) { "全量重建 (Rebuild)" } else { "增量构建 (Build)" }
    Write-Host ""
    Write-Host "==> [msbuild] $label  $Configuration/$Platform ..." -ForegroundColor Green
    $inner = "call `"$script:vcvars`" >nul 2>&1 && `"$script:msbuild`" `"$Project`" /m /p:Configuration=$Configuration /p:Platform=$Platform /v:minimal /nologo$Target"
    Write-Host "执行: cmd /c $inner" -ForegroundColor DarkGray
    & cmd /c $inner
    if ($LASTEXITCODE -ne 0) { throw "MSBuild $label 失败，退出码 $LASTEXITCODE" }
}

function Invoke-Mode([string]$action) {
    switch ($action) {
        "gen"         { Invoke-GenDefinitions }
        "build"       { Invoke-MsBuild "" }
        "rebuild"     { Invoke-MsBuild " /t:Rebuild" }
        "gen+build"   { Invoke-GenDefinitions; Invoke-MsBuild "" }
        "gen+rebuild" { Invoke-GenDefinitions; Invoke-MsBuild " /t:Rebuild" }
        default       { throw "未知模式: $action" }
    }
}

# ---------- TUI 菜单 ----------
function Show-Menu {
    try { Clear-Host } catch { }   # 无控制台（管道/CI）时优雅降级
    try { Ensure-VsEnv } catch { } # 尽力显示 VS 路径；构建类选项会自动解析
    $vsLabel = if ($script:vs) { $script:vs } else { "未找到（构建类选项会报错）" }
    Write-Host ""
    Write-Host "==================== Expander 一键构建 ====================" -ForegroundColor Cyan
    Write-Host "  VS      : $vsLabel" -ForegroundColor DarkGray
    Write-Host "  项目    : $Project"    -ForegroundColor DarkGray
    Write-Host "  配置    : $Configuration / $Platform" -ForegroundColor DarkGray
    Write-Host ""
    Write-Host "  [1] 只重新生成 Definitions.h（内置流程，无需 python）" -ForegroundColor Yellow
    Write-Host "  [2] 直接调用 VS 增量构建（MSBuild /t:Build）" -ForegroundColor Yellow
    Write-Host "  [3] 调用 VS 并全量重建（MSBuild /t:Rebuild）" -ForegroundColor Yellow
    Write-Host "  [4] 先 1 后 3：重新生成 + 全量重建" -ForegroundColor Yellow
    Write-Host "  [5] 先 1 后 2：重新生成 + 增量构建" -ForegroundColor Yellow
    Write-Host "  [0] 退出" -ForegroundColor Red
    Write-Host ""
    Write-Host "  提示：改动 Definitions.h / Resource.h / *.hpp 后建议选 3 或 4" -ForegroundColor DarkGray
    Write-Host "        （规避 MSBuild 增量构建误判）" -ForegroundColor DarkGray
    Write-Host ""
}

function Read-MenuChoice {
    $choice = ""
    do {
        try {
            $choice = Read-Host "请选择 [0-5]"
        }
        catch {
            Write-Host "无法交互读取输入（非终端环境），请改用 -Mode 参数。" -ForegroundColor Yellow
            exit 1
        }
        if ($null -eq $choice) { return "exit" }   # Ctrl+C / 空输入直接退出
        $choice = $choice.Trim()
    } while ($choice -notmatch '^[0-5]$')
    switch ([int]$choice) {
        1 { return "gen" }
        2 { return "build" }
        3 { return "rebuild" }
        4 { return "gen+rebuild" }
        5 { return "gen+build" }
        default { return "exit" }
    }
}

# ---------- 确定执行模式 ----------
# 注意：这里不用 $mode 作为变量名——$Mode 参数带 ValidateSet，PowerShell 变量不区分
# 大小写，脚本体内对 $mode 再次赋值会重新触发参数验证器（空串不在集合内即报错）。
$action = $Mode
if (-not $action -and $Rebuild) { $action = "rebuild" }   # 兼容旧用法 -Rebuild
if (-not $action) {
    if ($Host.Name -ne "ConsoleHost") {
        Write-Host "当前为非交互主机（$($Host.Name)），请用 -Mode 指定：gen / build / rebuild / gen+build / gen+rebuild" -ForegroundColor Yellow
        exit 1
    }
    Show-Menu
    $action = Read-MenuChoice
}

if ($action -eq "exit") { Write-Host "已退出。" -ForegroundColor Gray; exit 0 }

try {
    Invoke-Mode $action
    Write-Host ""
    Write-Host "构建完成。" -ForegroundColor Green
    exit 0
}
catch {
    Write-Host ""
    Write-Host "构建失败: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

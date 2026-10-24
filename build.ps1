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

    构建前可先运行 tools/gen_definitions.py 重新生成 Definitions.h
    （把 defs/*.txt 的记号定义文本编译进程序，绕过 rc.exe 的编码/转义问题）。

    TUI 菜单选项：
      1) 只重新生成 Definitions.h（gen_definitions.py）
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

function Resolve-Python {
    $c = Get-Command python -ErrorAction SilentlyContinue
    if ($c) { return $c.Source }
    $c = Get-Command py -ErrorAction SilentlyContinue
    if ($c) { return $c.Source }
    return $null
}

# ---------- 各构建步骤 ----------
function Invoke-GenDefinitions {
    $py = Resolve-Python
    if (-not $py) { throw "未找到 python / py 命令，无法运行 tools/gen_definitions.py" }
    $gen = Join-Path $root "tools\gen_definitions.py"
    if (-not (Test-Path -LiteralPath $gen)) { throw "未找到 $gen" }
    Write-Host ""
    Write-Host "==> [1] 重新生成 Definitions.h ..." -ForegroundColor Green
    & $py $gen
    if ($LASTEXITCODE -ne 0) { throw "gen_definitions.py 失败，退出码 $LASTEXITCODE" }
}

function Invoke-MsBuild {
    param([string]$Target)   # "" = 增量构建；" /t:Rebuild" = 全量重建
    $label = if ($Target) { "全量重建 (Rebuild)" } else { "增量构建 (Build)" }
    Write-Host ""
    Write-Host "==> [msbuild] $label  $Configuration/$Platform ..." -ForegroundColor Green
    $inner = "call `"$vcvars`" >nul 2>&1 && `"$msbuild`" `"$Project`" /m /p:Configuration=$Configuration /p:Platform=$Platform /v:minimal /nologo$Target"
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
    Write-Host ""
    Write-Host "==================== Expander 一键构建 ====================" -ForegroundColor Cyan
    Write-Host "  VS      : $vs"         -ForegroundColor DarkGray
    Write-Host "  项目    : $Project"    -ForegroundColor DarkGray
    Write-Host "  配置    : $Configuration / $Platform" -ForegroundColor DarkGray
    Write-Host ""
    Write-Host "  [1] 只重新生成 Definitions.h（tools/gen_definitions.py）" -ForegroundColor Yellow
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

# ---------- 定位并校验环境 ----------
$vs = Resolve-VsInstallPath
$vcvars, $msbuild = Resolve-VcvarsAndMsbuild $vs

if (-not (Test-Path -LiteralPath $Project)) { throw "未找到解决方案：$Project" }

Write-Host ""
Write-Host "Visual Studio : $vs"     -ForegroundColor Cyan
Write-Host "vcvars64.bat  : $vcvars" -ForegroundColor Cyan
Write-Host "MSBuild       : $msbuild" -ForegroundColor Cyan
Write-Host "项目          : $Project" -ForegroundColor Cyan
Write-Host "配置          : $Configuration / $Platform" -ForegroundColor Cyan
Write-Host ""

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

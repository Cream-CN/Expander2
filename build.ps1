<#
.SYNOPSIS
    一键构建 Expander：自动定位本机 Visual Studio 的 vcvars64.bat 环境并调用 MSBuild。

.DESCRIPTION
    脚本不硬编码 VS 安装路径，依次尝试：
      1) vswhere（Visual Studio Installer 官方定位工具，最可靠）；
      2) 环境变量 VSINSTALLDIR；
      3) 常见安装目录回退。
    找到 VS 安装根后，使用其 VC\Auxiliary\Build\vcvars64.bat 初始化编译环境，
    并在同一 cmd 会话内调用 MSBuild 构建 Expander.slnx。

    由于此前出现过 MSBuild 增量构建误判（改动了 Definitions.h 等头文件却未重编译），
    强烈建议在修改任何 .hpp / Definitions.h / Resource.h 后使用 -Rebuild 强制全量重建。

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File build.ps1

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File build.ps1 -Configuration Debug

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File build.ps1 -Rebuild
#>
[CmdletBinding()]
param(
    [string]$Configuration = "Release",
    [string]$Platform     = "x64",
    [switch]$Rebuild,                     # 强制全量重建（绕过增量误判）
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

# ---------- 定位并校验环境 ----------
$vs = Resolve-VsInstallPath
$vcvars, $msbuild = Resolve-VcvarsAndMsbuild $vs

if (-not (Test-Path $Project)) { throw "未找到解决方案：$Project" }

Write-Host ""
Write-Host "Visual Studio : $vs"    -ForegroundColor Cyan
Write-Host "vcvars64.bat  : $vcvars" -ForegroundColor Cyan
Write-Host "MSBuild       : $msbuild" -ForegroundColor Cyan
Write-Host "项目          : $Project" -ForegroundColor Cyan
Write-Host "配置          : $Configuration / $Platform$(if ($Rebuild) { '（全量重建）' } else { '（增量构建）' })" -ForegroundColor Cyan
Write-Host ""

# ---------- 在同一 cmd 会话内 call vcvars 并构建 ----------
$target = if ($Rebuild) { " /t:Rebuild" } else { "" }
$inner = "call `"$vcvars`" >nul 2>&1 && `"$msbuild`" `"$Project`" /m /p:Configuration=$Configuration /p:Platform=$Platform /v:minimal /nologo$target"
Write-Host "执行: cmd /c $inner" -ForegroundColor DarkGray

& cmd /c $inner
exit $LASTEXITCODE

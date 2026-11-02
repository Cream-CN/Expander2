<#
.SYNOPSIS
    从旧版 Expander.cpp 的 kNotationTable 提取全部 114 个记号条目，按能力归类，
    生成 OOP 注册中心（Header/notation/registry.hpp）的 register_all_notations 函数体。

.DESCRIPTION
    一次性迁移工具（2026-11 OOP 重构）：
      1) 解析 Expander.cpp 中 kNotationTable 的每个条目（id/display_name/绑定/
         definition_resource_id/参数/tags）；
      2) 按能力归类：parametric(_alter) / mrss / bms(版本) / upms /
         text_alter / text / sequence；
      3) 生成 C++ 注册代码（std::make_unique<...> 调用），写入 _gen_registry.inc；
      4) 同时输出条目清单 _gen_registry_list.txt 供人工审查。

    生成结果须人工审查后并入 registry.hpp。本脚本保留作为后续"表单一来源"维护的
    数据快照来源（若旧表已删，可改用 _gui_table.tsv / _gui_full.tsv 快照）。

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\gen_registry.ps1
#>
[CmdletBinding()]
param(
    [string]$Root = "",
    [string]$SrcCpp = "",        # 缺省 = $Root\Expander.cpp
    [switch]$UseSnapshot         # 用 _gui_table.tsv + _gui_full.tsv 快照（旧表已删时）
)

$ErrorActionPreference = "Stop"
if (-not $Root) {
    if ($PSScriptRoot) { $Root = Split-Path -Parent $PSScriptRoot }
    else { $Root = Split-Path -Parent $MyInvocation.MyCommand.Path }
}
if (-not $SrcCpp) { $SrcCpp = Join-Path $Root "Expander.cpp" }

# ---------- 1. 提取原始条目 ----------
$entries = [System.Collections.Generic.List[object]]::new()
if ($UseSnapshot) {
    $t = Get-Content -LiteralPath (Join-Path $Root "_gui_table.tsv") -Encoding UTF8
    $f = Get-Content -LiteralPath (Join-Path $Root "_gui_full.tsv") -Encoding UTF8
    for ($i = 0; $i -lt $t.Count; $i++) {
        $a = $t[$i] -split "`t"; $b = $f[$i] -split "`t"
        # a: id name expand def et eta etn etna ; b: id name pm:min/max/def label tags
        $pm = [regex]::Match($b[2], '^pm:(\d+)/(\d+)/(\d+)$')
        $tags = @()
        if ($b[4] -match '^tags:(.*)$') { $tags = @($Matches[1] -split '\|' | Where-Object { $_ }) }
        $entries.Add([pscustomobject]@{
            Id=$a[0]; Name=$a[1]; Expand=$a[2]; Def=$a[3];
            ET=$a[4]; ETA=$a[5]; ETN=$a[6]; ETNA=$a[7];
            ParamMin=[int]$pm.Groups[1].Value; ParamMax=[int]$pm.Groups[2].Value;
            ParamDefault=[int]$pm.Groups[3].Value; Label=$b[3].Substring(6);
            Tags=@($tags)
        })
    }
}
else {
    if (-not (Test-Path -LiteralPath $SrcCpp)) { throw "找不到 $SrcCpp" }
    $src = Get-Content -LiteralPath $SrcCpp -Raw -Encoding UTF8
    $m = [regex]::Match($src, 'static const NotationEntry kNotationTable\[\] = \{(?<body>.*?)\};',
        [Text.RegularExpressions.RegexOptions]::Singleline)
    if (-not $m.Success) { throw "无法在 $SrcCpp 中定位 kNotationTable" }
    $items = [regex]::Split($m.Groups['body'].Value, '\r?\n\s*\{') | Where-Object { $_ -match '\.display_name' }
    foreach ($it in $items) {
        $id   = [regex]::Match($it, '\.id = "([^"]*)"').Groups[1].Value
        $name = [regex]::Match($it, '\.display_name = (L?"([^"]*)"|nullptr)').Groups[2].Value
        $exp  = [regex]::Match($it, '\.expand = &([^,}]+)').Groups[1].Value.Trim()
        $def  = [regex]::Match($it, '\.definition_resource_id = ([^,}]+)').Groups[1].Value.Trim()
        $et   = [regex]::Match($it, '\.expand_text = &([^,}]+)').Groups[1].Value.Trim()
        $eta  = [regex]::Match($it, '\.expand_text_alter = &([^,}]+)').Groups[1].Value.Trim()
        $etn  = [regex]::Match($it, '\.expand_text_n = &([^,}]+)').Groups[1].Value.Trim()
        $etna = [regex]::Match($it, '\.expand_text_n_alter = &([^,}]+)').Groups[1].Value.Trim()
        $pmn  = [regex]::Match($it, '\.param_min = (\d+)').Groups[1].Value
        $pmx  = [regex]::Match($it, '\.param_max = (\d+)').Groups[1].Value
        $pmd  = [regex]::Match($it, '\.param_default = (\d+)').Groups[1].Value
        $plb  = [regex]::Match($it, '\.param_label = L?"([^"]*)"').Groups[1].Value
        $tags = ([regex]::Match($it, '\.tags = \{(?<t>[^}]*)\}').Groups['t'].Value -split ',') |
            ForEach-Object { $_.Trim().Trim('L').Trim('"') } | Where-Object { $_ -ne '' }
        $tagArr = @($tags)
        $entries.Add([pscustomobject]@{
            Id=$id; Name=$name; Expand=$exp; Def=$def;
            ET=$et; ETA=$eta; ETN=$etn; ETNA=$etna;
            ParamMin=$(if ($pmn) {[int]$pmn} else {0}); ParamMax=$(if ($pmx) {[int]$pmx} else {25});
            ParamDefault=$(if ($pmd) {[int]$pmd} else {0}); Label=$plb; Tags=$tagArr
        })
    }
}

# ---------- 2. 归类 ----------
function Get-Category([object]$e) {
    if ($e.ETN) {
        if ($e.ETNA) { return "parametric_alter" }
        return "parametric"
    }
    if ($e.ET) {
        if ($e.ET -eq "notation::Mrss121Notation::expand_string") { return "mrss" }
        if ($e.ET -match '^bmsExpandV(\d+)$') { return "bms" }
        if ($e.ET -eq "upmsExpandText") { return "upms" }
        if ($e.ETA) { return "text_alter" }
        return "text"
    }
    return "sequence"
}

function Get-BmsVersion([string]$fn) {
    $n = [regex]::Match($fn, '^bmsExpandV(\d+)$').Groups[1].Value
    return "notation::BMSFamilyNotation::Version::V$n"
}

function Esc([string]$s) {
    # C++ 字符串转义：双引号、反斜杠（名称/标签中不应有，防御处理）
    return $s.Replace('\', '\\').Replace('"', '\"')
}

$metaLines = [System.Collections.Generic.List[string]]::new()
$regLines = [System.Collections.Generic.List[string]]::new()
$listLines = [System.Collections.Generic.List[string]]::new()
$i = 0
foreach ($e in $entries) {
    $cat = Get-Category $e
    $tags = ($e.Tags | ForEach-Object { '"' + (Esc $_) + '"' }) -join ', '
    $meta = '.id = "' + (Esc $e.Id) + '", .display_name = "' + (Esc $e.Name) + '", .definition_resource_id = ' + $e.Def
    if ($tags) { $meta += ', .tags = { ' + $tags + ' }' }
    $param = ""
    if ($cat -eq "parametric" -or $cat -eq "parametric_alter") {
        $param = ", .param_min=$($e.ParamMin), .param_max=$($e.ParamMax), .param_default=$($e.ParamDefault)"
        if ($e.Label) { $param += ", .param_label=`"$(Esc $e.Label)`"" }
    }
    $ctor = $meta + $param
    switch ($cat) {
        "sequence"         { $fac = "MakeSeq<$($e.Expand -replace '^notation::','notation::' -replace '::expand$','')>" }
        "text"             { $fac = "MakeText<$($e.ET -replace '::expand_text$','')>" }
        "text_alter"       { $fac = "MakeTextAlter<$($e.ET -replace '::expand_text$','')>" }
        "parametric"       { $fac = "MakeParametric<$($e.ETN -replace '::expand_text_n$','')>" }
        "parametric_alter" { $fac = "MakeParametricAlter<$($e.ETN -replace '::expand_text_n$','')>" }
        "mrss"             { $fac = "MakeMrss" }
        "bms"              { $fac = "MakeBms<$(Get-BmsVersion $e.ET)>" }
        "upms"             { $fac = "MakeUpms" }
        default            { throw "未知类别 $cat" }
    }
    $metaLines.Add("    NotationMeta{ $ctor },")
    if ($i -lt $entries.Count - 1) { $regLines.Add("    Reg<$fac, $i>,") }
    else { $regLines.Add("    Reg<$fac, $i>") }
    $i++
    $listLines.Add("$($e.Id)`t$cat`t$($e.Name)")
}

# ---------- 3. 输出 ----------
$outInc  = Join-Path $Root "_gen_registry.inc"
$outList = Join-Path $Root "_gen_registry_list.txt"
$incLines = [System.Collections.Generic.List[string]]::new()
$incLines.Add("#ifndef EXPANDER_GEN_REGISTRY_META")
$incLines.Add("inline constexpr std::array<NotationMeta, $($entries.Count)> kMetaArray = {")
foreach ($m in $metaLines) { $incLines.Add($m) }
$incLines.Add("};")
$incLines.Add("#endif")
$incLines.Add("")
$incLines.Add("using AllNotations = TypeList<")
foreach ($r in $regLines) { $incLines.Add($r) }
$incLines.Add(">;")
[IO.File]::WriteAllLines($outInc, $incLines, (New-Object Text.UTF8Encoding($true)))
[IO.File]::WriteAllLines($outList, $listLines, (New-Object Text.UTF8Encoding($true)))
Write-Host "生成: $outInc  ($($incLines.Count) 行) + $outList"
Write-Host ""
Write-Host "类别统计:"
$listLines | ForEach-Object { ($_ -split "`t")[1] } | Group-Object | Sort-Object Name |
    ForEach-Object { Write-Host ("  {0,-18} {1}" -f $_.Name, $_.Count) }
$total = $listLines.Count
Write-Host "总计: $total"
if ($total -ne 114) { Write-Host "警告: 条目数 != 114，请检查解析" -ForegroundColor Yellow }

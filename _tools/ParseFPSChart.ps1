#requires -Version 5.0
<#
.SYNOPSIS
    解析 UE FPSChart 产出的 CSV，算出 Frame / Game / Draw / GPU 的中位数、P95、最差帧与 hitch 次数。

.DESCRIPTION
    FPSChart 的 CSV 由 UEngine::StopFPSChart 写出，位置：
      Saved/Profiling/FPSChartStats/<采集开始时间>/<地图名>-FPS-<平台>.csv
    文件内有两个区块：
      1) 25/50/75 分位的汇总块（Percentile, Frame (ms), GT (ms), RT (ms), GPU (ms), ...）
      2) 逐帧块（Time (sec), Frame (ms), GT (ms), RT (ms), GPU (ms), ...）
    本脚本只读第 2 个区块，因此可以用自定义 hitch 阈值重新统计。

.PARAMETER Csv
    指定 CSV 路径。留空时只在该目录下**恰好只有一份**采样的情况下自动使用；
    有多份时脚本会列出候选并报错，要求用 -Csv 显式指定——因为崩溃/中断的采样
    也可能留下残缺 CSV，按时间取最新可能正好取到不完整的那份。

.PARAMETER HitchMs
    hitch 判定阈值（毫秒），帧时间大于该值计为一次 hitch。默认 33.3（约 30 FPS）。

.PARAMETER OutFile
    把 Markdown 结果写到指定文件（UTF-8 无 BOM）；不指定则只打印到控制台。

.EXAMPLE
    .\_tools\ParseFPSChart.ps1

.EXAMPLE
    .\_tools\ParseFPSChart.ps1 -HitchMs 50 -OutFile .\fps_result.md
#>
[CmdletBinding()]
param(
    [string]$Csv,
    [double]$HitchMs = 33.3,
    [string]$OutFile
)

$ErrorActionPreference = 'Stop'
$Invariant = [System.Globalization.CultureInfo]::InvariantCulture

function Resolve-ChartCsv {
    param([string]$Root)

    if (-not (Test-Path -LiteralPath $Root)) {
        throw "采样目录不存在：$Root。先运行 pp.FPSChartCapture 生成采样。"
    }

    $files = @(Get-ChildItem -LiteralPath $Root -Recurse -File -Filter '*.csv' -ErrorAction SilentlyContinue |
               Sort-Object LastWriteTime -Descending)
    if ($files.Count -eq 0) {
        throw "目录内没有 CSV：$Root"
    }
    if ($files.Count -gt 1) {
        $list = ($files | ForEach-Object {
            "  {0}  ({1:N1} KB, {2:yyyy-MM-dd HH:mm:ss})" -f $_.FullName, ($_.Length / 1KB), $_.LastWriteTime
        }) -join [Environment]::NewLine
        $msg = "FPSChartStats 下有 {0} 份采样，无法判断哪份完整（崩溃或中断的采样也可能留下残缺 CSV）。" -f $files.Count
        $msg += "请用 -Csv 显式指定，例如 -Csv `"<上面某个路径>`"："
        $msg += [Environment]::NewLine + $list
        throw $msg
    }
    return $files[0].FullName
}

function Get-SeriesStats {
    param([double[]]$Values, [double]$Threshold)

    $sorted = @($Values | Sort-Object)
    $n = $sorted.Count
    if ($n -eq 0) { throw '样本为空，无法统计' }

    if ($n % 2 -eq 1) {
        $median = $sorted[[int](($n - 1) / 2)]
    }
    else {
        $median = ($sorted[$n / 2 - 1] + $sorted[$n / 2]) / 2.0
    }

    # 最近秩法求 P95
    $idx = [int][Math]::Ceiling(0.95 * $n) - 1
    if ($idx -lt 0) { $idx = 0 }
    if ($idx -ge $n) { $idx = $n - 1 }
    $p95 = $sorted[$idx]

    $mean = ($Values | Measure-Object -Average).Average
    $hitches = @($Values | Where-Object { $_ -gt $Threshold }).Count

    return [pscustomobject]@{
        Median = [double]$median
        P95    = [double]$p95
        Worst  = [double]$sorted[$n - 1]
        Mean   = [double]$mean
        Hitches = [int]$hitches
    }
}

if (-not $Csv) {
    $Csv = Resolve-ChartCsv -Root (Join-Path $PSScriptRoot '..\Saved\Profiling\FPSChartStats')
}
if (-not (Test-Path -LiteralPath $Csv)) {
    throw "CSV 不存在：$Csv"
}

$lines = Get-Content -LiteralPath $Csv -Encoding UTF8
$frame = New-Object System.Collections.Generic.List[double]
$game = New-Object System.Collections.Generic.List[double]
$draw = New-Object System.Collections.Generic.List[double]
$gpu = New-Object System.Collections.Generic.List[double]

$inFrameBlock = $false
foreach ($line in $lines) {
    # 逐帧块的表头：Time (sec),Frame (ms), GT (ms), RT (ms), GPU (ms), ...
    if ($line -match '^\s*Time\s*\(sec\)') {
        $inFrameBlock = $true
        continue
    }
    if (-not $inFrameBlock) { continue }
    if ([string]::IsNullOrWhiteSpace($line)) { continue }

    $cells = $line.Split(',')
    if ($cells.Count -lt 5) { continue }

    $v = 0.0
    if (-not [double]::TryParse($cells[1].Trim(), [System.Globalization.NumberStyles]::Float, $Invariant, [ref]$v)) { continue }
    $frame.Add($v)

    $v = 0.0
    if ([double]::TryParse($cells[2].Trim(), [System.Globalization.NumberStyles]::Float, $Invariant, [ref]$v)) { $game.Add($v) }
    $v = 0.0
    if ([double]::TryParse($cells[3].Trim(), [System.Globalization.NumberStyles]::Float, $Invariant, [ref]$v)) { $draw.Add($v) }
    $v = 0.0
    if ([double]::TryParse($cells[4].Trim(), [System.Globalization.NumberStyles]::Float, $Invariant, [ref]$v)) { $gpu.Add($v) }
}

if ($frame.Count -eq 0) {
    throw "没有解析到逐帧数据：$Csv（区块表头 'Time (sec)' 未命中，或文件被截断）"
}

$frameStats = Get-SeriesStats -Values $frame.ToArray() -Threshold $HitchMs
$gameStats  = if ($game.Count -gt 0) { Get-SeriesStats -Values $game.ToArray()  -Threshold $HitchMs } else { $null }
$drawStats  = if ($draw.Count -gt 0) { Get-SeriesStats -Values $draw.ToArray()  -Threshold $HitchMs } else { $null }
$gpuStats   = if ($gpu.Count -gt 0)  { Get-SeriesStats -Values $gpu.ToArray()   -Threshold $HitchMs } else { $null }

function Format-Num {
    param([double]$Value)
    return $Value.ToString('0.00', $Invariant)
}

function Format-Row {
    param([string]$Name, $Stats)
    if ($null -eq $Stats) { return "| $Name | - | - | - | - |" }
    return ("| {0} | {1} | {2} | {3} | {4} |" -f $Name,
        (Format-Num $Stats.Median), (Format-Num $Stats.P95), (Format-Num $Stats.Worst), $Stats.Hitches)
}

$avgFps = if ($frameStats.Mean -gt 0) { 1000.0 / $frameStats.Mean } else { 0.0 }

$md = @()
$md += "### FPSChart 采样结果"
$md += ""
$md += "- 源文件：``$Csv``"
$md += "- 样本数：$($frame.Count) 帧 / 平均帧时间 $(Format-Num $frameStats.Mean) ms / 折算 $(Format-Num $avgFps) FPS"
$md += "- Hitch 阈值：$(Format-Num $HitchMs) ms"
$md += ""
$md += "| 指标 | 中位数 (ms) | P95 (ms) | 最差帧 (ms) | Hitch 次数 |"
$md += "|---|---|---|---|---|"
$md += Format-Row -Name 'Frame' -Stats $frameStats
$md += Format-Row -Name 'Game'  -Stats $gameStats
$md += Format-Row -Name 'Draw'  -Stats $drawStats
$md += Format-Row -Name 'GPU'   -Stats $gpuStats

$text = ($md -join [Environment]::NewLine)
Write-Output $text

if ($OutFile) {
    [System.IO.File]::WriteAllText($OutFile, $text + [Environment]::NewLine, (New-Object System.Text.UTF8Encoding($false)))
    Write-Output ""
    Write-Output "已写入：$OutFile"
}

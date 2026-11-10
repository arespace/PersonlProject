<#
.SYNOPSIS
    检查五模块的单向依赖是否符合 Core <- Inventory <- Quest <- PersonalProject <- UI。
.DESCRIPTION
    只读脚本：解析各模块 Build.cs 里 Public/PrivateDependencyModuleNames 的项目内依赖，
    出现反向依赖、同层依赖或未知项目模块即报错。
    退出码 0 = 通过，1 = 有越界，可直接用于 CI。
.EXAMPLE
    powershell -ExecutionPolicy Bypass -File _tools\CheckModuleBoundaries.ps1
#>

[CmdletBinding()]
param(
    [string]$RepoRoot
)

$ErrorActionPreference = 'Stop'

# 仓库根目录：默认取本脚本所在 _tools 的上一级（脚本体内取，param 默认值里 $PSScriptRoot 可能为空）
if ([string]::IsNullOrWhiteSpace($RepoRoot))
{
    $RepoRoot = Split-Path -Parent $PSScriptRoot
}
if ([string]::IsNullOrWhiteSpace($RepoRoot))
{
    $RepoRoot = (Get-Location).Path
}

# 顺序即依赖方向：索引小的可以被索引大的依赖，反过来就是越界
$Order = @(
    'PersonalProjectCore',
    'PersonalProjectInventory',
    'PersonalProjectQuest',
    'PersonalProject',
    'PersonalProjectUI'
)

$Rank = @{}
for ($i = 0; $i -lt $Order.Count; $i++)
{
    $Rank[$Order[$i]] = $i
}

# 返回 "Scope|ModuleName" 形式，同时兼容 AddRange(new string[]{...}) 和 Add("X")
function Get-DeclaredProjectDeps
{
    param([string]$Text)

    $result = New-Object System.Collections.ArrayList

    $blockPattern = '(?s)(Public|Private)DependencyModuleNames\s*\.AddRange\s*\(\s*new\s+string\[\]\s*\{(?<body>.*?)\}\s*\)'
    foreach ($block in [regex]::Matches($Text, $blockPattern))
    {
        $scope = $block.Groups[1].Value
        foreach ($item in [regex]::Matches($block.Groups['body'].Value, '"([^"]+)"'))
        {
            [void]$result.Add(('{0}|{1}' -f $scope, $item.Groups[1].Value))
        }
    }

    $addPattern = '(Public|Private)DependencyModuleNames\s*\.Add\s*\(\s*"([^"]+)"\s*\)'
    foreach ($item in [regex]::Matches($Text, $addPattern))
    {
        [void]$result.Add(('{0}|{1}' -f $item.Groups[1].Value, $item.Groups[2].Value))
    }

    return $result
}

$Violations = New-Object System.Collections.ArrayList
$Summary = New-Object System.Collections.ArrayList

foreach ($Module in $Order)
{
    $RelativePath = "Source\$Module\$Module.Build.cs"
    $BuildFile = Join-Path $RepoRoot $RelativePath

    if (-not (Test-Path -LiteralPath $BuildFile))
    {
        [void]$Violations.Add("缺少文件：$RelativePath")
        continue
    }

    $Declared = New-Object System.Collections.ArrayList
    foreach ($Entry in (Get-DeclaredProjectDeps -Text (Get-Content -LiteralPath $BuildFile -Raw)))
    {
        $Parts = $Entry -split '\|', 2
        $Scope = $Parts[0]
        $Dep = $Parts[1]

        if ($Dep -notlike 'PersonalProject*')
        {
            continue
        }

        [void]$Declared.Add($Dep)

        if (-not $Rank.ContainsKey($Dep))
        {
            [void]$Violations.Add(("{0}：{1} 声明了未知项目模块 '{2}'" -f $RelativePath, $Scope, $Dep))
            continue
        }

        if ($Rank[$Dep] -ge $Rank[$Module])
        {
            [void]$Violations.Add(("{0}：{1} 依赖 '{2}'，越过单向顺序 Core <- Inventory <- Quest <- PersonalProject <- UI" -f $RelativePath, $Scope, $Dep))
        }
    }

    $List = if ($Declared.Count -gt 0) { ($Declared | Sort-Object -Unique) -join ', ' } else { '(无)' }
    [void]$Summary.Add(('{0,-26} -> {1}' -f $Module, $List))
}

Write-Host '各模块的项目内依赖声明：'
foreach ($Line in $Summary)
{
    Write-Host "  $Line"
}

if ($Violations.Count -gt 0)
{
    Write-Host ''
    Write-Host ("发现 {0} 处模块边界违规：" -f $Violations.Count) -ForegroundColor Red
    foreach ($Item in $Violations)
    {
        Write-Host "  $Item" -ForegroundColor Red
    }
    exit 1
}

Write-Host ''
Write-Host '模块边界检查通过：五模块依赖方向无越界。' -ForegroundColor Green
exit 0

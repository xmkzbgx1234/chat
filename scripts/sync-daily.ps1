[CmdletBinding()]
param(
    [string]$SourceRoot = "F:\code\clion\chat\chat1\chat2",
    [string]$TargetRoot = "F:\code\clion\chat\chat",
    [string[]]$Items,
    [switch]$Stage,
    [string]$CommitMessage,
    [switch]$Push,
    [string]$GiteeRemote = "origin",
    [string]$GitHubUrl = "https://github.com/xmkzbgx1234/chat.git"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-PathExists {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "$Name not found: $Path"
    }
}

function Copy-ItemFromSource {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RelativePath
    )

    $sourcePath = Join-Path $SourceRoot $RelativePath
    $targetPath = Join-Path $TargetRoot $RelativePath

    if (-not (Test-Path -LiteralPath $sourcePath)) {
        throw "Source item not found: $RelativePath"
    }

    $sourceItem = Get-Item -LiteralPath $sourcePath
    if ($sourceItem.PSIsContainer) {
        if (-not (Test-Path -LiteralPath $targetPath)) {
            New-Item -ItemType Directory -Path $targetPath | Out-Null
        }

        Get-ChildItem -LiteralPath $sourcePath -Force | ForEach-Object {
            Copy-Item -LiteralPath $_.FullName -Destination $targetPath -Recurse -Force
        }
    }
    else {
        $targetDir = Split-Path -Parent $targetPath
        if ($targetDir -and -not (Test-Path -LiteralPath $targetDir)) {
            New-Item -ItemType Directory -Path $targetDir | Out-Null
        }

        Copy-Item -LiteralPath $sourcePath -Destination $targetPath -Force
    }

    Write-Host "Synced $RelativePath"
}

Assert-PathExists -Path $SourceRoot -Name "Source root"
Assert-PathExists -Path $TargetRoot -Name "Target root"
Assert-PathExists -Path (Join-Path $TargetRoot ".git") -Name "Git repository"

if (-not $Items -or $Items.Count -eq 0) {
    throw "Provide at least one relative path with -Items, for example: -Items src include README.md"
}

foreach ($item in $Items) {
    Copy-ItemFromSource -RelativePath $item
}

if ($Stage -or $CommitMessage -or $Push) {
    git -C $TargetRoot status --short
}

$shouldStage = $Stage -or [bool]$CommitMessage

if ($shouldStage) {
    foreach ($item in $Items) {
        git -C $TargetRoot add -- $item
    }
}

if ($CommitMessage) {
    git -C $TargetRoot commit -m $CommitMessage
}

if ($Push) {
    $branch = (git -C $TargetRoot branch --show-current).Trim()
    if (-not $branch) {
        throw "Unable to determine current branch"
    }

    git -C $TargetRoot push $GiteeRemote $branch
    git -C $TargetRoot push $GitHubUrl $branch
}

git -C $TargetRoot status --short --branch

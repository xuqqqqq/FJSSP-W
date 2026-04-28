param(
    [Parameter(Mandatory = $true)]
    [string]$RunTag,
    [int]$WorkerPid = 0,
    [int]$PollSeconds = 300
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $Root

$WatcherLog = Join-Path $Root "results\layer23_watcher_$RunTag.log"
$StatusPath = Join-Path $Root "results\layer23_status_$RunTag.txt"
$PidPath = Join-Path $Root "results\layer23_pid_$RunTag.txt"
$RunLogPath = Join-Path $Root "results\layer23_run_$RunTag.log"

function Write-WatcherLog {
    param([string]$Message)
    $Line = "[$(Get-Date -Format s)] $Message"
    Add-Content -Path $WatcherLog -Value $Line -Encoding UTF8
    Write-Output $Line
}

function Get-StatusText {
    if (Test-Path $StatusPath) {
        return (Get-Content $StatusPath -Raw)
    }
    return ""
}

function Test-WorkerAlive {
    if ($WorkerPid -le 0) {
        return $false
    }
    return $null -ne (Get-Process -Id $WorkerPid -ErrorAction SilentlyContinue)
}

function Invoke-Git {
    param([string[]]$Arguments)
    Write-WatcherLog ("COMMAND: git " + ($Arguments -join " "))
    & git @Arguments 2>&1 | Tee-Object -FilePath $WatcherLog -Append
    if ($LASTEXITCODE -ne 0) {
        throw "git exited with code $LASTEXITCODE for: $($Arguments -join ' ')"
    }
}

Write-WatcherLog "Watching Layer 2/3 run_tag=$RunTag worker_pid=$WorkerPid poll_seconds=$PollSeconds"

while ($true) {
    $StatusText = Get-StatusText
    if ($StatusText.StartsWith("DONE") -or $StatusText.StartsWith("FAILED")) {
        Write-WatcherLog "Observed terminal status: $StatusText"
        break
    }

    if ($WorkerPid -gt 0 -and -not (Test-WorkerAlive)) {
        Write-WatcherLog "Worker process is no longer alive before terminal status. status='$StatusText'"
        break
    }

    Start-Sleep -Seconds $PollSeconds
}

# Give the runner a short window to flush final log writes after status update.
Start-Sleep -Seconds 10
$FinalStatus = Get-StatusText
$IsDone = $FinalStatus.StartsWith("DONE")

$ResultFiles = @(
    "results\layer23_pid_$RunTag.txt",
    "results\layer23_status_$RunTag.txt",
    "results\layer23_run_$RunTag.log",
    "results\layer23_watcher_$RunTag.log",
    "results\layer3_ablation_awls_$RunTag.csv",
    "results\layer3_ablation_summary_$RunTag.csv",
    "results\layer3_ablation_summary_$RunTag.md",
    "results\layer2_public402_awls_$RunTag.csv",
    "results\layer2_public402_summary_$RunTag.csv",
    "results\layer2_public402_summary_$RunTag.md"
) | Where-Object { Test-Path $_ }

if ($ResultFiles.Count -eq 0) {
    Write-WatcherLog "No result files found to commit."
    exit 0
}

Invoke-Git (@("add") + $ResultFiles)
$Porcelain = & git status --porcelain -- $ResultFiles
if ([string]::IsNullOrWhiteSpace(($Porcelain -join "`n"))) {
    Write-WatcherLog "No staged changes to commit."
    exit 0
}

if ($IsDone) {
    $Subject = "Record Layer 2 and 3 benchmark results $RunTag"
    $Body = "The unattended Layer 3 ablation and Layer 2 public benchmark run completed and its raw CSVs, summaries, status, and logs are preserved for paper-table generation."
    $Tested = "Layer 3 and Layer 2 runner completed with terminal DONE status"
}
else {
    $Subject = "Preserve partial Layer 2 and 3 benchmark results $RunTag"
    $Body = "The unattended Layer 2/3 benchmark run stopped before a DONE status, so the watcher preserves partial raw CSVs, status, and logs for recovery instead of discarding useful completed runs."
    $Tested = "Watcher observed terminal or stopped worker state and preserved available artifacts"
}

Invoke-Git @(
    "commit",
    "-m", $Subject,
    "-m", $Body,
    "-m", "Constraint: Long benchmark runs must preserve raw and partial results`nConfidence: medium`nScope-risk: narrow`nDirective: Do not overwrite these run-tagged artifacts; start a new run tag for reruns`nTested: $Tested`nNot-tested: Manual scientific review of all resulting makespans before commit"
)
Invoke-Git @("push", "origin", "main")
Write-WatcherLog "Watcher finished."

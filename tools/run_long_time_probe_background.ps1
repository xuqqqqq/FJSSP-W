param(
    [string]$RunTag = "",
    [string]$TasksCsv = "results\layer2_long_time_probe_tasks_2026-04-29_161000.csv",
    [int]$TimeLimitSec = 1200,
    [int]$TimeoutExtraSec = 300
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $Root

if ([string]::IsNullOrWhiteSpace($RunTag)) {
    $RunTag = Get-Date -Format "yyyy-MM-dd_HHmmss"
}

$RawCsv = "results\layer2_long_time_probe_${TimeLimitSec}s_$RunTag.csv"
$SummaryCsv = "results\layer2_long_time_probe_${TimeLimitSec}s_summary_$RunTag.csv"
$StatusPath = "results\layer2_long_time_probe_status_$RunTag.txt"
$LogPath = "results\layer2_long_time_probe_run_$RunTag.log"

function Write-RunLog {
    param([string]$Message)
    $Line = "[$(Get-Date -Format s)] $Message"
    Add-Content -Path $LogPath -Value $Line -Encoding UTF8
    Write-Output $Line
}

function Write-Status {
    param([string]$Status)
    $Status | Set-Content -Path $StatusPath -Encoding UTF8
}

function Invoke-Logged {
    param([string]$Exe, [string[]]$Arguments)
    Write-RunLog ("COMMAND: $Exe " + ($Arguments -join " "))
    & $Exe @Arguments 2>&1 | Tee-Object -FilePath $LogPath -Append
    if ($LASTEXITCODE -ne 0) {
        throw "$Exe exited with code $LASTEXITCODE for: $($Arguments -join ' ')"
    }
}

function Commit-RunArtifacts {
    param([string]$Subject, [string]$Body, [string]$Tested)

    $Artifacts = @($TasksCsv, $RawCsv, $SummaryCsv, $StatusPath, $LogPath) | Where-Object { Test-Path $_ }
    if ($Artifacts.Count -eq 0) {
        Write-RunLog "No long-time probe artifacts found to commit."
        return
    }

    & git add @Artifacts
    if ($LASTEXITCODE -ne 0) { throw "git add failed for long-time probe artifacts" }
    $Porcelain = & git status --porcelain -- $Artifacts
    if ([string]::IsNullOrWhiteSpace(($Porcelain -join "`n"))) {
        Write-RunLog "No staged long-time probe changes to commit."
        return
    }

    & git commit -m $Subject -m $Body -m "Constraint: Long-time probe uses the same selected source/seed pairs as the 10-second baseline comparison`nConfidence: medium`nScope-risk: narrow`nDirective: Use the summary CSV to decide whether gaps are time-budget or neighborhood/model limitations`nTested: $Tested`nNot-tested: Full 402-instance 1200-second campaign"
    if ($LASTEXITCODE -ne 0) { throw "git commit failed for long-time probe artifacts" }
    & git push origin main
    if ($LASTEXITCODE -ne 0) { throw "git push failed for long-time probe artifacts" }
}

try {
    Write-Status "RUNNING $(Get-Date -Format s)"
    Write-RunLog "Starting Layer 2 long-time probe. run_tag=$RunTag tasks=$TasksCsv time_limit_sec=$TimeLimitSec timeout_extra_sec=$TimeoutExtraSec"

    Invoke-Logged "python" @(
        "tools\run_awls_task_list.py",
        "--tasks-csv", $TasksCsv,
        "--output-csv", $RawCsv,
        "--time-limit-sec", "$TimeLimitSec",
        "--timeout-extra-sec", "$TimeoutExtraSec"
    )
    Invoke-Logged "python" @(
        "tools\summarize_long_time_probe.py",
        "--tasks-csv", $TasksCsv,
        "--probe-csv", $RawCsv,
        "--output-csv", $SummaryCsv
    )

    Write-Status "DONE $(Get-Date -Format s)"
    Write-RunLog "Layer 2 long-time probe completed."
    Commit-RunArtifacts `
        "Record Layer 2 long-time probe $RunTag" `
        "Five representative public FJSSP-W instances were rerun with an extended time budget to separate time-budget effects from algorithmic limitations." `
        "Completed run_awls_task_list.py and summarize_long_time_probe.py"
}
catch {
    Write-Status "FAILED $(Get-Date -Format s): $($_.Exception.Message)"
    Write-RunLog "FAILED: $($_.Exception.Message)"
    Commit-RunArtifacts `
        "Preserve partial Layer 2 long-time probe $RunTag" `
        "The long-time probe stopped before completion, so available artifacts are preserved for diagnosis." `
        "Failure handler preserved available long-time probe artifacts"
    throw
}

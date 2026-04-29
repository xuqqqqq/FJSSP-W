param(
    [string]$RunTag = "",
    [string]$PreviousTag = "2026-04-28_192734",
    [int]$TimeLimitSec = 10,
    [string]$TasksCsv = "results\layer2_public402_remaining_tasks_2026-04-28_192734.csv"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $Root

if ([string]::IsNullOrWhiteSpace($RunTag)) {
    $RunTag = Get-Date -Format "yyyy-MM-dd_HHmmss"
}

$PreviousRaw = "results\layer2_public402_awls_$PreviousTag.csv"
$ResumeRaw = "results\layer2_public402_resume_$RunTag.csv"
$CompleteRaw = "results\layer2_public402_awls_complete_$RunTag.csv"
$SummaryCsv = "results\layer2_public402_summary_$RunTag.csv"
$SummaryMd = "results\layer2_public402_summary_$RunTag.md"
$StatusPath = "results\layer2_resume_status_$RunTag.txt"
$LogPath = "results\layer2_resume_run_$RunTag.log"

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

    $Artifacts = @(
        $ResumeRaw,
        $CompleteRaw,
        $SummaryCsv,
        $SummaryMd,
        $StatusPath,
        $LogPath
    ) | Where-Object { Test-Path $_ }

    if ($Artifacts.Count -eq 0) {
        Write-RunLog "No resume artifacts found to commit."
        return
    }

    Write-RunLog "Staging resume artifacts for commit."
    & git add @Artifacts
    if ($LASTEXITCODE -ne 0) {
        throw "git add failed for resume artifacts"
    }
    $Porcelain = & git status --porcelain -- $Artifacts
    if ([string]::IsNullOrWhiteSpace(($Porcelain -join "`n"))) {
        Write-RunLog "No staged changes to commit."
        return
    }

    Write-RunLog "Committing resume artifacts: $Subject"
    & git add $LogPath
    if ($LASTEXITCODE -ne 0) {
        throw "git add failed for resume log"
    }
    & git @(
        "commit",
        "-m", $Subject,
        "-m", $Body,
        "-m", "Constraint: Layer 2 was resumed from a preserved exact seed-task list`nConfidence: high`nScope-risk: narrow`nDirective: Use $CompleteRaw as the full Layer 2 raw result for this resumed run`nTested: $Tested`nNot-tested: Manual scientific review of every individual makespan"
    )
    if ($LASTEXITCODE -ne 0) {
        throw "git commit failed for resume artifacts"
    }
    & git push origin main
    if ($LASTEXITCODE -ne 0) {
        throw "git push failed for resume artifacts"
    }
}

try {
    Write-Status "RUNNING $(Get-Date -Format s)"
    Write-RunLog "Resuming Layer 2 remaining tasks. run_tag=$RunTag previous_tag=$PreviousTag time_limit_sec=$TimeLimitSec"

    Invoke-Logged "python" @(
        "tools\run_awls_task_list.py",
        "--tasks-csv", $TasksCsv,
        "--output-csv", $ResumeRaw,
        "--time-limit-sec", "$TimeLimitSec"
    )
    Invoke-Logged "python" @(
        "tools\merge_awls_results.py",
        "--inputs", $PreviousRaw, $ResumeRaw,
        "--output-csv", $CompleteRaw
    )
    Invoke-Logged "python" @(
        "tools\summarize_layer_results.py",
        "--manifest", "experiments\layer2_public402.csv",
        "--results-csv", $CompleteRaw,
        "--output-csv", $SummaryCsv,
        "--output-md", $SummaryMd
    )

    Write-Status "DONE $(Get-Date -Format s)"
    Write-RunLog "Layer 2 resume completed."
    Commit-RunArtifacts `
        "Complete resumed Layer 2 public benchmark $RunTag" `
        "The saved remaining-task list was run to completion, then merged with the preserved partial Layer 2 raw result to produce a complete 402-instance public benchmark result and summary tables." `
        "Remaining 130 seed tasks completed; merged Layer 2 raw result and generated full summary"
}
catch {
    Write-Status "FAILED $(Get-Date -Format s): $($_.Exception.Message)"
    Write-RunLog "FAILED: $($_.Exception.Message)"
    Commit-RunArtifacts `
        "Preserve partial resumed Layer 2 run $RunTag" `
        "The resumed Layer 2 run stopped before completion, so available resume artifacts are preserved for recovery rather than being left only in the working tree." `
        "Resume script caught failure and preserved available artifacts"
    throw
}

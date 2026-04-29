param(
    [string]$RunTag = "",
    [string]$PreviousTag = "2026-04-29_143104",
    [int]$TimeLimitSec = 10,
    [int]$TimeoutExtraSec = 900
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $Root

if ([string]::IsNullOrWhiteSpace($RunTag)) {
    $RunTag = Get-Date -Format "yyyy-MM-dd_HHmmss"
}

$PreviousRaw = "results\layer2_public402_awls_complete_$PreviousTag.csv"
$RetryTasks = "results\layer2_public402_timeout_tasks_$RunTag.csv"
$RetryRaw = "results\layer2_public402_timeout_retry_$RunTag.csv"
$CompleteRaw = "results\layer2_public402_awls_timeout_retry_complete_$RunTag.csv"
$SummaryCsv = "results\layer2_public402_timeout_retry_summary_$RunTag.csv"
$SummaryMd = "results\layer2_public402_timeout_retry_summary_$RunTag.md"
$StatusPath = "results\layer2_timeout_retry_status_$RunTag.txt"
$LogPath = "results\layer2_timeout_retry_run_$RunTag.log"

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
        $RetryTasks,
        $RetryRaw,
        $CompleteRaw,
        $SummaryCsv,
        $SummaryMd,
        $StatusPath,
        $LogPath
    ) | Where-Object { Test-Path $_ }

    if ($Artifacts.Count -eq 0) {
        Write-RunLog "No retry artifacts found to commit."
        return
    }

    Write-RunLog "Staging timeout retry artifacts for commit."
    & git add @Artifacts
    if ($LASTEXITCODE -ne 0) {
        throw "git add failed for timeout retry artifacts"
    }
    $Porcelain = & git status --porcelain -- $Artifacts
    if ([string]::IsNullOrWhiteSpace(($Porcelain -join "`n"))) {
        Write-RunLog "No staged changes to commit."
        return
    }

    Write-RunLog "Committing timeout retry artifacts: $Subject"
    & git add $LogPath
    if ($LASTEXITCODE -ne 0) {
        throw "git add failed for timeout retry log"
    }
    & git @(
        "commit",
        "-m", $Subject,
        "-m", $Body,
        "-m", "Constraint: Retry must run exact timed-out source/seed pairs sequentially with a larger external process guard`nConfidence: high`nScope-risk: narrow`nDirective: Use $CompleteRaw as the Layer 2 raw result after timeout retry`nTested: $Tested`nNot-tested: Manual scientific review of every retried makespan"
    )
    if ($LASTEXITCODE -ne 0) {
        throw "git commit failed for timeout retry artifacts"
    }
    & git push origin main
    if ($LASTEXITCODE -ne 0) {
        throw "git push failed for timeout retry artifacts"
    }
}

try {
    Write-Status "RUNNING $(Get-Date -Format s)"
    Write-RunLog "Retrying Layer 2 TIMEOUT tasks. run_tag=$RunTag previous_tag=$PreviousTag time_limit_sec=$TimeLimitSec timeout_extra_sec=$TimeoutExtraSec"

    Invoke-Logged "python" @(
        "tools\build_awls_retry_tasks.py",
        "--manifest", "experiments\layer2_public402.csv",
        "--results-csv", $PreviousRaw,
        "--output-csv", $RetryTasks,
        "--status", "TIMEOUT"
    )
    Invoke-Logged "python" @(
        "tools\run_awls_task_list.py",
        "--tasks-csv", $RetryTasks,
        "--output-csv", $RetryRaw,
        "--time-limit-sec", "$TimeLimitSec",
        "--timeout-extra-sec", "$TimeoutExtraSec"
    )
    Invoke-Logged "python" @(
        "tools\merge_awls_results.py",
        "--inputs", $PreviousRaw, $RetryRaw,
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
    Write-RunLog "Layer 2 TIMEOUT retry completed."
    Commit-RunArtifacts `
        "Complete Layer 2 timeout retry $RunTag" `
        "The timed-out Layer 2 source/seed pairs were rerun sequentially with a larger external timeout guard, then merged back into the full public-402 result and summarized." `
        "Timeout retry runner completed and generated merged raw plus summary"
}
catch {
    Write-Status "FAILED $(Get-Date -Format s): $($_.Exception.Message)"
    Write-RunLog "FAILED: $($_.Exception.Message)"
    Commit-RunArtifacts `
        "Preserve partial Layer 2 timeout retry $RunTag" `
        "The Layer 2 timeout retry stopped before completion, so available retry artifacts are preserved for recovery." `
        "Timeout retry script caught failure and preserved available artifacts"
    throw
}

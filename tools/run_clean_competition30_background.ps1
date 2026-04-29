param(
    [string]$RunTag = "",
    [int]$TimeLimitSec = 300,
    [string]$Seeds = "1009,1013,1019,1021,1031,1033,1039,1049,1051,1061",
    [int]$TimeoutExtraSec = 120
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $Root

if ([string]::IsNullOrWhiteSpace($RunTag)) {
    $RunTag = Get-Date -Format "yyyy-MM-dd_HHmmss"
}

$Manifest = "experiments\layer1_competition30.csv"
$RawCsv = "results\competition30_clean_awls_${TimeLimitSec}s_$RunTag.csv"
$SummaryCsv = "results\competition30_clean_awls_${TimeLimitSec}s_summary_$RunTag.csv"
$SummaryMd = "results\competition30_clean_awls_${TimeLimitSec}s_summary_$RunTag.md"
$StatusPath = "results\competition30_clean_awls_status_$RunTag.txt"
$LogPath = "results\competition30_clean_awls_run_$RunTag.log"

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
    param([string]$Subject, [string]$Body, [string]$Tested, [string]$NotTested)

    $Artifacts = @($RawCsv, $SummaryCsv, $SummaryMd, $StatusPath, $LogPath) | Where-Object { Test-Path $_ }
    if ($Artifacts.Count -eq 0) {
        Write-RunLog "No clean competition-30 artifacts found to commit."
        return
    }

    & git add @Artifacts
    if ($LASTEXITCODE -ne 0) { throw "git add failed for clean competition-30 artifacts" }
    $Porcelain = & git status --porcelain -- $Artifacts
    if ([string]::IsNullOrWhiteSpace(($Porcelain -join "`n"))) {
        Write-RunLog "No staged clean competition-30 changes to commit."
        return
    }

    & git commit -m $Subject -m $Body -m "Constraint: Clean run reads only official .fjs instance files and never uses external submission schedules as initial solutions`nConfidence: high`nScope-risk: narrow`nDirective: Do not add baseline/submission CSV files to this clean pipeline; compare only against official LB/UB here`nTested: $Tested`nNot-tested: $NotTested"
    if ($LASTEXITCODE -ne 0) { throw "git commit failed for clean competition-30 artifacts" }
    & git push origin main
    if ($LASTEXITCODE -ne 0) { throw "git push failed for clean competition-30 artifacts" }
}

try {
    Write-Status "RUNNING $(Get-Date -Format s)"
    Write-RunLog "Starting clean competition-30 AWLS run. tag=$RunTag time_limit_sec=$TimeLimitSec seeds=$Seeds timeout_extra_sec=$TimeoutExtraSec"

    Invoke-Logged "python" @(
        "tools\run_awls_layer.py",
        "--manifest", $Manifest,
        "--output-csv", $RawCsv,
        "--time-limit-sec", "$TimeLimitSec",
        "--seeds", $Seeds,
        "--timeout-extra-sec", "$TimeoutExtraSec"
    )
    Invoke-Logged "python" @(
        "tools\summarize_layer_results.py",
        "--manifest", $Manifest,
        "--results-csv", $RawCsv,
        "--output-csv", $SummaryCsv,
        "--output-md", $SummaryMd,
        "--clean-mode"
    )

    Write-Status "DONE $(Get-Date -Format s)"
    Write-RunLog "Clean competition-30 AWLS run completed."
    Commit-RunArtifacts `
        "Record clean competition-30 AWLS run $RunTag" `
        "The competition-30 benchmark was rerun from original instance files only, with no imported schedules or external submission warm starts." `
        "Completed run_awls_layer.py and clean summarize_layer_results.py" `
        "No independent schedule validator beyond AWLS return status"
}
catch {
    Write-Status "FAILED $(Get-Date -Format s): $($_.Exception.Message)"
    Write-RunLog "FAILED: $($_.Exception.Message)"
    Commit-RunArtifacts `
        "Preserve partial clean competition-30 AWLS run $RunTag" `
        "The clean competition-30 run stopped before completion, so available artifacts are preserved for diagnosis." `
        "Failure handler preserved available clean-run artifacts" `
        "Full 30-instance clean campaign did not finish"
    throw
}

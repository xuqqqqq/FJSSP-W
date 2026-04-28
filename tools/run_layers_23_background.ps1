param(
    [int]$TimeLimitSec = 10,
    [string]$Seeds = "1009,1013,1019,1021,1031,1033,1039,1049,1051,1061",
    [string]$RunTag = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $Root

if ([string]::IsNullOrWhiteSpace($RunTag)) {
    $RunTag = Get-Date -Format "yyyy-MM-dd_HHmmss"
}

$ResultsDir = Join-Path $Root "results"
New-Item -ItemType Directory -Force -Path $ResultsDir | Out-Null

$LogPath = Join-Path $ResultsDir "layer23_run_$RunTag.log"
$StatusPath = Join-Path $ResultsDir "layer23_status_$RunTag.txt"

function Write-RunLog {
    param([string]$Message)
    $Line = "[$(Get-Date -Format s)] $Message"
    Add-Content -Path $LogPath -Value $Line -Encoding UTF8
    Write-Output $Line
}

function Invoke-LoggedPython {
    param([string[]]$Arguments)
    Write-RunLog ("COMMAND: python " + ($Arguments -join " "))
    & python @Arguments 2>&1 | Tee-Object -FilePath $LogPath -Append
    if ($LASTEXITCODE -ne 0) {
        throw "python exited with code $LASTEXITCODE for: $($Arguments -join ' ')"
    }
}

function Write-Status {
    param([string]$Status)
    $Status | Set-Content -Path $StatusPath -Encoding UTF8
}

$Layer3Raw = "results\layer3_ablation_awls_$RunTag.csv"
$Layer3SummaryCsv = "results\layer3_ablation_summary_$RunTag.csv"
$Layer3SummaryMd = "results\layer3_ablation_summary_$RunTag.md"
$Layer2Raw = "results\layer2_public402_awls_$RunTag.csv"
$Layer2SummaryCsv = "results\layer2_public402_summary_$RunTag.csv"
$Layer2SummaryMd = "results\layer2_public402_summary_$RunTag.md"

try {
    Write-Status "RUNNING layer3 started $(Get-Date -Format s)"
    Write-RunLog "Starting Layer 3 ablation representatives. time_limit_sec=$TimeLimitSec seeds=$Seeds"
    Invoke-LoggedPython @(
        "tools\run_awls_layer.py",
        "--manifest", "experiments\layer3_ablation_representatives.csv",
        "--output-csv", $Layer3Raw,
        "--time-limit-sec", "$TimeLimitSec",
        "--seeds", $Seeds
    )
    Invoke-LoggedPython @(
        "tools\summarize_layer_results.py",
        "--manifest", "experiments\layer3_ablation_representatives.csv",
        "--results-csv", $Layer3Raw,
        "--output-csv", $Layer3SummaryCsv,
        "--output-md", $Layer3SummaryMd
    )

    Write-Status "RUNNING layer2 started $(Get-Date -Format s)"
    Write-RunLog "Starting Layer 2 public 402 benchmark. time_limit_sec=$TimeLimitSec seeds=$Seeds"
    Invoke-LoggedPython @(
        "tools\run_awls_layer.py",
        "--manifest", "experiments\layer2_public402.csv",
        "--output-csv", $Layer2Raw,
        "--time-limit-sec", "$TimeLimitSec",
        "--seeds", $Seeds
    )
    Invoke-LoggedPython @(
        "tools\summarize_layer_results.py",
        "--manifest", "experiments\layer2_public402.csv",
        "--results-csv", $Layer2Raw,
        "--output-csv", $Layer2SummaryCsv,
        "--output-md", $Layer2SummaryMd
    )

    Write-Status "DONE $(Get-Date -Format s)"
    Write-RunLog "Finished Layer 3 and Layer 2 runs."
}
catch {
    Write-Status "FAILED $(Get-Date -Format s): $($_.Exception.Message)"
    Write-RunLog "FAILED: $($_.Exception.Message)"
    throw
}

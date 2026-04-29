param(
    [int]$IntervalMinutes = 5,
    [int]$MaxIterations = 0,
    [string]$RunTag = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $Root

if ([string]::IsNullOrWhiteSpace($RunTag)) {
    $RunTag = Get-Date -Format "yyyy-MM-dd_HHmmss"
}

$AutomationDir = Join-Path $Root ".omx\automation"
New-Item -ItemType Directory -Force -Path $AutomationDir | Out-Null

$PidPath = Join-Path $AutomationDir "clean_awls_optimizer_pid_$RunTag.txt"
$LatestPath = Join-Path $AutomationDir "clean_awls_optimizer_latest_tag.txt"
$StdoutPath = Join-Path $AutomationDir "clean_awls_optimizer_launcher_$RunTag.out.log"
$StderrPath = Join-Path $AutomationDir "clean_awls_optimizer_launcher_$RunTag.err.log"

$Args = @(
    "-NoProfile",
    "-ExecutionPolicy", "Bypass",
    "-File", (Join-Path $PSScriptRoot "run_clean_autonomous_optimizer_loop.ps1"),
    "-IntervalMinutes", "$IntervalMinutes",
    "-MaxIterations", "$MaxIterations",
    "-RunTag", "$RunTag"
)

$Process = Start-Process -FilePath powershell -ArgumentList $Args -WindowStyle Hidden -PassThru -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath
$Process.Id | Set-Content -Path $PidPath -Encoding UTF8
$RunTag | Set-Content -Path $LatestPath -Encoding UTF8

Write-Output "Started clean autonomous optimizer tag=$RunTag pid=$($Process.Id)"
Write-Output "Status: $AutomationDir\clean_awls_optimizer_status_$RunTag.txt"
Write-Output "Loop log: $AutomationDir\clean_awls_optimizer_loop_$RunTag.log"

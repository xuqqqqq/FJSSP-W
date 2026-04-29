param(
    [int]$IntervalMinutes = 5,
    [int]$MaxIterations = 0,
    [string]$RunTag = ""
)

$ErrorActionPreference = "Continue"
Set-StrictMode -Version Latest

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $Root

if ([string]::IsNullOrWhiteSpace($RunTag)) {
    $RunTag = Get-Date -Format "yyyy-MM-dd_HHmmss"
}

$AutomationDir = Join-Path $Root ".omx\automation"
New-Item -ItemType Directory -Force -Path $AutomationDir | Out-Null

$LoopLog = Join-Path $AutomationDir "clean_awls_optimizer_loop_$RunTag.log"
$StatusPath = Join-Path $AutomationDir "clean_awls_optimizer_status_$RunTag.txt"
$LastPromptPath = Join-Path $AutomationDir "clean_awls_optimizer_last_prompt.md"
$LockPath = Join-Path $AutomationDir "clean_awls_optimizer.lock"

function Write-RunLog {
    param([string]$Message)
    $Line = "[$(Get-Date -Format s)] $Message"
    Add-Content -Path $LoopLog -Value $Line -Encoding UTF8
    Write-Output $Line
}

function Write-Status {
    param([string]$Message)
    $Message | Set-Content -Path $StatusPath -Encoding UTF8
}

function Test-ProcessAlive {
    param([int]$ProcessId)
    return $null -ne (Get-Process -Id $ProcessId -ErrorAction SilentlyContinue)
}

if (Test-Path $LockPath) {
    $Existing = Get-Content $LockPath -ErrorAction SilentlyContinue | Select-Object -First 1
    $ExistingPid = 0
    if ([int]::TryParse($Existing, [ref]$ExistingPid) -and (Test-ProcessAlive $ExistingPid)) {
        Write-RunLog "Another clean optimizer loop is already running with pid=$ExistingPid; exiting."
        exit 0
    }
}

$PID | Set-Content -Path $LockPath -Encoding UTF8

try {
    $Iteration = 0
    while ($true) {
        $Iteration++
        $IterationTag = Get-Date -Format "yyyy-MM-dd_HHmmss"
        $PromptPath = Join-Path $AutomationDir "clean_awls_optimizer_prompt_$IterationTag.md"
        $CodexLog = Join-Path $AutomationDir "clean_awls_optimizer_codex_$IterationTag.log"
        $CodexFinal = Join-Path $AutomationDir "clean_awls_optimizer_final_$IterationTag.md"

        Write-Status "RUNNING iteration=$Iteration tag=$IterationTag started=$(Get-Date -Format s)"
        Write-RunLog "Starting autonomous optimization iteration $Iteration tag=$IterationTag"

        $Prompt = @"
你是这个仓库的自治优化代理。工作目录：
C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\FJSSP-W-Benchmarking-main

任务：继续优化 AWLS 求解器，先解决 FJSSP-W 比赛 30 个算例。

硬约束：
- 严禁使用任何别人提交的解、submission CSV、atpts_final_submission.csv 或 StartTimes/MachineAssignments/WorkerAssignments 作为初解、warm start、修复对象。
- 只能从官方 .fjs 算例自行构造解；可以用官方 LB/UB 作为评测指标。
- 任何可行代码版本都必须编译、运行 clean probe、提交并 push 到 origin main。
- 不要删除或回滚用户/其他代理的未跟踪文件；旧的 untracked probe 文件可以忽略。
- 如果当前工作区有别人正在改的文件，先读懂再协作，不要 reset/checkout。

当前已知事实：
- clean 30 例 10 秒三 seed 结果在 results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv，30/30 OK，但只有 2/30 到 UB。
- 最新诊断：Kacem4 60 秒可以到 UB；BrandimarteMk12/Fattahi20 小幅改善；DPpaulli15 60 秒仍远高于 UB。
- 大例的主要短板很可能是 worker-aware 析取图/邻域不完整，而不是再借外部初解。
- 重点实例优先级：DPpaulli15/18/1/9、Hurinkrdata38、Hurinksdata38/40/54、BrandimarteMk12、Fattahi20。

本轮请做一个小而可验证的推进：
1. 先阅读最新 clean summary 和 AWLS 相关代码，选择一个明确改动方向。
2. 优先实现 worker-aware 邻域、合法性或初解/多起点调度的一个小改动；不要大面积重写。
3. 编译：MSBuild AWLS\AWLS.vcxproj Release x64。
4. 至少跑一个 clean targeted probe，包含一个小例回归（如 Kacem3/Kacem4）和一个差距大例（如 DPpaulli15、BrandimarteMk12 或 Fattahi20）。
5. 写 results/autonomous_iteration_$IterationTag.md，说明改了什么、clean 结果如何、下一轮目标。
6. 如果结果不退化或提供了有价值诊断，按 Lore Commit Protocol 提交并 push。提交信息必须说明 Tested/Not-tested。

如果你判断本轮不应该改代码，就运行一个新的 clean 诊断实验，提交结果和 autonomous_iteration 报告。
不要在最终消息里只给计划，必须执行到有证据或明确失败。
"@

        $Prompt | Set-Content -Path $PromptPath -Encoding UTF8
        Copy-Item -Path $PromptPath -Destination $LastPromptPath -Force

        try {
            & omx exec `
                --dangerously-bypass-approvals-and-sandbox `
                -C "$Root" `
                -c 'model_reasoning_effort="high"' `
                -o "$CodexFinal" `
                "$Prompt" *> "$CodexLog"

            $ExitCode = $LASTEXITCODE
            Write-RunLog "Iteration $Iteration finished with exit_code=$ExitCode final=$CodexFinal log=$CodexLog"
        }
        catch {
            Write-RunLog "Iteration $Iteration threw: $($_.Exception.Message)"
        }

        if ($MaxIterations -gt 0 -and $Iteration -ge $MaxIterations) {
            Write-Status "DONE iteration=$Iteration completed=$(Get-Date -Format s)"
            Write-RunLog "Reached MaxIterations=$MaxIterations; exiting."
            break
        }

        Write-Status "SLEEPING iteration=$Iteration next_after_minutes=$IntervalMinutes at=$(Get-Date -Format s)"
        Write-RunLog "Sleeping $IntervalMinutes minute(s) before next autonomous iteration."
        Start-Sleep -Seconds ([Math]::Max(1, $IntervalMinutes) * 60)
    }
}
finally {
    Remove-Item -Path $LockPath -Force -ErrorAction SilentlyContinue
}

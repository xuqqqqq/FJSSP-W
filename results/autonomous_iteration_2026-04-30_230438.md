# Autonomous iteration 2026-04-30 23:04:38

## Scope
Small AWLS change focused on large-instance worker-aware search throughput for the competition 30 set.

## What changed
- Added a worker-aware **cross-machine candidate shortlist** in `AWLS/TabuSearch.cpp`.
- Added `machine_change_shortlist_size` in `AWLS/TabuSearch.h` and enabled it only for larger FJSSP-W instances (`has_worker_flexibility`, `op_num >= 250`, `relative_machine_flexibility >= 0.2`).
- For those large instances, `CHANGE_MACHINE_FRONT/BACK` now probes only the 2 best alternate machines per critical op, ranked by the operation's best worker-aware duration on that machine.
- Small instances keep the previous exhaustive behavior.

## Why this direction
The current bottleneck on large FJSSP-W cases is likely neighborhood explosion rather than initialization. `CHANGE_MACHINE_*` was still enumerating every alternate machine and every insertion position, while worker-change probing already had large-instance shortlists. This patch reduces the cross-machine branching factor without changing construction or using any external warm start.

## Build
- Command: `MSBuild AWLS\\AWLS.vcxproj Release x64`
- Executed via: `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe`
- Result: success, 0 errors, 0 warnings.

## Clean targeted probe
Manifest: `experiments/layer1_competition30.csv`
Instances: `Kacem4`, `DPpaulli15`
Seeds: `1009,1013,1019`
Time limit: `20s`

### Baseline before change
- Raw: `results/awls_targeted_pre_2026-04-30_230438.csv`
- Summary: `results/awls_targeted_pre_2026-04-30_230438_summary.csv`
- `Kacem4`: best 12, avg 12
- `DPpaulli15`: best 3122, avg 3284.667

### Final result after change
- Raw: `results/awls_targeted_post3_2026-04-30_230438.csv`
- Summary: `results/awls_targeted_post3_2026-04-30_230438_summary.csv`
- `Kacem4`: best 12, avg 12
- `DPpaulli15`: best 3061, avg 3126

### Delta
- `Kacem4`: unchanged.
- `DPpaulli15`: best improved by **61**, average improved by **158.667**.

## Interpretation
The shortlist did not hurt the small regression case and materially improved the large weak instance in this clean 3-seed probe, which supports the hypothesis that large-instance search is over-spending time on cross-machine branching.

## Next target
1. Apply the same idea to **insertion-position screening** on the target machine, not just alternate-machine screening.
2. Re-probe `DPpaulli18`, `DPpaulli9`, and one Hurink large case (`Hurinkrdata38` or `Hurinksdata38`) to see whether the gain generalizes.
3. If gains persist, rerun a broader clean competition30 slice around the current weak set.

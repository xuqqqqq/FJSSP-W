# Autonomous iteration 2026-04-30 23:28:28

## Scope
- Re-read `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv` and the latest kept AWLS machine-change shortlist patch in `AWLS/TabuSearch.cpp` / `AWLS/TabuSearch.h`.
- Chose one small follow-up: keep the new cross-machine shortlist, and add a **worker-aware insertion-position shortlist** for `CHANGE_MACHINE_*` on larger FJSSP-W instances.
- Stayed fully clean: all runs below used only official `.fjs` instances from `experiments/layer1_competition30.csv`. No submission CSV, `atpts_final_submission.csv`, `StartTimes`, `MachineAssignments`, or `WorkerAssignments` data was used.

## What changed
Tracked source changes in `AWLS/TabuSearch.cpp` and `AWLS/TabuSearch.h`:
- added `machine_change_position_shortlist_size` alongside the existing large-instance `machine_change_shortlist_size` gate;
- for the same large worker-aware bucket (`has_worker_flexibility`, `op_num >= 250`, `relative_machine_flexibility >= 0.2`), limited per-machine `CHANGE_MACHINE_*` insertion probes to **4 scored positions** instead of every insertion point;
- scored candidate positions by a local estimate built from:
  - job predecessor readiness,
  - target-machine predecessor completion time,
  - worker-aware best duration on the candidate machine,
  - and a successor-slack overrun penalty;
- always kept the front insertion and the tail insertion in the shortlist, then filled the remaining budget with the best-scored positions.

## Why this direction
- The previous kept patch (`results/autonomous_iteration_2026-04-30_230438.md`) already showed that large FJSSP-W search was over-spending time on cross-machine branching.
- The remaining `CHANGE_MACHINE_*` loop still evaluated **every insertion position** on each shortlisted target machine, and each evaluation pays the full legality + schedule rebuild path.
- This patch is still small and local, but adds the missing **position-level screening** instead of another worker-block rewrite.

## Build
- Command: `MSBuild AWLS\AWLS.vcxproj Release x64`
- Executed via: `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe`
- Result: success, 0 errors, 0 warnings.

## Clean targeted probe
- Manifest: `experiments/layer1_competition30.csv`
- Seeds: `1009,1013,1019`
- Time limit: `20s`
- Instances:
  - small regression guard: `Kacem4`
  - hard DP target: `DPpaulli15`
  - second large worker-aware guard: `Hurinkrdata38`

### Baseline before change
- Raw: `results/awls_targeted_pre_2026-04-30_232828.csv`
- Summary: `results/awls_targeted_pre_2026-04-30_232828_summary.csv`
- `Kacem4`: best `12`, avg `12`
- `DPpaulli15`: best `3085`, avg `3129.333`
- `Hurinkrdata38`: best `1967`, avg `2293`

### Result after change
- Raw: `results/awls_targeted_post_2026-04-30_232828.csv`
- Summary: `results/awls_targeted_post_2026-04-30_232828_summary.csv`
- `Kacem4`: best `12`, avg `12`
- `DPpaulli15`: best `2676`, avg `2794`
- `Hurinkrdata38`: best `1967`, avg `2293`

### Delta
- `Kacem4`: unchanged.
- `DPpaulli15`: best improved by **409**; average improved by **335.333**.
- `Hurinkrdata38`: unchanged.

## Interpretation
- The new position shortlist produced a **material clean gain on DPpaulli15** without hurting the small regression guard or the extra large worker-aware guard.
- That supports the current hypothesis that the large-instance bottleneck is still neighborhood explosion, now specifically inside target-machine insertion-position enumeration.
- The unchanged `Hurinkrdata38` readout suggests this pass is at least safe on another large worker-flexible case, even if the immediate gain was concentrated on the DP instance.

## Next target
1. Re-probe this kept direction on `DPpaulli18`, `DPpaulli9`, and one non-DP weak case such as `BrandimarteMk12` or `Fattahi20`.
2. If the gain generalizes, rerun a broader clean slice of the priority weak set before another full 30-instance campaign.
3. If the gain stays DP-specific, refine the position scoring signal rather than reopening broad worker-block pruning.

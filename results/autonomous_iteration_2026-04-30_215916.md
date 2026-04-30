# Autonomous iteration 2026-04-30 21:59:16

## Scope
- Re-read `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv` and the current worker-aware neighborhood flow in `AWLS/TabuSearch.cpp`.
- Chose one small worker-aware neighborhood-budget refinement: remove duplicate `CHANGE_WORKER` evaluations inside a single `TabuSearch::find_move(...)` pass, so the same `(operation, worker)` swap is not re-scored multiple times when it appears via both machine-critical and worker-critical blocks.
- Stayed fully clean: the probe below used only official `.fjs` instances. No submission CSV / `atpts_final_submission.csv` / `StartTimes` / `MachineAssignments` / `WorkerAssignments` data was used.

## Why this direction
- The current AWLS search already injects extra worker moves from machine-critical blocks, and it also enumerates worker-critical-block changes.
- On worker-flexible instances that means the same `CHANGE_WORKER` move can be surfaced more than once in one move-generation pass.
- That looked like a good bounded target: keep the neighborhood content conceptually the same, but stop duplicate worker swaps from being re-evaluated and implicitly over-weighted.

## Temporary code change tested
Temporary patch in `AWLS/TabuSearch.cpp` (discarded after the probe):
- added a per-pass `seen_worker_moves` set keyed by `(op, worker)`
- routed both machine-block and worker-block `CHANGE_WORKER` generation through that dedupe gate
- logged the number of duplicate worker-move skips when tracing is enabled

I reverted this code after the probe because the clean targeted results regressed on the hard instances.

## Baseline used for comparison
Current tracked solver code before this experiment matched the latest clean targeted quartet already recorded on `main`:
- `results/awls_targeted_post_2026-04-30_214120_summary.csv`
  - `DPpaulli15`: best `3517`, avg `3798.333`
  - `DPpaulli18`: best `3487`, avg `3797`
  - `Kacem4`: best `12`, avg `12.667`
  - `Fattahi20`: best `1260`, avg `1374`

## Build
- Experimental build:
  - `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe AWLS\AWLS.vcxproj /p:Configuration=Release /p:Platform=x64`
  - result: success
- Final rebuild after reverting tracked source:
  - same command
  - result: success

## Clean targeted probe
- Raw result: `results/awls_targeted_post_2026-04-30_215916.csv`
- Summary: `results/awls_targeted_post_2026-04-30_215916_summary.csv`
- Markdown summary: `results/awls_targeted_post_2026-04-30_215916_summary.md`
- Seeds: `1009,1013,1019`
- Time limit: `10s`
- Instances:
  - small regression guard: `Kacem4`
  - large-gap DP targets: `DPpaulli15`, `DPpaulli18`
  - non-DP guard: `Fattahi20`

### Readout vs baseline
From `results/awls_targeted_post_2026-04-30_215916_summary.csv`:
- `DPpaulli15`: best `3698`, avg `3815.667` (**worse** than baseline; best `+181`, avg `+17.334`)
- `DPpaulli18`: best `3529`, avg `3875` (**worse** than baseline; best `+42`, avg `+78`)
- `Kacem4`: best `12`, avg `12` (**best unchanged, avg improved by `-0.667`**)
- `Fattahi20`: best `1330`, avg `1371.667` (**best much worse by `+70`, avg slightly better by `-2.333`**)

## Decision
- I **did not keep the code change**.
- The duplicate-worker-move filter was too aggressive as a main-line change: it removed useful search behavior on both DP targets and especially on `Fattahi20` best quality.
- Tracked AWLS source was restored to the pre-iteration baseline and rebuilt successfully.

## Diagnosis gained
- The repeated appearance of the same `CHANGE_WORKER` move is not just wasted effort; it is also acting like a **move-weighting mechanism** in the current stochastic search.
- Removing that repetition reduced exploration quality on both `DPpaulli15/18` and `Fattahi20`, which suggests the current solver is partly relying on duplicate worker moves to reinforce high-impact reassignment candidates.
- That means future worker-aware cleanup should be more selective:
  - avoid blunt dedupe of worker moves across all sources,
  - and prefer explicit structural weighting / ordering signals over deleting repeated worker probes outright.

## Final code state
- No tracked AWLS solver source changes were kept.
- Final compiled executable is the baseline solver rebuilt after reverting the temporary dedupe patch.

## Next target
1. Keep the current baseline worker-move multiplicity on `main`.
2. If the next iteration stays in the neighborhood layer, convert this into a **selective weighting signal** instead of a global dedupe (for example distinguish machine-block-origin vs worker-block-origin worker swaps rather than collapsing them).
3. Re-probe against the same clean quartet before widening back to `BrandimarteMk12` / `Hurink*` / full competition30.

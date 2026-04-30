# Autonomous iteration 2026-04-30 20:56:09

## Scope
- Re-read `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv` and the current AWLS worker-aware neighborhood code in `AWLS/TabuSearch.cpp` / `AWLS/TabuSearch.h`.
- Chose one narrow direction: test whether the existing worker-change shortlist budget should target the large, high-flex `DPpaulli` family more aggressively.
- All runs below were clean: official `.fjs` instances only, no submission CSV / warm start / repair data.

## What I tested
### Baseline
- Current `origin/main` / `main` before any edits.
- Evidence:
  - raw: `results/awls_targeted_pre_2026-04-30_205609.csv`
  - summary: `results/awls_targeted_pre_2026-04-30_205609_summary.csv`

### Variant A — tighten large high-flex worker probes, including worker-block moves
- Temporary code change in `AWLS/TabuSearch.h` / `AWLS/TabuSearch.cpp`.
- For instances with `has_worker_flexibility`, `op_num >= 250`, and relative machine flexibility `>= 0.25`:
  - machine-block extra `CHANGE_WORKER` shortlist budget `2/3 -> 1/2`
  - worker-critical-block `CHANGE_WORKER` enumeration also replaced by the same bounded shortlist logic
- Evidence:
  - raw: `results/awls_targeted_post_2026-04-30_205609.csv`
  - summary: `results/awls_targeted_post_2026-04-30_205609_summary.csv`

### Variant B — tighten only the existing machine-block extra worker shortlist gate
- Temporary code change in `AWLS/TabuSearch.h` only.
- Same structural trigger as Variant A, but **no worker-block gating**: only the existing machine-block extra worker probes were tightened from `2/3 -> 1/2`.
- Evidence:
  - raw: `results/awls_targeted_post2_2026-04-30_205609.csv`
  - summary: `results/awls_targeted_post2_2026-04-30_205609_summary.csv`

## Why this direction
- The current clean 30-instance summary still shows very large gaps on `DPpaulli15` / `DPpaulli18`.
- The large `DPpaulli` cases are also the only competition30 instances that simultaneously satisfy:
  - many operations (`>= 250`), and
  - high machine flexibility (`>= 0.25` in `experiments/layer1_competition30.csv`).
- That made them a good target for a small worker-aware neighborhood-budget experiment without touching low-flex guards like `Fattahi20`.

## Build
- Command:
  - `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe AWLS\AWLS.vcxproj /p:Configuration=Release /p:Platform=x64`
- Result:
  - success for the tested variants

## Clean targeted probe
- Manifest: `experiments/layer1_competition30.csv`
- Seeds: `1009,1013,1019`
- Time limit: `10s`
- Instances:
  - small regression guard: `Kacem4`
  - DP-family hard cases: `DPpaulli15`, `DPpaulli18`
  - low-flex guard: `Fattahi20`

### Baseline
- `DPpaulli15`: best `3436`, avg `3642`, UB gap `+1365`
- `DPpaulli18`: best `3549`, avg `3834`, UB gap `+1480`
- `Kacem4`: best `12`, avg `12.667`, UB gap `+1`
- `Fattahi20`: best `1260`, avg `1374`, UB gap `+113`

### Variant A (machine-block + worker-block tightening)
- `DPpaulli15`: best `3437`, avg `3657.667`, UB gap `+1366`
- `DPpaulli18`: best `3427`, avg `3727.667`, UB gap `+1358`
- `Kacem4`: best `12`, avg `12.667`, UB gap `+1`
- `Fattahi20`: best `1260`, avg `1374`, UB gap `+113`
- Readout:
  - strong gain on `DPpaulli18`
  - clear regression on `DPpaulli15`
  - guards stayed flat

### Variant B (machine-block tightening only)
- `DPpaulli15`: best `3428`, avg `3641`, UB gap `+1357`
- `DPpaulli18`: best `3558`, avg `3832`, UB gap `+1489`
- `Kacem4`: best `12`, avg `12.667`, UB gap `+1`
- `Fattahi20`: best `1260`, avg `1374`, UB gap `+113`
- Readout:
  - slight `DPpaulli15` improvement (`best -8`, `avg -1`)
  - `DPpaulli18` best regressed slightly (`+9`), average improved only marginally (`-2`)
  - guards stayed flat

## Decision
- I **did not keep either code variant**.
- Variant A is too family-inconsistent inside `DPpaulli` itself.
- Variant B is safer, but the gain is too small and too uneven to justify changing `main` yet.
- Current tracked code was restored to the pre-experiment baseline before closing the iteration.

## Diagnosis gained
- Large high-flex `DPpaulli` instances do respond to worker-neighborhood budget changes, but **`DPpaulli15` and `DPpaulli18` do not react in the same direction**.
- Tightening worker-critical-block `CHANGE_WORKER` moves appears to help `DPpaulli18` more than `DPpaulli15`.
- Tightening only the extra machine-block worker shortlist is safer, but currently too weak to be a worthwhile keep.
- This points to a more specific missing signal than just "large + high-flex" — likely something about critical-block shape, worker-duration spread, or move acceptance mix.

## Final code state
- No tracked source changes kept.
- Current code remains at the baseline search behavior from `main`.

## Next target
1. Instrument or approximate a stronger **per-block / per-op signal** before tightening worker neighborhoods again (for example worker-duration spread or block length, not just instance-level flexibility).
2. Re-probe `DPpaulli15`, `DPpaulli18`, and `DPpaulli9` together so the next gate is judged at the family level instead of on a single instance.
3. If that signal still looks unstable, pivot to a worker-aware initialization / multistart tweak instead of more neighborhood-budget pruning.

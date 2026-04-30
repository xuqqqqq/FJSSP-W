# Autonomous iteration 2026-04-30 21:16:48

## Scope
- Re-read `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv` plus the current AWLS worker-aware neighborhood code in `AWLS/TabuSearch.cpp` / `AWLS/TabuSearch.h`.
- Chose one narrow next step from the latest diagnosis: add a **per-op / per-block signal** before tightening worker-critical-block `CHANGE_WORKER` probes on the large-gap `DPpaulli` family.
- Stayed fully clean: every run below used official `.fjs` instances only, with no submission CSV / StartTimes / MachineAssignments / WorkerAssignments warm start or repair.

## What I tested
### Baseline references used for comparison
- `results/awls_targeted_pre_2026-04-30_205609_summary.csv`
  - `DPpaulli15`: best `3436`, avg `3642`
  - `DPpaulli18`: best `3549`, avg `3834`
  - `Kacem4`: best `12`, avg `12.667`
  - `Fattahi20`: best `1260`, avg `1374`
- `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv`
  - `DPpaulli9`: best `2935`, avg `3154`

### Variant A — worker-block signal gating with bounded endpoint/middle shortlists
Temporary code change in `AWLS/TabuSearch.h` / `AWLS/TabuSearch.cpp`:
- only for worker-aware instances with `op_num >= 250` and relative machine flexibility `>= 0.25`
- worker-critical blocks:
  - endpoints probed with a bounded shortlist
  - middle ops probed only when `has_strong_worker_alternative(...)` is true
- machine-block logic stayed unchanged

Evidence:
- raw: `results/awls_targeted_post_2026-04-30_211648.csv`
- summary: `results/awls_targeted_post_2026-04-30_211648_summary.csv`

### Variant B — preserve full endpoint enumeration, shortlist only strong-signal middle ops
Second temporary change on top of Variant A:
- worker-critical block endpoints restored to full alternative-worker enumeration
- only middle ops with strong worker-speed signal kept the bounded shortlist

Evidence:
- raw: `results/awls_targeted_post2_2026-04-30_211648.csv`
- summary: `results/awls_targeted_post2_2026-04-30_211648_summary.csv`

## Why this direction
- The previous round (`results/autonomous_iteration_2026-04-30_205609.md`) showed that blunt instance-level worker-block tightening helped `DPpaulli18` but hurt `DPpaulli15`.
- That suggested the next small step should be a **more local signal** than just “large + high-flex”: keep worker-block pruning tied to block position and worker-duration advantage.
- This stays inside the existing worker-aware neighborhood instead of rewriting AWLS search.

## Build
- Command:
  - `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe AWLS\AWLS.vcxproj /p:Configuration=Release /p:Platform=x64`
- Result:
  - success for both temporary variants
  - success again after reverting tracked source back to the baseline main-line code

## Clean targeted probe
- Seeds: `1009,1013,1019`
- Time limit: `10s`
- Instances:
  - small regression guard: `Kacem4`
  - DP-family hard cases: `DPpaulli15`, `DPpaulli18`, `DPpaulli9`
  - non-DP guard: `Fattahi20`

### Variant A readout
From `results/awls_targeted_post_2026-04-30_211648_summary.csv`:
- `DPpaulli15`: best `3491`, avg `3787.667` (**worse than baseline**; best `+55`, avg `+145.667`)
- `DPpaulli18`: best `3457`, avg `3779.333` (**better**; best `-92`, avg `-54.667`)
- `DPpaulli9`: best `2745`, avg `3042.333` (**better vs clean30 reference**; best `-190`, avg `-111.667`)
- `Kacem4`: best `12`, avg `12.667` (**unchanged**)
- `Fattahi20`: best `1260`, avg `1374` (**unchanged**)

### Variant B readout
From `results/awls_targeted_post2_2026-04-30_211648_summary.csv`:
- `DPpaulli15`: best `3531`, avg `3803` (**worse than Variant A and baseline**)
- `DPpaulli18`: best `3657`, avg `3886` (**worse than Variant A and baseline**)
- `DPpaulli9`: best `2784`, avg `3165.667` (**worse than Variant A; avg slightly above clean30 reference**)
- `Kacem4`: best `12`, avg `12.667` (**unchanged**)
- `Fattahi20`: best `1260`, avg `1374` (**unchanged**)

## Decision
- I **did not keep either source-code variant**.
- Variant A produced useful positive signal on `DPpaulli18/9`, but the `DPpaulli15` regression is too large to keep on `main`.
- Variant B removed too much search power and regressed the whole tested DP subset.
- Tracked solver code was reverted to the baseline after the experiment; only clean diagnostic artifacts are kept.

## Diagnosis gained
- A per-op worker-speed signal is more promising than the previous blunt instance-level worker-block pruning: it can improve `DPpaulli18` and `DPpaulli9` without touching `Kacem4` / `Fattahi20`.
- But `DPpaulli15` still needs richer worker-block exploration than `DPpaulli18/9`; the family is **not internally uniform**.
- The bad Variant B result strongly suggests the helpful part was not “fewer worker-block moves” alone, but specifically the mix of:
  - keeping some endpoint behavior,
  - pruning weak middle ops, and
  - preserving strong-signal worker swaps.

## Final code state
- No tracked AWLS source changes kept.
- Current tracked code is back at the baseline main-line solver; only result artifacts and this report were added.

## Next target
1. Add a stronger local signal before touching worker-block pruning again — likely block length and/or worker-duration spread, not just endpoint-vs-middle.
2. Re-probe `DPpaulli15/18/9` together again, but only after separating the `DPpaulli15` behavior from the other two.
3. If that still stays split, pivot the next small iteration to worker-aware initialization / multistart rather than more worker-block pruning.

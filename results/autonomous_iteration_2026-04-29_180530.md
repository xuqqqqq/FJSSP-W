# Autonomous iteration 2026-04-29 18:05:30

## Scope
- Re-read `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv` and the latest autonomous notes.
- Re-read the worker-aware search code in `AWLS/TabuSearch.cpp` together with the timing/legality flow in `AWLS/Schedule.cpp`.
- Chose one small worker-aware neighborhood expansion instead of changing initialization or rewriting the whole search.

## Code change
- File: `AWLS/TabuSearch.cpp`
- Added `collect_worker_shortlist(...)` to rank alternative workers for a critical operation on its current machine by processing time and keep only a tiny shortlist.
- Extended machine-critical block exploration so each critical machine-block operation can also try up to two `CHANGE_WORKER` moves, even when it is not currently part of a worker critical block.
- Kept the existing full worker-block neighborhood intact; this patch only fills a gap where machine-critical operations previously had no bounded worker reassignment probe.

## Why this direction
- The clean 30-instance summary still shows the biggest gaps on worker-flexible large instances, especially `DPpaulli15`.
- Current AWLS already has worker-aware scheduling and worker-block moves, but machine-critical operations can still miss useful worker reassignments unless the worker chain itself becomes critical.
- This patch is a bounded way to make the neighborhood more worker-aware without enabling unstable large neighborhoods or touching external warm starts.

## Build
- Command: `& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' AWLS\AWLS.vcxproj /p:Configuration=Release /p:Platform=x64`
- Result: success (`AWLS\x64\Release\AWLS.exe` rebuilt).

## Clean targeted probe
- Manifest: `experiments/layer2_public402.csv`
- Seeds: `1009,1013,1019`
- Time limit: `10s`
- Instances: `Kacem4`, `DPpaulli15`

### Pre-change summary
- Raw: `results/awls_targeted_pre_2026-04-29_180530.csv`
- Summary: `results/awls_targeted_pre_2026-04-29_180530_summary.csv`
- `Kacem4`: best `13`, avg `13`, UB gap `+2`
- `DPpaulli15`: best `3784`, avg `3927`, UB gap `+1713`

### Post-change summary
- Raw: `results/awls_targeted_post_2026-04-29_180530.csv`
- Summary: `results/awls_targeted_post_2026-04-29_180530_summary.csv`
- `Kacem4`: best `12`, avg `12.667`, UB gap `+1`
- `DPpaulli15`: best `3717`, avg `3947.667`, UB gap `+1646`

### Readout
- Small regression guard improved slightly instead of regressing: `Kacem4` best `13 -> 12`.
- `DPpaulli15` improved on two seeds and regressed on one:
  - `1009`: `4191 -> 4095`
  - `1013`: `3806 -> 3717`
  - `1019`: `3784 -> 4031`
- Net effect on the hard case is mixed but still useful:
  - best: `3784 -> 3717`
  - UB gap: `+1713 -> +1646`
  - average: `3927 -> 3947.667` (worse)
- Diagnosis: bounded worker reassignment inside machine-critical neighborhoods can unlock better incumbents, but it also increases search noise. The next step should stabilize when these extra worker moves are attempted, rather than expanding neighborhood size again.

## Next target
- Gate the new worker shortlist by stronger signals (for example only on machine-block endpoints or only when worker duration spread is large), then re-probe `DPpaulli15` plus one additional large-gap instance such as `BrandimarteMk12` or `Fattahi20`.

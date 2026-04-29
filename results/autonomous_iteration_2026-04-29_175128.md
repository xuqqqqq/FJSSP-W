# Autonomous iteration 2026-04-29 17:51:28

## Scope
- Re-read `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv` and kept the focus on worker-flexible large-gap instances, with `DPpaulli15` as the primary hard case and `Kacem4` as the small regression guard.
- Read the current AWLS neighborhood/timing code in `AWLS/Schedule.cpp`, `AWLS/TabuSearch.cpp`, and `AWLS/Graph.cpp`.
- Chose one small worker-aware legality/scheduling refinement instead of changing the whole search loop.

## Code change
- File: `AWLS/Schedule.cpp`
- Change 1: added `select_contextual_worker(...)`, which chooses the feasible worker that gives the earliest local completion time for an operation on its current machine, using the current job predecessor, machine predecessor, and already-built worker chain state.
- Change 2: for `CHANGE_MACHINE_FRONT` / `CHANGE_MACHINE_BACK`, `Schedule::make_move` now clears the moved operation's worker assignment to `-1` so `update_time()` can re-pick a worker with the new machine context instead of forcing the static shortest-duration worker.
- Why: the previous patch made machine-change moves worker-aware, but it still chose the destination worker purely by per-machine processing time. On FJSSP-W instances that can be too myopic when the fastest worker is busy and another feasible worker would finish earlier under the current worker chain.

## Build
- Command: `& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' AWLS\AWLS.vcxproj /p:Configuration=Release /p:Platform=x64`
- Result: success (`AWLS\x64\Release\AWLS.exe` rebuilt).

## Clean targeted probe
- Manifest: `experiments/layer2_public402.csv`
- Seeds: `1009,1013,1019`
- Time limit: `10s`
- Instances: `Kacem4`, `DPpaulli15`

### Pre-change summary
- Raw: `results/awls_targeted_pre_2026-04-29_175128.csv`
- Summary: `results/awls_targeted_pre_2026-04-29_175128_summary.csv`
- `Kacem4`: best `13`, avg `13`, UB gap `+2`
- `DPpaulli15`: best `3809`, avg `3955.333`, UB gap `+1738`

### Post-change summary
- Raw: `results/awls_targeted_post_2026-04-29_175128.csv`
- Summary: `results/awls_targeted_post_2026-04-29_175128_summary.csv`
- `Kacem4`: best `13`, avg `13`, UB gap `+2`
- `DPpaulli15`: best `3790`, avg `3951`, UB gap `+1719`

### Readout
- Small regression case stayed flat: `Kacem4` remained `13` on all three seeds.
- Hard gap case improved slightly in aggregate:
  - best: `3809 -> 3790`
  - average: `3955.333 -> 3951`
  - UB gap: `+1738 -> +1719`
- Seed-level behavior was mixed (`4120 -> 4236`, `3809 -> 3827`, `3937 -> 3790`), so this patch appears directionally useful but still noisy. The main value is diagnostic: context-aware worker choice after machine moves can help, but it is not yet strong enough to stabilize large-instance improvement by itself.

## Next target
- Keep this direction small: extend worker-aware move evaluation so machine-critical operations can test a short worker shortlist beyond pure worker-block moves, or add lightweight tabu control for `CHANGE_WORKER` oscillation before trying broader search rewrites.

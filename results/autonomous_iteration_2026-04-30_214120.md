# Autonomous iteration 2026-04-30 21:41:20

## Scope
- Re-read `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv` and the current AWLS initialization / multistart path in `AWLS/Solver.cpp`, `AWLS/Graph.cpp`, and `AWLS/Schedule.cpp`.
- Chose one small direction: test whether **one forced random construction tail start** can complement the current worker-aware greedy initialization on the hard DP-family instances, without touching neighborhood logic or using any external warm start.
- Stayed fully clean: all runs below used official `.fjs` instances only; no submission CSV / `atpts_final_submission.csv` / `StartTimes` / `MachineAssignments` / `WorkerAssignments` data was used.

## Why this direction
- The last several worker-neighborhood probes showed the same pattern: `DPpaulli18` and `DPpaulli9` can improve under tighter worker-aware search gating, but `DPpaulli15` often reacts in the opposite direction.
- The current constructor path on these large worker-aware DP instances is effectively **all greedy starts** under the existing auto policy, because they are high-flex but not in the very-large resource-parallel branch that already injects random starts.
- That made a minimal initialization-only probe reasonable: keep population size unchanged, keep TS unchanged, and change only the last construction sample to a forced random start on large high-flex worker-aware instances.

## Temporary code change tested
Discarded experiment in `AWLS/Graph.h`, `AWLS/Graph.cpp`, `AWLS/Schedule.h`, and `AWLS/Solver.cpp`:
- added a temporary `force_random_construction` path to `Graph::random_init(...)`
- for large worker-aware instances with `population_size >= 2`, `op_num >= 250`, and relative machine flexibility `>= 0.25`
- only the **last** initial population member was forced onto the random-construction branch; all earlier samples stayed on the normal auto policy

I reverted this code after the probe because the family response was still inconsistent.

## Baseline used for comparison
Current tracked solver code before this experiment matched the already-recorded clean targeted baseline:
- `results/awls_targeted_pre_2026-04-30_205609_summary.csv`
  - `DPpaulli15`: best `3436`, avg `3642`
  - `DPpaulli18`: best `3549`, avg `3834`
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
- Tasks file: `results/awls_targeted_tasks_2026-04-30_214120.csv`
- Raw result: `results/awls_targeted_post_2026-04-30_214120.csv`
- Summary: `results/awls_targeted_post_2026-04-30_214120_summary.csv`
- Markdown summary: `results/awls_targeted_post_2026-04-30_214120_summary.md`
- Seeds: `1009,1013,1019`
- Time limit: `10s`
- Instances:
  - small regression guard: `Kacem4`
  - large-gap DP targets: `DPpaulli15`, `DPpaulli18`
  - non-DP guard: `Fattahi20`

### Readout vs baseline
From `results/awls_targeted_post_2026-04-30_214120_summary.csv`:
- `DPpaulli15`: best `3517`, avg `3798.333` (**worse** than baseline; best `+81`, avg `+156.333`)
- `DPpaulli18`: best `3487`, avg `3797` (**better** than baseline; best `-62`, avg `-37`)
- `Kacem4`: best `12`, avg `12.667` (**unchanged**)
- `Fattahi20`: best `1260`, avg `1374` (**unchanged**)

## Decision
- I **did not keep the code change**.
- The forced-random tail start was too similar to the earlier worker-neighborhood diagnostics: it helped `DPpaulli18`, but clearly hurt `DPpaulli15` while leaving guards flat.
- That is valuable diagnosis, but not a safe main-line improvement.
- Tracked AWLS source was restored to the baseline state before closing the iteration.

## Diagnosis gained
- The DP-family split is not confined to worker-neighborhood pruning; it also appears in **initialization diversity**.
- A single random extra start can improve `DPpaulli18` without harming `Kacem4` / `Fattahi20`, so there is real untapped basin diversity on some large DP instances.
- But `DPpaulli15` still prefers the current baseline start mix, meaning a blunt “add more random starts” policy is not good enough.
- This points to a more selective next step:
  - either a stronger structural signal before injecting random starts (for example worker-contention or machine-load skew), or
  - a return to worker-aware neighborhood work, but with a DP15-specific signal instead of family-wide gating.

## Final code state
- No tracked AWLS source changes kept.
- Final compiled executable is the baseline solver rebuilt after reverting the temporary initialization experiment.

## Next target
1. Keep the current baseline constructor on `main`.
2. If the next iteration stays on initialization, add a **more local signal** than `op_num + flexibility` before injecting random starts.
3. Otherwise pivot back to worker-aware neighborhood structure, but judge `DPpaulli15` separately from `DPpaulli18/9` instead of assuming one family-wide gate.

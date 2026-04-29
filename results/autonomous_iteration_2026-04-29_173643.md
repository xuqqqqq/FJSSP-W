# Autonomous iteration 2026-04-29 17:36:43

## Scope
- Read `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv` and confirmed the main clean gaps are still dominated by worker-flexible large instances, especially `DPpaulli15`.
- Chose one bounded worker-aware neighborhood fix instead of a rewrite.

## Code change
- File: `AWLS/Schedule.cpp`
- Change: when a `CHANGE_MACHINE_FRONT` / `CHANGE_MACHINE_BACK` move is applied, the moved operation now immediately resets its worker assignment to `best_worker_for_machine(op, new_machine)` before recomputing times.
- Why: before this patch, a machine-change move could keep the old worker as long as it was merely feasible on the new machine, which made machine-change evaluation under-explore the worker-aware version of that move. The search was therefore scoring some machine moves with a stale worker rather than the canonical best worker for the target machine.

## Build
- Command: `& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' AWLS\AWLS.vcxproj /p:Configuration=Release /p:Platform=x64`
- Result: success (`AWLS\x64\Release\AWLS.exe` rebuilt).

## Clean targeted probe
Manifest: `experiments/layer2_public402.csv`
Seeds: `1009,1013,1019`
Time limit: `10s`
Instances: `Kacem4`, `DPpaulli15`

### Pre-change summary
Source: `results/awls_targeted_pre_2026-04-29_173643_summary.csv`
- `Kacem4`: best `12`, avg `12.667`, UB gap `+1`
- `DPpaulli15`: best `3763`, avg `3951`, UB gap `+1692`

### Post-change summary
Source: `results/awls_targeted_post_2026-04-29_173643_summary.csv`
- `Kacem4`: best `12`, avg `12.333`, UB gap `+1`
- `DPpaulli15`: best `3679`, avg `3834.667`, UB gap `+1608`

### Readout
- No regression on the small regression case (`Kacem4` best unchanged, average slightly improved).
- The hard gap case improved on all three tested seeds:
  - `1009`: `4124 -> 4031`
  - `1013`: `3763 -> 3679`
  - `1019`: `3966 -> 3794`
- This supports the hypothesis that AWLS still leaves value on the table in worker-aware machine-change neighborhoods.

## Next target
- Extend this direction one step further without a rewrite: make machine-change evaluation consider more than one worker candidate on the target machine (for example best-duration plus current-worker-if-feasible, or a tiny shortlist ranked by duration/slack) and re-probe `DPpaulli15`, `BrandimarteMk12`, and `Fattahi20`.

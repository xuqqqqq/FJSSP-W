# Autonomous iteration 2026-04-29 18:38:17

## Scope
- Re-read `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv` plus the current worker-aware AWLS search path in `AWLS/TabuSearch.cpp`.
- Chose one small worker-aware neighborhood change: keep endpoint `CHANGE_WORKER` probes on machine-critical blocks, but also allow non-endpoint probes when an operation has a clearly faster feasible worker alternative.
- Stayed fully clean: all probes were run from official `.fjs` instances only, with no submission CSV / StartTimes / MachineAssignments / WorkerAssignments warm start.

## Code change
- File: `AWLS/TabuSearch.cpp`
- Added `has_strong_worker_alternative(...)` to detect when the current worker is materially slower than another feasible worker for the same `(operation, machine)`.
- Expanded the machine-critical-block worker probe from:
  - **before:** endpoints only, shortlist size `2`
  - **after:** endpoints always, plus non-endpoints with a strong worker-duration signal; those non-endpoints get shortlist size `3`
- Kept worker-block probing and machine-change logic unchanged.

## Why this direction
- The current solver already has worker-aware worker-change moves, but the machine-block neighborhood still misses many obvious worker upgrades inside long critical machine blocks.
- A full “probe every middle op” expansion had been too noisy earlier; this patch adds back only the cases with an explicit speed signal.
- This is a bounded worker-aware completeness tweak, not a rewrite.

## Build
- Requested command shape: `MSBuild AWLS\AWLS.vcxproj Release x64`
- `MSBuild` was not on `PATH`, so I used:
  - `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe AWLS\AWLS.vcxproj /p:Configuration=Release /p:Platform=x64`
- Result: success, `AWLS\x64\Release\AWLS.exe` rebuilt.

## Clean targeted probe
- Manifest: `experiments/layer1_competition30.csv`
- Seeds: `1009,1013,1019`
- Time limit: `10s`
- Instances:
  - small regression: `Kacem4`
  - hard gaps: `DPpaulli15`, `Fattahi20`

### Baseline on current `main` before this patch
- Raw: `results/awls_targeted_pre_2026-04-29_183817.csv`
- Summary: `results/awls_targeted_pre_2026-04-29_183817_summary.csv`
- `DPpaulli15`: best `3639`, avg `3812.667`, UB gap `+1568`
- `Kacem4`: best `12`, avg `12.667`, UB gap `+1`
- `Fattahi20`: best `1269`, avg `1378`, UB gap `+122`

### Discarded variant (not kept)
- Raw: `results/awls_targeted_post2_2026-04-29_183817.csv`
- Summary: `results/awls_targeted_post2_2026-04-29_183817_summary.csv`
- Tried adding an extra large-instance gate on the new middle-of-block worker probe.
- Result: worse than the simpler strong-signal version on `DPpaulli15`, while leaving `Kacem4` and `Fattahi20` flat.
- Conclusion: the extra node-count gate removed useful worker-aware moves, so it was discarded.

### Final kept patch
- Raw: `results/awls_targeted_post_2026-04-29_183817.csv`
- Summary: `results/awls_targeted_post_2026-04-29_183817_summary.csv`
- `DPpaulli15`: best `3628`, avg `3748.667`, UB gap `+1557`
- `Kacem4`: best `12`, avg `12.667`, UB gap `+1`
- `Fattahi20`: best `1260`, avg `1374`, UB gap `+113`

## Readout
- `Kacem4` stayed flat on the small regression guard:
  - best `12 -> 12`
  - avg `12.667 -> 12.667`
- `DPpaulli15` improved on both best and average:
  - best `3639 -> 3628` (`-11`)
  - avg `3812.667 -> 3748.667` (`-64`)
  - seed breakdown:
    - `1009`: `3894 -> 3841`
    - `1013`: `3639 -> 3628`
    - `1019`: `3905 -> 3777`
- `Fattahi20` also improved slightly on the current baseline:
  - best `1269 -> 1260` (`-9`)
  - avg `1378 -> 1374` (`-4`)
- Diagnosis: there is useful worker-aware signal inside machine-critical blocks beyond endpoints, but it should stay tied to explicit worker-duration advantage instead of broad middle-block expansion.

## Next target
1. Apply the same signal-gated idea to worker-critical blocks, where all alternative workers are still explored.
2. Re-probe `BrandimarteMk12` and one Hurink/DPpaulli large-gap case with the same clean seeds.
3. If the gain persists, rerun a wider clean subset before another 30-instance pass.

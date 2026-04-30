# Autonomous iteration 2026-04-30 19:56:31

## Scope
- Re-read `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv` and the current AWLS worker-aware search path in `AWLS/TabuSearch.cpp`, `AWLS/Schedule.cpp`, `AWLS/Graph.cpp`, and `AWLS/Solver.cpp`.
- Chose one narrow direction to test: reduce noise from the existing worker-critical-block `CHANGE_WORKER` neighborhood instead of rewriting the worker-aware disjunctive graph.
- Stayed fully clean: every run used official `.fjs` instances only, with no submission CSV / StartTimes / MachineAssignments / WorkerAssignments warm start or repair.

## Direction tested
- Hypothesis: the current worker-critical-block neighborhood may still be too broad on large worker-flexible instances, so a tighter endpoint / strong-signal probe could improve 10s search quality.
- I tested two small variants in `AWLS/TabuSearch.cpp` and discarded both after clean probes:
  1. **Variant A**: for worker-critical blocks, probe only endpoints or strong middle-node signals, with shortlist-based worker alternatives.
  2. **Variant B**: keep full endpoint enumeration, but restrict non-endpoints to strong-signal shortlist probes.
- Final code state for this iteration: **reverted to the pre-iteration baseline** because both variants regressed at least one guard instance.

## Build
- Requested shape: `MSBuild AWLS\AWLS.vcxproj Release x64`
- Actual command used:
  - `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe AWLS\AWLS.vcxproj /p:Configuration=Release /p:Platform=x64`
- Result: success before and after the experiment; final baseline `AWLS\x64\Release\AWLS.exe` rebuilt cleanly.

## Clean targeted probe
- Manifest: `experiments/layer1_competition30.csv`
- Seeds: `1009,1013,1019`
- Time limit: `10s`
- Instances:
  - small regression guard: `Kacem4`
  - large-gap targets: `DPpaulli15`, `BrandimarteMk12`, `Fattahi20`

### Baseline on current main / final kept code
- Raw: `results/awls_targeted_pre_2026-04-30_195631.csv`
- Summary: `results/awls_targeted_pre_2026-04-30_195631_summary.csv`
- `BrandimarteMk12`: best `579`, avg `615.667`, UB gap `+108`
- `DPpaulli15`: best `3577`, avg `3725`, UB gap `+1506`
- `Kacem4`: best `12`, avg `12.667`, UB gap `+1`
- `Fattahi20`: best `1260`, avg `1374`, UB gap `+113`

### Discarded Variant A
- Raw: `results/awls_targeted_post_2026-04-30_195631.csv`
- Summary: `results/awls_targeted_post_2026-04-30_195631_summary.csv`
- `BrandimarteMk12`: best `576`, avg `621.667`, UB gap `+105`
- `DPpaulli15`: best `3459`, avg `3705`, UB gap `+1388`
- `Kacem4`: best `13`, avg `13`, UB gap `+2`
- `Fattahi20`: best `1399`, avg `1467.333`, UB gap `+252`
- Readout: excellent best/avg movement on `DPpaulli15` and slightly better `BrandimarteMk12` best, but unacceptable regressions on both guard families (`Kacem4`, `Fattahi20`).

### Discarded Variant B
- Raw: `results/awls_targeted_post2_2026-04-30_195631.csv`
- Summary: `results/awls_targeted_post2_2026-04-30_195631_summary.csv`
- `BrandimarteMk12`: best `582`, avg `599.333`, UB gap `+111`
- `DPpaulli15`: best `3566`, avg `3664`, UB gap `+1495`
- `Kacem4`: best `12`, avg `12.333`, UB gap `+1`
- `Fattahi20`: best `1338`, avg `1414.333`, UB gap `+191`
- Readout: softer than Variant A and better than baseline on `DPpaulli15` average and `Kacem4` average, but still clearly worse on `Fattahi20` best/avg and slightly worse on `BrandimarteMk12` best.

## Diagnosis gained
- The current worker-critical-block neighborhood is **not just noisy**; on `Fattahi20` it is still carrying useful worker reassignment opportunities that the tighter gates suppress.
- By contrast, `DPpaulli15` responds well to tighter worker-block filtering, which means the large-gap families are not aligned enough for a single simple gate to help all of them.
- This points to a more instance-structure-aware next step:
  - either gate worker-block moves by richer local structure than endpoint/processing-time alone,
  - or leave worker blocks broad and instead target machine-block / change-machine interaction, where the recent clean wins have been more stable.

## Next target
1. Keep the current baseline code and avoid merging worker-block endpoint gating.
2. Probe a more selective **machine-change + worker-aware repair** tweak, or add move-mix instrumentation to explain why `DPpaulli15` and `Fattahi20` react in opposite directions.
3. Re-test on the same clean quartet before any wider 30-instance rerun.

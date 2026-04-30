# Autonomous iteration 2026-04-30 20:32:32

## Scope
- Re-read `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv` and the current AWLS worker-aware search path in `AWLS/TabuSearch.cpp`, `AWLS/TabuSearch.h`, and `AWLS/Solver.cpp`.
- Picked one small, clean direction first, measured it, discarded it when it was too inconsistent, then kept a narrower worker-aware neighborhood refinement.
- All runs below were clean: official `.fjs` instances only, with no submission CSV / warm start / repair data.

## What I changed
### Discarded diagnostic tweak
- Tried collapsing very large, highly flexible worker-aware instances to a single construction sample in `AWLS/Solver.cpp`.
- Result: mixed inside the same family (`DPpaulli18` improved, `DPpaulli15` regressed), so I reverted it.
- Evidence:
  - baseline: `results/awls_targeted_pre_2026-04-30_203232_summary.csv`
  - discarded variant: `results/awls_targeted_post_2026-04-30_203232_summary.csv`

### Final kept tweak
- Moved the extra machine-block worker shortlist sizes from global constants into `TabuSearch` instance state.
- For **very large, highly machine-flexible worker-aware instances** only (`has_worker_flexibility`, `op_num >= 250`, `relative_machine_flexibility >= 0.45`):
  - normal extra machine-block worker probe shortlist: `2 -> 1`
  - strong-signal shortlist: `3 -> 2`
- For all other instances, the baseline shortlist sizes remain unchanged.

### Why this direction
- The earlier global tightening (`1/2` for everyone) improved `DPpaulli15` but hurt `BrandimarteMk12` and `Fattahi20`.
- Those regressions aligned with structure: `DPpaulli15` is large and very machine-flexible, while `BrandimarteMk12` and `Fattahi20` are much lower-flex.
- So the kept patch makes the cheaper worker-aware neighborhood **instance-structure-aware** instead of globally shrinking it.

## Build
- Command:
  - `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe AWLS\AWLS.vcxproj /p:Configuration=Release /p:Platform=x64`
- Result: success

## Clean targeted probe
- Manifest: `experiments/layer1_competition30.csv`
- Seeds: `1009,1013,1019`
- Time limit: `10s`
- Instances:
  - small regression guard: `Kacem4`
  - hard cases: `DPpaulli15`, `BrandimarteMk12`, `Fattahi20`

### Baseline on current main before the kept patch
Source: `results/awls_targeted_pre_2026-04-30_195631_summary.csv`
- `BrandimarteMk12`: best `579`, avg `615.667`, UB gap `+108`
- `DPpaulli15`: best `3577`, avg `3725`, UB gap `+1506`
- `Kacem4`: best `12`, avg `12.667`, UB gap `+1`
- `Fattahi20`: best `1260`, avg `1374`, UB gap `+113`

### Final kept patch
- Raw: `results/awls_targeted_post3_2026-04-30_203232.csv`
- Summary: `results/awls_targeted_post3_2026-04-30_203232_summary.csv`
- `BrandimarteMk12`: best `579`, avg `615.667`, UB gap `+108` (**unchanged**)
- `DPpaulli15`: best `3428`, avg `3640.667`, UB gap `+1357` (**best -149, avg -84.333**)
- `Kacem4`: best `12`, avg `12.667`, UB gap `+1` (**unchanged**)
- `Fattahi20`: best `1260`, avg `1374`, UB gap `+113` (**unchanged**)

## Diagnosis gained
- The extra machine-block `CHANGE_WORKER` probe is still useful on big worker-flexible families, but its budget should not be spent equally on every family.
- `DPpaulli15` benefits from a **cheaper** extra worker-aware probe mix.
- `BrandimarteMk12` and `Fattahi20` do **not** benefit from the same global tightening, so the gate should stay structural rather than universal.
- This is consistent with the repository-level hypothesis that the current search is partly neighborhood-budget-limited on large worker-aware cases.

## Final kept code state
- Kept:
  - `AWLS/TabuSearch.h`
  - `AWLS/TabuSearch.cpp`
- Reverted:
  - the temporary `AWLS/Solver.cpp` single-start experiment

## Next target
1. Re-probe the same structural shortlist gate on `DPpaulli18` and `DPpaulli9`.
2. If the DPpaulli family keeps improving without hurting `Kacem4` / `Fattahi20`, extend the guard set with `Hurinkrdata38` before any wider clean rerun.
3. After that, decide whether to keep this gate for a new clean 30-instance 10s sweep.

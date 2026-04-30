# Autonomous iteration 2026-04-30 22:50:15

## Scope
- Re-read `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv` and the current AWLS worker-aware neighborhood code in `AWLS/TabuSearch.cpp` / `AWLS/TabuSearch.h`.
- Chose one narrow, local follow-up to the previous kept gate: **keep the existing large/high-flex worker-aware shortlist gate, but make only the shortlist ordering completion-aware; leave the strong-alternative trigger duration-based.**
- Stayed fully clean: all runs below used only official `.fjs` instances from `experiments/layer1_competition30.csv`. No submission CSV, `atpts_final_submission.csv`, `StartTimes`, `MachineAssignments`, or `WorkerAssignments` data was used.

## Why this direction
- The previous diagnostic (`results/autonomous_iteration_2026-04-30_223010.md`) showed that the likely overreach was the **completion-aware strong-alternative trigger**, not necessarily the shortlist ordering itself.
- The current kept `main` code already narrows extra worker probes to very large, highly machine-flexible worker-aware instances.
- So the smallest next step was to reuse that same structural gate and test **contextual shortlist ordering only**.

## Temporary code change tested
Temporary patch in `AWLS/TabuSearch.cpp` / `AWLS/TabuSearch.h` (discarded after the probe):
- added a flag tied to the existing large/high-flex worker probe gate,
- for that gated bucket only, ranked `collect_worker_shortlist(...)` by estimated completion time in the current worker timeline instead of raw duration,
- left `has_strong_worker_alternative(...)` unchanged on the original duration-gap rule.

I reverted this source change after the probe because it did not beat the current kept targeted baselines.

## Build
- Experimental build before probe:
  - `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe AWLS\AWLS.vcxproj /p:Configuration=Release /p:Platform=x64`
  - result: success
- Final rebuild after reverting tracked source:
  - same command
  - result: success

## Clean targeted probe
- Raw result: `results/awls_targeted_post_2026-04-30_225015.csv`
- Summary CSV: `results/awls_targeted_post_2026-04-30_225015_summary.csv`
- Summary MD: `results/awls_targeted_post_2026-04-30_225015_summary.md`
- Seeds: `1009,1013,1019`
- Time limit: `10s`
- Instances:
  - small regression guard: `Kacem4`
  - large-gap DP targets: `DPpaulli15`, `DPpaulli18`
  - non-DP guard: `Fattahi20`

## Readout
From `results/awls_targeted_post_2026-04-30_225015_summary.csv`:
- `DPpaulli15`: best `3662`, avg `3756`
- `DPpaulli18`: best `3494`, avg `3819`
- `Kacem4`: best `12`, avg `12.667`
- `Fattahi20`: best `1260`, avg `1374`

Against the clean 30-instance baseline in `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv`:
- `DPpaulli15`: best improved from `3876 -> 3662`, avg improved from `4059 -> 3756`
- `DPpaulli18`: best improved from `3779 -> 3494`, avg improved from `4120 -> 3819`
- `Kacem4`: best improved from `13 -> 12`, avg improved from `13 -> 12.667`
- `Fattahi20`: unchanged versus the current kept targeted readout (`1260`, avg `1374`) and still slightly worse than the clean30 best (`1258`)

Against the current kept targeted baselines already on `main`:
- `DPpaulli15` vs `results/awls_targeted_post3_2026-04-30_203232_summary.csv`: best regressed from `3428 -> 3662`, avg regressed from `3640.667 -> 3756`
- `DPpaulli18` vs `results/awls_targeted_post_2026-04-30_214120_summary.csv`: best regressed slightly from `3487 -> 3494`, avg regressed from `3797 -> 3819`
- `Kacem4`: unchanged at best `12`, avg `12.667`
- `Fattahi20`: unchanged at best `1260`, avg `1374`

## Decision
- I **did not keep the code change**.
- The patch stayed clean and preserved the current `Kacem4` / `Fattahi20` behavior, and it remained better than the old clean30 baseline on both DP targets.
- But it failed the more important comparison against the current kept targeted baselines already on `main`, especially on `DPpaulli15`.
- Tracked AWLS solver source was restored to the kept baseline and rebuilt successfully.

## Diagnosis gained
- The previous hypothesis was too optimistic: even **without** contextual strong-alternative gating, completion-aware shortlist ordering alone is still not reliably better for the current search.
- `DPpaulli18` stayed close to the current kept result, but `DPpaulli15` regressed materially, which suggests the current worker-tail completion estimate is too crude as a shortlist-ordering signal for at least part of the DP family.
- The useful narrowing from this pass is:
  1. the structural large/high-flex gate is still the right experimental lane,
  2. but the next worker-aware move should be **more local than instance-level completion ordering**,
  3. likely using worker-critical block structure or legality/position signals rather than whole-instance worker-tail readiness.

## Final code state
- No tracked AWLS solver source changes were kept.
- Final compiled executable is the baseline solver rebuilt after reverting the temporary patch.

## Next target
1. Keep the current structural shortlist gate already on `main`.
2. Pivot away from instance-level completion ordering and test a more local worker-aware signal inside worker- or machine-critical blocks (for example block-length / worker-spread / endpoint-only refinement).
3. Keep `DPpaulli15`, `DPpaulli18`, `Kacem4`, and `Fattahi20` as the minimum clean guard set for that next pass.

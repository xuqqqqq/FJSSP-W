# Autonomous iteration 2026-04-30 22:30:10

## Scope
- Re-read `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv` and the AWLS worker-aware neighborhood helpers in `AWLS/TabuSearch.cpp` / `AWLS/TabuSearch.h`.
- Chose one small, localized direction: keep the existing large/high-flex DP-style gate from the kept 20:32 patch, but make its extra machine-block worker shortlist / strong-alternative trigger completion-aware instead of duration-only.
- Stayed fully clean: all runs below used only official `.fjs` instances. No submission CSV, `atpts_final_submission.csv`, `StartTimes`, `MachineAssignments`, or `WorkerAssignments` data was used.

## Why this direction
- The latest reverted report (`results/autonomous_iteration_2026-04-30_221546.md`) showed that completion-aware worker ranking helped `DPpaulli15` but hurt `BrandimarteMk12` when applied globally.
- The currently kept code already has a structural gate for very large, highly machine-flexible worker-aware instances in `TabuSearch`.
- So the narrowest next step was: **reuse that same gate**, keep non-DP guards on the baseline duration-first logic, and only change the DP-style branch.

## Temporary code change tested
Temporary patch in `AWLS/TabuSearch.h` / `AWLS/TabuSearch.cpp` (discarded after the probe):
- added a `use_contextual_worker_completion_ranking` flag tied to the existing large/high-flex worker-aware gate,
- ranked shortlist workers by estimated completion time in the current worker timeline instead of raw duration when that flag was active,
- let `has_strong_worker_alternative(...)` also fire on completion-time improvement before falling back to the old duration-gap rule.

I reverted this source change after the probe because it was not a clear win over the current kept main-line behavior.

## Build
- Experimental build:
  - `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe AWLS\AWLS.vcxproj /p:Configuration=Release /p:Platform=x64`
  - result: success
- Final rebuild after reverting tracked source:
  - same command
  - result: success

## Clean targeted probe
- Raw result: `results/awls_targeted_post_2026-04-30_223010.csv`
- Summary CSV: `results/awls_targeted_post_2026-04-30_223010_summary.csv`
- Summary MD: `results/awls_targeted_post_2026-04-30_223010_summary.md`
- Seeds: `1009,1013,1019`
- Time limit: `10s`
- Instances:
  - small regression guard: `Kacem4`
  - large-gap DP targets: `DPpaulli15`, `DPpaulli18`
  - non-DP hard guard: `Fattahi20`

## Readout
From `results/awls_targeted_post_2026-04-30_223010_summary.csv`:
- `DPpaulli15`: best `3458`, avg `3645.333`
- `DPpaulli18`: best `3580`, avg `3841.667`
- `Kacem4`: best `12`, avg `12.667`
- `Fattahi20`: best `1260`, avg `1374`

Against the clean 30-instance baseline in `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv`:
- `DPpaulli15`: best improved from `3876 -> 3458`, avg improved from `4059 -> 3645.333`
- `DPpaulli18`: best improved from `3779 -> 3580`, avg improved from `4120 -> 3841.667`
- `Kacem4`: best improved from `13 -> 12`, avg improved from `13 -> 12.667`
- `Fattahi20`: best improved from `1258 -> 1260` **did not improve**; avg improved from `1277.667 -> 1374` **did not improve** relative to clean30, but this comparison is not apples-to-apples with the currently kept main-line targeted baselines.

Against the current kept targeted baselines already recorded on `main`:
- `DPpaulli15` vs `results/awls_targeted_post3_2026-04-30_203232_summary.csv`: best regressed from `3428 -> 3458`, avg regressed from `3640.667 -> 3645.333`
- `DPpaulli18` vs `results/awls_targeted_post_2026-04-30_214120_summary.csv`: best regressed from `3487 -> 3580`, avg regressed from `3797 -> 3841.667`
- `Kacem4` vs the same targeted baselines: unchanged at best `12`, avg `12.667`
- `Fattahi20` vs the same targeted baselines: unchanged at best `1260`, avg `1374`

## Decision
- I **did not keep the code change**.
- The gated completion-aware ranking stayed harmless on `Kacem4` / `Fattahi20` and remained much better than the original clean30 baseline on both DP targets.
- But it still failed the more important test: it did **not** beat the current kept main-line targeted baselines on either `DPpaulli15` or `DPpaulli18`.
- Tracked AWLS solver source was restored to the current main-line baseline and rebuilt successfully.

## Diagnosis gained
- The structural gate was the right place to try contextual worker ranking: unlike the global version, it avoided the earlier `BrandimarteMk12` / `Fattahi20` style collateral damage.
- But the full gated package is still too broad for the current search:
  - `DPpaulli15` stayed close to the kept main-line best,
  - `DPpaulli18` regressed more clearly,
  - so the extra completion-aware trigger inside `has_strong_worker_alternative(...)` is the most likely overreach.
- That suggests the next worker-aware pass should be even narrower:
  1. try **contextual shortlist ordering only**, while leaving the strong-alternative trigger duration-based, or
  2. gate the completion-aware trigger by an additional local signal (for example block length or worker-duration spread) instead of turning it on for the whole large/high-flex bucket.

## Final code state
- No tracked AWLS solver source changes were kept.
- Final compiled executable is the baseline solver rebuilt after reverting the temporary patch.

## Next target
1. Keep the current structural shortlist gate already on `main`.
2. If the next pass stays in neighborhoods, test **contextual shortlist ordering without contextual strong-alternative gating** on `DPpaulli15/18`, `Kacem4`, and `Fattahi20`.
3. If that still splits the DP pair, pivot to a more local signal inside worker-critical blocks instead of another instance-level gate.

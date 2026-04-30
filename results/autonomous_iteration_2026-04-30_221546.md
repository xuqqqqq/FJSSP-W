# Autonomous iteration 2026-04-30 22:15:46

## Scope
- Re-read `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv` and the current worker-aware neighborhood helpers in `AWLS/TabuSearch.cpp`.
- Chose one narrow worker-aware neighborhood tweak: make the extra `CHANGE_WORKER` shortlist and strong-alternative trigger more contextual by ranking alternative workers by estimated completion time in the current schedule, not by raw duration alone.
- Stayed fully clean: all runs below used only official `.fjs` instances. No submission CSV, `atpts_final_submission.csv`, `StartTimes`, `MachineAssignments`, or `WorkerAssignments` data was used.

## Why this direction
- The current extra machine-block worker probes in `TabuSearch::find_move(...)` call `collect_worker_shortlist(...)` and `has_strong_worker_alternative(...)`.
- Both helpers were still duration-only:
  - shortlist ranking preferred the fastest raw worker even if that worker was already busy in the current schedule,
  - strong-alternative gating ignored whether a slower worker could still finish earlier because of worker readiness.
- That made a small, local worker-aware refinement plausible without rewriting the search loop.

## Temporary code change tested
Temporary patch in `AWLS/TabuSearch.cpp` (discarded after the probe):
- added `estimate_worker_ready_time(...)` and `estimate_worker_completion_time(...)` helpers,
- changed `collect_worker_shortlist(...)` to sort by estimated completion time first, then duration,
- changed `has_strong_worker_alternative(...)` to fire on estimated completion-time improvement before falling back to the old duration-gap rule.

I reverted this code after the probe because the hard-instance response was mixed and `BrandimarteMk12` regressed too much.

## Build
- Experimental build:
  - `MSBuild AWLS\AWLS.vcxproj /p:Configuration=Release /p:Platform=x64`
  - result: success
- Final rebuild after reverting tracked source:
  - same command
  - result: success

## Clean targeted probe
- Raw result: `results/awls_targeted_post_2026-04-30_221546.csv`
- Summary CSV: `results/awls_targeted_post_2026-04-30_221546_summary.csv`
- Summary MD: `results/awls_targeted_post_2026-04-30_221546_summary.md`
- Seeds: `1009,1013,1019`
- Time limit: `10s`
- Instances:
  - small regression guard: `Kacem4`
  - large-gap target: `DPpaulli15`
  - second hard guard: `BrandimarteMk12`

## Readout
From `results/awls_targeted_post_2026-04-30_221546_summary.csv`:
- `DPpaulli15`: best `3533`, avg `3755`
- `BrandimarteMk12`: best `564`, avg `610`
- `Kacem4`: best `12`, avg `12.333`

Against the 30-instance clean baseline in `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv`:
- `DPpaulli15`: best improved from `3876 -> 3533`, avg improved from `4059 -> 3755`
- `Kacem4`: best improved from `13 -> 12`, avg improved from `13 -> 12.333`
- `BrandimarteMk12`: best regressed from `513 -> 564`, avg regressed from `533.667 -> 610`

Compared with the latest targeted baseline already on `main` (`results/awls_targeted_post_2026-04-30_214120_summary.csv`), `DPpaulli15` best was still slightly worse (`3517 -> 3533`), so this patch did not earn a keep despite the average improvement.

## Decision
- I **did not keep the code change**.
- The completion-aware worker shortlist looked directionally useful for `DPpaulli15` and `Kacem4`, but it hurt `BrandimarteMk12` badly enough that it is not a safe main-line improvement.
- Tracked AWLS solver source was restored to the pre-iteration baseline and rebuilt successfully.

## Diagnosis gained
- Worker-aware move scoring on large DP instances benefits from more context than raw duration.
- But applying that context uniformly at the worker-shortlist/gating layer shifts search pressure too far away from the raw fast-worker alternatives that still matter on `BrandimarteMk12`.
- So the next step should probably be **instance- or structure-selective contextual ranking**, not a global replacement.

## Final code state
- No tracked AWLS source changes were kept.
- Final compiled executable is the baseline solver rebuilt after reverting the temporary patch.

## Next target
1. Keep the current baseline source on `main`.
2. If the next iteration stays in neighborhoods, restrict completion-aware worker ranking to the DP-style large-worker cases that already benefit from tighter worker search, instead of applying it globally.
3. Re-test that narrower gate against `Kacem4`, `DPpaulli15`, and one non-DP hard guard (`BrandimarteMk12` or `Fattahi20`) before any wider rerun.

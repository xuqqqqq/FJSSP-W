# Autonomous iteration 2026-04-29 18:20:33

## Scope
- Re-read `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv` and the current AWLS worker-aware search path.
- Focused on one narrow issue in `AWLS/TabuSearch.cpp`: machine-critical blocks were probing `CHANGE_WORKER` on every op in the block, which likely added too much noise on large worker-flexible instances.
- Kept the existing worker-aware construction, legality, and worker-block neighborhood intact.

## Code change
- File: `AWLS/TabuSearch.cpp`
- Tightened the extra machine-block worker probe so it only fires on machine critical-block endpoints (or both ops in a 2-op block), instead of every op inside the block.
- Kept the existing worker shortlist size and full worker-critical-block exploration unchanged.

## Why this direction
- The previous machine-block worker expansion had already shown that worker reassignment can help hard FJSSP-W cases, but it also increased search noise.
- On long critical machine blocks, middle operations generate many low-leverage worker flips; endpoints are the more standard high-impact positions for block neighborhoods.
- This is a small worker-aware neighborhood refinement, not a rewrite.

## Build
- Command: `MSBuild AWLS\AWLS.vcxproj /p:Configuration=Release /p:Platform=x64`
- Result: success (`AWLS\x64\Release\AWLS.exe` rebuilt).

## Clean targeted probe
- Manifest: `experiments/layer1_competition30.csv`
- Seeds: `1009,1013,1019`
- Time limit: `10s`
- Instances: `Kacem4`, `DPpaulli15`

### Baseline before this patch
- Raw: `results/awls_targeted_pre_2026-04-29_182033.csv`
- Summary: `results/awls_targeted_pre_2026-04-29_182033_summary.csv`
- `Kacem4`: best `12`, avg `12.667`, UB gap `+1`
- `DPpaulli15`: best `3679`, avg `3886.333`, UB gap `+1608`

### Intermediate failed experiment (not kept)
- Raw: `results/awls_targeted_post_2026-04-29_182033.csv`
- Summary: `results/awls_targeted_post_2026-04-29_182033_summary.csv`
- Tried ranking worker alternatives by a projected completion heuristic.
- Result regressed on both targets:
  - `Kacem4`: best `13`, avg `13.000`
  - `DPpaulli15`: best `3730`, avg `3974.667`
- Conclusion: that heuristic overfit local worker readiness and hurt search quality, so it was discarded.

### Final kept patch
- Raw: `results/awls_targeted_post2_2026-04-29_182033.csv`
- Summary: `results/awls_targeted_post2_2026-04-29_182033_summary.csv`
- `Kacem4`: best `12`, avg `12.667`, UB gap `+1` (no regression)
- `DPpaulli15`: best `3680`, avg `3829.667`, UB gap `+1609`

## Readout
- Small regression guard stayed flat:
  - `Kacem4` best `12 -> 12`
  - `Kacem4` avg `12.667 -> 12.667`
- Hard large-gap case improved in average quality with essentially unchanged best:
  - `DPpaulli15` best `3679 -> 3680` (`+1`, effectively flat)
  - `DPpaulli15` avg `3886.333 -> 3829.667` (`-56.666` improvement)
  - seed breakdown:
    - `1009`: `3919 -> 3901`
    - `1013`: `3679 -> 3680`
    - `1019`: `4061 -> 3908`
- Diagnosis: extra worker moves are still useful, but they need strong gating. Endpoint-only probing is noticeably more stable than probing every op in a machine critical block.

## Next target
- Apply the same “only when signal is strong” principle to the remaining worker-aware neighborhoods:
  1. gate machine-block worker probes further by duration spread or worker-chain criticality;
  2. inspect whether worker-block moves should also prioritize endpoints first;
  3. re-probe `DPpaulli15` together with `BrandimarteMk12` or `Fattahi20`.

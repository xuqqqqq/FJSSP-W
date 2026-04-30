# Autonomous iteration 2026-04-30 20:16:28

## Scope
- Re-read `results/competition30_clean_awls_10s_summary_2026-04-29_165455.csv` and the current AWLS worker-aware search path in `AWLS/TabuSearch.cpp`, `AWLS/Schedule.cpp`, and `AWLS/NeighborhoodMove.h`.
- Chose one bounded follow-up to the existing worker-aware machine-change line: test whether a tiny explicit worker probe on `CHANGE_MACHINE_*` moves can recover large-instance improvements without using any external warm start data.
- Stayed clean: all evidence below comes from official `.fjs` instances only.

## Direction tested
- Hypothesis: the current machine-change neighborhood may still miss useful worker-aware variants because each `CHANGE_MACHINE_*` move is scored only with the contextual worker picked during recomputation.
- Experimental patch (discarded):
  - temporarily extended `NeighborhoodMove` with an optional worker field,
  - let `CHANGE_MACHINE_*` moves carry one explicit fast worker on the target machine,
  - kept the existing contextual move and added one explicit-worker variant per target insertion.
- Result: **discarded and reverted**. The explicit-worker expansion increased neighborhood size but regressed every guard instance in the clean targeted quartet.

## Build
- Command:
  - `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe AWLS\AWLS.vcxproj /p:Configuration=Release /p:Platform=x64`
- Result:
  - experimental build: success
  - final kept baseline rebuild after revert: success

## Clean targeted probe
- Manifest: `experiments/layer1_competition30.csv`
- Seeds: `1009,1013,1019`
- Time limit: `10s`
- Instances:
  - small regression guard: `Kacem4`
  - large-gap targets: `BrandimarteMk12`, `DPpaulli15`, `Fattahi20`

### Baseline kept on main before this experiment
Source: `results/awls_targeted_pre_2026-04-30_195631_summary.csv`
- `BrandimarteMk12`: best `579`, avg `615.667`, UB gap `+108`
- `DPpaulli15`: best `3577`, avg `3725`, UB gap `+1506`
- `Kacem4`: best `12`, avg `12.667`, UB gap `+1`
- `Fattahi20`: best `1260`, avg `1374`, UB gap `+113`

### Discarded explicit-worker machine-change variant
- Raw: `results/awls_targeted_post_2026-04-30_201628.csv`
- Summary: `results/awls_targeted_post_2026-04-30_201628_summary.csv`
- `BrandimarteMk12`: best `595`, avg `620.333`, UB gap `+124`
- `DPpaulli15`: best `3839`, avg `3983`, UB gap `+1768`
- `Kacem4`: best `13`, avg `13.333`, UB gap `+2`
- `Fattahi20`: best `1300`, avg `1353.333`, UB gap `+153`

## Diagnosis gained
- The machine-change neighborhood is **very sensitive to evaluation count**. Even a one-worker explicit expansion was enough to hurt both the small guard (`Kacem4`) and all three large-gap instances.
- This suggests the current search is iteration-budget-limited more than worker-choice-limited inside `CHANGE_MACHINE_*`: adding more machine-change variants costs more than the extra worker specificity returns.
- That points away from broader explicit worker enumeration on machine changes, and toward either:
  1. cheaper gating / prioritization of existing worker-aware moves, or
  2. initial-solution / multistart scheduling tweaks that improve the starting basin without inflating per-iteration neighborhood cost.

## Final kept code state
- Reverted the experimental machine-change worker expansion.
- Rebuilt the baseline `AWLS\x64\Release\AWLS.exe` successfully.
- No solver code changes were kept from this iteration.

## Next target
1. Keep `CHANGE_MACHINE_*` at its current single contextual-worker scoring.
2. Probe a cheaper worker-aware improvement next: either selective move-ordering/gating inside the existing neighborhoods, or a small worker-aware multistart / initialization tweak aimed at `DPpaulli15`, `BrandimarteMk12`, and `Fattahi20`.
3. Reuse the same clean quartet before any wider rerun.

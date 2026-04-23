# ATPTS Reference Workflow

This workspace contains a verified warm-start path for improving the
reference Scenario 1 ATPTS submission under
`C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS`.

## Scripts

- `improve_submission_from_baseline.py`
  - Warm-starts the external `atpts_solver.py` from an existing submission CSV.
  - Writes an improved submission CSV plus a JSON summary.
- `improve_atpts_reference.py`
  - Warm-starts from the external per-run JSON files in `submission_results/`.

## Verified Command

The repo-local script can be run from
`C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\FJSSP-W-Benchmarking-main`
and targets the external ATPTS folder via `--atpts-dir`.

The following command was re-run successfully on April 23, 2026:

```powershell
cd C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\FJSSP-W-Benchmarking-main
python tools\improve_submission_from_baseline.py `
  --atpts-dir ..\Submission_HUST-SMART-Lab_Scenario1_ATPTS `
  --instances 6_Fattahi_14_workers `
  --seeds 7,19,29 `
  --max-evaluations 50000 `
  --max-iterations 500000 `
  --neighborhood-size 60 `
  --output-csv temp_improved_submission.csv `
  --summary-json temp_improvement_summary.json
```

Observed result:

- `6_Fattahi_14_workers: 546 -> 543`

The summary file produced by that run is:

- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS\temp_improvement_summary.json`

## Notes

- This path is currently the only reproduced strict improvement over the provided ATPTS reference baseline.
- The current AWLS baseline remains stable on the 10-instance smoke set, but it has not yet beaten the ATPTS reference on `5_Kacem_4_workers`.

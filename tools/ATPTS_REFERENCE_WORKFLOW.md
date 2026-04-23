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
- `batch_scan_reference_csv.py`
  - Runs `improve_submission_from_baseline.py` in manageable batches and collates the summaries.
  - Useful because a full 30-instance sweep in one process timed out in this workspace, while 3 to 5 instances per batch completed reliably.
- `generate_submission_from_baseline.py`
  - Generates submission-grade rows for targeted instances using 10 distinct seeds.
  - Replaces all 10 rows of each targeted instance, instead of swapping only a single best row.
- `batch_generate_submission_from_baseline.py`
  - Runs the submission-grade generator in smaller batches and merges the targeted rows back into one CSV.
  - Useful because a large 10-seed submission regeneration can still time out when too many instances are grouped together.

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
- `improve_submission_from_baseline.py` is for exploratory intensification.
- `generate_submission_from_baseline.py` is the safer path when a result must satisfy the competition-style 10-run requirement with distinct seeds.

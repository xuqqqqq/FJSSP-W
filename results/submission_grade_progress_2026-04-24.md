# Submission-Grade Progress 2026-04-24

- Baseline CSV: `Submission_HUST-SMART-Lab_Scenario1_ATPTS/atpts_final_submission.csv`
- Submission seeds: `7,19,29,41,53,67,79,97,131,173`
- Exact evaluation budget per run: `20000`
- Iteration cap per run: `200000`
- Neighborhood size: `50`

## Verified compliant subset

The following instances were regenerated with 10 distinct seeds using
`tools/batch_generate_submission_from_baseline.py` and still improved over the
baseline submission:

| Instance | Baseline Best | Submission Best | Best Delta | Baseline Avg | Submission Avg |
| --- | ---: | ---: | ---: | ---: | ---: |
| `6_Fattahi_14_workers` | 546 | 543 | -3 | 570.6 | 544.0 |
| `6_Fattahi_20_workers` | 1155 | 1153 | -2 | 1177.5 | 1154.8 |

## External artifacts

- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS\fattahi_submission_2026-04-24_merged_submission.csv`
- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS\fattahi_submission_2026-04-24_aggregate_summary.json`
- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS\fattahi_submission_2026-04-24_aggregate_summary.md`

## Notes

- This is a compliant subset check, not yet a full 30-instance submission regeneration.
- The 8-instance 10-seed attempt timed out when run as one large job, which is why the batch coordinator exists.

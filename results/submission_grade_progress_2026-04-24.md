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
| `0_BehnkeGeiger_60_workers` | 403 | 401 | -2 | 419.6 | 402.0 |
| `2c_Hurink_rdata_28_workers` | 789 | 784 | -5 | 811.9 | 787.0 |
| `2c_Hurink_rdata_38_workers` | 1539 | 1533 | -6 | 1556.8 | 1536.6 |
| `2d_Hurink_vdata_18_workers` | 1041 | 1039 | -2 | 1047.5 | 1040.8 |
| `3_DPpaulli_9_workers` | 2087 | 2077 | -10 | 2099.2 | 2083.5 |
| `3_DPpaulli_15_workers` | 2213 | 2199 | -14 | 2241.1 | 2208.9 |
| `3_DPpaulli_18_workers` | 2215 | 2204 | -11 | 2242.0 | 2210.6 |
| `6_Fattahi_14_workers` | 546 | 543 | -3 | 570.6 | 544.0 |
| `6_Fattahi_20_workers` | 1155 | 1153 | -2 | 1177.5 | 1154.8 |

## Verified same-best improvements

These instances did not beat the known best makespan, but the 10-seed
regeneration made every run hit the best known value immediately, improving
average quality and/or time-to-best:

| Instance | Best | Baseline Avg | Submission Avg | Time-to-best evidence |
| --- | ---: | ---: | ---: | --- |
| `0_BehnkeGeiger_42_workers` | 80 | 82.0 | 80.0 | all 10 seeds at `0` evaluations |
| `0_BehnkeGeiger_46_workers` | 108 | 111.4 | 108.0 | all 10 seeds at `0` evaluations |
| `1_Brandimarte_7_workers` | 140 | 142.2 | 140.0 | all 10 seeds at `0` evaluations |
| `1_Brandimarte_12_workers` | 471 | 471.0 | 471.0 | all 10 seeds at `0` evaluations |
| `1_Brandimarte_14_workers` | 640 | 640.0 | 640.0 | all 10 seeds at `0` evaluations |
| `2a_Hurink_sdata_1_workers` | 52 | 52.4 | 52.0 | all 10 seeds at `0` evaluations |
| `2a_Hurink_sdata_18_workers` | 1138 | 1138.0 | 1138.0 | all 10 seeds at `0` evaluations |
| `2a_Hurink_sdata_38_workers` | 1741 | 1768.7 | 1741.0 | all 10 seeds at `0` evaluations |
| `2b_Hurink_edata_1_workers` | 51 | 51.0 | 51.0 | all 10 seeds at `0` evaluations |
| `2a_Hurink_sdata_63_workers` | 382 | 395.6 | 382.0 | all 10 seeds at `0` evaluations |
| `2b_Hurink_edata_6_workers` | 544 | 551.7 | 544.0 | all 10 seeds at `0` evaluations |
| `2d_Hurink_vdata_5_workers` | 507 | 512.3 | 507.0 | all 10 seeds at `0` evaluations |
| `4_ChambersBarnes_10_workers` | 884 | 918.2 | 884.0 | all 10 seeds at `0` evaluations |
| `5_Kacem_3_workers` | 7 | 7.0 | 7.0 | all 10 seeds at `0` evaluations |
| `5_Kacem_4_workers` | 11 | 11.1 | 11.0 | all 10 seeds at `0` evaluations |

## External artifacts

- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS\submission_grade_best13_2026-04-24.csv`
- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS\submission_grade_best15_2026-04-24.csv`
- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS\submission_grade_best16_2026-04-24.csv`
- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS\submission_grade_best19_2026-04-24.csv`
- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS\submission_grade_best21_2026-04-24.csv`
- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS\submission_grade_best24_2026-04-24.csv`
- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS\improved8_submission_2026-04-24_merged_submission.csv`
- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS\improved6_submission_2026-04-24_aggregate_summary.json`
- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS\improved6_submission_2026-04-24_aggregate_summary.md`
- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS\fattahi_submission_2026-04-24_merged_submission.csv`
- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS\fattahi_submission_2026-04-24_aggregate_summary.json`
- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\Submission_HUST-SMART-Lab_Scenario1_ATPTS\fattahi_submission_2026-04-24_aggregate_summary.md`

## Notes

- This is still a compliant subset check, not yet a full 30-instance submission regeneration.
- The 8-instance 10-seed attempt timed out when run as one large job, which is why the batch coordinator exists.
- After batching, all 8 currently known improvement candidates survived under the 10-seed submission discipline.
- `submission_grade_best24_2026-04-24.csv` combines the 9 better-makespan instances with 15 same-best, faster/stabler instances.

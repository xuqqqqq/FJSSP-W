# ATPTS Reference Scan 2026-04-23

- Source baseline: `Submission_HUST-SMART-Lab_Scenario1_ATPTS/atpts_final_submission.csv`
- Seeds: `7,19,29`
- Extra evaluations per warm start: `20000`
- Iteration cap per warm start: `200000`
- Neighborhood size: `50`
- Scanned instances: `16`
- Improved instances: `5`

## Improvements

| Instance | Baseline | Improved | Delta |
| --- | ---: | ---: | ---: |
| 3_DPpaulli_15_workers | 2213 | 2204 | -9 |
| 3_DPpaulli_18_workers | 2215 | 2207 | -8 |
| 3_DPpaulli_9_workers | 2087 | 2077 | -10 |
| 6_Fattahi_14_workers | 546 | 543 | -3 |
| 6_Fattahi_20_workers | 1155 | 1153 | -2 |

## Batch Files

- `batch_a_improvement_summary.json`: 4_ChambersBarnes_10_workers, 5_Kacem_3_workers, 5_Kacem_4_workers, 6_Fattahi_14_workers, 6_Fattahi_20_workers
- `batch_b1_improvement_summary.json`: 1_Brandimarte_7_workers, 1_Brandimarte_12_workers, 1_Brandimarte_14_workers
- `batch_b2a_improvement_summary.json`: 3_DPpaulli_1_workers, 3_DPpaulli_9_workers
- `batch_b2b_improvement_summary.json`: 3_DPpaulli_15_workers, 3_DPpaulli_18_workers
- `batch_c_improvement_summary.json`: 2a_Hurink_sdata_1_workers, 2b_Hurink_edata_1_workers, 2b_Hurink_edata_6_workers, 2d_Hurink_vdata_5_workers

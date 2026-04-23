# ATPTS Reference Scan 2026-04-23

- Source baseline: `Submission_HUST-SMART-Lab_Scenario1_ATPTS/atpts_final_submission.csv`
- Seeds: `7,19,29`
- Extra evaluations per warm start: `20000`
- Iteration cap per warm start: `200000`
- Neighborhood size: `50`
- Scanned instances: `30` / `30`
- Improved instances: `8`
- Matched instances: `22`

## Improved

| Instance | Baseline | Current | Delta |
| --- | ---: | ---: | ---: |
| 0_BehnkeGeiger_60_workers | 403 | 402 | -1 |
| 2c_Hurink_rdata_28_workers | 789 | 784 | -5 |
| 2c_Hurink_rdata_38_workers | 1539 | 1535 | -4 |
| 3_DPpaulli_15_workers | 2213 | 2204 | -9 |
| 3_DPpaulli_18_workers | 2215 | 2207 | -8 |
| 3_DPpaulli_9_workers | 2087 | 2077 | -10 |
| 6_Fattahi_14_workers | 546 | 543 | -3 |
| 6_Fattahi_20_workers | 1155 | 1153 | -2 |

## Matched

- `0_BehnkeGeiger_42_workers`: `80`
- `0_BehnkeGeiger_46_workers`: `108`
- `1_Brandimarte_12_workers`: `471`
- `1_Brandimarte_14_workers`: `640`
- `1_Brandimarte_7_workers`: `140`
- `2a_Hurink_sdata_18_workers`: `1138`
- `2a_Hurink_sdata_1_workers`: `52`
- `2a_Hurink_sdata_38_workers`: `1741`
- `2a_Hurink_sdata_40_workers`: `1373`
- `2a_Hurink_sdata_54_workers`: `8251`
- `2a_Hurink_sdata_61_workers`: `874`
- `2a_Hurink_sdata_63_workers`: `382`
- `2b_Hurink_edata_1_workers`: `51`
- `2b_Hurink_edata_6_workers`: `544`
- `2c_Hurink_rdata_50_workers`: `5697`
- `2d_Hurink_vdata_18_workers`: `1041`
- `2d_Hurink_vdata_30_workers`: `1075`
- `2d_Hurink_vdata_5_workers`: `507`
- `3_DPpaulli_1_workers`: `2482`
- `4_ChambersBarnes_10_workers`: `884`
- `5_Kacem_3_workers`: `7`
- `5_Kacem_4_workers`: `11`

## Comparison CSV

- `C:\Users\ASUS\Downloads\FJSSP-W-Benchmarking-main\FJSSP-W-Benchmarking-main\results\atpts_reference_comparison_2026-04-23.csv`

## Batch Files

- `batch_a_improvement_summary.json`: 4_ChambersBarnes_10_workers, 5_Kacem_3_workers, 5_Kacem_4_workers, 6_Fattahi_14_workers, 6_Fattahi_20_workers
- `batch_b1_improvement_summary.json`: 1_Brandimarte_7_workers, 1_Brandimarte_12_workers, 1_Brandimarte_14_workers
- `batch_b2a_improvement_summary.json`: 3_DPpaulli_1_workers, 3_DPpaulli_9_workers
- `batch_b2b_improvement_summary.json`: 3_DPpaulli_15_workers, 3_DPpaulli_18_workers
- `batch_c_improvement_summary.json`: 2a_Hurink_sdata_1_workers, 2b_Hurink_edata_1_workers, 2b_Hurink_edata_6_workers, 2d_Hurink_vdata_5_workers
- `remaining_scan_2026-04-23_batch_01.json`: 0_BehnkeGeiger_42_workers, 0_BehnkeGeiger_46_workers, 0_BehnkeGeiger_60_workers, 2a_Hurink_sdata_18_workers
- `remaining_scan_2026-04-23_batch_02.json`: 2a_Hurink_sdata_38_workers, 2a_Hurink_sdata_40_workers, 2a_Hurink_sdata_54_workers, 2a_Hurink_sdata_61_workers
- `remaining_scan_2026-04-23_batch_03.json`: 2a_Hurink_sdata_63_workers, 2c_Hurink_rdata_28_workers, 2c_Hurink_rdata_38_workers, 2c_Hurink_rdata_50_workers
- `remaining_scan_2026-04-23_batch_04.json`: 2d_Hurink_vdata_18_workers, 2d_Hurink_vdata_30_workers

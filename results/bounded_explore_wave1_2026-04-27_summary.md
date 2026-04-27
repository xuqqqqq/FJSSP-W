# Bounded Exploration Report

- Baseline CSV: `submission_grade_best30_2026-04-24.csv`
- Seeds: `419,421,431`
- Max evaluations: `10000`
- Max iterations: `100000`
- Neighborhood size: `60`
- Per-instance timeout seconds: `900`
- Improved instances: `1`
- Timed out instances: `0`

| Instance | Status | Baseline Best | Improved Best | Notes |
| --- | --- | ---: | ---: | --- |
| `2a_Hurink_sdata_54_workers` | `OK` | 8251 | 8251 | no strict improvement |
| `2c_Hurink_rdata_50_workers` | `OK` | 5697 | 5697 | no strict improvement |
| `2a_Hurink_sdata_40_workers` | `OK` | 1373 | 1373 | no strict improvement |
| `2d_Hurink_vdata_30_workers` | `IMPROVED` | 1072 | 1069 | strict improvement found |

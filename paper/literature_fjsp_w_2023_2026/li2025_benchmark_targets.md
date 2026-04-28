# Li et al. 2025 CEAM-CP benchmark targets

Source paper: Changao Li, Leilei Meng, Saif Ullah, Peng Duan, Biao Zhang, and Hongyan Sang, "Novel Hybrid Algorithm of Cooperative Evolutionary Algorithm and Constraint Programming for Dual Resource Constrained Flexible Job Shop Scheduling Problems," Complex System Modeling and Simulation, 2025, DOI: 10.23919/CSMS.2024.0041.

The paper reports that its `DFJSP01-13` benchmark instances come from Gong et al. 2018 and Gong et al. 2020. Table 3 / Table 7 give the original-instance CEAM-CP makespan targets; Tables 4 and 5 give doubled and tripled variants.

Important reproducibility note:

- I found and converted a public Baidu Netdisk document named `Benchmarks of DFJSPs 01-10.docx` from https://pan.baidu.com/s/1mhHfv6K.
- That document describes a different DFJSP set with 3 worker levels and green/cost columns. Converting the processing-time column and running our solver on `DFJSP01` gives makespan 54 in a 5-second probe.
- Li et al. 2025 report CEAM-CP best 7 on their `DFJSP01`; therefore the public Baidu `DFJSP01-10` set is not the exact Li `DFJSP01-13` target set and must not be used for direct comparison against Li's Table 3 / Table 7.

Original-instance targets:

| Instance | CEAM-CP best | CEAM-CP average |
| --- | ---: | ---: |
| DFJSP01 | 7 | 7 |
| DFJSP02 | 9 | 9 |
| DFJSP03 | 4 | 4 |
| DFJSP04 | 31 | 31.6 |
| DFJSP05 | 19 | 19.3 |
| DFJSP06 | 189 | 189 |
| DFJSP07 | 47 | 48.2 |
| DFJSP08 | 154 | 157.9 |
| DFJSP09 | 45 | 45.7 |
| DFJSP10 | 124 | 127.4 |
| DFJSP11 | 459 | 459.3 |
| DFJSP12 | 279 | 284.3 |
| DFJSP13 | 174 | 180.6 |

Decision for our paper:

- Cite Li et al. as recent DRCFJSP evidence and as a target benchmark family.
- Do not claim we beat Li until the exact `DFJSP01-13` raw instance files are obtained or reconstructed from an authoritative source.
- If the exact files are obtained, convert them to the same `.fjs` operation-machine-worker format and compare against `li2025_ceamcp_targets.csv`.

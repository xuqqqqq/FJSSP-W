# External DFJSP Gong 2018 Data

This directory is for local converted instances from the public Baidu Netdisk document titled `Benchmarks of DFJSPs 01-10.docx`.

Source link: https://pan.baidu.com/s/1mhHfv6K

Use:

```powershell
python tools\convert_dfjsp_baidu_to_fjsspw.py paper\literature_fjsp_w_2023_2026\Benchmarks_DFJSPs_01-10_from_baidu.txt instances\External_DFJSP_Gong2018
```

Notes:

- The raw PDF/text extracted from the public document is kept local under `paper/literature_fjsp_w_2023_2026` and ignored by git.
- The generated `.fjs` files are also ignored by git until redistribution rights are checked.
- This `DFJSP01-10` set is related DFJSP worker-flexibility data, but it is not the exact `DFJSP01-13` target set used in Li et al. 2025 CEAM-CP. A quick validation run on the converted `DFJSP01` produced makespan 54, while Li et al. report 7 for their `DFJSP01`, so the sets are not comparable.

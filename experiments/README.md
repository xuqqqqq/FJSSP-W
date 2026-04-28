# Experiment Layers

This directory separates the paper experiments into reproducible data layers.

## Layer 1: Competition 30

- File: `layer1_competition30.csv`
- Instances: 30
- Role: main Scenario 1 competition table and ATPTS/reference comparison.
- Protocol: 10 independent runs per instance, distinct random seeds, official feasibility and makespan evaluation.
- Manifest paths point to the repository-local public `.fjs` files; these have been verified identical to the adjacent ATPTS submission-instance copies on this workspace.
- The `lb`/`ub` columns are the repository public best-known table; competition claims should use `reference_best`, `reference_avg`, `current_best`, and `current_avg`.

## Layer 2: Public 402

- File: `layer2_public402.csv`
- Instances: 402
- Role: public generalization study against the repository-provided LB/UB table.
- Protocol: use the same solver settings as Layer 1 where feasible; report solved-to-UB, improved-UB, gap-to-LB, and runtime summaries by family and size.

## Layer 3: Ablation Representatives

- File: `layer3_ablation_representatives.csv`
- Instances: 30
- Role: configuration and mechanism ablations.
- Selection rule: for each benchmark family, choose the minimum-, median-, and maximum-operation instance after sorting by operation count.
- Planned ablations: worker-aware graph on/off, worker-aware neighborhood on/off, approximate screening on/off, warm start on/off, and seed count sensitivity.

## Notes

- Do not use newly generated FJSSP-W instances for best-known comparisons unless a fresh baseline is generated for every compared solver.
- Synthetic stress tests are intentionally out of scope for the current paper pass.

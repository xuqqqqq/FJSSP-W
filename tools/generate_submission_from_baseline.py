import argparse
import ast
import csv
import importlib
import json
from pathlib import Path
import statistics
import sys
from typing import Dict, List


DEFAULT_SUBMISSION_SEEDS = "7,19,29,41,53,67,79,97,131,173"


def parse_args() -> argparse.Namespace:
    default_atpts_dir = (
        Path(__file__).resolve().parents[2] / "Submission_HUST-SMART-Lab_Scenario1_ATPTS"
    )
    parser = argparse.ArgumentParser(
        description=(
            "Generate submission-grade ATPTS rows from a baseline CSV using 10 distinct warm-start seeds."
        )
    )
    parser.add_argument(
        "--atpts-dir",
        default=str(default_atpts_dir),
        help="Path to Submission_HUST-SMART-Lab_Scenario1_ATPTS.",
    )
    parser.add_argument(
        "--baseline-csv",
        default="atpts_final_submission.csv",
        help="Existing submission CSV used as the warm-start baseline. Relative paths are resolved under --atpts-dir.",
    )
    parser.add_argument(
        "--instances-dir",
        default="instances/fjssp-w",
        help="Directory containing .fjs instances. Relative paths are resolved under --atpts-dir.",
    )
    parser.add_argument(
        "--instances",
        default="",
        help="Comma-separated instance names to regenerate. Defaults to all instances in the CSV.",
    )
    parser.add_argument(
        "--seeds",
        default=DEFAULT_SUBMISSION_SEEDS,
        help="Comma-separated submission seeds. Must be 10 distinct integers.",
    )
    parser.add_argument(
        "--max-evaluations",
        type=int,
        default=50_000,
        help="Exact evaluation budget for each submission run.",
    )
    parser.add_argument(
        "--max-iterations",
        type=int,
        default=500_000,
        help="Iteration cap for each submission run.",
    )
    parser.add_argument(
        "--neighborhood-size",
        type=int,
        default=60,
        help="Neighborhood size for each submission run.",
    )
    parser.add_argument(
        "--output-csv",
        default="codex_submission_runs.csv",
        help="Path of the regenerated submission CSV. Relative paths are resolved under --atpts-dir.",
    )
    parser.add_argument(
        "--summary-json",
        default="codex_submission_runs_summary.json",
        help="Path of the JSON summary file. Relative paths are resolved under --atpts-dir.",
    )
    return parser.parse_args()


def resolve_path(base_dir: Path, value: str) -> Path:
    path = Path(value)
    if path.is_absolute():
        return path
    return (base_dir / path).resolve()


def bootstrap_imports(atpts_dir: Path):
    repo_root = Path(__file__).resolve().parents[1]
    for path in (str(repo_root), str(atpts_dir)):
        if path not in sys.path:
            sys.path.insert(0, path)

    atpts_solver = importlib.import_module("atpts_solver")
    benchmark_parser = importlib.import_module("util.benchmark_parser")
    return atpts_solver, benchmark_parser.WorkerBenchmarkParser


def load_rows(path: Path) -> List[Dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def parse_list(value: str) -> List[int]:
    return list(ast.literal_eval(value))


def serialize_list(values: List[int]) -> str:
    return json.dumps([int(value) for value in values], ensure_ascii=False)


def normalize_instances(value: str, rows: List[Dict[str, str]]) -> List[str]:
    if value.strip():
        return [item.strip() for item in value.split(",") if item.strip()]
    return sorted({row["Instance"] for row in rows})


def normalize_submission_seeds(value: str) -> List[int]:
    seeds = [int(item.strip()) for item in value.split(",") if item.strip()]
    if len(seeds) != 10:
        raise SystemExit("Submission mode requires exactly 10 seeds.")
    if len(set(seeds)) != 10:
        raise SystemExit("Submission mode requires 10 distinct seeds.")
    return seeds


def build_warm_start_solution(solution_cls, instance_name: str, row: Dict[str, str], parser, instances_dir: Path):
    encoding = parser.parse_benchmark(str(instances_dir / f"{instance_name}.fjs"))
    durations = encoding.durations()
    operation_jobs = encoding.job_sequence()
    start_times = parse_list(row["StartTimes"])
    machines = parse_list(row["MachineAssignments"])
    workers = parse_list(row["WorkerAssignments"])

    operations = list(range(len(operation_jobs)))
    operations.sort(
        key=lambda op: (
            start_times[op],
            start_times[op] + int(durations[op][machines[op]][workers[op]]),
            op,
        )
    )
    sequence = [operation_jobs[op] for op in operations]

    return solution_cls(
        job_sequence=sequence,
        machine_assignments=machines,
        worker_assignments=workers,
        start_times=start_times,
        makespan=float(row["Fitness"]),
    )


def result_to_row(instance_name: str, result: Dict) -> Dict[str, str]:
    return {
        "Instance": instance_name,
        "Fitness": str(int(result["makespan"])),
        "FunctionEvaluations": str(int(result["evaluations"])),
        "StartTimes": serialize_list(result["start_times"]),
        "MachineAssignments": serialize_list(result["machine_assignments"]),
        "WorkerAssignments": serialize_list(result["worker_assignments"]),
    }


def main() -> None:
    args = parse_args()
    atpts_dir = Path(args.atpts_dir).resolve()
    atpts_solver, worker_benchmark_parser = bootstrap_imports(atpts_dir)

    baseline_csv = resolve_path(atpts_dir, args.baseline_csv)
    instances_dir = resolve_path(atpts_dir, args.instances_dir)
    output_csv = resolve_path(atpts_dir, args.output_csv)
    summary_json = resolve_path(atpts_dir, args.summary_json)

    rows = load_rows(baseline_csv)
    fieldnames = list(rows[0].keys())
    target_instances = set(normalize_instances(args.instances, rows))
    seeds = normalize_submission_seeds(args.seeds)
    parser = worker_benchmark_parser()

    rows_by_instance: Dict[str, List[Dict[str, str]]] = {}
    for row in rows:
        rows_by_instance.setdefault(row["Instance"], []).append(dict(row))

    summary = []

    for instance_name in sorted(target_instances):
        instance_rows = rows_by_instance.get(instance_name, [])
        if len(instance_rows) != 10:
            raise SystemExit(
                f"Expected 10 baseline rows for {instance_name}, found {len(instance_rows)}."
            )

        best_row = min(instance_rows, key=lambda item: int(item["Fitness"]))
        baseline_best = int(best_row["Fitness"])
        baseline_average = statistics.mean(int(item["Fitness"]) for item in instance_rows)
        warm_start = build_warm_start_solution(
            solution_cls=atpts_solver.Solution,
            instance_name=instance_name,
            row=best_row,
            parser=parser,
            instances_dir=instances_dir,
        )

        instance_path = str(instances_dir / f"{instance_name}.fjs")
        generated_rows: List[Dict[str, str]] = []
        seed_results = []

        for seed in seeds:
            result = atpts_solver.solve_fjssp_w(
                instance_path=instance_path,
                max_evaluations=args.max_evaluations,
                max_iterations=args.max_iterations,
                neighborhood_size=args.neighborhood_size,
                seed=seed,
                initial_solution=warm_start,
                verbose=False,
            )
            generated_rows.append(result_to_row(instance_name, result))
            seed_results.append(
                {
                    "seed": seed,
                    "makespan": int(result["makespan"]),
                    "evaluations_to_best": int(result["evaluations"]),
                }
            )

        rows_by_instance[instance_name] = generated_rows
        submission_best = min(int(row["Fitness"]) for row in generated_rows)
        submission_average = statistics.mean(int(row["Fitness"]) for row in generated_rows)
        summary.append(
            {
                "instance": instance_name,
                "baseline_best": baseline_best,
                "baseline_average": baseline_average,
                "submission_best": submission_best,
                "submission_average": submission_average,
                "best_delta": submission_best - baseline_best,
                "average_delta": submission_average - baseline_average,
                "seeds": seed_results,
            }
        )

    output_rows: List[Dict[str, str]] = []
    for instance_name in sorted(rows_by_instance):
        output_rows.extend(rows_by_instance[instance_name])

    with output_csv.open("w", encoding="utf-8-sig", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(output_rows)

    summary_payload = {
        "baseline_csv": str(baseline_csv),
        "output_csv": str(output_csv),
        "instances": summary,
        "parameters": {
            "seeds": seeds,
            "max_evaluations": args.max_evaluations,
            "max_iterations": args.max_iterations,
            "neighborhood_size": args.neighborhood_size,
        },
    }
    with summary_json.open("w", encoding="utf-8") as handle:
        json.dump(summary_payload, handle, indent=2, ensure_ascii=False)

    improved = [item for item in summary if item["best_delta"] < 0]
    print(f"Submission instances regenerated: {len(summary)}")
    print(f"Best-score improvements vs baseline: {len(improved)}")
    for item in improved:
        print(
            f"{item['instance']}: best {item['baseline_best']} -> {item['submission_best']}, "
            f"avg {item['baseline_average']:.1f} -> {item['submission_average']:.1f}"
        )
    print(f"Submission CSV written to: {output_csv}")
    print(f"Summary written to: {summary_json}")


if __name__ == "__main__":
    main()

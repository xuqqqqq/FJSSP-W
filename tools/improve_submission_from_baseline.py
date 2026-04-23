import argparse
import ast
import csv
import importlib
import json
from pathlib import Path
import sys
from typing import Any, Dict, List, Optional


def parse_args() -> argparse.Namespace:
    default_atpts_dir = (
        Path(__file__).resolve().parents[2] / "Submission_HUST-SMART-Lab_Scenario1_ATPTS"
    )
    parser = argparse.ArgumentParser(
        description="Warm-start ATPTS from an existing submission CSV and write an improved submission."
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
        help="Comma-separated instance names to intensify. Defaults to all instances in the CSV.",
    )
    parser.add_argument(
        "--seeds",
        default="7,19,29",
        help="Comma-separated warm-start seeds.",
    )
    parser.add_argument(
        "--max-evaluations",
        type=int,
        default=50_000,
        help="Extra exact-evaluation budget for each warm-start run.",
    )
    parser.add_argument(
        "--max-iterations",
        type=int,
        default=500_000,
        help="Iteration cap for each warm-start run.",
    )
    parser.add_argument(
        "--neighborhood-size",
        type=int,
        default=60,
        help="Neighborhood size for each warm-start run.",
    )
    parser.add_argument(
        "--output-csv",
        default="atpts_improved_submission.csv",
        help="Path of the improved submission CSV. Relative paths are resolved under --atpts-dir.",
    )
    parser.add_argument(
        "--summary-json",
        default="atpts_improvement_summary.json",
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
    with path.open("r", encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


def parse_list(value: str) -> List[int]:
    return list(ast.literal_eval(value))


def serialize_list(values: List[int]) -> str:
    return json.dumps([int(value) for value in values], ensure_ascii=False)


def to_builtin(value):
    if isinstance(value, dict):
        return {key: to_builtin(item) for key, item in value.items()}
    if isinstance(value, list):
        return [to_builtin(item) for item in value]
    if hasattr(value, "item"):
        return value.item()
    return value


def normalize_instances(value: str, rows: List[Dict[str, str]]) -> List[str]:
    if value.strip():
        return [item.strip() for item in value.split(",") if item.strip()]
    return sorted({row["Instance"] for row in rows})


def normalize_seeds(value: str) -> List[int]:
    return [int(item.strip()) for item in value.split(",") if item.strip()]


def build_warm_start_solution(
    solution_cls,
    instance_name: str,
    row: Dict[str, str],
    parser,
    instances_dir: Path,
):
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
    target_instances = normalize_instances(args.instances, rows)
    seeds = normalize_seeds(args.seeds)
    parser = worker_benchmark_parser()

    summary = []

    for instance_name in target_instances:
        instance_rows = [
            (idx, row) for idx, row in enumerate(rows) if row["Instance"] == instance_name
        ]
        if not instance_rows:
            continue

        best_idx, best_row = min(instance_rows, key=lambda item: int(item[1]["Fitness"]))
        baseline_best = int(best_row["Fitness"])
        warm_start = build_warm_start_solution(
            solution_cls=atpts_solver.Solution,
            instance_name=instance_name,
            row=best_row,
            parser=parser,
            instances_dir=instances_dir,
        )

        best_result: Optional[Dict] = None
        seed_results = []

        instance_path = str(instances_dir / f"{instance_name}.fjs")
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
            seed_results.append(
                {
                    "seed": seed,
                    "makespan": int(result["makespan"]),
                    "evaluations_to_best": int(result["evaluations"]),
                }
            )
            if best_result is None or result["makespan"] < best_result["makespan"]:
                best_result = result

        improved = best_result is not None and best_result["makespan"] < baseline_best
        if improved and best_result is not None:
            rows[best_idx] = result_to_row(instance_name, best_result)

        summary.append(
            {
                "instance": instance_name,
                "baseline_best": baseline_best,
                "improved_best": int(best_result["makespan"]) if best_result else baseline_best,
                "improved": improved,
                "seeds": seed_results,
                "replaced_row_index": best_idx if improved else None,
            }
        )

    with output_csv.open("w", encoding="utf-8-sig", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    with summary_json.open("w", encoding="utf-8") as f:
        json.dump(to_builtin(summary), f, indent=2, ensure_ascii=False)

    improved_instances = [item for item in summary if item["improved"]]
    print(f"Improved instances: {len(improved_instances)} / {len(summary)}")
    for item in improved_instances:
        print(
            f"{item['instance']}: {item['baseline_best']} -> {item['improved_best']}"
        )
    print(f"Improved CSV written to: {output_csv}")
    print(f"Summary written to: {summary_json}")


if __name__ == "__main__":
    main()

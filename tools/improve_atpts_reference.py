import argparse
import csv
import importlib
import json
import os
import random
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional

import numpy as np


@dataclass
class ReferenceRun:
    instance_name: str
    json_path: Path
    makespan: int
    workload_balance: float
    evaluations: int
    total_evaluations: int
    approximate_evaluations: int
    job_sequence: List[int]
    machine_assignments: List[int]
    worker_assignments: List[int]
    start_times: List[int]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Warm-start the ATPTS solver from existing best JSON runs."
    )
    parser.add_argument(
        "--atpts-dir",
        required=True,
        help="Path to Submission_HUST-SMART-Lab_Scenario1_ATPTS",
    )
    parser.add_argument(
        "--instances",
        nargs="*",
        default=None,
        help="Optional subset of instance names without .fjs suffix.",
    )
    parser.add_argument(
        "--max-evaluations",
        type=int,
        default=1_000_000,
        help="Exact evaluation budget per warm-start run.",
    )
    parser.add_argument(
        "--max-iterations",
        type=int,
        default=1_000_000,
        help="Iteration budget per warm-start run.",
    )
    parser.add_argument(
        "--neighborhood-size",
        type=int,
        default=60,
        help="Neighborhood size per iteration.",
    )
    parser.add_argument(
        "--tabu-tenure",
        type=int,
        default=20,
        help="Tabu tenure.",
    )
    parser.add_argument(
        "--seeds",
        nargs="*",
        type=int,
        default=[123],
        help="Seeds to try from the same reference incumbent.",
    )
    parser.add_argument(
        "--output-dir",
        default=None,
        help="Directory for summary CSV and improved JSON runs. Defaults inside atpts-dir.",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Enable solver progress output.",
    )
    parser.add_argument(
        "--perturb-sequence-swaps",
        type=int,
        default=0,
        help="Number of random job-sequence swaps applied before each warm-start run.",
    )
    parser.add_argument(
        "--perturb-resource-changes",
        type=int,
        default=0,
        help="Number of random machine-worker reassignments applied before each warm-start run.",
    )
    return parser.parse_args()


def load_best_reference_runs(reference_dir: Path) -> Dict[str, ReferenceRun]:
    best: Dict[str, ReferenceRun] = {}

    for json_path in sorted(reference_dir.glob("*.json")):
        with json_path.open("r", encoding="utf-8") as handle:
            data = json.load(handle)

        instance_name = data["instance_name"]
        candidate = ReferenceRun(
            instance_name=instance_name,
            json_path=json_path,
            makespan=int(data["makespan"]),
            workload_balance=float(data["workload_balance"]),
            evaluations=int(data["evaluations"]),
            total_evaluations=int(data["total_evaluations"]),
            approximate_evaluations=int(data["approximate_evaluations"]),
            job_sequence=list(data["job_sequence"]),
            machine_assignments=list(data["machine_assignments"]),
            worker_assignments=list(data["worker_assignments"]),
            start_times=list(data["start_times"]),
        )

        incumbent = best.get(instance_name)
        if incumbent is None or candidate.makespan < incumbent.makespan:
            best[instance_name] = candidate

    return best


def filter_instances(
    reference_runs: Dict[str, ReferenceRun], requested: Optional[Iterable[str]]
) -> List[ReferenceRun]:
    if not requested:
        return [reference_runs[name] for name in sorted(reference_runs)]

    missing = [name for name in requested if name not in reference_runs]
    if missing:
        raise SystemExit(f"Missing reference JSON for instances: {', '.join(missing)}")

    return [reference_runs[name] for name in requested]


def load_atpts_solver(atpts_dir: Path):
    sys.path.insert(0, str(atpts_dir))
    return importlib.import_module("atpts_solver")

def reconstruct_job_sequence(
    instance_job_sequence: List[int], start_times: List[int]
) -> List[int]:
    order = sorted(range(len(start_times)), key=lambda op: (start_times[op], op))
    return [instance_job_sequence[op] for op in order]


def build_initial_solution(
    atpts_solver, reference: ReferenceRun, instance_job_sequence: List[int]
):
    reconstructed_job_sequence = reconstruct_job_sequence(
        instance_job_sequence, reference.start_times
    )
    return atpts_solver.Solution(
        job_sequence=reconstructed_job_sequence,
        machine_assignments=reference.machine_assignments.copy(),
        worker_assignments=reference.worker_assignments.copy(),
        start_times=reference.start_times.copy(),
        makespan=reference.makespan,
    )


def warm_start_search(
    atpts_solver,
    instance_path: Path,
    reference: ReferenceRun,
    seed: int,
    max_evaluations: int,
    max_iterations: int,
    neighborhood_size: int,
    tabu_tenure: int,
    perturb_sequence_swaps: int,
    perturb_resource_changes: int,
    verbose: bool,
):
    parser = atpts_solver.WorkerBenchmarkParser()
    encoding = parser.parse_benchmark(str(instance_path))
    durations = encoding.durations()
    job_sequence = encoding.job_sequence()

    class WarmStartTabuSearch(atpts_solver.TabuSearch):
        def __init__(
            self,
            initial_solution,
            perturb_sequence_swaps,
            perturb_resource_changes,
            *args,
            **kwargs,
        ):
            self._initial_solution = initial_solution.copy()
            self._perturb_sequence_swaps = perturb_sequence_swaps
            self._perturb_resource_changes = perturb_resource_changes
            super().__init__(*args, **kwargs)

        def _perturb_solution(self, solution):
            candidate = solution.copy()

            for _ in range(self._perturb_sequence_swaps):
                if len(candidate.job_sequence) < 2:
                    break
                i, j = sorted(random.sample(range(len(candidate.job_sequence)), 2))
                if candidate.job_sequence[i] == candidate.job_sequence[j]:
                    continue
                candidate.job_sequence[i], candidate.job_sequence[j] = (
                    candidate.job_sequence[j],
                    candidate.job_sequence[i],
                )

            for _ in range(self._perturb_resource_changes):
                op = random.randrange(self.n_operations)
                current_pair = (
                    candidate.machine_assignments[op],
                    candidate.worker_assignments[op],
                )
                alternatives = []
                for machine in self.valid_machines[op]:
                    for worker in self.valid_workers[(op, machine)]:
                        if (machine, worker) != current_pair:
                            alternatives.append((machine, worker))
                if not alternatives:
                    continue
                machine, worker = random.choice(alternatives)
                candidate.machine_assignments[op] = machine
                candidate.worker_assignments[op] = worker

            return candidate

        def generate_initial_solution(self):
            solution = self._initial_solution.copy()
            if self._perturb_sequence_swaps or self._perturb_resource_changes:
                solution = self._perturb_solution(solution)
            self.evaluate(solution)
            return solution

        def search(self, verbose: bool = True):
            start_time = time.time()

            reference_solution = self._initial_solution.copy()
            self.evaluate(reference_solution)

            current_solution = reference_solution.copy()
            if self._perturb_sequence_swaps or self._perturb_resource_changes:
                current_solution = self._perturb_solution(current_solution)
                self.evaluate(current_solution)

            self.best_solution = reference_solution.copy()
            self.best_makespan = reference_solution.makespan
            self.best_solution_evaluations = self.evaluation_count
            self.current_iteration = 0
            self.last_improvement_iteration = 0
            self.stagnation_detected = False
            self.progress_history = [
                (0, self.best_makespan, self.best_solution_evaluations)
            ]

            if verbose:
                print(f"Reference makespan: {self.best_makespan}")
                if current_solution.makespan != self.best_makespan:
                    print(f"Perturbed start makespan: {current_solution.makespan}")

            phase = "estimate"
            phase_round = 1

            while (
                self.current_iteration < self.max_iterations
                and self.evaluation_count < self.max_evaluations
            ):
                self.tabu_list.clear()

                if verbose:
                    phase_name = (
                        "estimated screening" if phase == "estimate" else "exact tabu"
                    )
                    print(f"\n{'=' * 60}")
                    print(f"Phase {phase_round}: {phase_name}")
                    print(f"{'=' * 60}")
                    print(f"Current best makespan: {self.best_makespan}")
                    print(f"Global iterations used: {self.current_iteration}")
                    print(f"Exact evaluations used: {self.evaluation_count}")

                if phase == "estimate":
                    current_solution, exit_reason = self._run_estimation_phase(
                        current_solution, verbose
                    )
                    if exit_reason == "stagnation":
                        phase = "exact"
                    else:
                        break
                else:
                    current_solution, exit_reason = self._continue_with_tabu_search(
                        current_solution, verbose
                    )
                    if exit_reason == "stagnation":
                        phase = "estimate"
                    else:
                        break

                phase_round += 1

            if verbose:
                elapsed_time = time.time() - start_time
                print()
                print("Search completed!")
                print(f"Best makespan: {self.best_makespan}")
                print(f"Exact evaluations: {self.evaluation_count}")
                print(f"Approximate evaluations: {self.approximation_count}")
                print(f"Time elapsed: {elapsed_time:.2f}s")

            return self.best_solution

    initial_solution = build_initial_solution(atpts_solver, reference, job_sequence)
    ts = WarmStartTabuSearch(
        initial_solution=initial_solution,
        perturb_sequence_swaps=perturb_sequence_swaps,
        perturb_resource_changes=perturb_resource_changes,
        durations=durations,
        job_sequence=job_sequence,
        tabu_tenure=tabu_tenure,
        max_iterations=max_iterations,
        max_evaluations=max_evaluations,
        neighborhood_size=neighborhood_size,
        aspiration=True,
        seed=seed,
    )
    best_solution = ts.search(verbose=verbose)

    if best_solution.start_times:
        final_makespan = int(best_solution.makespan)
    else:
        final_makespan = int(ts.evaluate(best_solution))

    return {
        "seed": seed,
        "reference_makespan": reference.makespan,
        "improved_makespan": final_makespan,
        "delta": final_makespan - reference.makespan,
        "evaluations_to_best": ts.best_solution_evaluations,
        "total_evaluations": ts.evaluation_count,
        "approximate_evaluations": ts.approximation_count,
        "start_times": list(best_solution.start_times),
        "machine_assignments": list(best_solution.machine_assignments),
        "worker_assignments": list(best_solution.worker_assignments),
        "job_sequence": list(best_solution.job_sequence),
    }


def write_json(path: Path, data: Dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        json.dump(to_serializable(data), handle, indent=2)


def to_serializable(value):
    if isinstance(value, np.integer):
        return int(value)
    if isinstance(value, np.floating):
        return float(value)
    if isinstance(value, dict):
        return {key: to_serializable(item) for key, item in value.items()}
    if isinstance(value, list):
        return [to_serializable(item) for item in value]
    return value


def write_summary_csv(path: Path, rows: List[Dict]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=[
                "instance",
                "reference_makespan",
                "best_improved_makespan",
                "delta",
                "best_seed",
                "reference_json",
                "improved_json",
            ],
        )
        writer.writeheader()
        writer.writerows(rows)


def main() -> int:
    args = parse_args()
    atpts_dir = Path(args.atpts_dir).resolve()
    submission_dir = atpts_dir / "submission_results"
    instances_dir = atpts_dir / "instances" / "fjssp-w"
    output_dir = (
        Path(args.output_dir).resolve()
        if args.output_dir
        else (atpts_dir / "warm_start_results").resolve()
    )

    atpts_solver = load_atpts_solver(atpts_dir)
    references = load_best_reference_runs(submission_dir)
    selected = filter_instances(references, args.instances)

    summary_rows: List[Dict] = []
    improvement_count = 0

    for reference in selected:
        instance_path = instances_dir / f"{reference.instance_name}.fjs"
        print(f"Running {reference.instance_name} from reference {reference.makespan}")

        best_run: Optional[Dict] = None
        for seed in args.seeds:
            run = warm_start_search(
                atpts_solver=atpts_solver,
                instance_path=instance_path,
                reference=reference,
                seed=seed,
                max_evaluations=args.max_evaluations,
                max_iterations=args.max_iterations,
                neighborhood_size=args.neighborhood_size,
                tabu_tenure=args.tabu_tenure,
                perturb_sequence_swaps=args.perturb_sequence_swaps,
                perturb_resource_changes=args.perturb_resource_changes,
                verbose=args.verbose,
            )
            print(
                f"  seed={seed} improved={run['improved_makespan']} delta={run['delta']}"
            )
            if best_run is None or run["improved_makespan"] < best_run["improved_makespan"]:
                best_run = run

        assert best_run is not None

        improved_json_path = output_dir / "json" / f"{reference.instance_name}.json"
        improved_payload = {
            "instance_name": reference.instance_name,
            "reference_json": str(reference.json_path),
            **best_run,
        }
        write_json(improved_json_path, improved_payload)

        if best_run["improved_makespan"] < reference.makespan:
            improvement_count += 1

        summary_rows.append(
            {
                "instance": reference.instance_name,
                "reference_makespan": reference.makespan,
                "best_improved_makespan": best_run["improved_makespan"],
                "delta": best_run["delta"],
                "best_seed": best_run["seed"],
                "reference_json": str(reference.json_path),
                "improved_json": str(improved_json_path),
            }
        )

    summary_path = output_dir / "warm_start_summary.csv"
    write_summary_csv(summary_path, summary_rows)

    print()
    print(f"Improved instances: {improvement_count}/{len(summary_rows)}")
    print(f"Summary CSV: {summary_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

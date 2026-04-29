#!/usr/bin/env python3
"""Run AWLS for an exact task list containing one seed per manifest row."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Dict, List, Set, Tuple

from run_awls_layer import run_one, write_results


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=str(root))
    parser.add_argument("--tasks-csv", required=True)
    parser.add_argument("--output-csv", required=True)
    parser.add_argument("--time-limit-sec", type=int, default=10)
    parser.add_argument("--awls-exe", default=str(root / "AWLS" / "x64" / "Release" / "AWLS.exe"))
    parser.add_argument("--awls-cwd", default=str(root / "AWLS"))
    parser.add_argument("--best", type=int, default=0)
    parser.add_argument("--timeout-extra-sec", type=int, default=20)
    return parser.parse_args()


def read_csv(path: Path) -> List[Dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def task_key(row: Dict[str, str]) -> Tuple[str, int]:
    return (row["source"], int(row["seed"]))


def existing_results(path: Path) -> List[Dict[str, str]]:
    if not path.exists():
        return []
    return read_csv(path)


def main() -> None:
    args = parse_args()
    tasks = read_csv(Path(args.tasks_csv))
    results = existing_results(Path(args.output_csv))
    completed: Set[Tuple[str, int]] = {task_key(row) for row in results}

    if not tasks:
        raise SystemExit("No task rows selected.")

    for task in tasks:
        seed = int(task["seed"])
        key = task_key(task)
        if key in completed:
            print(f"Skipping {task['source']} seed={seed}; already in {args.output_csv}", flush=True)
            continue

        print(f"Running {task['source']} seed={seed} ...", flush=True)
        result = run_one(args, task, seed)
        print(f"  {result['status']} makespan={result['makespan']}", flush=True)
        results.append(result)
        completed.add(key)
        write_results(Path(args.output_csv), results)

    ok = sum(1 for row in results if row["status"] == "OK")
    print(f"Wrote {len(results)} rows to {args.output_csv}; OK={ok}")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Build an exact AWLS retry task list from raw results by status."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Dict, Iterable, List, Set, Tuple


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--results-csv", required=True)
    parser.add_argument("--output-csv", required=True)
    parser.add_argument("--status", default="TIMEOUT")
    return parser.parse_args()


def read_csv(path: Path) -> List[Dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def write_csv(path: Path, fieldnames: List[str], rows: Iterable[Dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def result_key(row: Dict[str, str]) -> Tuple[str, str]:
    return (row["source"], row["seed"])


def main() -> None:
    args = parse_args()
    manifest_rows = read_csv(Path(args.manifest))
    result_rows = read_csv(Path(args.results_csv))
    manifest_by_source = {row["source"]: row for row in manifest_rows}

    retry_keys: Set[Tuple[str, str]] = {
        result_key(row) for row in result_rows if row.get("status") == args.status
    }
    tasks: List[Dict[str, str]] = []
    seen: Set[Tuple[str, str]] = set()

    for row in result_rows:
        key = result_key(row)
        if key not in retry_keys or key in seen:
            continue
        source, seed = key
        if source not in manifest_by_source:
            raise SystemExit(f"Missing manifest row for source: {source}")
        task = dict(manifest_by_source[source])
        task["seed"] = seed
        tasks.append(task)
        seen.add(key)

    if not manifest_rows:
        raise SystemExit("Empty manifest.")
    write_csv(Path(args.output_csv), list(manifest_rows[0].keys()) + ["seed"], tasks)
    print(f"Wrote {len(tasks)} {args.status} retry tasks to {args.output_csv}")
    if tasks:
        print(f"First retry task: {tasks[0]['source']} seed={tasks[0]['seed']}")
        print(f"Last retry task: {tasks[-1]['source']} seed={tasks[-1]['seed']}")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Merge AWLS raw result CSVs, de-duplicating by source and seed."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Dict, Iterable, List, Tuple


FIELDNAMES = [
    "layer",
    "source",
    "competition_instance",
    "family",
    "seed",
    "time_limit_sec",
    "makespan",
    "status",
    "returncode",
    "elapsed_sec",
    "lb",
    "ub",
    "stdout_tail",
    "stderr_tail",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--inputs", nargs="+", required=True)
    parser.add_argument("--output-csv", required=True)
    return parser.parse_args()


def read_csv(path: Path) -> List[Dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def result_key(row: Dict[str, str]) -> Tuple[str, str]:
    return (row["source"], row["seed"])


def write_csv(path: Path, rows: Iterable[Dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=FIELDNAMES)
        writer.writeheader()
        writer.writerows(rows)


def main() -> None:
    args = parse_args()
    rows_by_key: Dict[Tuple[str, str], Dict[str, str]] = {}
    order: List[Tuple[str, str]] = []

    for input_path in args.inputs:
        for row in read_csv(Path(input_path)):
            key = result_key(row)
            if key not in rows_by_key:
                order.append(key)
            rows_by_key[key] = {field: row.get(field, "") for field in FIELDNAMES}

    merged = [rows_by_key[key] for key in order]
    write_csv(Path(args.output_csv), merged)
    print(f"Wrote {len(merged)} merged rows to {args.output_csv}")


if __name__ == "__main__":
    main()

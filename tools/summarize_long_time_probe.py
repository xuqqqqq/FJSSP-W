#!/usr/bin/env python3
"""Summarize long-time probe runs against baseline best and public UB."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Dict, List, Optional


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--tasks-csv", required=True)
    parser.add_argument("--probe-csv", required=True)
    parser.add_argument("--output-csv", required=True)
    return parser.parse_args()


def read_csv(path: Path) -> List[Dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def maybe_float(value: str) -> Optional[float]:
    if value is None or value == "":
        return None
    return float(value)


def fmt(value: Optional[float]) -> str:
    if value is None:
        return ""
    if abs(value - round(value)) < 1e-9:
        return str(int(round(value)))
    return f"{value:.3f}"


def main() -> None:
    args = parse_args()
    tasks = {row["source"]: row for row in read_csv(Path(args.tasks_csv))}
    probes = read_csv(Path(args.probe_csv))
    rows: List[Dict[str, str]] = []
    for probe in probes:
        source = probe["source"]
        task = tasks[source]
        baseline = maybe_float(task.get("baseline_best_10s", ""))
        ub = maybe_float(task.get("ub", ""))
        probe_best = maybe_float(probe.get("makespan", "")) if probe.get("status") == "OK" else None
        rows.append(
            {
                "source": source,
                "family": probe["family"],
                "seed": probe["seed"],
                "time_limit_sec": probe["time_limit_sec"],
                "status": probe["status"],
                "baseline_best_10s": fmt(baseline),
                "probe_makespan": fmt(probe_best),
                "ub": fmt(ub),
                "baseline_gap_to_ub": fmt(baseline - ub) if baseline is not None and ub is not None else "",
                "probe_gap_to_ub": fmt(probe_best - ub) if probe_best is not None and ub is not None else "",
                "improvement_vs_10s": fmt(baseline - probe_best) if baseline is not None and probe_best is not None else "",
                "elapsed_sec": probe.get("elapsed_sec", ""),
            }
        )
    output = Path(args.output_csv)
    output.parent.mkdir(parents=True, exist_ok=True)
    fieldnames = [
        "source",
        "family",
        "seed",
        "time_limit_sec",
        "status",
        "baseline_best_10s",
        "probe_makespan",
        "ub",
        "baseline_gap_to_ub",
        "probe_gap_to_ub",
        "improvement_vs_10s",
        "elapsed_sec",
    ]
    with output.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    print(f"Wrote {len(rows)} long-time probe summary rows to {output}")


if __name__ == "__main__":
    main()

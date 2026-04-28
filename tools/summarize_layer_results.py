#!/usr/bin/env python3
"""Summarize raw AWLS layer-run results against the layer manifest targets."""

from __future__ import annotations

import argparse
import csv
from collections import defaultdict
from pathlib import Path
from statistics import mean
from typing import Dict, Iterable, List, Optional


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--results-csv", required=True)
    parser.add_argument("--output-csv", required=True)
    parser.add_argument("--output-md", default="")
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


def key_for(row: Dict[str, str]) -> str:
    return row.get("competition_instance") or row["source"]


def summarize(manifest_rows: List[Dict[str, str]], result_rows: List[Dict[str, str]]) -> List[Dict[str, str]]:
    manifest_by_key = {key_for(row): row for row in manifest_rows}
    results_by_key: Dict[str, List[Dict[str, str]]] = defaultdict(list)
    for row in result_rows:
        results_by_key[key_for(row)].append(row)

    summaries = []
    for key in sorted(results_by_key):
        rows = results_by_key[key]
        manifest = manifest_by_key.get(key, {})
        ok_rows = [row for row in rows if row["status"] == "OK" and row["makespan"] != ""]
        makespans = [float(row["makespan"]) for row in ok_rows]
        best = min(makespans) if makespans else None
        avg = mean(makespans) if makespans else None
        ub = maybe_float(manifest.get("ub", ""))
        lb = maybe_float(manifest.get("lb", ""))
        reference_best = maybe_float(manifest.get("reference_best", ""))
        current_best = maybe_float(manifest.get("current_best", ""))

        summaries.append(
            {
                "key": key,
                "source": rows[0]["source"],
                "competition_instance": rows[0].get("competition_instance", ""),
                "family": rows[0]["family"],
                "runs": str(len(rows)),
                "ok_runs": str(len(ok_rows)),
                "best": fmt(best),
                "average": fmt(avg),
                "lb": fmt(lb),
                "ub": fmt(ub),
                "reference_best": fmt(reference_best),
                "current_best": fmt(current_best),
                "best_minus_lb": fmt(best - lb) if best is not None and lb is not None else "",
                "best_minus_ub": fmt(best - ub) if best is not None and ub is not None else "",
                "best_minus_reference": fmt(best - reference_best)
                if best is not None and reference_best is not None
                else "",
                "best_minus_current": fmt(best - current_best)
                if best is not None and current_best is not None
                else "",
                "status": "OK" if len(ok_rows) == len(rows) else "PARTIAL_OR_ERROR",
            }
        )
    return summaries


def write_csv(path: Path, rows: Iterable[Dict[str, str]]) -> None:
    rows = list(rows)
    path.parent.mkdir(parents=True, exist_ok=True)
    fieldnames = [
        "key",
        "source",
        "competition_instance",
        "family",
        "runs",
        "ok_runs",
        "best",
        "average",
        "lb",
        "ub",
        "reference_best",
        "current_best",
        "best_minus_lb",
        "best_minus_ub",
        "best_minus_reference",
        "best_minus_current",
        "status",
    ]
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def write_md(path: Path, rows: List[Dict[str, str]]) -> None:
    solved_or_better = [
        row for row in rows if row["best_minus_ub"] != "" and float(row["best_minus_ub"]) <= 0
    ]
    improved_reference = [
        row
        for row in rows
        if row["best_minus_reference"] != "" and float(row["best_minus_reference"]) < 0
    ]
    lines = [
        "# Layer Result Summary",
        "",
        f"- Instances summarized: {len(rows)}",
        f"- At or below provided UB: {len(solved_or_better)}",
        f"- Strictly better than reference best: {len(improved_reference)}",
        "",
        "| Instance | Family | Runs | Best | Avg | LB | UB | Best-UB | Best-Reference | Status |",
        "| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |",
    ]
    for row in rows:
        label = row["competition_instance"] or row["source"]
        lines.append(
            f"| `{label}` | {row['family']} | {row['runs']} | {row['best']} | {row['average']} | "
            f"{row['lb']} | {row['ub']} | {row['best_minus_ub']} | {row['best_minus_reference']} | `{row['status']}` |"
        )
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    args = parse_args()
    manifest_rows = read_csv(Path(args.manifest))
    result_rows = read_csv(Path(args.results_csv))
    summaries = summarize(manifest_rows, result_rows)
    write_csv(Path(args.output_csv), summaries)
    if args.output_md:
        write_md(Path(args.output_md), summaries)
    print(f"Wrote {len(summaries)} summary rows to {args.output_csv}")
    if args.output_md:
        print(f"Wrote Markdown summary to {args.output_md}")


if __name__ == "__main__":
    main()

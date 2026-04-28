#!/usr/bin/env python3
"""Run AWLS over an experiment-layer manifest and write a compact result CSV."""

from __future__ import annotations

import argparse
import csv
import re
import subprocess
import time
from pathlib import Path
from typing import Dict, Iterable, List, Optional


MAKESPAN_RE = re.compile(r"(?:makespan=|Final solution:\s*)(\d+)", re.IGNORECASE)
DEFAULT_SEEDS = "1009,1013,1019,1021,1031,1033,1039,1049,1051,1061"


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=str(root))
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--output-csv", required=True)
    parser.add_argument("--time-limit-sec", type=int, default=10)
    parser.add_argument("--seeds", default=DEFAULT_SEEDS)
    parser.add_argument("--limit", type=int, default=0, help="Optional max number of manifest instances to run.")
    parser.add_argument("--instances", default="", help="Optional comma-separated source names or competition labels.")
    parser.add_argument("--awls-exe", default=str(root / "AWLS" / "x64" / "Release" / "AWLS.exe"))
    parser.add_argument("--awls-cwd", default=str(root / "AWLS"))
    parser.add_argument("--best", type=int, default=0, help="Optional incumbent/best hint passed to AWLS.")
    parser.add_argument("--timeout-extra-sec", type=int, default=20)
    return parser.parse_args()


def read_manifest(path: Path) -> List[Dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def parse_seeds(value: str) -> List[int]:
    return [int(item.strip()) for item in value.split(",") if item.strip()]


def wanted_set(value: str) -> Optional[set[str]]:
    items = {item.strip() for item in value.split(",") if item.strip()}
    return items or None


def select_rows(rows: List[Dict[str, str]], names: Optional[set[str]], limit: int) -> List[Dict[str, str]]:
    if names:
        rows = [
            row
            for row in rows
            if row.get("source") in names or row.get("competition_instance") in names
        ]
    if limit > 0:
        rows = rows[:limit]
    return rows


def parse_makespan(stdout: str) -> Optional[int]:
    matches = MAKESPAN_RE.findall(stdout)
    if not matches:
        return None
    return int(matches[-1])


def tail(text: str, max_chars: int = 500) -> str:
    text = text.replace("\r", "")
    if len(text) <= max_chars:
        return text
    return text[-max_chars:]


def run_one(args: argparse.Namespace, row: Dict[str, str], seed: int) -> Dict[str, str]:
    start = time.perf_counter()
    root = Path(args.root).resolve()
    benchmark_path = Path(row["benchmark_path"])
    if not benchmark_path.is_absolute():
        benchmark_path = root / benchmark_path
    command = [
        str(Path(args.awls_exe).resolve()),
        str(args.time_limit_sec),
        str(seed),
        str(args.best),
        str(benchmark_path.resolve()),
    ]
    timeout = max(args.time_limit_sec + args.timeout_extra_sec, args.timeout_extra_sec)
    try:
        completed = subprocess.run(
            command,
            cwd=Path(args.awls_cwd).resolve(),
            text=True,
            capture_output=True,
            timeout=timeout,
            check=False,
        )
        elapsed = time.perf_counter() - start
        makespan = parse_makespan(completed.stdout)
        status = "OK" if completed.returncode == 0 and makespan is not None else f"ERROR_{completed.returncode}"
        return {
            "layer": row["layer"],
            "source": row["source"],
            "competition_instance": row.get("competition_instance", ""),
            "family": row["family"],
            "seed": str(seed),
            "time_limit_sec": str(args.time_limit_sec),
            "makespan": "" if makespan is None else str(makespan),
            "status": status,
            "returncode": str(completed.returncode),
            "elapsed_sec": f"{elapsed:.3f}",
            "lb": row.get("lb", ""),
            "ub": row.get("ub", ""),
            "stdout_tail": tail(completed.stdout),
            "stderr_tail": tail(completed.stderr),
        }
    except subprocess.TimeoutExpired as exc:
        elapsed = time.perf_counter() - start
        return {
            "layer": row["layer"],
            "source": row["source"],
            "competition_instance": row.get("competition_instance", ""),
            "family": row["family"],
            "seed": str(seed),
            "time_limit_sec": str(args.time_limit_sec),
            "makespan": "",
            "status": "TIMEOUT",
            "returncode": "",
            "elapsed_sec": f"{elapsed:.3f}",
            "lb": row.get("lb", ""),
            "ub": row.get("ub", ""),
            "stdout_tail": tail(exc.stdout or ""),
            "stderr_tail": tail(exc.stderr or ""),
        }


def write_results(path: Path, rows: Iterable[Dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fieldnames = [
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
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def main() -> None:
    args = parse_args()
    manifest = read_manifest(Path(args.manifest))
    rows = select_rows(manifest, wanted_set(args.instances), args.limit)
    seeds = parse_seeds(args.seeds)
    if not rows:
        raise SystemExit("No manifest rows selected.")
    if not seeds:
        raise SystemExit("No seeds selected.")

    results: List[Dict[str, str]] = []
    for row in rows:
        for seed in seeds:
            print(f"Running {row['source']} seed={seed} ...", flush=True)
            result = run_one(args, row, seed)
            print(f"  {result['status']} makespan={result['makespan']}", flush=True)
            results.append(result)
            write_results(Path(args.output_csv), results)

    ok = sum(1 for row in results if row["status"] == "OK")
    print(f"Wrote {len(results)} rows to {args.output_csv}; OK={ok}")


if __name__ == "__main__":
    main()

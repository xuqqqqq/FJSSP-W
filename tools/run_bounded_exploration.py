import argparse
import csv
import json
from pathlib import Path
import subprocess
import sys
from typing import Dict, List, Optional


def parse_args() -> argparse.Namespace:
    default_atpts_dir = (
        Path(__file__).resolve().parents[2] / "Submission_HUST-SMART-Lab_Scenario1_ATPTS"
    )
    parser = argparse.ArgumentParser(
        description=(
            "Run bounded second-stage warm-start explorations one instance at a time "
            "so automation cannot hang indefinitely on hard instances."
        )
    )
    parser.add_argument("--atpts-dir", default=str(default_atpts_dir))
    parser.add_argument(
        "--baseline-csv",
        default="submission_grade_best30_2026-04-24.csv",
        help="Baseline CSV, resolved under --atpts-dir when relative.",
    )
    parser.add_argument(
        "--instances",
        required=True,
        help="Comma-separated instance names to explore.",
    )
    parser.add_argument("--seeds", default="331,337,347")
    parser.add_argument("--max-evaluations", type=int, default=10_000)
    parser.add_argument("--max-iterations", type=int, default=100_000)
    parser.add_argument("--neighborhood-size", type=int, default=60)
    parser.add_argument(
        "--timeout-seconds",
        type=int,
        default=900,
        help="Wall-clock timeout for each instance exploration.",
    )
    parser.add_argument(
        "--output-prefix",
        default="bounded_explore",
        help="Prefix for per-instance files under --atpts-dir and aggregate files under --results-dir.",
    )
    parser.add_argument(
        "--results-dir",
        default=str(Path(__file__).resolve().parents[1] / "results"),
        help="Directory for aggregate JSON/Markdown reports.",
    )
    return parser.parse_args()


def resolve_path(base_dir: Path, value: str) -> Path:
    path = Path(value)
    if path.is_absolute():
        return path
    return (base_dir / path).resolve()


def normalize_instances(value: str) -> List[str]:
    return [item.strip() for item in value.split(",") if item.strip()]


def safe_name(instance: str) -> str:
    return "".join(ch if ch.isalnum() else "_" for ch in instance).strip("_")


def load_summary(path: Path) -> Optional[List[Dict]]:
    if not path.exists():
        return None
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def run_instance(args: argparse.Namespace, atpts_dir: Path, instance: str) -> Dict:
    script = Path(__file__).resolve().parent / "improve_submission_from_baseline.py"
    label = safe_name(instance)
    child_csv = f"{args.output_prefix}_{label}_submission.csv"
    child_json = f"{args.output_prefix}_{label}_summary.json"
    child_json_path = resolve_path(atpts_dir, child_json)

    command = [
        sys.executable,
        str(script),
        "--atpts-dir",
        str(atpts_dir),
        "--baseline-csv",
        args.baseline_csv,
        "--instances",
        instance,
        "--seeds",
        args.seeds,
        "--max-evaluations",
        str(args.max_evaluations),
        "--max-iterations",
        str(args.max_iterations),
        "--neighborhood-size",
        str(args.neighborhood_size),
        "--output-csv",
        child_csv,
        "--summary-json",
        child_json,
    ]

    try:
        completed = subprocess.run(
            command,
            cwd=Path(__file__).resolve().parents[1],
            text=True,
            capture_output=True,
            timeout=args.timeout_seconds,
            check=False,
        )
    except subprocess.TimeoutExpired as exc:
        return {
            "instance": instance,
            "status": "TIMEOUT",
            "returncode": None,
            "timeout_seconds": args.timeout_seconds,
            "stdout": exc.stdout or "",
            "stderr": exc.stderr or "",
            "summary": None,
        }

    summary = load_summary(child_json_path)
    improved = False
    if summary:
        improved = any(bool(item.get("improved")) for item in summary)

    return {
        "instance": instance,
        "status": "IMPROVED" if improved else ("OK" if completed.returncode == 0 else "ERROR"),
        "returncode": completed.returncode,
        "timeout_seconds": args.timeout_seconds,
        "stdout": completed.stdout,
        "stderr": completed.stderr,
        "summary": summary,
    }


def write_markdown(path: Path, records: List[Dict], args: argparse.Namespace) -> None:
    improved_count = sum(1 for record in records if record["status"] == "IMPROVED")
    timeout_count = sum(1 for record in records if record["status"] == "TIMEOUT")
    lines = [
        "# Bounded Exploration Report",
        "",
        f"- Baseline CSV: `{args.baseline_csv}`",
        f"- Seeds: `{args.seeds}`",
        f"- Max evaluations: `{args.max_evaluations}`",
        f"- Max iterations: `{args.max_iterations}`",
        f"- Neighborhood size: `{args.neighborhood_size}`",
        f"- Per-instance timeout seconds: `{args.timeout_seconds}`",
        f"- Improved instances: `{improved_count}`",
        f"- Timed out instances: `{timeout_count}`",
        "",
        "| Instance | Status | Baseline Best | Improved Best | Notes |",
        "| --- | --- | ---: | ---: | --- |",
    ]

    for record in records:
        baseline_best = ""
        improved_best = ""
        notes = ""
        summary = record.get("summary") or []
        if summary:
            item = summary[0]
            baseline_best = str(item.get("baseline_best", ""))
            improved_best = str(item.get("improved_best", ""))
            if item.get("improved"):
                notes = "strict improvement found"
            else:
                notes = "no strict improvement"
        elif record["status"] == "TIMEOUT":
            notes = f"timeout after {record['timeout_seconds']}s"
        else:
            notes = "no summary produced"
        lines.append(
            f"| `{record['instance']}` | `{record['status']}` | {baseline_best} | {improved_best} | {notes} |"
        )

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    args = parse_args()
    atpts_dir = Path(args.atpts_dir).resolve()
    results_dir = Path(args.results_dir).resolve()
    results_dir.mkdir(parents=True, exist_ok=True)

    records = []
    for instance in normalize_instances(args.instances):
        print(f"Exploring {instance} ...", flush=True)
        record = run_instance(args, atpts_dir, instance)
        print(f"{instance}: {record['status']}", flush=True)
        records.append(record)

    json_path = results_dir / f"{args.output_prefix}_summary.json"
    md_path = results_dir / f"{args.output_prefix}_summary.md"
    with json_path.open("w", encoding="utf-8") as f:
        json.dump(records, f, indent=2, ensure_ascii=False)
    write_markdown(md_path, records, args)

    improved_count = sum(1 for record in records if record["status"] == "IMPROVED")
    timeout_count = sum(1 for record in records if record["status"] == "TIMEOUT")
    print(f"Improved instances: {improved_count} / {len(records)}")
    print(f"Timed out instances: {timeout_count} / {len(records)}")
    print(f"Aggregate JSON written to: {json_path}")
    print(f"Aggregate Markdown written to: {md_path}")


if __name__ == "__main__":
    main()

import argparse
import csv
import json
import math
import subprocess
import sys
from pathlib import Path
from typing import Dict, List


def parse_args() -> argparse.Namespace:
    default_atpts_dir = (
        Path(__file__).resolve().parents[2] / "Submission_HUST-SMART-Lab_Scenario1_ATPTS"
    )
    parser = argparse.ArgumentParser(
        description="Run improve_submission_from_baseline.py in manageable batches and collate the summaries."
    )
    parser.add_argument(
        "--atpts-dir",
        default=str(default_atpts_dir),
        help="Path to Submission_HUST-SMART-Lab_Scenario1_ATPTS.",
    )
    parser.add_argument(
        "--baseline-csv",
        default="atpts_final_submission.csv",
        help="Baseline submission CSV. Relative paths are resolved under --atpts-dir.",
    )
    parser.add_argument(
        "--instances",
        default="",
        help="Optional comma-separated subset of instances. Defaults to all instances in the baseline CSV.",
    )
    parser.add_argument(
        "--batch-size",
        type=int,
        default=5,
        help="Maximum number of instances per batch.",
    )
    parser.add_argument(
        "--seeds",
        default="7,19,29",
        help="Comma-separated warm-start seeds passed to improve_submission_from_baseline.py.",
    )
    parser.add_argument(
        "--max-evaluations",
        type=int,
        default=20_000,
        help="Extra exact-evaluation budget for each warm-start run.",
    )
    parser.add_argument(
        "--max-iterations",
        type=int,
        default=200_000,
        help="Iteration cap for each warm-start run.",
    )
    parser.add_argument(
        "--neighborhood-size",
        type=int,
        default=50,
        help="Neighborhood size for each warm-start run.",
    )
    parser.add_argument(
        "--output-prefix",
        default="batched_scan",
        help="Prefix for generated CSV/JSON/Markdown artifacts inside --atpts-dir.",
    )
    parser.add_argument(
        "--python",
        default=sys.executable,
        help="Python executable used to launch the child batches.",
    )
    return parser.parse_args()


def resolve_path(base_dir: Path, value: str) -> Path:
    path = Path(value)
    if path.is_absolute():
        return path
    return (base_dir / path).resolve()


def load_instances(baseline_csv: Path, requested: str) -> List[str]:
    if requested.strip():
        return [item.strip() for item in requested.split(",") if item.strip()]

    with baseline_csv.open("r", encoding="utf-8-sig", newline="") as handle:
        rows = list(csv.DictReader(handle))
    return sorted({row["Instance"] for row in rows})


def chunked(items: List[str], size: int) -> List[List[str]]:
    return [items[idx : idx + size] for idx in range(0, len(items), size)]


def write_markdown_report(path: Path, batches: List[Dict], improvements: List[Dict]) -> None:
    lines = [
        "# ATPTS Batched Scan Report",
        "",
        f"- Batches run: {len(batches)}",
        f"- Improved instances: {len(improvements)}",
        "",
    ]

    if improvements:
        lines.extend(
            [
                "## Improvements",
                "",
                "| Instance | Baseline | Improved | Delta |",
                "| --- | ---: | ---: | ---: |",
            ]
        )
        for item in sorted(improvements, key=lambda entry: entry["instance"]):
            delta = item["improved_best"] - item["baseline_best"]
            lines.append(
                f"| {item['instance']} | {item['baseline_best']} | {item['improved_best']} | {delta} |"
            )
        lines.append("")

    lines.extend(["## Batch Artifacts", ""])
    for batch in batches:
        lines.append(
            f"- `{batch['batch_name']}`: {', '.join(batch['instances'])} -> `{batch['summary_json']}`"
        )
    lines.append("")

    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    args = parse_args()
    atpts_dir = Path(args.atpts_dir).resolve()
    baseline_csv = resolve_path(atpts_dir, args.baseline_csv)
    instances = load_instances(baseline_csv, args.instances)
    batches = chunked(instances, args.batch_size)

    script_path = Path(__file__).resolve().with_name("improve_submission_from_baseline.py")
    aggregate_batches: List[Dict] = []
    aggregate_summary: List[Dict] = []

    for batch_index, batch_instances in enumerate(batches, start=1):
        batch_name = f"{args.output_prefix}_batch_{batch_index:02d}"
        output_csv = atpts_dir / f"{batch_name}.csv"
        summary_json = atpts_dir / f"{batch_name}.json"
        command = [
            args.python,
            str(script_path),
            "--atpts-dir",
            str(atpts_dir),
            "--instances",
            ",".join(batch_instances),
            "--seeds",
            args.seeds,
            "--max-evaluations",
            str(args.max_evaluations),
            "--max-iterations",
            str(args.max_iterations),
            "--neighborhood-size",
            str(args.neighborhood_size),
            "--output-csv",
            str(output_csv),
            "--summary-json",
            str(summary_json),
        ]

        print(f"[batch {batch_index}/{len(batches)}] {' ,'.join(batch_instances)}")
        subprocess.run(command, check=True)

        with summary_json.open("r", encoding="utf-8") as handle:
            batch_summary = json.load(handle)

        aggregate_batches.append(
            {
                "batch_name": batch_name,
                "instances": batch_instances,
                "summary_json": str(summary_json),
                "output_csv": str(output_csv),
                "improved_instances": [item["instance"] for item in batch_summary if item["improved"]],
            }
        )
        aggregate_summary.extend(batch_summary)

    aggregate_json_path = atpts_dir / f"{args.output_prefix}_aggregate_summary.json"
    aggregate_md_path = atpts_dir / f"{args.output_prefix}_aggregate_summary.md"
    improvements = [item for item in aggregate_summary if item["improved"]]

    aggregate_payload = {
        "total_instances": len(aggregate_summary),
        "total_batches": len(aggregate_batches),
        "batch_size": args.batch_size,
        "parameters": {
            "seeds": args.seeds,
            "max_evaluations": args.max_evaluations,
            "max_iterations": args.max_iterations,
            "neighborhood_size": args.neighborhood_size,
        },
        "improvements": improvements,
        "batches": aggregate_batches,
    }

    aggregate_json_path.write_text(
        json.dumps(aggregate_payload, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )
    write_markdown_report(aggregate_md_path, aggregate_batches, improvements)

    print(f"Completed {len(aggregate_batches)} batches across {len(aggregate_summary)} instances.")
    print(f"Improved instances: {len(improvements)}")
    print(f"Aggregate JSON: {aggregate_json_path}")
    print(f"Aggregate Markdown: {aggregate_md_path}")


if __name__ == "__main__":
    main()

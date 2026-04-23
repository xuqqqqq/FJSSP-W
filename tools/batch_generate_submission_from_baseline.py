import argparse
import csv
import json
import subprocess
import sys
from pathlib import Path
from typing import Dict, List


def parse_args() -> argparse.Namespace:
    default_atpts_dir = (
        Path(__file__).resolve().parents[2] / "Submission_HUST-SMART-Lab_Scenario1_ATPTS"
    )
    parser = argparse.ArgumentParser(
        description=(
            "Run generate_submission_from_baseline.py in manageable batches and merge the targeted rows."
        )
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
        default=2,
        help="Maximum number of instances per batch.",
    )
    parser.add_argument(
        "--seeds",
        default="7,19,29,41,53,67,79,97,131,173",
        help="Comma-separated submission seeds passed to generate_submission_from_baseline.py.",
    )
    parser.add_argument(
        "--max-evaluations",
        type=int,
        default=20_000,
        help="Exact evaluation budget for each submission run.",
    )
    parser.add_argument(
        "--max-iterations",
        type=int,
        default=200_000,
        help="Iteration cap for each submission run.",
    )
    parser.add_argument(
        "--neighborhood-size",
        type=int,
        default=50,
        help="Neighborhood size for each submission run.",
    )
    parser.add_argument(
        "--output-prefix",
        default="batched_submission",
        help="Prefix for generated batch files inside --atpts-dir.",
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


def load_rows(path: Path) -> List[Dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def write_markdown_report(path: Path, batches: List[Dict], combined_instances: List[Dict]) -> None:
    improved_best = [item for item in combined_instances if item["best_delta"] < 0]
    lines = [
        "# ATPTS Batched Submission Report",
        "",
        f"- Batches run: {len(batches)}",
        f"- Targeted instances: {len(combined_instances)}",
        f"- Instances with improved best score: {len(improved_best)}",
        "",
    ]

    if improved_best:
        lines.extend(
            [
                "## Improved Best Scores",
                "",
                "| Instance | Baseline Best | Submission Best | Best Delta | Submission Avg |",
                "| --- | ---: | ---: | ---: | ---: |",
            ]
        )
        for item in sorted(improved_best, key=lambda entry: entry["instance"]):
            lines.append(
                f"| {item['instance']} | {item['baseline_best']} | {item['submission_best']} | "
                f"{item['best_delta']} | {item['submission_average']:.1f} |"
            )
        lines.append("")

    lines.extend(["## Batch Artifacts", ""])
    for batch in batches:
        lines.append(
            f"- `{batch['batch_name']}`: {', '.join(batch['instances'])} -> `{batch['submission_csv']}`"
        )
    lines.append("")

    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    args = parse_args()
    atpts_dir = Path(args.atpts_dir).resolve()
    baseline_csv = resolve_path(atpts_dir, args.baseline_csv)
    instances = load_instances(baseline_csv, args.instances)
    batches = chunked(instances, args.batch_size)

    script_path = Path(__file__).resolve().with_name("generate_submission_from_baseline.py")
    baseline_rows = load_rows(baseline_csv)
    fieldnames = list(baseline_rows[0].keys())
    rows_by_instance = {}
    for row in baseline_rows:
        rows_by_instance.setdefault(row["Instance"], []).append(dict(row))

    aggregate_batches: List[Dict] = []
    aggregate_instance_summaries: List[Dict] = []

    for batch_index, batch_instances in enumerate(batches, start=1):
        batch_name = f"{args.output_prefix}_batch_{batch_index:02d}"
        output_csv = atpts_dir / f"{batch_name}.csv"
        summary_json = atpts_dir / f"{batch_name}.json"
        command = [
            args.python,
            str(script_path),
            "--atpts-dir",
            str(atpts_dir),
            "--baseline-csv",
            str(baseline_csv),
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

        batch_rows = load_rows(output_csv)
        batch_rows_by_instance = {}
        for row in batch_rows:
            batch_rows_by_instance.setdefault(row["Instance"], []).append(dict(row))
        for instance_name in batch_instances:
            rows_by_instance[instance_name] = batch_rows_by_instance[instance_name]

        with summary_json.open("r", encoding="utf-8") as handle:
            batch_summary = json.load(handle)

        aggregate_batches.append(
            {
                "batch_name": batch_name,
                "instances": batch_instances,
                "submission_csv": str(output_csv),
                "summary_json": str(summary_json),
            }
        )
        aggregate_instance_summaries.extend(batch_summary["instances"])

    aggregate_csv_path = atpts_dir / f"{args.output_prefix}_merged_submission.csv"
    aggregate_json_path = atpts_dir / f"{args.output_prefix}_aggregate_summary.json"
    aggregate_md_path = atpts_dir / f"{args.output_prefix}_aggregate_summary.md"

    merged_rows: List[Dict[str, str]] = []
    for instance_name in sorted(rows_by_instance):
        merged_rows.extend(rows_by_instance[instance_name])

    with aggregate_csv_path.open("w", encoding="utf-8-sig", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(merged_rows)

    aggregate_payload = {
        "baseline_csv": str(baseline_csv),
        "merged_submission_csv": str(aggregate_csv_path),
        "total_instances": len(instances),
        "batch_size": args.batch_size,
        "parameters": {
            "seeds": args.seeds,
            "max_evaluations": args.max_evaluations,
            "max_iterations": args.max_iterations,
            "neighborhood_size": args.neighborhood_size,
        },
        "instances": aggregate_instance_summaries,
        "batches": aggregate_batches,
    }
    aggregate_json_path.write_text(
        json.dumps(aggregate_payload, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )
    write_markdown_report(aggregate_md_path, aggregate_batches, aggregate_instance_summaries)

    improved_best = [item for item in aggregate_instance_summaries if item["best_delta"] < 0]
    print(f"Completed {len(aggregate_batches)} submission batches across {len(instances)} instances.")
    print(f"Instances with improved best score: {len(improved_best)}")
    print(f"Merged submission CSV: {aggregate_csv_path}")
    print(f"Aggregate JSON: {aggregate_json_path}")
    print(f"Aggregate Markdown: {aggregate_md_path}")


if __name__ == "__main__":
    main()

import argparse
import csv
import json
from collections import defaultdict
from pathlib import Path
from statistics import mean
from typing import Any, Dict, Iterable, List, Optional


REQUIRED_COLUMNS = {
    "Instance",
    "Fitness",
    "FunctionEvaluations",
    "StartTimes",
    "MachineAssignments",
    "WorkerAssignments",
}


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parents[1]
    default_atpts_dir = root.parent / "Submission_HUST-SMART-Lab_Scenario1_ATPTS"
    parser = argparse.ArgumentParser(
        description=(
            "Audit a submission-grade CSV against the ATPTS reference: exact row counts, "
            "official-instance coverage, and best/average fitness comparison."
        )
    )
    parser.add_argument("--baseline-csv", default=str(default_atpts_dir / "atpts_final_submission.csv"))
    parser.add_argument("--candidate-csv", default="", help="Defaults to the latest results/submission_grade_best*.csv")
    parser.add_argument("--rows-per-instance", type=int, default=10)
    parser.add_argument("--summary-json", action="append", default=[], help="Optional generator summaries to audit seed evidence")
    parser.add_argument("--output-json", default="")
    parser.add_argument("--output-md", default="")
    return parser.parse_args()


def resolve_latest_candidate(root: Path) -> Path:
    candidates = [
        path
        for path in (root / "results").glob("submission_grade_best*.csv")
        if "comparison" not in path.name
    ]
    if not candidates:
        raise FileNotFoundError("No results/submission_grade_best*.csv artifact found")
    return max(candidates, key=lambda path: path.stat().st_mtime)


def load_rows(path: Path) -> List[Dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        missing = REQUIRED_COLUMNS - set(reader.fieldnames or [])
        if missing:
            raise ValueError(f"{path} is missing required columns: {sorted(missing)}")
        return list(reader)


def group_rows(rows: Iterable[Dict[str, str]]) -> Dict[str, List[Dict[str, str]]]:
    grouped: Dict[str, List[Dict[str, str]]] = defaultdict(list)
    for row in rows:
        grouped[row["Instance"]].append(row)
    return dict(grouped)


def numeric(value: str) -> float:
    return float(value)


def summarize_instance(rows: List[Dict[str, str]]) -> Dict[str, float]:
    fitnesses = [numeric(row["Fitness"]) for row in rows]
    evaluations = [numeric(row["FunctionEvaluations"]) for row in rows]
    return {
        "best": min(fitnesses),
        "average": mean(fitnesses),
        "average_function_evaluations": mean(evaluations),
    }


def load_seed_evidence(summary_paths: Iterable[Path]) -> Dict[str, Dict[str, Any]]:
    evidence: Dict[str, Dict[str, Any]] = {}
    for path in summary_paths:
        with path.open("r", encoding="utf-8") as handle:
            payload = json.load(handle)
        instances = payload if isinstance(payload, list) else payload.get("instances", [])
        for item in instances:
            seeds = [seed_row.get("seed") for seed_row in item.get("seeds", [])]
            evidence[item["instance"]] = {
                "source": str(path),
                "seed_count": len(seeds),
                "distinct_seed_count": len(set(seeds)),
                "seeds": seeds,
            }
    return evidence


def audit(
    baseline_csv: Path,
    candidate_csv: Path,
    rows_per_instance: int,
    summary_paths: Iterable[Path],
) -> Dict[str, Any]:
    baseline_rows = load_rows(baseline_csv)
    candidate_rows = load_rows(candidate_csv)
    baseline_by_instance = group_rows(baseline_rows)
    candidate_by_instance = group_rows(candidate_rows)
    official_instances = sorted(baseline_by_instance)

    row_count_issues = []
    for instance in official_instances:
        count = len(candidate_by_instance.get(instance, []))
        if count != rows_per_instance:
            row_count_issues.append({"instance": instance, "rows": count, "expected": rows_per_instance})

    extra_instances = sorted(set(candidate_by_instance) - set(official_instances))
    missing_instances = sorted(set(official_instances) - set(candidate_by_instance))
    seed_evidence = load_seed_evidence(summary_paths)

    comparisons = []
    for instance in official_instances:
        if instance not in candidate_by_instance:
            continue
        baseline_summary = summarize_instance(baseline_by_instance[instance])
        candidate_summary = summarize_instance(candidate_by_instance[instance])
        evidence = seed_evidence.get(instance)
        comparisons.append(
            {
                "instance": instance,
                "baseline_best": baseline_summary["best"],
                "candidate_best": candidate_summary["best"],
                "best_delta": candidate_summary["best"] - baseline_summary["best"],
                "baseline_average": baseline_summary["average"],
                "candidate_average": candidate_summary["average"],
                "average_delta": candidate_summary["average"] - baseline_summary["average"],
                "baseline_average_function_evaluations": baseline_summary["average_function_evaluations"],
                "candidate_average_function_evaluations": candidate_summary["average_function_evaluations"],
                "seed_evidence": evidence,
            }
        )

    strict_best_improvements = [item for item in comparisons if item["best_delta"] < 0]
    regressions = [item for item in comparisons if item["best_delta"] > 0]
    same_best_average_improvements = [
        item for item in comparisons if item["best_delta"] == 0 and item["average_delta"] < 0
    ]
    seed_issues = [
        {
            "instance": item["instance"],
            "seed_count": item["seed_evidence"]["seed_count"] if item["seed_evidence"] else None,
            "distinct_seed_count": item["seed_evidence"]["distinct_seed_count"] if item["seed_evidence"] else None,
        }
        for item in comparisons
        if item["seed_evidence"] is not None
        and (
            item["seed_evidence"]["seed_count"] != rows_per_instance
            or item["seed_evidence"]["distinct_seed_count"] != rows_per_instance
        )
    ]

    return {
        "baseline_csv": str(baseline_csv),
        "candidate_csv": str(candidate_csv),
        "rows_per_instance": rows_per_instance,
        "official_instance_count": len(official_instances),
        "candidate_row_count": len(candidate_rows),
        "expected_row_count": len(official_instances) * rows_per_instance,
        "missing_instances": missing_instances,
        "extra_instances": extra_instances,
        "row_count_issues": row_count_issues,
        "seed_issues": seed_issues,
        "strict_best_improvement_count": len(strict_best_improvements),
        "same_best_average_improvement_count": len(same_best_average_improvements),
        "regression_count": len(regressions),
        "is_submission_grade_shape": (
            not missing_instances
            and not extra_instances
            and not row_count_issues
            and len(candidate_rows) == len(official_instances) * rows_per_instance
        ),
        "comparisons": comparisons,
    }


def fmt_number(value: float) -> str:
    if abs(value - round(value)) < 1e-9:
        return str(int(round(value)))
    return f"{value:.3f}"


def write_markdown(path: Path, payload: Dict[str, Any]) -> None:
    lines = [
        "# Submission-Grade Audit",
        "",
        f"- Candidate: `{payload['candidate_csv']}`",
        f"- Baseline: `{payload['baseline_csv']}`",
        f"- Official instances: {payload['official_instance_count']}",
        f"- Rows: {payload['candidate_row_count']} / {payload['expected_row_count']}",
        f"- Shape valid: {payload['is_submission_grade_shape']}",
        f"- Strict best improvements: {payload['strict_best_improvement_count']}",
        f"- Same-best average improvements: {payload['same_best_average_improvement_count']}",
        f"- Best-score regressions: {payload['regression_count']}",
        "",
    ]
    if payload["missing_instances"] or payload["extra_instances"] or payload["row_count_issues"]:
        lines.extend(["## Shape Issues", ""])
        for instance in payload["missing_instances"]:
            lines.append(f"- Missing instance: `{instance}`")
        for instance in payload["extra_instances"]:
            lines.append(f"- Extra instance: `{instance}`")
        for issue in payload["row_count_issues"]:
            lines.append(f"- `{issue['instance']}` has {issue['rows']} rows; expected {issue['expected']}")
        lines.append("")

    improved = [item for item in payload["comparisons"] if item["best_delta"] < 0]
    lines.extend(
        [
            "## Strict Best Improvements",
            "",
            "| Instance | Baseline Best | Candidate Best | Delta | Candidate Avg |",
            "| --- | ---: | ---: | ---: | ---: |",
        ]
    )
    for item in sorted(improved, key=lambda row: (row["best_delta"], row["instance"])):
        lines.append(
            f"| `{item['instance']}` | {fmt_number(item['baseline_best'])} | "
            f"{fmt_number(item['candidate_best'])} | {fmt_number(item['best_delta'])} | "
            f"{fmt_number(item['candidate_average'])} |"
        )

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    args = parse_args()
    baseline_csv = Path(args.baseline_csv).resolve()
    candidate_csv = Path(args.candidate_csv).resolve() if args.candidate_csv else resolve_latest_candidate(root)
    summary_paths = [Path(item).resolve() for item in args.summary_json]
    payload = audit(baseline_csv, candidate_csv, args.rows_per_instance, summary_paths)

    if args.output_json:
        output_json = Path(args.output_json).resolve()
        output_json.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")
    if args.output_md:
        write_markdown(Path(args.output_md).resolve(), payload)

    print(json.dumps({key: payload[key] for key in (
        "candidate_csv",
        "candidate_row_count",
        "expected_row_count",
        "is_submission_grade_shape",
        "strict_best_improvement_count",
        "same_best_average_improvement_count",
        "regression_count",
    )}, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()

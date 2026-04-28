#!/usr/bin/env python3
"""Build reproducible experiment-layer manifests for FJSSP-W experiments.

The generated layers separate:

1. the official 30-instance competition set,
2. the full public 402-instance FJSSP-W set, and
3. a deterministic representative ablation subset.
"""

from __future__ import annotations

import argparse
import csv
from collections import defaultdict
from pathlib import Path
from typing import Dict, Iterable, List, Optional


FIELDNAMES = [
    "layer",
    "source",
    "competition_instance",
    "family",
    "benchmark_path",
    "best_known_name",
    "n_operations",
    "n_jobs",
    "n_machines",
    "n_workers",
    "flexibility",
    "duration_variety",
    "lb",
    "ub",
    "is_optimal_known",
    "reference_best",
    "reference_avg",
    "current_best",
    "current_avg",
    "in_competition30",
    "selection_reason",
]


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=str(root))
    parser.add_argument(
        "--competition-csv",
        default=str(root / "results" / "submission_grade_best30_v18_2026-04-27.csv"),
        help="Submission-style CSV whose Instance column defines the official 30-instance layer.",
    )
    parser.add_argument(
        "--reference-csv",
        default=str(root.parent / "Submission_HUST-SMART-Lab_Scenario1_ATPTS" / "atpts_final_submission.csv"),
        help="Optional reference submission CSV used to populate Layer 1 reference best/average values.",
    )
    parser.add_argument("--output-dir", default=str(root / "experiments"))
    return parser.parse_args()


def read_csv(path: Path, delimiter: str = ",") -> List[Dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle, delimiter=delimiter))


def numeric(value: str) -> float:
    return float(value)


def source_number(source: str, prefix: str) -> str:
    if not source.startswith(prefix):
        raise ValueError(f"{source} does not start with {prefix}")
    return source[len(prefix) :]


def family_from_source(source: str) -> str:
    if source.startswith("Behnke"):
        return "BehnkeGeiger"
    if source.startswith("BrandimarteMk"):
        return "Brandimarte"
    if source.startswith("Hurinksdata"):
        return "Hurink_sdata"
    if source.startswith("Hurinkedata"):
        return "Hurink_edata"
    if source.startswith("Hurinkrdata"):
        return "Hurink_rdata"
    if source.startswith("Hurinkvdata"):
        return "Hurink_vdata"
    if source.startswith("DPpaulli"):
        return "DPpaulli"
    if source.startswith("ChambersBarnes"):
        return "ChambersBarnes"
    if source.startswith("Kacem"):
        return "Kacem"
    if source.startswith("Fattahi"):
        return "Fattahi"
    raise ValueError(f"Unknown source family: {source}")


def best_known_name_from_source(source: str) -> str:
    if source.startswith("Behnke"):
        return "BehnkeGeiger" + source_number(source, "Behnke")
    if source.startswith("BrandimarteMk"):
        return "Brandimarte" + source_number(source, "BrandimarteMk")
    return source


def source_from_competition(instance: str) -> str:
    parts = instance.split("_")
    if len(parts) < 4:
        raise ValueError(f"Unexpected competition instance label: {instance}")

    group = parts[0]
    if group == "0":
        return f"Behnke{parts[2]}"
    if group == "1":
        return f"BrandimarteMk{parts[2]}"
    if group in {"2a", "2b", "2c", "2d"}:
        return f"Hurink{parts[2]}{parts[3]}"
    if group == "3":
        return f"DPpaulli{parts[2]}"
    if group == "4":
        return f"ChambersBarnes{parts[2]}"
    if group == "5":
        return f"Kacem{parts[2]}"
    if group == "6":
        return f"Fattahi{parts[2]}"
    raise ValueError(f"Unknown competition group: {instance}")


def relative_path(root: Path, path: Path) -> str:
    try:
        return str(path.resolve().relative_to(root))
    except ValueError:
        return str(Path("..") / path.resolve().relative_to(root.parent))


def public_benchmark_path(root: Path, source: str) -> Path:
    return root / "instances" / "Example_Instances_FJSSP-WF" / f"{source}.fjs"


def normalize_float(value: Optional[str]) -> str:
    if value is None or value == "":
        return ""
    number = numeric(value)
    if abs(number - round(number)) < 1e-9:
        return str(int(round(number)))
    return f"{number:.12g}"


def base_row(
    layer: str,
    data_row: Dict[str, str],
    best_row: Optional[Dict[str, str]],
    root: Path,
    benchmark: Path,
    competition_instance: str,
    reference_summary: Optional[Dict[str, float]],
    current_summary: Optional[Dict[str, float]],
    in_competition30: bool,
    selection_reason: str,
) -> Dict[str, str]:
    source = data_row["source"]
    lb = best_row["LB"] if best_row else ""
    ub = best_row["UB"] if best_row else ""
    return {
        "layer": layer,
        "source": source,
        "competition_instance": competition_instance,
        "family": family_from_source(source),
        "benchmark_path": relative_path(root, benchmark),
        "best_known_name": best_known_name_from_source(source),
        "n_operations": data_row["n_operations"],
        "n_jobs": data_row["n_jobs"],
        "n_machines": data_row["n_machines"],
        "n_workers": data_row["n_worker"],
        "flexibility": normalize_float(data_row["flexibility"]),
        "duration_variety": normalize_float(data_row["duration_variety"]),
        "lb": normalize_float(lb),
        "ub": normalize_float(ub),
        "is_optimal_known": str(lb != "" and ub != "" and numeric(lb) == numeric(ub)).lower(),
        "reference_best": normalize_float(str(reference_summary["best"])) if reference_summary else "",
        "reference_avg": normalize_float(str(reference_summary["avg"])) if reference_summary else "",
        "current_best": normalize_float(str(current_summary["best"])) if current_summary else "",
        "current_avg": normalize_float(str(current_summary["avg"])) if current_summary else "",
        "in_competition30": str(in_competition30).lower(),
        "selection_reason": selection_reason,
    }


def unique_competition_instances(path: Path) -> List[str]:
    rows = read_csv(path)
    seen = set()
    ordered = []
    for row in rows:
        instance = row["Instance"]
        if instance not in seen:
            seen.add(instance)
            ordered.append(instance)
    return ordered


def write_manifest(path: Path, rows: Iterable[Dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=FIELDNAMES)
        writer.writeheader()
        writer.writerows(rows)


def summarize_submission(path: Path) -> Dict[str, Dict[str, float]]:
    if not path.exists():
        return {}
    summaries: Dict[str, List[float]] = defaultdict(list)
    for row in read_csv(path):
        summaries[row["Instance"]].append(float(row["Fitness"]))
    return {
        instance: {"best": min(values), "avg": sum(values) / len(values)}
        for instance, values in summaries.items()
    }


def choose_representatives(rows: List[Dict[str, str]]) -> List[Dict[str, str]]:
    by_family: Dict[str, List[Dict[str, str]]] = defaultdict(list)
    for row in rows:
        by_family[family_from_source(row["source"])].append(row)

    selected: List[Dict[str, str]] = []
    for family in sorted(by_family):
        family_rows = sorted(
            by_family[family],
            key=lambda item: (
                int(float(item["n_operations"])),
                -float(item["flexibility"]),
                item["source"],
            ),
        )
        indexes = {0, len(family_rows) // 2, len(family_rows) - 1}
        for index in sorted(indexes):
            selected.append(family_rows[index])
    return selected


def write_readme(
    output_dir: Path,
    layer1: List[Dict[str, str]],
    layer2: List[Dict[str, str]],
    layer3: List[Dict[str, str]],
) -> None:
    lines = [
        "# Experiment Layers",
        "",
        "This directory separates the paper experiments into reproducible data layers.",
        "",
        "## Layer 1: Competition 30",
        "",
        f"- File: `layer1_competition30.csv`",
        f"- Instances: {len(layer1)}",
        "- Role: main Scenario 1 competition table and ATPTS/reference comparison.",
        "- Protocol: 10 independent runs per instance, distinct random seeds, official feasibility and makespan evaluation.",
        "- Manifest paths point to the repository-local public `.fjs` files; these have been verified identical to the adjacent ATPTS submission-instance copies on this workspace.",
        "- The `lb`/`ub` columns are the repository public best-known table; competition claims should use `reference_best`, `reference_avg`, `current_best`, and `current_avg`.",
        "",
        "## Layer 2: Public 402",
        "",
        f"- File: `layer2_public402.csv`",
        f"- Instances: {len(layer2)}",
        "- Role: public generalization study against the repository-provided LB/UB table.",
        "- Protocol: use the same solver settings as Layer 1 where feasible; report solved-to-UB, improved-UB, gap-to-LB, and runtime summaries by family and size.",
        "",
        "## Layer 3: Ablation Representatives",
        "",
        f"- File: `layer3_ablation_representatives.csv`",
        f"- Instances: {len(layer3)}",
        "- Role: configuration and mechanism ablations.",
        "- Selection rule: for each benchmark family, choose the minimum-, median-, and maximum-operation instance after sorting by operation count.",
        "- Planned ablations: worker-aware graph on/off, worker-aware neighborhood on/off, approximate screening on/off, warm start on/off, and seed count sensitivity.",
        "",
        "## Notes",
        "",
        "- Do not use newly generated FJSSP-W instances for best-known comparisons unless a fresh baseline is generated for every compared solver.",
        "- Synthetic stress tests are intentionally out of scope for the current paper pass.",
        "",
    ]
    (output_dir / "README.md").write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    args = parse_args()
    root = Path(args.root).resolve()
    output_dir = Path(args.output_dir).resolve()
    data_csv = root / "instances" / "InstanceData" / "FJSSP-W" / "data.csv"
    best_csv = root / "instances" / "InstanceData" / "FJSSP-W" / "best_known.csv"
    competition_csv = Path(args.competition_csv).resolve()
    reference_csv = Path(args.reference_csv).resolve()

    data_rows = read_csv(data_csv)
    data_by_source = {row["source"]: row for row in data_rows}
    best_rows = {row["Instance"]: row for row in read_csv(best_csv, delimiter=";")}

    competition_labels = unique_competition_instances(competition_csv)
    reference_summaries = summarize_submission(reference_csv)
    current_summaries = summarize_submission(competition_csv)
    competition_by_source = {
        source_from_competition(label): label for label in competition_labels
    }

    layer1 = []
    for label in competition_labels:
        source = source_from_competition(label)
        layer1.append(
            base_row(
                "layer1_competition30",
                data_by_source[source],
                best_rows.get(best_known_name_from_source(source)),
                root,
                public_benchmark_path(root, source),
                label,
                reference_summaries.get(label),
                current_summaries.get(label),
                True,
                "official_competition_instance",
            )
        )

    layer2 = []
    for data_row in sorted(data_rows, key=lambda item: (family_from_source(item["source"]), item["source"])):
        source = data_row["source"]
        layer2.append(
            base_row(
                "layer2_public402",
                data_row,
                best_rows.get(best_known_name_from_source(source)),
                root,
                public_benchmark_path(root, source),
                competition_by_source.get(source, ""),
                None,
                None,
                source in competition_by_source,
                "full_public_fjsspw_set",
            )
        )

    representatives = choose_representatives(data_rows)
    layer3 = []
    for data_row in representatives:
        source = data_row["source"]
        layer3.append(
            base_row(
                "layer3_ablation_representatives",
                data_row,
                best_rows.get(best_known_name_from_source(source)),
                root,
                public_benchmark_path(root, source),
                competition_by_source.get(source, ""),
                None,
                None,
                source in competition_by_source,
                "family_min_median_max_operation_count",
            )
        )

    write_manifest(output_dir / "layer1_competition30.csv", layer1)
    write_manifest(output_dir / "layer2_public402.csv", layer2)
    write_manifest(output_dir / "layer3_ablation_representatives.csv", layer3)
    write_readme(output_dir, layer1, layer2, layer3)

    print(f"Wrote {len(layer1)} rows to {output_dir / 'layer1_competition30.csv'}")
    print(f"Wrote {len(layer2)} rows to {output_dir / 'layer2_public402.csv'}")
    print(f"Wrote {len(layer3)} rows to {output_dir / 'layer3_ablation_representatives.csv'}")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Convert the Gong DFJSP01-10 benchmark text dump to FJSSP-W .fjs files.

The source document lists, for each operation-machine option, one row per
worker with six fields:
processing cost, processing time, energy, noise, recycling, safety.
This converter keeps the processing-time column because the Li et al. DRCFJSP
comparison minimizes makespan.
"""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path


INSTANCE_RE = re.compile(
    r"(?:^|\n)\s*(?:\d+\.)?\s*DFJSP\s*0?(\d+)\s*"
    r"\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)",
    re.IGNORECASE,
)
NUMBER_RE = re.compile(r"-?\d+(?:\.\d+)?")


@dataclass
class DfjspInstance:
    name: str
    job_count: int
    machine_count: int
    worker_count: int
    operations_per_job: list[int]
    machine_counts: list[int]
    machine_ids: list[int]
    data_rows: list[list[float]]


def normalize_text(text: str) -> str:
    return (
        text.replace("\uff08", "(")
        .replace("\uff09", ")")
        .replace("\u3001", ".")
        .replace("\ufeff", "")
    )


def ints_from(text: str) -> list[int]:
    return [int(float(x)) for x in NUMBER_RE.findall(text)]


def extract_between(section: str, start_label: str, end_label: str) -> str:
    lower = section.lower()
    start = lower.find(start_label.lower())
    if start < 0:
        raise ValueError(f"Missing label: {start_label}")
    end = lower.find(end_label.lower(), start + len(start_label))
    if end < 0:
        raise ValueError(f"Missing label: {end_label}")
    return section[start + len(start_label) : end]


def parse_data_rows(section: str) -> list[list[float]]:
    lower = section.lower()
    start = lower.find("data array")
    if start < 0:
        raise ValueError("Missing label: Data array")

    rows: list[list[float]] = []
    for line in section[start:].replace("\f", "\n").splitlines():
        values = NUMBER_RE.findall(line)
        if len(values) == 6:
            rows.append([float(value) for value in values])
    return rows


def parse_instances(text: str) -> list[DfjspInstance]:
    text = normalize_text(text)
    matches = list(INSTANCE_RE.finditer(text))
    if not matches:
        raise ValueError("No DFJSP instance headers found.")

    instances: list[DfjspInstance] = []
    for idx, match in enumerate(matches):
        end = matches[idx + 1].start() if idx + 1 < len(matches) else len(text)
        section = text[match.end() : end]

        instance_id = int(match.group(1))
        job_count = int(match.group(2))
        machine_count = int(match.group(3))
        worker_count = int(match.group(4))

        operations = ints_from(
            extract_between(
                section,
                "Operations vector:",
                "Available machine quantity vector:",
            )
        )
        machine_counts = ints_from(
            extract_between(
                section,
                "Available machine quantity vector:",
                "Available machine vector:",
            )
        )
        machine_ids = ints_from(extract_between(section, "Available machine vector:", "Data array"))
        data_rows = parse_data_rows(section)

        expected_operations = sum(operations)
        expected_machine_rows = sum(machine_counts)
        expected_data_rows = expected_machine_rows * worker_count
        if len(operations) != job_count:
            raise ValueError(f"DFJSP{instance_id:02d}: operation vector has wrong length")
        if len(machine_counts) != expected_operations:
            raise ValueError(f"DFJSP{instance_id:02d}: machine-count vector has wrong length")
        if len(machine_ids) != expected_machine_rows:
            raise ValueError(f"DFJSP{instance_id:02d}: machine-id vector has wrong length")
        if len(data_rows) != expected_data_rows:
            raise ValueError(
                f"DFJSP{instance_id:02d}: expected {expected_data_rows} data rows, "
                f"found {len(data_rows)}"
            )

        instances.append(
            DfjspInstance(
                name=f"DFJSP{instance_id:02d}",
                job_count=job_count,
                machine_count=machine_count,
                worker_count=worker_count,
                operations_per_job=operations,
                machine_counts=machine_counts,
                machine_ids=machine_ids,
                data_rows=data_rows,
            )
        )

    return instances


def to_fjsspw_lines(instance: DfjspInstance) -> list[str]:
    lines = [f"{instance.job_count} {instance.machine_count} {instance.worker_count}"]

    op_cursor = 0
    machine_cursor = 0
    data_cursor = 0
    for operation_count in instance.operations_per_job:
        tokens = [str(operation_count)]
        for _ in range(operation_count):
            machine_count = instance.machine_counts[op_cursor]
            tokens.append(str(machine_count))
            for _ in range(machine_count):
                machine_id = instance.machine_ids[machine_cursor]
                machine_cursor += 1
                tokens.extend([str(machine_id), str(instance.worker_count)])
                for worker_id in range(1, instance.worker_count + 1):
                    row = instance.data_rows[data_cursor]
                    data_cursor += 1
                    processing_time = int(round(row[1]))
                    tokens.extend([str(worker_id), str(processing_time)])
            op_cursor += 1
        lines.append(" ".join(tokens))

    if machine_cursor != len(instance.machine_ids):
        raise ValueError(f"{instance.name}: not all machine ids were consumed")
    if data_cursor != len(instance.data_rows):
        raise ValueError(f"{instance.name}: not all data rows were consumed")
    return lines


def convert(input_path: Path, output_dir: Path) -> list[Path]:
    text = input_path.read_text(encoding="utf-8", errors="ignore")
    instances = parse_instances(text)
    output_dir.mkdir(parents=True, exist_ok=True)

    written: list[Path] = []
    for instance in instances:
        output_path = output_dir / f"{instance.name}.fjs"
        output_path.write_text("\n".join(to_fjsspw_lines(instance)) + "\n", encoding="utf-8")
        written.append(output_path)
    return written


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("input_text", type=Path)
    parser.add_argument("output_dir", type=Path)
    args = parser.parse_args()

    written = convert(args.input_text, args.output_dir)
    for path in written:
        print(path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path


def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else "benchmarks/results/mda_jx10_matrix")
    files = sorted(root.glob("*.json"))
    if not files:
        print(f"No benchmark JSON files found under {root}", file=sys.stderr)
        return 1

    rows = []
    for path in files:
        data = json.loads(path.read_text())
        audio = data["audio"]
        midi = data["workload"]["midi"]
        timing = data["timing"]
        rows.append(
            (
                audio["buffer_samples"],
                midi["voices"],
                audio["deadline_ms"],
                timing["mean_ms"],
                timing["p95_ms"],
                timing["p99_ms"],
                timing["worst_ms"],
                timing["deadline_misses"],
                data["process"]["peak_rss_bytes"] / (1024 * 1024),
            )
        )

    rows.sort(key=lambda r: (-r[0], r[1]))
    print("buffer  voices  deadline_ms  mean_ms   p95_ms   p99_ms   worst_ms  misses  peak_rss_mb")
    print("------  ------  -----------  --------  --------  --------  --------  ------  -----------")
    for row in rows:
        b, v, dl, mean, p95, p99, worst, misses, rss = row
        print(f"{b:6d}  {v:6d}  {dl:11.4f}  {mean:8.4f}  {p95:8.4f}  {p99:8.4f}  {worst:8.4f}  {misses:6d}  {rss:11.2f}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

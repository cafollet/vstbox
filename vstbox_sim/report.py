from __future__ import annotations

import json
from dataclasses import asdict
from pathlib import Path

from .model import SimulationResult


def render_text(result: SimulationResult) -> str:
    return f"""VSTBOX PI 5 FEASIBILITY REPORT
================================
Hardware: {result.hardware['name']}
Audio: {result.audio['sample_rate_hz']} Hz / {result.audio['buffer_samples']} samples
Deadline: {result.deadline_ms:.3f} ms
Workload: {result.workload['name']}
Backend: {result.workload['backend']}

Realtime timing
---------------
Mean:   {result.mean_ms:.3f} ms
P50:    {result.p50_ms:.3f} ms
P95:    {result.p95_ms:.3f} ms
P99:    {result.p99_ms:.3f} ms
Worst:  {result.worst_ms:.3f} ms
P99 realtime-core utilization: {result.realtime_core_utilization_p99:.1%}
Deadline misses: {result.deadline_misses}/{result.callbacks}
Estimated xruns/hour: {result.estimated_xruns_per_hour:.3f}

Memory
------
Used:      {result.ram_used_mb} MB
Remaining: {result.ram_remaining_mb} MB

Rating: {result.rating}
Reason(s):
""" + "\n".join(f"- {reason}" for reason in result.reasons)


def write_json(result: SimulationResult, path: str | Path) -> None:
    Path(path).write_text(json.dumps(asdict(result), indent=2), encoding="utf-8")

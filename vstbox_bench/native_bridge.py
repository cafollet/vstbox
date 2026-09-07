from __future__ import annotations

import json
import shutil
import subprocess
from pathlib import Path
from typing import Any


def default_native_benchmark_path() -> Path:
    return Path(__file__).resolve().parents[1] / "build" / "native" / "vstbox_synthetic_bench"


def run_native_synthetic(
    *,
    sample_rate: int,
    buffer_samples: int,
    voices: int,
    callbacks: int,
    warmup_callbacks: int,
    seed: int,
    executable: str | Path | None = None,
) -> dict[str, Any]:
    candidate = Path(executable) if executable else default_native_benchmark_path()
    resolved = shutil.which(str(candidate)) if not candidate.exists() else str(candidate)
    if resolved is None:
        raise FileNotFoundError(
            f"Native benchmark not found at {candidate}. Build it with ./scripts/build_native.sh"
        )

    command = [
        resolved,
        "--sample-rate", str(sample_rate),
        "--buffer", str(buffer_samples),
        "--voices", str(voices),
        "--callbacks", str(callbacks),
        "--warmup", str(warmup_callbacks),
        "--seed", str(seed),
        "--json",
    ]
    completed = subprocess.run(command, check=True, capture_output=True, text=True)
    return json.loads(completed.stdout)

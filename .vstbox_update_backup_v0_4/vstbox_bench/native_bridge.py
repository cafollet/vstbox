from __future__ import annotations

import json
import shutil
import subprocess
from pathlib import Path
from typing import Any


def default_native_benchmark_path() -> Path:
    return Path(__file__).resolve().parents[1] / "build" / "native" / "vstbox_synthetic_bench"


def default_vst3_benchmark_path() -> Path:
    return Path(__file__).resolve().parents[1] / "build" / "native-vst3" / "vstbox_vst3_bench"


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


def run_native_vst3(
    *,
    plugin: str | Path,
    class_name: str | None,
    sample_rate: int,
    buffer_samples: int,
    callbacks: int,
    warmup_callbacks: int,
    midi_enabled: bool,
    midi_note: int,
    midi_velocity: float,
    midi_cycle: int,
    midi_gate: int,
    executable: str | Path | None = None,
) -> dict[str, Any]:
    candidate = Path(executable) if executable else default_vst3_benchmark_path()
    resolved = shutil.which(str(candidate)) if not candidate.exists() else str(candidate)
    if resolved is None:
        raise FileNotFoundError(
            f"Native VST3 benchmark not found at {candidate}. Build it with ./scripts/build_vst3_benchmark.sh"
        )

    command = [
        resolved,
        "--plugin", str(Path(plugin).expanduser()),
        "--sample-rate", str(sample_rate),
        "--buffer", str(buffer_samples),
        "--callbacks", str(callbacks),
        "--warmup", str(warmup_callbacks),
        "--midi-note", str(midi_note),
        "--midi-velocity", str(midi_velocity),
        "--midi-cycle", str(midi_cycle),
        "--midi-gate", str(midi_gate),
        "--json",
    ]
    if class_name:
        command.extend(["--class", class_name])
    if not midi_enabled:
        command.append("--no-midi")

    completed = subprocess.run(command, check=True, capture_output=True, text=True)
    return json.loads(completed.stdout)

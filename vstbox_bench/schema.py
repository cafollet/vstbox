from __future__ import annotations

from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any
import json


SCHEMA_VERSION = 1


@dataclass(frozen=True)
class MachineProfile:
    schema_version: int
    captured_at_utc: str
    hostname: str
    os_name: str
    os_version: str
    architecture: str
    cpu_brand: str
    physical_cores: int | None
    logical_cores: int | None
    memory_bytes: int | None
    python_version: str

    @classmethod
    def now(cls, **kwargs: Any) -> "MachineProfile":
        return cls(
            schema_version=SCHEMA_VERSION,
            captured_at_utc=datetime.now(timezone.utc).isoformat(),
            **kwargs,
        )

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass(frozen=True)
class AudioBenchmarkConfig:
    sample_rate_hz: int
    buffer_samples: int
    callbacks: int
    warmup_callbacks: int
    voices: int
    seed: int = 42

    @property
    def deadline_ms(self) -> float:
        return self.buffer_samples / self.sample_rate_hz * 1000.0

    def validate(self) -> None:
        if self.sample_rate_hz <= 0:
            raise ValueError("sample_rate_hz must be positive")
        if self.buffer_samples <= 0:
            raise ValueError("buffer_samples must be positive")
        if self.callbacks <= 0:
            raise ValueError("callbacks must be positive")
        if self.warmup_callbacks < 0:
            raise ValueError("warmup_callbacks cannot be negative")
        if self.voices <= 0:
            raise ValueError("voices must be positive")


@dataclass(frozen=True)
class TimingSummary:
    mean_ms: float
    p50_ms: float
    p95_ms: float
    p99_ms: float
    worst_ms: float
    deadline_misses: int
    miss_rate: float


@dataclass(frozen=True)
class BenchmarkResult:
    schema_version: int
    benchmark_kind: str
    benchmark_version: str
    captured_at_utc: str
    machine: dict[str, Any]
    audio: dict[str, Any]
    workload: dict[str, Any]
    timing: dict[str, Any]
    process: dict[str, Any]
    notes: list[str]

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


def write_json(data: dict[str, Any], path: str | Path) -> None:
    destination = Path(path)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def read_json(path: str | Path) -> dict[str, Any]:
    return json.loads(Path(path).read_text(encoding="utf-8"))

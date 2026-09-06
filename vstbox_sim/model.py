from __future__ import annotations

from dataclasses import dataclass, asdict
from math import inf
import random
from statistics import mean
from typing import Any


@dataclass(frozen=True)
class HardwareProfile:
    name: str = "Raspberry Pi 5 8GB"
    cpu_cores: int = 4
    cpu_ghz: float = 2.4
    ram_mb: int = 8192
    reserved_ram_mb: int = 1024


@dataclass(frozen=True)
class AudioProfile:
    sample_rate_hz: int = 48_000
    buffer_samples: int = 128

    @property
    def deadline_ms(self) -> float:
        return self.buffer_samples / self.sample_rate_hz * 1000.0


@dataclass(frozen=True)
class WorkloadProfile:
    name: str
    backend: str
    dsp_mean_ms: float
    dsp_jitter_ms: float
    host_mean_ms: float = 0.10
    host_jitter_ms: float = 0.03
    ipc_mean_ms: float = 0.0
    ipc_jitter_ms: float = 0.0
    translation_multiplier: float = 1.0
    scheduler_spike_probability: float = 0.001
    scheduler_spike_ms: float = 0.50
    plugin_ram_mb: int = 512
    runtime_ram_mb: int = 256
    sample_ram_mb: int = 0


@dataclass(frozen=True)
class SimulationConfig:
    callbacks: int = 100_000
    seed: int = 42


@dataclass
class SimulationResult:
    hardware: dict[str, Any]
    audio: dict[str, Any]
    workload: dict[str, Any]
    callbacks: int
    p50_ms: float
    p95_ms: float
    p99_ms: float
    worst_ms: float
    mean_ms: float
    deadline_ms: float
    deadline_misses: int
    miss_rate: float
    estimated_xruns_per_hour: float
    realtime_core_utilization_p99: float
    ram_used_mb: int
    ram_remaining_mb: int
    rating: str
    reasons: list[str]


def _quantile(sorted_values: list[float], q: float) -> float:
    if not sorted_values:
        return 0.0
    idx = min(len(sorted_values) - 1, max(0, round((len(sorted_values) - 1) * q)))
    return sorted_values[idx]


def _normal_nonnegative(rng: random.Random, mean_value: float, jitter: float) -> float:
    if jitter <= 0:
        return max(0.0, mean_value)
    return max(0.0, rng.gauss(mean_value, jitter))


def run_simulation(
    hardware: HardwareProfile,
    audio: AudioProfile,
    workload: WorkloadProfile,
    config: SimulationConfig,
) -> SimulationResult:
    rng = random.Random(config.seed)
    callback_times: list[float] = []
    deadline = audio.deadline_ms

    for _ in range(config.callbacks):
        dsp = _normal_nonnegative(rng, workload.dsp_mean_ms, workload.dsp_jitter_ms)
        dsp *= workload.translation_multiplier
        host = _normal_nonnegative(rng, workload.host_mean_ms, workload.host_jitter_ms)
        ipc = _normal_nonnegative(rng, workload.ipc_mean_ms, workload.ipc_jitter_ms)
        spike = workload.scheduler_spike_ms if rng.random() < workload.scheduler_spike_probability else 0.0
        callback_times.append(dsp + host + ipc + spike)

    callback_times.sort()
    misses = sum(value > deadline for value in callback_times)
    miss_rate = misses / config.callbacks if config.callbacks else 0.0

    callbacks_per_second = audio.sample_rate_hz / audio.buffer_samples
    xruns_per_hour = miss_rate * callbacks_per_second * 3600.0

    p50 = _quantile(callback_times, 0.50)
    p95 = _quantile(callback_times, 0.95)
    p99 = _quantile(callback_times, 0.99)
    worst = callback_times[-1] if callback_times else 0.0
    avg = mean(callback_times) if callback_times else 0.0
    realtime_core_util_p99 = p99 / deadline if deadline > 0 else inf

    ram_used = (
        hardware.reserved_ram_mb
        + workload.plugin_ram_mb
        + workload.runtime_ram_mb
        + workload.sample_ram_mb
    )
    ram_remaining = hardware.ram_mb - ram_used
    ram_fraction = ram_used / hardware.ram_mb if hardware.ram_mb else 1.0

    reasons: list[str] = []
    if misses > 0:
        reasons.append(f"{misses} modeled callback deadlines missed")
    if realtime_core_util_p99 >= 0.75:
        reasons.append(f"P99 realtime-core utilization is {realtime_core_util_p99:.0%}")
    if ram_fraction >= 0.80:
        reasons.append(f"Modeled RAM use is {ram_fraction:.0%} of physical memory")

    if misses == 0 and realtime_core_util_p99 < 0.60 and ram_fraction < 0.70:
        rating = "GREEN"
    elif miss_rate <= 1e-5 and realtime_core_util_p99 < 0.80 and ram_fraction < 0.85:
        rating = "YELLOW"
    else:
        rating = "RED"

    if not reasons:
        reasons.append("No modeled bottleneck crossed the conservative warning thresholds")

    return SimulationResult(
        hardware=asdict(hardware),
        audio={**asdict(audio), "deadline_ms": deadline},
        workload=asdict(workload),
        callbacks=config.callbacks,
        p50_ms=p50,
        p95_ms=p95,
        p99_ms=p99,
        worst_ms=worst,
        mean_ms=avg,
        deadline_ms=deadline,
        deadline_misses=misses,
        miss_rate=miss_rate,
        estimated_xruns_per_hour=xruns_per_hour,
        realtime_core_utilization_p99=realtime_core_util_p99,
        ram_used_mb=ram_used,
        ram_remaining_mb=ram_remaining,
        rating=rating,
        reasons=reasons,
    )

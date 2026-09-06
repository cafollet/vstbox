from __future__ import annotations

import json
from pathlib import Path

from .model import AudioProfile, HardwareProfile, SimulationConfig, WorkloadProfile


def load_scenario(path: str | Path):
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    hardware = HardwareProfile(**data.get("hardware", {}))
    audio = AudioProfile(**data.get("audio", {}))
    workload = WorkloadProfile(**data["workload"])
    simulation = SimulationConfig(**data.get("simulation", {}))
    return hardware, audio, workload, simulation

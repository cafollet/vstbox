from __future__ import annotations

import argparse
import json
from pathlib import Path

from vstbox_bench.cli_vst3 import build_result
from vstbox_bench.native_bridge import default_vst3_benchmark_path, run_native_vst3


def test_default_vst3_benchmark_path_points_to_native_vst3_build() -> None:
    path = default_vst3_benchmark_path()
    assert path.parts[-3:] == ("build", "native-vst3", "vstbox_vst3_bench")


def test_run_native_vst3_parses_json_and_passes_plugin(tmp_path: Path) -> None:
    executable = tmp_path / "fake_vst3_bench"
    executable.write_text(
        "#!/bin/sh\n"
        "printf '%s\\n' '{\"plugin\":{\"class_name\":\"ADelay\"},\"timing\":{\"p99_ms\":0.1},\"process\":{\"peak_rss_bytes\":123}}'\n",
        encoding="utf-8",
    )
    executable.chmod(0o755)

    result = run_native_vst3(
        plugin="Example.vst3",
        class_name=None,
        sample_rate=48_000,
        buffer_samples=128,
        callbacks=10,
        warmup_callbacks=1,
        midi_enabled=False,
        midi_note=48,
        midi_note_step=1,
        midi_voices=4,
        midi_velocity=0.8,
        midi_cycle=128,
        midi_gate=96,
        executable=executable,
    )
    assert result["plugin"]["class_name"] == "ADelay"
    assert result["timing"]["p99_ms"] == 0.1


def test_vst3_result_keeps_raw_measurement_and_machine_metadata() -> None:
    args = argparse.Namespace(
        plugin="/tmp/adelay.vst3",
        class_name=None,
        sample_rate=48_000,
        buffer=128,
        callbacks=20_000,
        warmup=1_000,
        no_midi=True,
        midi_note=48,
        midi_note_step=1,
        voices=4,
        midi_velocity=0.8,
        midi_cycle=128,
        midi_gate=96,
    )
    native = {
        "plugin": {"class_name": "ADelay", "parameters": []},
        "timing": {"p99_ms": 0.25, "deadline_misses": 0},
        "process": {"peak_rss_bytes": 1024},
    }
    machine = {"architecture": "x86_64", "cpu_brand": "test"}
    result = build_result(args, native, machine)

    assert result["benchmark_kind"] == "vst3-offline-callback"
    assert result["benchmark_version"] == "0.4.0"
    assert result["machine"] == machine
    assert result["plugin"]["class_name"] == "ADelay"
    assert result["timing"]["p99_ms"] == 0.25
    assert result["audio"]["deadline_ms"] == 128 / 48_000 * 1000
    assert result["workload"]["midi"]["voices"] == 4
    assert result["workload"]["midi"]["note_step"] == 1
    assert "No Mac-to-Pi performance scaling has been applied." in result["notes"]

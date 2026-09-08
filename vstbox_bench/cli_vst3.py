from __future__ import annotations

import argparse
import json
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from .machine import capture_machine_profile
from .native_bridge import run_native_vst3
from .schema import SCHEMA_VERSION, write_json


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Benchmark a real VST3 plug-in through the native VSTBox offline host")
    parser.add_argument("--plugin", required=True, help="Path to a .vst3 bundle")
    parser.add_argument("--class", dest="class_name", help="Optional exact VST3 component class name")
    parser.add_argument("--sample-rate", type=int, default=48_000)
    parser.add_argument("--buffer", type=int, default=128)
    parser.add_argument("--callbacks", type=int, default=20_000)
    parser.add_argument("--warmup", type=int, default=1_000)
    parser.add_argument("--no-midi", action="store_true", help="Do not inject deterministic note events")
    parser.add_argument("--midi-note", type=int, default=60)
    parser.add_argument("--midi-velocity", type=float, default=0.8)
    parser.add_argument("--midi-cycle", type=int, default=128)
    parser.add_argument("--midi-gate", type=int, default=96)
    parser.add_argument("--native-exe", help="Override the native VST3 benchmark executable path")
    parser.add_argument("--json-out", help="Write the combined benchmark profile to this path")
    return parser


def build_result(args: argparse.Namespace, native: dict[str, Any], machine: dict[str, Any]) -> dict[str, Any]:
    deadline_ms = args.buffer / args.sample_rate * 1000.0
    plugin = native.get("plugin", {})
    return {
        "schema_version": SCHEMA_VERSION,
        "benchmark_kind": "vst3-offline-callback",
        "benchmark_version": "0.3.0",
        "captured_at_utc": datetime.now(timezone.utc).isoformat(),
        "machine": machine,
        "audio": {
            "sample_rate_hz": args.sample_rate,
            "buffer_samples": args.buffer,
            "deadline_ms": deadline_ms,
        },
        "workload": {
            "plugin_path": str(Path(args.plugin).expanduser()),
            "plugin_class": plugin.get("class_name"),
            "callbacks": args.callbacks,
            "warmup_callbacks": args.warmup,
            "input_signal": "220 Hz sine wave for audio-input plug-ins",
            "midi": {
                "enabled": not args.no_midi,
                "note": args.midi_note,
                "velocity": args.midi_velocity,
                "cycle_callbacks": args.midi_cycle,
                "gate_callbacks": args.midi_gate,
            },
        },
        "plugin": plugin,
        "timing": native["timing"],
        "process": native["process"],
        "notes": [
            "This benchmark calls the plug-in directly through the VST3 processing API; JACK/CoreAudio device timing is not included.",
            "Timing includes the lightweight VSTBox host wrapper around each process call as well as the plug-in DSP.",
            "No Mac-to-Pi performance scaling has been applied.",
        ],
    }


def main() -> int:
    args = build_parser().parse_args()
    if args.sample_rate <= 0 or args.buffer <= 0 or args.callbacks <= 0 or args.warmup < 0:
        raise SystemExit("sample-rate, buffer, and callbacks must be positive; warmup cannot be negative")
    if not 0 <= args.midi_note <= 127:
        raise SystemExit("midi-note must be between 0 and 127")
    if not 0.0 <= args.midi_velocity <= 1.0:
        raise SystemExit("midi-velocity must be between 0 and 1")
    if args.midi_cycle <= 0 or args.midi_gate < 0 or args.midi_gate >= args.midi_cycle:
        raise SystemExit("midi-cycle must be positive and midi-gate must be in [0, midi-cycle)")

    native = run_native_vst3(
        plugin=args.plugin,
        class_name=args.class_name,
        sample_rate=args.sample_rate,
        buffer_samples=args.buffer,
        callbacks=args.callbacks,
        warmup_callbacks=args.warmup,
        midi_enabled=not args.no_midi,
        midi_note=args.midi_note,
        midi_velocity=args.midi_velocity,
        midi_cycle=args.midi_cycle,
        midi_gate=args.midi_gate,
        executable=args.native_exe,
    )
    result = build_result(args, native, capture_machine_profile().to_dict())
    print(json.dumps(result, indent=2, sort_keys=True))
    if args.json_out:
        write_json(result, args.json_out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

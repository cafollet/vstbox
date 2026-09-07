from __future__ import annotations

import argparse
import json
from datetime import datetime, timezone

from .machine import capture_machine_profile
from .native_bridge import run_native_synthetic
from .schema import SCHEMA_VERSION, write_json


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Run the native VSTBox synthetic audio benchmark")
    parser.add_argument("--sample-rate", type=int, default=48_000)
    parser.add_argument("--buffer", type=int, default=128)
    parser.add_argument("--voices", type=int, default=16)
    parser.add_argument("--callbacks", type=int, default=20_000)
    parser.add_argument("--warmup", type=int, default=1_000)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--native-exe", help="Override the native benchmark executable path")
    parser.add_argument("--json-out", help="Write the combined benchmark profile to this path")
    return parser


def main() -> int:
    args = build_parser().parse_args()
    if args.sample_rate <= 0 or args.buffer <= 0 or args.voices <= 0 or args.callbacks <= 0 or args.warmup < 0:
        raise SystemExit("sample-rate, buffer, voices, and callbacks must be positive; warmup cannot be negative")

    native = run_native_synthetic(
        sample_rate=args.sample_rate,
        buffer_samples=args.buffer,
        voices=args.voices,
        callbacks=args.callbacks,
        warmup_callbacks=args.warmup,
        seed=args.seed,
        executable=args.native_exe,
    )
    machine = capture_machine_profile().to_dict()
    deadline_ms = args.buffer / args.sample_rate * 1000.0

    result = {
        "schema_version": SCHEMA_VERSION,
        "benchmark_kind": "synthetic-audio-callback",
        "benchmark_version": "0.2.0",
        "captured_at_utc": datetime.now(timezone.utc).isoformat(),
        "machine": machine,
        "audio": {
            "sample_rate_hz": args.sample_rate,
            "buffer_samples": args.buffer,
            "deadline_ms": deadline_ms,
        },
        "workload": {
            "processor": "vstbox-synthetic-voice-bank",
            "voices": args.voices,
            "callbacks": args.callbacks,
            "warmup_callbacks": args.warmup,
            "seed": args.seed,
        },
        "timing": native["timing"],
        "process": native["process"],
        "notes": [
            "This measures the VSTBox synthetic DSP callback on the current desktop CPU.",
            "It is a measurement harness validation workload, not a VST plugin benchmark.",
            "No Mac-to-Pi performance scaling has been applied.",
        ],
    }

    print(json.dumps(result, indent=2, sort_keys=True))
    if args.json_out:
        write_json(result, args.json_out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

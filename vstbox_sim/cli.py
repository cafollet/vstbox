from __future__ import annotations

import argparse

from .config import load_scenario
from .model import run_simulation
from .report import render_text, write_json


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Simulate VSTBox workload feasibility on target hardware")
    parser.add_argument("scenario", help="Path to scenario JSON")
    parser.add_argument("--json-out", help="Optional path for machine-readable result JSON")
    return parser


def main() -> int:
    args = build_parser().parse_args()
    hardware, audio, workload, simulation = load_scenario(args.scenario)
    result = run_simulation(hardware, audio, workload, simulation)
    print(render_text(result))
    if args.json_out:
        write_json(result, args.json_out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

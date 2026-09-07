from __future__ import annotations

import argparse
import json

from .machine import capture_machine_profile
from .schema import write_json


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Capture the machine profile used for VSTBox benchmarks")
    parser.add_argument("--json-out", help="Write the profile to this JSON file")
    return parser


def main() -> int:
    args = build_parser().parse_args()
    profile = capture_machine_profile().to_dict()
    print(json.dumps(profile, indent=2, sort_keys=True))
    if args.json_out:
        write_json(profile, args.json_out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

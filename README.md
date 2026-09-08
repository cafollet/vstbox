# VSTBox Pre-Hardware Prototype

This repository is the first executable stage of the VSTBox project: a standalone hardware appliance intended to host software instruments/effects without requiring a laptop during use.

## Project status and independence

VSTBox is an **independent, developer-led prototype and research project created by Callum Follett**. It is not currently a product of, sponsored by, endorsed by, or representative of any company, employer, plug-in vendor, Steinberg Media Technologies GmbH, or hardware manufacturer.

The project is exploring the technical feasibility of a standalone hardware host for software instruments and effects, beginning with inexpensive ARM hardware such as Raspberry Pi-class devices. Results in this repository should be treated as prototype/research findings rather than product guarantees or compatibility certifications.

## Current goal

Before buying a Raspberry Pi 5, determine whether an 8 GB Pi 5 is *plausibly* capable of the target workloads.

This simulator does **not** claim to reproduce Raspberry Pi USB-audio timing, Linux scheduler behavior, thermals, or Box64/Wine performance exactly. Instead, it provides a conservative, explicit workload model that we can progressively replace with measured data.

## What exists in v0.1

- Pi 5 hardware profile
- 48 kHz / configurable audio-buffer deadline calculations
- Monte Carlo callback-time simulation
- P50/P95/P99/worst-case timing
- estimated deadline misses / xruns per hour
- realtime-core utilization estimate
- memory budget model
- GREEN / YELLOW / RED feasibility rating
- separate native-ARM and Windows-x64/Box64 example scenarios
- JSON output for later benchmark aggregation

## Run it

From the repository root:

```bash
python -m vstbox_sim.cli configs/native_arm_baseline.json
```

Windows-compatibility scenario:

```bash
python -m vstbox_sim.cli configs/windows_x64_box64_conservative.json
```

Machine-readable results:

```bash
python -m vstbox_sim.cli configs/native_arm_baseline.json --json-out native-result.json
```

## Tests

```bash
python -m pytest -q
```

## Important limitation

The current workload numbers are deliberately labeled **synthetic**. They are placeholders for the first architecture test only.

The next milestone is a desktop plugin benchmark harness that records real per-buffer DSP timing and memory use. Those measurements will become inputs to this simulator, making the Pi feasibility estimate much more defensible.

## Intended development sequence

1. Pre-hardware simulator (current)
2. Desktop reference plugin benchmark harness
3. Plugin inspector and package manifest
4. Real plugin workload profiles
5. ARM64 cross-build / functional emulation
6. Pi purchase gate based on measured workloads
7. Physical Pi validation

## macOS local development

The recommended working model is to keep a normal Git checkout on your Mac and use it as the canonical local development copy.

### One-time prerequisites

1. Install Apple's Command Line Tools if they are not already present:

```bash
xcode-select --install
```

2. Python 3.10+ is required. Homebrew is optional, but the included `Brewfile` can install the CMake/Ninja/Python tooling we will need as the project grows:

```bash
brew bundle
```

### Bootstrap the repo

From the repository root:

```bash
./scripts/bootstrap_macos.sh
```

This creates `.venv`, installs VSTBox in editable mode with development dependencies, and runs the tests.

Later sessions only need:

```bash
cd ~/Desktop/vstbox
source .venv/bin/activate
```

### IDE

The repo includes VS Code extension recommendations for Python, C/C++, CMake, and Remote SSH. This keeps the same editor useful later when development moves from the Mac to a Raspberry Pi over SSH.

## Git workflow

For ongoing synchronization, use the Git remote (for example the public GitHub repository) rather than exchanging ZIP files. A typical workflow is:

```bash
git status
git add .
git commit -m "Describe the change"
git push
```

and on another machine:

```bash
git pull
```

If no remote has been configured yet, the repository still works normally as a local Git repository; a remote can be attached later without changing the project layout.


## Intel macOS Python note

If `python3` points to an older system Python, explicitly select a newer interpreter, for example:

```bash
PYTHON_BIN=/usr/local/bin/python3.13 ./scripts/bootstrap_macos.sh
```

The bootstrap script now also searches for Python 3.13 through 3.10 automatically.

## v0.2: measured desktop benchmark baseline

v0.2 adds the first measured benchmark layer:

- `vstbox-machine` captures the reference computer's CPU, architecture, RAM, and OS metadata.
- `native/` contains a small C++17 realtime-style processing interface and synthetic DSP workload.
- `vstbox-bench-synthetic` runs that native workload and stores machine + audio + timing metadata in one JSON result.
- `docs/BENCHMARKING.md` defines the benchmark methodology and the boundary for the upcoming VST3 adapter.

On macOS, after the Python environment is active:

```bash
brew install cmake ninja   # only if these are not already installed
./scripts/run_v02_smoke.sh
```

Or run each stage separately:

```bash
vstbox-machine --json-out benchmarks/profiles/current_machine.json
./scripts/build_native.sh
vstbox-bench-synthetic --voices 16 --json-out benchmarks/results/synthetic_16v_48k_128.json
```

The v0.2 synthetic C++ workload validates measurement infrastructure; it is not yet a real VST plugin. The next native implementation step replaces `SyntheticProcessor` with a VST3-backed `AudioProcessor` while keeping the benchmark/result format stable.

For an optional real-VST3 **functional** smoke test using Steinberg's external reference host:

```bash
./scripts/fetch_vst3_sdk.sh
./scripts/build_vst3_reference_host.sh
./scripts/vst3_reference_smoke.sh /path/to/Plugin.vst3
```

The SDK checkout stays under `external/vst3sdk/` and is ignored by Git.

## v0.3 — real VST3 benchmarking

VSTBox can now load a real VST3 bundle through a native offline host and benchmark its processing callbacks without JACK or an audio device. The benchmark records plug-in metadata/parameters, P50/P95/P99/worst processing time, deadline misses, latency, and peak memory while keeping desktop measurements separate from Raspberry Pi projections.

```bash
./scripts/fetch_vst3_sdk.sh
./scripts/build_vst3_benchmark.sh
vstbox-bench-vst3 --plugin /path/to/Plugin.vst3 --json-out benchmarks/results/plugin.json
```

For the Steinberg `adelay` sample already used for validation:

```bash
./scripts/run_adelay_benchmark.sh
```

See `docs/BENCHMARKING.md` for the benchmark methodology and instrument/MIDI options.

## License

Original VSTBox code and documentation in this repository are released under the [MIT License](LICENSE), unless a file states otherwise.

Third-party software is **not** relicensed by VSTBox. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for dependency and plug-in licensing notes. In particular, the Steinberg VST 3 SDK and test plug-ins are fetched or supplied separately rather than committed as VSTBox source.

## Third-party names and trademarks

VST, VST3, Steinberg, Raspberry Pi, and other third-party names, products, and trademarks belong to their respective owners. References in this repository are descriptive and do not imply affiliation, endorsement, or certification.


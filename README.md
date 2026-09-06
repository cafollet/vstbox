# VSTBox Pre-Hardware Prototype

This repository is the first executable stage of the VSTBox project: a standalone hardware appliance intended to host software instruments/effects without requiring a laptop during use.

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

For ongoing synchronization, use a private Git remote (GitHub or another Git server) rather than exchanging ZIP files. A typical workflow is:

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

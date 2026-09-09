# VSTBox v0.4 — Controlled Synth Benchmark

v0.4 moves from the lightweight ADelay effect to a deterministic polyphonic instrument workload.

## Why MDA JX10

The Steinberg SDK's `mda-vst3` example bundle contains multiple instrument classes, including `mda DX10`, `mda EPiano`, `mda JX10`, and `mda Piano`. VSTBox uses `mda JX10` first because it is a real polyphonic synth example while the MDA target does not require VSTGUI.

The MDA source is fetched as part of the external Steinberg SDK and is not committed to the VSTBox repository.

## Benchmark design

The host injects a deterministic chord at callback 0. `--voices N` controls how many simultaneous Note On events are emitted, with unique note IDs. The chord is held throughout warmup and all measured callbacks, so the timing distribution reflects active polyphonic DSP rather than idle callbacks.

Default matrix:

- sample rate: 48 kHz
- buffers: 256, 128, 64 samples
- requested voices: 1, 4, 8, 16, 32
- warmup: 1,000 callbacks
- measured callbacks: 10,000
- root note: MIDI 48
- note spacing: 1 semitone

The 32-voice point is a stress request. If the plug-in has a lower internal voice limit, its timing curve may plateau due to voice stealing; that is itself useful information and should not be interpreted as 32 independently active voices without further instrumentation.

## Run

```bash
./scripts/build_mda_synth_sample.sh
python3 -m pip install -e '.[dev]'
./scripts/build_vst3_benchmark.sh
./scripts/run_mda_jx10_smoke.sh
./scripts/run_mda_jx10_matrix.sh
```

Results are written under `benchmarks/results/mda_jx10_matrix/`.

## Interpretation

These are offline DSP callback measurements on the current desktop machine. They exclude CoreAudio/JACK/USB latency and do not yet apply Mac-to-Pi performance scaling. The matrix is intended to establish a controlled CPU-scaling curve before testing a heavier real-world open-source synth.

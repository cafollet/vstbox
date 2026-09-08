# VSTBox desktop benchmarking

## Purpose

VSTBox measures desktop audio workloads before any Raspberry Pi purchase. Raw measurements are intentionally kept separate from Pi projections so later hardware calibration can replace assumptions without invalidating the benchmark dataset.

The benchmark pipeline is:

1. capture the reference machine profile;
2. build the native timing harness;
3. run a controlled callback workload;
4. save timing/metadata as JSON;
5. run the same harness against a real VST3 plug-in;
6. feed measured workloads into the Pi 5 feasibility model later.

## Reference machine profile

```bash
vstbox-machine --json-out benchmarks/profiles/current_machine.json
```

On macOS this records the CPU brand, physical/logical core counts, RAM, architecture, OS version, and Python version.

## Synthetic harness

Build:

```bash
./scripts/build_native.sh
```

Run:

```bash
vstbox-bench-synthetic \
  --sample-rate 48000 \
  --buffer 128 \
  --voices 16 \
  --callbacks 20000 \
  --warmup 1000 \
  --json-out benchmarks/results/synthetic_16v_48k_128.json
```

The synthetic processor is a stateful oscillator/filter workload used to validate timing instrumentation. It is not intended to approximate a particular plug-in.

## Real VST3 offline benchmark (v0.3)

VSTBox does **not** vendor the Steinberg SDK. Fetch it separately:

```bash
./scripts/fetch_vst3_sdk.sh
```

Build the VSTBox VST3 benchmark target:

```bash
./scripts/build_vst3_benchmark.sh
```

This produces:

```text
build/native-vst3/vstbox_vst3_bench
```

Benchmark a plug-in:

```bash
vstbox-bench-vst3 \
  --plugin /path/to/Plugin.vst3 \
  --sample-rate 48000 \
  --buffer 128 \
  --callbacks 20000 \
  --warmup 1000 \
  --json-out benchmarks/results/plugin_48k_128.json
```

The native loader:

- discovers the first VST3 Audio Module Class in the bundle (or an exact class supplied with `--class`);
- initializes its component and audio processor;
- enumerates controller parameters when available;
- activates audio/event busses;
- configures 32-bit realtime-style processing at the requested sample rate and block size;
- provides a deterministic 220 Hz input signal to effect plug-ins;
- optionally injects deterministic note-on/note-off events for instrument plug-ins;
- times every processing callback;
- reports mean, P50, P95, P99, worst callback, deadline misses, latency, peak RSS, bus layout, and parameter metadata.

The benchmark is **offline**: it calls the VST3 process API directly and does not depend on JACK, CoreAudio device selection, or a physical audio interface. This deliberately isolates plug-in/host processing cost from device-driver behavior.

### First validated effect workload

After building Steinberg's `adelay.vst3` sample, run:

```bash
./scripts/run_adelay_benchmark.sh
```

or pass the path explicitly:

```bash
./scripts/run_adelay_benchmark.sh /full/path/to/adelay.vst3
```

### Instrument driving

For plug-ins with an event input bus, MIDI is enabled by default. The benchmark periodically sends a note-on and note-off. Example:

```bash
vstbox-bench-vst3 \
  --plugin /path/to/Synth.vst3 \
  --midi-note 60 \
  --midi-velocity 0.8 \
  --midi-cycle 128 \
  --midi-gate 96 \
  --json-out benchmarks/results/synth_48k_128.json
```

For effects such as `adelay`, use `--no-midi`.

## Interpretation

A 48 kHz / 128-sample block has a 2.667 ms realtime deadline. The desktop benchmark compares each measured callback against that deadline, but **does not claim the desktop result equals Raspberry Pi 5 performance**. Pi projection/scaling remains a separate model layer.

The benchmark timing includes the small VSTBox wrapper around each `process()` call (buffer preparation/copy and deterministic event/input generation). That is intentional: the eventual device must pay some hosting overhead too.

## Optional Steinberg reference-host smoke test

The Steinberg reference host remains useful for independent functional validation:

```bash
./scripts/build_vst3_reference_host.sh
./scripts/vst3_reference_smoke.sh /path/to/SomePlugin.vst3
```

Its live JACK/CoreAudio behavior is not used as VSTBox benchmark data.

# VSTBox desktop benchmarking

## Purpose

v0.2 establishes a measured desktop baseline before any Raspberry Pi purchase. Measurements are intentionally kept separate from Pi projections.

The benchmark pipeline is:

1. capture the reference machine profile;
2. build the native timing harness;
3. run a controlled callback workload;
4. save raw timings/metadata as JSON;
5. later replace the synthetic processor with a VST3 adapter using the same benchmark interface.

## Reference machine profile

```bash
vstbox-machine --json-out benchmarks/profiles/current_machine.json
```

On macOS this records the CPU brand, physical/logical core counts, RAM, architecture, OS version, and Python version.

## Build the native harness

```bash
./scripts/build_native.sh
```

The executable is written to:

```text
build/native/vstbox_synthetic_bench
```

## Run a benchmark

```bash
vstbox-bench-synthetic \
  --sample-rate 48000 \
  --buffer 128 \
  --voices 16 \
  --callbacks 20000 \
  --warmup 1000 \
  --json-out benchmarks/results/synthetic_16v_48k_128.json
```

The workload is a stateful polyphonic oscillator/filter bank implemented in C++. It exists to validate our callback measurement infrastructure. It is **not** intended to approximate a particular VST.

## Recommended initial matrix

Run the same benchmark at 1, 4, 8, 16, and 32 voices, all at 48 kHz / 128 samples. Then repeat 16 voices at 64 and 256 samples.

This gives us a first scaling curve and validates that the results are stable enough to use the same instrumentation for real plugins.

## VST3 integration boundary

The native benchmark core exposes a small `AudioProcessor` interface in `native/include/vstbox/processor.hpp`. The synthetic processor implements it today. A VST3 adapter will implement the same interface next.

We intentionally do not vendor a third-party VST3 SDK in the repository. When the adapter is implemented, the SDK will be supplied as an external dependency and the benchmark result schema will remain unchanged.

## Optional VST3 reference-host smoke test

Before the custom VSTBox VST3 adapter is complete, we can independently verify the Mac toolchain and a real `.vst3` bundle using Steinberg's official SDK hosting example. The SDK is **not vendored** into VSTBox.

```bash
./scripts/fetch_vst3_sdk.sh
./scripts/build_vst3_reference_host.sh
./scripts/vst3_reference_smoke.sh /path/to/SomePlugin.vst3
```

This is a functional reference test only. Timing from Steinberg's sample host is not treated as a VSTBox benchmark. Our measured benchmark path remains the C++ `AudioProcessor` harness; the next adapter will put a real VST3 processor behind that interface.

# VSTBox v0.5 host-completeness milestone

This milestone closes three gaps exposed by Surge XT before performance projection to Raspberry Pi 5:

1. **Separated controller support** — VSTBox supplies a minimal `IComponentHandler`, connects processor and controller connection points, synchronizes the controller from `IComponent::getState()` via `IEditController::setComponentState()`, and then enumerates parameters.
2. **Deterministic process context** — each callback supplies a playing 120 BPM, 4/4 `ProcessContext` with advancing sample and musical positions.
3. **Output verification** — the benchmark records main-output RMS, peak, non-silent sample count, and observed sample count outside the timed processing window.

Auxiliary audio inputs such as a synth sidechain are kept silent; only main audio input buses receive the deterministic 220 Hz test signal used for effects.

## Surge acceptance smoke

```bash
./scripts/build_vst3_benchmark.sh
./scripts/run_surge_host_smoke.sh
```

The smoke test fails unless Surge exposes a large parameter set, its separated controller is connected/state-synchronized, process context is enabled, output is non-silent, and no 48 kHz / 128-sample deadline miss occurs.

After that passes:

```bash
./scripts/run_surge_matrix.sh
```

The matrix covers 1/4/8/16 simultaneous MIDI notes at 48 kHz with 256/128/64-sample buffers.

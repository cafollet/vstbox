# VSTBox Prototype Roadmap

## Gate 0 — Pre-hardware feasibility

Goal: determine whether purchasing a Raspberry Pi 5 8 GB is technically justified.

Deliverables:
- simulator
- repeatable benchmark format
- real desktop plugin workload measurements
- conservative ARM projection
- native ARM scenario
- Windows x64 compatibility scenario
- written go/no-go report

## Gate 1 — Physical audio feasibility

Requires Raspberry Pi 5.

Pass criteria:
- stable 48 kHz operation
- 128-sample target tested
- measured xruns
- measured end-to-end latency
- thermal behavior documented

## Gate 2 — Native plugin feasibility

Pass criteria:
- third-party ARM-compatible VST3 hosted reliably
- MIDI, parameter enumeration, state save/restore validated

## Gate 3 — Source conversion feasibility

Pass criteria:
- external open-source JUCE/CMake plugin compiled for ARM64 with minimal manual intervention

## Gate 4 — Windows compatibility feasibility

Pass criteria:
- simple non-DRM x86-64 Windows VST3 runs via compatibility worker with usable latency and stability

## v0.2 status

- [x] Desktop machine profiler
- [x] Versioned benchmark JSON schema
- [x] Native C++ callback timing harness
- [x] Synthetic DSP processor for instrumentation validation
- [x] Python bridge/CLI that combines native timings with machine metadata
- [x] C++ processor abstraction designed for a VST3 adapter
- [ ] Real VST3 adapter and controlled MIDI event generator
- [ ] Real plugin benchmark profiles
- [ ] Explicit Mac-to-Pi projection model calibrated with external/physical data

## v0.3 status

- [x] Offline VST3 module loading behind the native benchmark layer
- [x] Audio component discovery and 32-bit process setup
- [x] Audio/event bus activation
- [x] Parameter metadata enumeration when a controller is available
- [x] Deterministic audio-input stimulus for effects
- [x] Deterministic MIDI note events for instruments
- [x] Real VST3 callback timing + peak RSS JSON output
- [x] Python CLI combining plug-in measurements with machine metadata
- [ ] Run and record the first `adelay.vst3` benchmark on the Intel Mac
- [ ] Run and record the first instrument/synth benchmark
- [ ] Add explicit Mac-to-Pi projection model calibrated against public/physical data

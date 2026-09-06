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

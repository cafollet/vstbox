from vstbox_sim.model import AudioProfile, HardwareProfile, SimulationConfig, WorkloadProfile, run_simulation


def test_audio_deadline_48k_128():
    assert abs(AudioProfile(48000, 128).deadline_ms - 2.6666666667) < 1e-6


def test_light_workload_is_green():
    result = run_simulation(
        HardwareProfile(),
        AudioProfile(),
        WorkloadProfile(
            name="light",
            backend="native",
            dsp_mean_ms=0.20,
            dsp_jitter_ms=0.01,
            scheduler_spike_probability=0.0,
            plugin_ram_mb=200,
            runtime_ram_mb=200,
        ),
        SimulationConfig(callbacks=5000, seed=1),
    )
    assert result.rating == "GREEN"
    assert result.deadline_misses == 0


def test_overloaded_workload_is_red():
    result = run_simulation(
        HardwareProfile(),
        AudioProfile(),
        WorkloadProfile(
            name="overload",
            backend="native",
            dsp_mean_ms=3.0,
            dsp_jitter_ms=0.1,
            scheduler_spike_probability=0.0,
            plugin_ram_mb=200,
            runtime_ram_mb=200,
        ),
        SimulationConfig(callbacks=2000, seed=1),
    )
    assert result.rating == "RED"
    assert result.deadline_misses > 0

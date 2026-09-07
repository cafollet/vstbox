from vstbox_bench.schema import AudioBenchmarkConfig, MachineProfile, SCHEMA_VERSION


def test_audio_deadline_48k_128():
    config = AudioBenchmarkConfig(
        sample_rate_hz=48_000,
        buffer_samples=128,
        callbacks=100,
        warmup_callbacks=10,
        voices=16,
    )
    assert abs(config.deadline_ms - 2.6666666667) < 1e-6


def test_audio_config_validation():
    config = AudioBenchmarkConfig(
        sample_rate_hz=48_000,
        buffer_samples=0,
        callbacks=100,
        warmup_callbacks=10,
        voices=16,
    )
    try:
        config.validate()
    except ValueError as exc:
        assert "buffer_samples" in str(exc)
    else:
        raise AssertionError("expected validation failure")


def test_machine_profile_schema_version():
    profile = MachineProfile.now(
        hostname="test",
        os_name="Darwin",
        os_version="13.0",
        architecture="x86_64",
        cpu_brand="Intel test CPU",
        physical_cores=4,
        logical_cores=8,
        memory_bytes=16 * 1024**3,
        python_version="3.13.0",
    )
    assert profile.schema_version == SCHEMA_VERSION
    assert profile.to_dict()["architecture"] == "x86_64"

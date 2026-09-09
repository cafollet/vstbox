from pathlib import Path


def _source() -> str:
    return Path("native/src/vst3_processor.cpp").read_text()


def test_vst3_tchar_uses_vst_namespace():
    text = _source()
    assert "Steinberg::Vst::TChar" in text
    assert "Steinberg::TChar" not in text


def test_vst3_parameter_flags_cast_matches_metadata_type():
    text = _source()
    assert "static_cast<std::uint32_t>(parameter.flags)" in text


def test_vst3_separated_controller_is_connected_and_state_synced():
    text = _source()
    assert "ConnectionProxy" in text
    assert "setComponentHandler" in text
    assert "setComponentState" in text
    assert "ResizableMemoryIBStream" in text


def test_vst3_process_context_is_supplied():
    text = _source()
    assert "process_data_.processContext = &vst_process_context_" in text
    assert "kTempoValid" in text
    assert "kTimeSigValid" in text
    assert "kProjectTimeMusicValid" in text


def test_aux_audio_inputs_are_silent_in_benchmark_host():
    text = _source()
    assert "bus_info.busType == kMain" in text


def test_vst3_benchmark_measures_output_signal():
    text = Path("native/src/vst3_bench.cpp").read_text()
    assert '\\"output\\"' in text
    assert "output_sum_squares" in text
    assert "output_peak" in text

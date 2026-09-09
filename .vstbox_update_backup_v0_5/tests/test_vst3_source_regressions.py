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

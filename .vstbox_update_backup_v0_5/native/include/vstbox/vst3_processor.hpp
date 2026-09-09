#pragma once

#include "vstbox/processor.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace vstbox {

struct Vst3ParameterInfo {
    std::uint32_t id{};
    std::string title;
    std::string units;
    double default_normalized_value{};
    std::int32_t step_count{};
    std::uint32_t flags{};
};

struct Vst3PluginInfo {
    std::string path;
    std::string class_name;
    std::string vendor;
    std::string version;
    std::string sdk_version;
    std::string subcategories;
    std::int32_t audio_input_buses{};
    std::int32_t audio_output_buses{};
    std::int32_t event_input_buses{};
    std::int32_t event_output_buses{};
    std::int32_t audio_input_channels{};
    std::int32_t audio_output_channels{};
    std::uint32_t latency_samples{};
    std::vector<Vst3ParameterInfo> parameters;
};

struct MidiDriveConfig {
    bool enabled{true};
    std::int16_t note{60};
    std::int16_t note_step{1};
    std::size_t voices{1};
    float velocity{0.8f};
    std::size_t cycle_callbacks{128};
    std::size_t gate_callbacks{96};
};

class Vst3Processor final : public AudioProcessor {
public:
    Vst3Processor(std::string plugin_path, std::string requested_class = {}, MidiDriveConfig midi = {});
    ~Vst3Processor() override;

    Vst3Processor(const Vst3Processor&) = delete;
    Vst3Processor& operator=(const Vst3Processor&) = delete;

    const char* name() const noexcept override;
    void prepare(double sample_rate_hz, std::size_t max_frames) override;
    void process(const ProcessContext& context, float* left, float* right) noexcept override;

    const Vst3PluginInfo& plugin_info() const noexcept { return info_; }
    const std::string& last_error() const noexcept { return last_error_; }
    bool processing_ok() const noexcept { return processing_ok_; }

private:
    class Impl;

    // info_ must exist before Impl construction because Impl::load() populates it.
    Vst3PluginInfo info_;
    std::unique_ptr<Impl> impl_;
    std::string last_error_;
    bool processing_ok_{true};
};

}  // namespace vstbox

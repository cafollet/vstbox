#pragma once

#include "vstbox/processor.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vstbox {

class SyntheticProcessor final : public AudioProcessor {
public:
    explicit SyntheticProcessor(std::size_t voices, std::uint32_t seed = 42);

    const char* name() const noexcept override { return "vstbox-synthetic-voice-bank"; }
    void prepare(double sample_rate_hz, std::size_t max_frames) override;
    void process(const ProcessContext& context, float* left, float* right) noexcept override;

private:
    struct Voice {
        double phase{};
        double phase_increment{};
        float filter_state{};
        float gain{};
    };

    std::size_t voices_count_{};
    std::uint32_t seed_{};
    double sample_rate_hz_{48000.0};
    std::vector<Voice> voices_;
};

}  // namespace vstbox

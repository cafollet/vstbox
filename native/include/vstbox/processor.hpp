#pragma once

#include <cstddef>
#include <cstdint>

namespace vstbox {

struct ProcessContext {
    double sample_rate_hz{48000.0};
    std::size_t frames{128};
    std::uint64_t callback_index{0};
};

class AudioProcessor {
public:
    virtual ~AudioProcessor() = default;
    virtual const char* name() const noexcept = 0;
    virtual void prepare(double sample_rate_hz, std::size_t max_frames) = 0;
    virtual void process(const ProcessContext& context, float* left, float* right) noexcept = 0;
};

}  // namespace vstbox

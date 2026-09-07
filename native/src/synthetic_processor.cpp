#include "vstbox/synthetic_processor.hpp"

#include <algorithm>
#include <cmath>
#include <random>

namespace vstbox {
namespace {
constexpr double kTwoPi = 6.283185307179586476925286766559;
}

SyntheticProcessor::SyntheticProcessor(std::size_t voices, std::uint32_t seed)
    : voices_count_(std::max<std::size_t>(1, voices)), seed_(seed) {}

void SyntheticProcessor::prepare(double sample_rate_hz, std::size_t /*max_frames*/) {
    sample_rate_hz_ = sample_rate_hz;
    std::mt19937 rng(seed_);
    std::uniform_real_distribution<double> frequency(55.0, 1760.0);
    std::uniform_real_distribution<float> gain(0.015f, 0.05f);

    voices_.clear();
    voices_.reserve(voices_count_);
    for (std::size_t i = 0; i < voices_count_; ++i) {
        const double hz = frequency(rng);
        voices_.push_back(Voice{0.0, kTwoPi * hz / sample_rate_hz_, 0.0f, gain(rng)});
    }
}

void SyntheticProcessor::process(const ProcessContext& context, float* left, float* right) noexcept {
    std::fill(left, left + context.frames, 0.0f);
    std::fill(right, right + context.frames, 0.0f);

    // Deliberately simple but audio-shaped workload: polyphonic oscillators feeding
    // a one-pole stateful filter. This validates callback timing instrumentation.
    for (auto& voice : voices_) {
        for (std::size_t frame = 0; frame < context.frames; ++frame) {
            const float oscillator = static_cast<float>(std::sin(voice.phase));
            voice.phase += voice.phase_increment;
            if (voice.phase >= kTwoPi) {
                voice.phase -= kTwoPi;
            }

            const float nonlinear = std::tanh(oscillator * 1.4f);
            voice.filter_state += 0.16f * (nonlinear - voice.filter_state);
            const float sample = voice.filter_state * voice.gain;
            left[frame] += sample;
            right[frame] += sample;
        }
    }
}

}  // namespace vstbox

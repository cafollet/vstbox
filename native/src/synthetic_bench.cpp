#include "vstbox/synthetic_processor.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(__APPLE__) || defined(__linux__)
#include <sys/resource.h>
#endif

namespace {

struct Options {
    int sample_rate{48000};
    std::size_t buffer{128};
    std::size_t voices{16};
    std::size_t callbacks{20000};
    std::size_t warmup{1000};
    std::uint32_t seed{42};
    bool json{false};
};

std::size_t parse_size(const char* value, const char* label, bool allow_zero = false) {
    try {
        const auto parsed = std::stoull(value);
        if (!allow_zero && parsed == 0) throw std::invalid_argument("zero");
        return static_cast<std::size_t>(parsed);
    } catch (...) {
        throw std::runtime_error(std::string("Invalid ") + label + ": " + value);
    }
}

Options parse_args(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto require_value = [&](const char* label) -> const char* {
            if (i + 1 >= argc) throw std::runtime_error(std::string("Missing value for ") + label);
            return argv[++i];
        };
        if (arg == "--sample-rate") options.sample_rate = static_cast<int>(parse_size(require_value("--sample-rate"), "sample rate"));
        else if (arg == "--buffer") options.buffer = parse_size(require_value("--buffer"), "buffer");
        else if (arg == "--voices") options.voices = parse_size(require_value("--voices"), "voices");
        else if (arg == "--callbacks") options.callbacks = parse_size(require_value("--callbacks"), "callbacks");
        else if (arg == "--warmup") options.warmup = parse_size(require_value("--warmup"), "warmup", true);
        else if (arg == "--seed") options.seed = static_cast<std::uint32_t>(parse_size(require_value("--seed"), "seed", true));
        else if (arg == "--json") options.json = true;
        else if (arg == "--help" || arg == "-h") {
            std::cout << "vstbox_synthetic_bench [--sample-rate 48000] [--buffer 128] [--voices 16] "
                         "[--callbacks 20000] [--warmup 1000] [--seed 42] [--json]\n";
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }
    return options;
}

double quantile(const std::vector<double>& sorted, double q) {
    if (sorted.empty()) return 0.0;
    const auto position = static_cast<std::size_t>(std::llround((sorted.size() - 1) * q));
    return sorted[std::min(position, sorted.size() - 1)];
}

std::uint64_t peak_rss_bytes() {
#if defined(__APPLE__) || defined(__linux__)
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
#if defined(__APPLE__)
        return static_cast<std::uint64_t>(usage.ru_maxrss);
#else
        return static_cast<std::uint64_t>(usage.ru_maxrss) * 1024ULL;
#endif
    }
#endif
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_args(argc, argv);
        vstbox::SyntheticProcessor processor(options.voices, options.seed);
        processor.prepare(static_cast<double>(options.sample_rate), options.buffer);

        std::vector<float> left(options.buffer);
        std::vector<float> right(options.buffer);
        vstbox::ProcessContext context{static_cast<double>(options.sample_rate), options.buffer, 0};

        for (std::size_t i = 0; i < options.warmup; ++i) {
            context.callback_index = i;
            processor.process(context, left.data(), right.data());
        }

        std::vector<double> timings;
        timings.reserve(options.callbacks);
        using clock = std::chrono::steady_clock;
        for (std::size_t i = 0; i < options.callbacks; ++i) {
            context.callback_index = i;
            const auto start = clock::now();
            processor.process(context, left.data(), right.data());
            const auto stop = clock::now();
            timings.push_back(std::chrono::duration<double, std::milli>(stop - start).count());
        }

        std::sort(timings.begin(), timings.end());
        double total = 0.0;
        for (double value : timings) total += value;
        const double mean = timings.empty() ? 0.0 : total / static_cast<double>(timings.size());
        const double deadline = static_cast<double>(options.buffer) / static_cast<double>(options.sample_rate) * 1000.0;
        const auto misses = static_cast<std::size_t>(std::count_if(timings.begin(), timings.end(), [deadline](double ms) { return ms > deadline; }));
        const double miss_rate = timings.empty() ? 0.0 : static_cast<double>(misses) / static_cast<double>(timings.size());
        const auto rss = peak_rss_bytes();

        if (options.json) {
            std::cout << std::fixed << std::setprecision(9)
                      << "{\n"
                      << "  \"timing\": {\n"
                      << "    \"mean_ms\": " << mean << ",\n"
                      << "    \"p50_ms\": " << quantile(timings, 0.50) << ",\n"
                      << "    \"p95_ms\": " << quantile(timings, 0.95) << ",\n"
                      << "    \"p99_ms\": " << quantile(timings, 0.99) << ",\n"
                      << "    \"worst_ms\": " << (timings.empty() ? 0.0 : timings.back()) << ",\n"
                      << "    \"deadline_ms\": " << deadline << ",\n"
                      << "    \"deadline_misses\": " << misses << ",\n"
                      << "    \"miss_rate\": " << miss_rate << "\n"
                      << "  },\n"
                      << "  \"process\": {\n"
                      << "    \"peak_rss_bytes\": " << rss << "\n"
                      << "  }\n"
                      << "}\n";
        } else {
            std::cout << "VSTBox synthetic benchmark\n"
                      << "sample rate: " << options.sample_rate << " Hz\n"
                      << "buffer:      " << options.buffer << " samples\n"
                      << "voices:      " << options.voices << "\n"
                      << "deadline:    " << deadline << " ms\n"
                      << "p99:         " << quantile(timings, 0.99) << " ms\n"
                      << "worst:       " << (timings.empty() ? 0.0 : timings.back()) << " ms\n"
                      << "misses:      " << misses << "/" << timings.size() << "\n";
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 2;
    }
}

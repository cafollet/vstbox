#include "vstbox/vst3_processor.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(__APPLE__) || defined(__linux__)
#include <sys/resource.h>
#endif

namespace {

struct Options {
    std::string plugin;
    std::string class_name;
    int sample_rate{48000};
    std::size_t buffer{128};
    std::size_t callbacks{20000};
    std::size_t warmup{1000};
    bool midi{true};
    int midi_note{60};
    int midi_note_step{1};
    std::size_t voices{1};
    double midi_velocity{0.8};
    std::size_t midi_cycle{128};
    std::size_t midi_gate{96};
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

double parse_double(const char* value, const char* label) {
    try {
        return std::stod(value);
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
        if (arg == "--plugin") options.plugin = require_value("--plugin");
        else if (arg == "--class") options.class_name = require_value("--class");
        else if (arg == "--sample-rate") options.sample_rate = static_cast<int>(parse_size(require_value("--sample-rate"), "sample rate"));
        else if (arg == "--buffer") options.buffer = parse_size(require_value("--buffer"), "buffer");
        else if (arg == "--callbacks") options.callbacks = parse_size(require_value("--callbacks"), "callbacks");
        else if (arg == "--warmup") options.warmup = parse_size(require_value("--warmup"), "warmup", true);
        else if (arg == "--no-midi") options.midi = false;
        else if (arg == "--midi-note") options.midi_note = static_cast<int>(parse_size(require_value("--midi-note"), "MIDI note", true));
        else if (arg == "--midi-note-step") options.midi_note_step = static_cast<int>(parse_size(require_value("--midi-note-step"), "MIDI note step"));
        else if (arg == "--voices") options.voices = parse_size(require_value("--voices"), "voices");
        else if (arg == "--midi-velocity") options.midi_velocity = parse_double(require_value("--midi-velocity"), "MIDI velocity");
        else if (arg == "--midi-cycle") options.midi_cycle = parse_size(require_value("--midi-cycle"), "MIDI cycle");
        else if (arg == "--midi-gate") options.midi_gate = parse_size(require_value("--midi-gate"), "MIDI gate", true);
        else if (arg == "--json") options.json = true;
        else if (arg == "--help" || arg == "-h") {
            std::cout << "vstbox_vst3_bench --plugin /path/Plugin.vst3 [--class NAME] [--sample-rate 48000] "
                         "[--buffer 128] [--callbacks 20000] [--warmup 1000] [--no-midi] "
                         "[--midi-note 60] [--midi-note-step 1] [--voices 1] [--midi-velocity 0.8] "
                         "[--midi-cycle 128] [--midi-gate 96] [--json]\n";
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }
    if (options.plugin.empty()) throw std::runtime_error("--plugin is required");
    if (options.midi_note < 0 || options.midi_note > 127) throw std::runtime_error("MIDI note must be in [0, 127]");
    if (options.midi_velocity < 0.0 || options.midi_velocity > 1.0) throw std::runtime_error("MIDI velocity must be in [0, 1]");
    const auto highest_note = options.midi_note + static_cast<int>((options.voices - 1) * static_cast<std::size_t>(options.midi_note_step));
    if (highest_note > 127) throw std::runtime_error("Requested chord exceeds MIDI note 127; lower --midi-note, --midi-note-step, or --voices");
    if (options.midi_gate >= options.midi_cycle) throw std::runtime_error("MIDI gate must be smaller than MIDI cycle");
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

std::string json_escape(const std::string& value) {
    std::ostringstream out;
    for (unsigned char ch : value) {
        switch (ch) {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (ch < 0x20) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch) << std::dec;
                else out << ch;
        }
    }
    return out.str();
}

void print_plugin_json(const vstbox::Vst3PluginInfo& info) {
    std::cout << "  \"plugin\": {\n"
              << "    \"path\": \"" << json_escape(info.path) << "\",\n"
              << "    \"class_name\": \"" << json_escape(info.class_name) << "\",\n"
              << "    \"vendor\": \"" << json_escape(info.vendor) << "\",\n"
              << "    \"version\": \"" << json_escape(info.version) << "\",\n"
              << "    \"sdk_version\": \"" << json_escape(info.sdk_version) << "\",\n"
              << "    \"subcategories\": \"" << json_escape(info.subcategories) << "\",\n"
              << "    \"audio_input_buses\": " << info.audio_input_buses << ",\n"
              << "    \"audio_output_buses\": " << info.audio_output_buses << ",\n"
              << "    \"event_input_buses\": " << info.event_input_buses << ",\n"
              << "    \"event_output_buses\": " << info.event_output_buses << ",\n"
              << "    \"audio_input_channels\": " << info.audio_input_channels << ",\n"
              << "    \"audio_output_channels\": " << info.audio_output_channels << ",\n"
              << "    \"latency_samples\": " << info.latency_samples << ",\n"
              << "    \"parameters\": [\n";
    for (std::size_t i = 0; i < info.parameters.size(); ++i) {
        const auto& p = info.parameters[i];
        std::cout << "      {\"id\": " << p.id
                  << ", \"title\": \"" << json_escape(p.title)
                  << "\", \"units\": \"" << json_escape(p.units)
                  << "\", \"default_normalized_value\": " << p.default_normalized_value
                  << ", \"step_count\": " << p.step_count
                  << ", \"flags\": " << p.flags << "}";
        if (i + 1 != info.parameters.size()) std::cout << ',';
        std::cout << '\n';
    }
    std::cout << "    ]\n  },\n";
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_args(argc, argv);
        vstbox::MidiDriveConfig midi{};
        midi.enabled = options.midi;
        midi.note = static_cast<std::int16_t>(options.midi_note);
        midi.note_step = static_cast<std::int16_t>(options.midi_note_step);
        midi.voices = options.voices;
        midi.velocity = static_cast<float>(options.midi_velocity);
        midi.cycle_callbacks = options.midi_cycle;
        midi.gate_callbacks = options.midi_gate;

        vstbox::Vst3Processor processor(options.plugin, options.class_name, midi);
        processor.prepare(static_cast<double>(options.sample_rate), options.buffer);

        std::vector<float> left(options.buffer);
        std::vector<float> right(options.buffer);
        vstbox::ProcessContext context{static_cast<double>(options.sample_rate), options.buffer, 0};

        for (std::size_t i = 0; i < options.warmup; ++i) {
            context.callback_index = i;
            processor.process(context, left.data(), right.data());
            if (!processor.processing_ok()) throw std::runtime_error(processor.last_error());
        }

        std::vector<double> timings;
        timings.reserve(options.callbacks);
        using clock = std::chrono::steady_clock;
        for (std::size_t i = 0; i < options.callbacks; ++i) {
            context.callback_index = options.warmup + i;
            const auto start = clock::now();
            processor.process(context, left.data(), right.data());
            const auto stop = clock::now();
            if (!processor.processing_ok()) throw std::runtime_error(processor.last_error());
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
            std::cout << std::fixed << std::setprecision(9) << "{\n";
            print_plugin_json(processor.plugin_info());
            std::cout << "  \"timing\": {\n"
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
            const auto& info = processor.plugin_info();
            std::cout << "VSTBox VST3 benchmark\n"
                      << "plugin:      " << info.class_name << "\n"
                      << "vendor:      " << info.vendor << "\n"
                      << "sample rate: " << options.sample_rate << " Hz\n"
                      << "buffer:      " << options.buffer << " samples\n"
                      << "deadline:    " << deadline << " ms\n"
                      << "p99:         " << quantile(timings, 0.99) << " ms\n"
                      << "worst:       " << (timings.empty() ? 0.0 : timings.back()) << " ms\n"
                      << "misses:      " << misses << "/" << timings.size() << "\n"
                      << "parameters:  " << info.parameters.size() << "\n"
                      << "voices:      " << (options.midi ? options.voices : 0) << "\n";
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 2;
    }
}

#include "vstbox/vst3_processor.hpp"

#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/processdata.h"
#include "public.sdk/source/vst/utility/stringconvert.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstevents.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace vstbox {
namespace {

using namespace Steinberg;
using namespace Steinberg::Vst;

constexpr double kTwoPi = 6.283185307179586476925286766559;

std::string to_utf8(const Steinberg::TChar* text) {
    if (!text) return {};
    return Steinberg::Vst::StringConvert::convert(text, 128);
}

int count_audio_channels(IComponent& component, BusDirection direction) {
    int total = 0;
    const auto buses = component.getBusCount(kAudio, direction);
    for (int32 bus = 0; bus < buses; ++bus) {
        BusInfo info{};
        if (component.getBusInfo(kAudio, direction, bus, info) == kResultOk) {
            total += info.channelCount;
        }
    }
    return total;
}

void activate_all_buses(IComponent& component, MediaType type, BusDirection direction) {
    const auto count = component.getBusCount(type, direction);
    for (int32 index = 0; index < count; ++index) {
        component.activateBus(type, direction, index, true);
    }
}

}  // namespace

class Vst3Processor::Impl {
public:
    Impl(std::string plugin_path, std::string requested_class, MidiDriveConfig midi, Vst3PluginInfo& info)
        : plugin_path_(std::move(plugin_path)), requested_class_(std::move(requested_class)), midi_(midi) {
        load(info);
    }

    ~Impl() { shutdown(); }

    const char* class_name_cstr() const noexcept { return class_name_.c_str(); }

    void prepare(double sample_rate_hz, std::size_t max_frames, Vst3PluginInfo& info) {
        if (!component_ || !processor_) throw std::runtime_error("VST3 component is not loaded");
        if (max_frames == 0) throw std::runtime_error("VST3 max buffer size must be positive");

        stop_processing();
        sample_rate_hz_ = sample_rate_hz;
        max_frames_ = max_frames;

        activate_all_buses(*component_, kAudio, kInput);
        activate_all_buses(*component_, kAudio, kOutput);
        activate_all_buses(*component_, kEvent, kInput);
        activate_all_buses(*component_, kEvent, kOutput);

        if (processor_->canProcessSampleSize(kSample32) != kResultTrue) {
            throw std::runtime_error("VST3 plug-in does not support 32-bit sample processing");
        }

        ProcessSetup setup{};
        setup.processMode = kRealtime;
        setup.symbolicSampleSize = kSample32;
        setup.maxSamplesPerBlock = static_cast<int32>(max_frames_);
        setup.sampleRate = sample_rate_hz_;
        if (processor_->setupProcessing(setup) != kResultOk) {
            throw std::runtime_error("VST3 setupProcessing failed");
        }

        if (!process_data_.prepare(*component_, static_cast<int32>(max_frames_), kSample32)) {
            throw std::runtime_error("Could not allocate VST3 process buffers");
        }
        process_data_.processMode = kRealtime;
        process_data_.symbolicSampleSize = kSample32;
        process_data_.inputEvents = component_->getBusCount(kEvent, kInput) > 0 ? &events_ : nullptr;

        if (component_->setActive(true) != kResultOk) {
            throw std::runtime_error("VST3 setActive(true) failed");
        }
        active_ = true;
        if (processor_->setProcessing(true) != kResultOk) {
            component_->setActive(false);
            active_ = false;
            throw std::runtime_error("VST3 setProcessing(true) failed");
        }
        processing_ = true;
        info.latency_samples = processor_->getLatencySamples();
    }

    bool process(const ProcessContext& context, float* left, float* right, std::string& error) noexcept {
        if (!processing_ || context.frames > max_frames_) {
            error = "VST3 process called before prepare or with an oversized buffer";
            return false;
        }

        try {
            process_data_.numSamples = static_cast<int32>(context.frames);
            fill_inputs(context);
            clear_outputs(context.frames);
            prepare_events(context.callback_index);

            if (processor_->process(process_data_) != kResultOk) {
                error = "VST3 processor->process returned an error";
                return false;
            }
            copy_main_output(context.frames, left, right);
            return true;
        } catch (const std::exception& exc) {
            error = exc.what();
            return false;
        } catch (...) {
            error = "Unknown exception while processing VST3 block";
            return false;
        }
    }

private:
    void load(Vst3PluginInfo& info) {
        std::string module_error;
        module_ = VST3::Hosting::Module::create(plugin_path_, module_error);
        if (!module_) {
            throw std::runtime_error("Could not load VST3 module: " + module_error);
        }

        auto factory = module_->getFactory();
        const auto factory_info = factory.info();
        VST3::Hosting::ClassInfo selected{};
        bool found = false;
        for (const auto& class_info : factory.classInfos()) {
            if (class_info.category() != kVstAudioEffectClass) continue;
            if (!requested_class_.empty() && class_info.name() != requested_class_) continue;
            selected = class_info;
            found = true;
            break;
        }
        if (!found) {
            if (requested_class_.empty()) {
                throw std::runtime_error("No VST3 Audio Module Class found in bundle");
            }
            throw std::runtime_error("Requested VST3 class not found: " + requested_class_);
        }

        class_name_ = selected.name();
        host_ = Steinberg::owned(new HostApplication());
        factory.setHostContext(host_.get());

        component_ = factory.createInstance<IComponent>(selected.ID());
        if (!component_) throw std::runtime_error("Could not instantiate VST3 component");
        if (component_->initialize(host_.get()) != kResultOk) {
            component_.reset();
            throw std::runtime_error("Could not initialize VST3 component");
        }
        component_initialized_ = true;

        IAudioProcessor* processor_raw = nullptr;
        if (component_->queryInterface(IAudioProcessor::iid, reinterpret_cast<void**>(&processor_raw)) != kResultTrue || !processor_raw) {
            throw std::runtime_error("VST3 component does not expose IAudioProcessor");
        }
        processor_ = Steinberg::owned(processor_raw);

        TUID controller_cid{};
        if (component_->getControllerClassId(controller_cid) == kResultTrue) {
            controller_ = factory.createInstance<IEditController>(VST3::UID(controller_cid));
            if (controller_) {
                if (controller_->initialize(host_.get()) == kResultOk) {
                    controller_initialized_ = true;
                } else {
                    controller_.reset();
                }
            }
        }

        info.path = plugin_path_;
        info.class_name = selected.name();
        info.vendor = selected.vendor().empty() ? factory_info.vendor() : selected.vendor();
        info.version = selected.version();
        info.sdk_version = selected.sdkVersion();
        info.subcategories = selected.subCategoriesString();
        info.audio_input_buses = component_->getBusCount(kAudio, kInput);
        info.audio_output_buses = component_->getBusCount(kAudio, kOutput);
        info.event_input_buses = component_->getBusCount(kEvent, kInput);
        info.event_output_buses = component_->getBusCount(kEvent, kOutput);
        info.audio_input_channels = count_audio_channels(*component_, kInput);
        info.audio_output_channels = count_audio_channels(*component_, kOutput);

        if (controller_) {
            const auto count = controller_->getParameterCount();
            info.parameters.reserve(static_cast<std::size_t>(std::max<int32>(0, count)));
            for (int32 index = 0; index < count; ++index) {
                ParameterInfo parameter{};
                if (controller_->getParameterInfo(index, parameter) != kResultOk) continue;
                info.parameters.push_back(Vst3ParameterInfo{
                    static_cast<std::uint32_t>(parameter.id),
                    to_utf8(parameter.title),
                    to_utf8(parameter.units),
                    parameter.defaultNormalizedValue,
                    parameter.stepCount,
                    parameter.flags,
                });
            }
        }
    }

    void fill_inputs(const ProcessContext& context) noexcept {
        const double frame_base = static_cast<double>(context.callback_index * context.frames);
        for (int32 bus = 0; bus < process_data_.numInputs; ++bus) {
            auto& bus_buffers = process_data_.inputs[bus];
            bus_buffers.silenceFlags = 0;
            for (int32 channel = 0; channel < bus_buffers.numChannels; ++channel) {
                auto* buffer = bus_buffers.channelBuffers32[channel];
                if (!buffer) continue;
                for (std::size_t frame = 0; frame < context.frames; ++frame) {
                    const double sample_index = frame_base + static_cast<double>(frame);
                    buffer[frame] = static_cast<float>(0.2 * std::sin(kTwoPi * input_frequency_hz_ * sample_index / sample_rate_hz_));
                }
            }
        }
    }

    void clear_outputs(std::size_t frames) noexcept {
        for (int32 bus = 0; bus < process_data_.numOutputs; ++bus) {
            auto& bus_buffers = process_data_.outputs[bus];
            bus_buffers.silenceFlags = 0;
            for (int32 channel = 0; channel < bus_buffers.numChannels; ++channel) {
                auto* buffer = bus_buffers.channelBuffers32[channel];
                if (buffer) std::fill(buffer, buffer + frames, 0.0f);
            }
        }
    }

    void prepare_events(std::uint64_t callback_index) noexcept {
        events_.clear();
        if (!midi_.enabled || component_->getBusCount(kEvent, kInput) <= 0 || midi_.cycle_callbacks == 0 || midi_.voices == 0) return;

        const auto phase = static_cast<std::size_t>(callback_index % midi_.cycle_callbacks);
        const auto emit_notes = [&](bool note_on) {
            for (std::size_t voice = 0; voice < midi_.voices; ++voice) {
                const int pitch = static_cast<int>(midi_.note) + static_cast<int>(voice) * static_cast<int>(midi_.note_step);
                if (pitch < 0 || pitch > 127) break;

                Event event{};
                event.busIndex = 0;
                event.sampleOffset = 0;
                event.ppqPosition = 0.0;
                event.flags = Event::kIsLive;
                if (note_on) {
                    event.type = Event::kNoteOnEvent;
                    event.noteOn.channel = 0;
                    event.noteOn.pitch = static_cast<int16>(pitch);
                    event.noteOn.tuning = 0.0f;
                    event.noteOn.velocity = midi_.velocity;
                    event.noteOn.length = 0;
                    event.noteOn.noteId = static_cast<int32>(voice + 1);
                } else {
                    event.type = Event::kNoteOffEvent;
                    event.noteOff.channel = 0;
                    event.noteOff.pitch = static_cast<int16>(pitch);
                    event.noteOff.tuning = 0.0f;
                    event.noteOff.velocity = 0.0f;
                    event.noteOff.noteId = static_cast<int32>(voice + 1);
                }
                events_.addEvent(event);
            }
        };

        if (phase == 0) emit_notes(true);
        if (midi_.gate_callbacks < midi_.cycle_callbacks && phase == midi_.gate_callbacks) emit_notes(false);
    }

    void copy_main_output(std::size_t frames, float* left, float* right) noexcept {
        std::fill(left, left + frames, 0.0f);
        std::fill(right, right + frames, 0.0f);
        if (process_data_.numOutputs <= 0) return;

        const auto& bus = process_data_.outputs[0];
        if (bus.numChannels <= 0 || !bus.channelBuffers32) return;
        const float* first = bus.channelBuffers32[0];
        const float* second = bus.numChannels > 1 ? bus.channelBuffers32[1] : first;
        if (first) std::copy(first, first + frames, left);
        if (second) std::copy(second, second + frames, right);
    }

    void stop_processing() noexcept {
        if (processing_ && processor_) {
            processor_->setProcessing(false);
            processing_ = false;
        }
        if (active_ && component_) {
            component_->setActive(false);
            active_ = false;
        }
        process_data_.unprepare();
    }

    void shutdown() noexcept {
        stop_processing();
        processor_.reset();
        if (controller_ && controller_initialized_) controller_->terminate();
        controller_.reset();
        if (component_ && component_initialized_) component_->terminate();
        component_.reset();
        host_.reset();
        module_.reset();
    }

    std::string plugin_path_;
    std::string requested_class_;
    std::string class_name_;
    MidiDriveConfig midi_;
    VST3::Hosting::Module::Ptr module_;
    Steinberg::IPtr<HostApplication> host_;
    Steinberg::IPtr<IComponent> component_;
    Steinberg::IPtr<IAudioProcessor> processor_;
    Steinberg::IPtr<IEditController> controller_;
    HostProcessData process_data_;
    EventList events_;
    bool component_initialized_{false};
    bool controller_initialized_{false};
    bool active_{false};
    bool processing_{false};
    double sample_rate_hz_{48000.0};
    std::size_t max_frames_{128};
    double input_frequency_hz_{220.0};
};

Vst3Processor::Vst3Processor(std::string plugin_path, std::string requested_class, MidiDriveConfig midi)
    : info_{},
      impl_(std::make_unique<Impl>(std::move(plugin_path), std::move(requested_class), midi, info_)) {}

Vst3Processor::~Vst3Processor() = default;

const char* Vst3Processor::name() const noexcept { return impl_->class_name_cstr(); }

void Vst3Processor::prepare(double sample_rate_hz, std::size_t max_frames) {
    impl_->prepare(sample_rate_hz, max_frames, info_);
}

void Vst3Processor::process(const ProcessContext& context, float* left, float* right) noexcept {
    if (!impl_->process(context, left, right, last_error_)) {
        processing_ok_ = false;
    }
}

}  // namespace vstbox

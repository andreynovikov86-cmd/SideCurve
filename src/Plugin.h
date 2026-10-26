#pragma once
#include "Engine.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include <atomic>
namespace SideKick {
inline const Steinberg::FUID processorID(0x8BCF4CE2, 0x2C124DA6, 0xA91D8E86, 0x1890C3E1);
inline const Steinberg::FUID controllerID(0x550B36A5, 0x50334D3A, 0xBA274D71, 0xB9FEA4EB);
class AtomicSnapshot {
    std::array<std::atomic<float>, 256> main{}, side{};
    std::atomic<float> phase{1}, gain{1};
    std::atomic<int> active{0}, mode{0}, denominator{4};
    std::atomic<std::uint64_t> hits{0};

  public:
    void store(const Snapshot &s) {
        for (int i = 0; i < 256; ++i) {
            main[i].store(s.main[i], std::memory_order_relaxed);
            side[i].store(s.side[i], std::memory_order_relaxed);
        }
        phase.store(s.phase);
        gain.store(s.gain);
        active.store(s.active);
        mode.store(s.mode);
        denominator.store(s.denominator);
        hits.store(s.hits);
    }
    Snapshot load() const {
        Snapshot s;
        for (int i = 0; i < 256; ++i) {
            s.main[i] = main[i].load(std::memory_order_relaxed);
            s.side[i] = side[i].load(std::memory_order_relaxed);
        }
        s.phase = phase.load();
        s.gain = gain.load();
        s.active = active.load();
        s.mode = mode.load();
        s.denominator = denominator.load();
        s.hits = hits.load();
        return s;
    }
};
class Processor : public Steinberg::Vst::AudioEffect {
  public:
    Processor();
    static Steinberg::FUnknown *create(void *) {
        return static_cast<Steinberg::Vst::IAudioProcessor *>(new Processor);
    }
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown *) override;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup &) override;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool) override;
    Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement *,
                                                     Steinberg::int32,
                                                     Steinberg::Vst::SpeakerArrangement *,
                                                     Steinberg::int32) override;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData &) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream *) override;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream *) override;
    Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage *) override;

  private:
    std::array<std::atomic<double>, paramCount> state{};
    std::atomic<bool> visible{false};
    Engine engine;
    AtomicSnapshot display;
    double fallbackBeat = 0;
    template <class Sample> void render(Steinberg::Vst::ProcessData &);
};
class Controller : public Steinberg::Vst::EditControllerEx1 {
  public:
    static Steinberg::FUnknown *create(void *) {
        return static_cast<Steinberg::Vst::IEditController *>(new Controller);
    }
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown *) override;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream *) override;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream *) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream *) override;
    Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage *) override;
    Steinberg::IPlugView *PLUGIN_API createView(Steinberg::FIDString) override;
    Parameters values();
    void edit(int id, double real, bool gesture = true);
    void requestDisplay(bool open);
    void reset();
    Snapshot snapshot;
    int width = 698, height = 478;
};
} // namespace SideKick

#include "Plugin.h"
#include "Editor.h"
#include "Preferences.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "public.sdk/source/main/pluginfactory.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

using namespace Steinberg;
using namespace Steinberg::Vst;
namespace SideKick {
namespace {
constexpr int32 stateMagic = 0x534B5633, stateVersion = 1;
bool readParameters(IBStream *stream, Parameters &p) {
    if (!stream)
        return false;
    IBStreamer r(stream, kLittleEndian);
    int32 magic, version;
    if (!r.readInt32(magic) || !r.readInt32(version) || magic != stateMagic ||
        version != stateVersion)
        return false;
    Parameters staged;
    for (int i = 0; i < paramCount; ++i) {
        double x;
        if (!r.readDouble(x) || !std::isfinite(x))
            return false;
        staged.setNormalized(i, x);
    }
    p = staged;
    return true;
}
bool writeParameters(IBStream *stream, const Parameters &p) {
    if (!stream)
        return false;
    IBStreamer w(stream, kLittleEndian);
    if (!w.writeInt32(stateMagic) || !w.writeInt32(stateVersion))
        return false;
    for (int i = 0; i < paramCount; ++i)
        if (!w.writeDouble(i == Listen ? 0 : p.normalized(i)))
            return false;
    return true;
}
class DisplayParameter : public Parameter {
  public:
    explicit DisplayParameter(int id) : Parameter() {
        info.id = id;
        info.stepCount = specs[id].steps;
        info.flags = ParameterInfo::kCanAutomate;
        info.defaultNormalizedValue = normalize(id, specs[id].initial);
        valueNormalized = info.defaultNormalizedValue;
        UString(info.title, 128).fromAscii(specs[id].name);
        if (id == Mix)
            UString(info.units, 128).fromAscii("%");
        if (id == Threshold)
            UString(info.units, 128).fromAscii("dB");
        if (id == HighPass || id == LowPass)
            UString(info.units, 128).fromAscii("Hz");
        if (id == Retrigger)
            UString(info.units, 128).fromAscii("ms");
    }
    ParamValue toPlain(ParamValue v) const override { return plain(info.id, v); }
    ParamValue toNormalized(ParamValue v) const override { return normalize(info.id, v); }
    void toString(ParamValue n, String128 out) const override {
        double v = toPlain(n);
        char text[80];
        if (info.id == Mode)
            std::snprintf(text, sizeof text, "%s", v >= .5 ? "Audio" : "Grid");
        else if (info.id == Length) {
            int i = int(v);
            if (!i)
                std::snprintf(text, sizeof text, "2/1");
            else
                std::snprintf(text, sizeof text, "1/%d", 1 << (i - 1));
        } else if (info.id == Listen || info.id == Invert)
            std::snprintf(text, sizeof text, "%s", v >= .5 ? "On" : "Off");
        else
            std::snprintf(text, sizeof text, info.id >= FirstX ? "%.4f" : "%.1f", v);
        UString(out, 128).fromAscii(text);
    }
    bool fromString(const TChar *text, ParamValue &value) const override {
        char tmp[128]{};
        UString(const_cast<TChar *>(text), 128).toAscii(tmp, 128);
        char *end = nullptr;
        double x = std::strtod(tmp, &end);
        if (end == tmp || !std::isfinite(x))
            return false;
        value = toNormalized(x);
        return true;
    }
};
// Cockos's documented VST3 host extension. No Lua bridge is used.
// ABI: https://github.com/justinfrankel/reaper-sdk/blob/main/sdk/reaper_vst3_interfaces.h
class ReaperHost : public FUnknown {
  public:
    virtual void *PLUGIN_API getReaperApi(CStringA) = 0;
    virtual void *PLUGIN_API getReaperParent(uint32) = 0;
    virtual void *PLUGIN_API reaperExtended(uint32, void *, void *, void *) = 0;
    static const FUID iid;
};
const FUID ReaperHost::iid(0x79655E36, 0x77EE4267, 0xA573FEF7, 0x4912C27C);
} // namespace
Processor::Processor() {
    processContextRequirements.needTempo()
        .needProjectTimeMusic()
        .needTimeSignature()
        .needTransportState();
    setControllerClass(controllerID);
    Parameters p;
    for (int i = 0; i < paramCount; ++i)
        state[i].store(p.normalized(i));
}
tresult PLUGIN_API Processor::initialize(FUnknown *context) {
    auto result = AudioEffect::initialize(context);
    if (result != kResultOk)
        return result;
    addAudioInput(STR16("Main"), SpeakerArr::kStereo);
    addAudioInput(STR16("Sidechain 3/4"), SpeakerArr::kStereo, kAux, 0);
    addAudioOutput(STR16("Main"), SpeakerArr::kStereo);
    return kResultOk;
}
tresult PLUGIN_API Processor::setupProcessing(ProcessSetup &setup) {
    engine.prepare(setup.sampleRate);
    fallbackBeat = 0;
    return AudioEffect::setupProcessing(setup);
}
tresult PLUGIN_API Processor::setActive(TBool value) {
    if (value) {
        engine.prepare(processSetup.sampleRate);
        fallbackBeat = 0;
    }
    return AudioEffect::setActive(value);
}
tresult PLUGIN_API Processor::canProcessSampleSize(int32 size) {
    return size == kSample32 || size == kSample64 ? kResultTrue : kResultFalse;
}
tresult PLUGIN_API Processor::setBusArrangements(SpeakerArrangement *in, int32 ni,
                                                 SpeakerArrangement *out, int32 no) {
    if (ni < 1 || ni > 2 || no != 1)
        return kResultFalse;
    if ((in[0] != SpeakerArr::kMono && in[0] != SpeakerArr::kStereo) || in[0] != out[0])
        return kResultFalse;
    if (ni == 2 && in[1] != SpeakerArr::kMono && in[1] != SpeakerArr::kStereo)
        return kResultFalse;
    getAudioInput(0)->setArrangement(in[0]);
    if (ni == 2)
        getAudioInput(1)->setArrangement(in[1]);
    getAudioOutput(0)->setArrangement(out[0]);
    return kResultTrue;
}
tresult PLUGIN_API Processor::process(ProcessData &data) {
    if (data.inputParameterChanges)
        for (int i = 0; i < data.inputParameterChanges->getParameterCount(); ++i) {
            auto *q = data.inputParameterChanges->getParameterData(i);
            if (!q)
                continue;
            auto id = q->getParameterId();
            int32 offset;
            ParamValue value;
            if (id < paramCount &&
                q->getPoint(q->getPointCount() - 1, offset, value) == kResultTrue &&
                std::isfinite(value))
                state[id].store(clamp(value, 0, 1), std::memory_order_relaxed);
        }
    if (data.numSamples <= 0 || data.numOutputs < 1 || data.numInputs < 1)
        return kResultOk;
    if (data.symbolicSampleSize == kSample64)
        render<double>(data);
    else
        render<float>(data);
    return kResultOk;
}
template <class Sample> void Processor::render(ProcessData &data) {
    Parameters p;
    for (int i = 0; i < paramCount; ++i)
        p.setNormalized(i, state[i].load(std::memory_order_relaxed));
    Timeline time;
    time.beat = fallbackBeat;
    if (data.processContext) {
        auto &c = *data.processContext;
        time.playing = c.state & ProcessContext::kPlaying;
        if ((c.state & ProcessContext::kTempoValid) && std::isfinite(c.tempo) && c.tempo > 0)
            time.tempo = c.tempo;
        if (c.state & ProcessContext::kProjectTimeMusicValid)
            time.beat = c.projectTimeMusic;
        if (c.state & ProcessContext::kTimeSigValid)
            time.denominator = c.timeSigDenominator;
    }
    engine.begin(p, time, visible.load());
    auto &in = data.inputs[0];
    auto &out = data.outputs[0];
    auto **inputs = sizeof(Sample) == sizeof(double)
                        ? reinterpret_cast<Sample **>(in.channelBuffers64)
                        : reinterpret_cast<Sample **>(in.channelBuffers32);
    auto **outputs = sizeof(Sample) == sizeof(double)
                         ? reinterpret_cast<Sample **>(out.channelBuffers64)
                         : reinterpret_cast<Sample **>(out.channelBuffers32);
    Sample **sideInput = nullptr;
    int sideChannels = 0;
    uint64 sideSilence = ~uint64(0);
    if (data.numInputs > 1) {
        auto &sc = data.inputs[1];
        sideChannels = sc.numChannels;
        sideSilence = sc.silenceFlags;
        sideInput = sizeof(Sample) == sizeof(double)
                        ? reinterpret_cast<Sample **>(sc.channelBuffers64)
                        : reinterpret_cast<Sample **>(sc.channelBuffers32);
    }
    for (int n = 0; n < data.numSamples; ++n) {
        double l =
            inputs && in.numChannels > 0 && inputs[0] && !(in.silenceFlags & 1) ? inputs[0][n] : 0;
        double r = in.numChannels > 1
                       ? (inputs && inputs[1] && !(in.silenceFlags & 2) ? inputs[1][n] : 0)
                       : l;
        double sl = sideInput && sideChannels > 0 && sideInput[0] && !(sideSilence & 1)
                        ? sideInput[0][n]
                        : 0;
        double sr = sideChannels > 1
                        ? (sideInput && sideInput[1] && !(sideSilence & 2) ? sideInput[1][n] : 0)
                        : sl;
        double ol, orr;
        engine.sample(l, r, sl, sr, ol, orr);
        if (outputs && out.numChannels > 0 && outputs[0])
            outputs[0][n] = Sample(ol);
        if (outputs && out.numChannels > 1 && outputs[1])
            outputs[1][n] = Sample(orr);
    }
    out.silenceFlags = 0;
    fallbackBeat = time.beat + data.numSamples / processSetup.sampleRate * time.tempo / 60;
    display.store(engine.snapshot());
}
tresult PLUGIN_API Processor::getState(IBStream *stream) {
    Parameters p;
    for (int i = 0; i < paramCount; ++i)
        p.setNormalized(i, state[i].load());
    return writeParameters(stream, p) ? kResultOk : kResultFalse;
}
tresult PLUGIN_API Processor::setState(IBStream *stream) {
    Parameters p;
    if (!readParameters(stream, p))
        return kResultFalse;
    for (int i = 0; i < paramCount; ++i)
        state[i].store(i == Listen ? 0 : p.normalized(i));
    return kResultOk;
}
tresult PLUGIN_API Processor::notify(IMessage *msg) {
    if (!msg)
        return kInvalidArgument;
    if (!std::strcmp(msg->getMessageID(), "SideKick.display")) {
        int64 open = 0;
        msg->getAttributes()->getInt("open", open);
        visible.store(open != 0);
        if (!open) {
            state[Listen].store(0);
            return kResultOk;
        }
        auto s = display.load();
        if (auto *reply = allocateMessage()) {
            reply->setMessageID("SideKick.snapshot");
            reply->getAttributes()->setBinary("snapshot", &s, sizeof s);
            sendMessage(reply);
            reply->release();
        }
        return kResultOk;
    }
    return AudioEffect::notify(msg);
}
tresult PLUGIN_API Controller::initialize(FUnknown *context) {
    auto r = EditControllerEx1::initialize(context);
    if (r != kResultOk)
        return r;
    for (int i = 0; i < paramCount; ++i)
        parameters.addParameter(new DisplayParameter(i));
    auto dims = loadWindowSize();
    width = dims.first;
    height = dims.second;
    return kResultOk;
}
Parameters Controller::values() {
    Parameters p;
    for (int i = 0; i < paramCount; ++i)
        p.setNormalized(i, getParamNormalized(i));
    return p;
}
void Controller::edit(int id, double value, bool gesture) {
    if (id < 0 || id >= paramCount)
        return;
    double n = normalize(id, value);
    if (gesture)
        beginEdit(id);
    setParamNormalized(id, n);
    performEdit(id, n);
    if (gesture)
        endEdit(id);
}
tresult PLUGIN_API Controller::setComponentState(IBStream *stream) {
    Parameters p;
    if (!readParameters(stream, p))
        return kResultFalse;
    for (int i = 0; i < paramCount; ++i)
        setParamNormalized(i, i == Listen ? 0 : p.normalized(i));
    return kResultOk;
}
tresult PLUGIN_API Controller::getState(IBStream *stream) {
    if (!stream)
        return kResultFalse;
    IBStreamer w(stream, kLittleEndian);
    return w.writeInt32(1) && w.writeInt32(width) && w.writeInt32(height) ? kResultOk
                                                                          : kResultFalse;
}
tresult PLUGIN_API Controller::setState(IBStream *stream) {
    if (!stream)
        return kResultFalse;
    IBStreamer r(stream, kLittleEndian);
    int32 version, w, h;
    if (!r.readInt32(version) || version != 1 || !r.readInt32(w) || !r.readInt32(h))
        return kResultFalse;
    if (w >= 640 && w <= 2400 && h >= 460 && h <= 1800) {
        width = w;
        height = h;
    }
    return kResultOk;
}
IPlugView *PLUGIN_API Controller::createView(FIDString name) {
    if (std::strcmp(name, ViewType::kEditor))
        return nullptr;
    auto dims = loadWindowSize();
    width = dims.first;
    height = dims.second;
    return new Editor(this);
}
void Controller::requestDisplay(bool open) {
    if (auto *m = allocateMessage()) {
        m->setMessageID("SideKick.display");
        m->getAttributes()->setInt("open", open ? 1 : 0);
        sendMessage(m);
        m->release();
    }
}
tresult PLUGIN_API Controller::notify(IMessage *m) {
    if (m && !std::strcmp(m->getMessageID(), "SideKick.snapshot")) {
        const void *data = nullptr;
        uint32 bytes = 0;
        if (m->getAttributes()->getBinary("snapshot", data, bytes) == kResultOk &&
            bytes == sizeof(Snapshot))
            std::memcpy(&snapshot, data, bytes);
        return kResultOk;
    }
    return EditControllerEx1::notify(m);
}
void Controller::reset() {
    FUnknownPtr<ReaperHost> reaper(getHostContext());
    if (reaper) {
        using Focus = int (*)(int *, int *, int *);
        using Track = void *(*)(int, bool);
        using Preset = bool (*)(void *, int, int);
        auto focus = reinterpret_cast<Focus>(reaper->getReaperApi("GetFocusedFX"));
        auto track = reinterpret_cast<Track>(reaper->getReaperApi("CSurf_TrackFromID"));
        auto preset = reinterpret_cast<Preset>(reaper->getReaperApi("TrackFX_SetPresetByIndex"));
        int ti = 0, item = 0, fx = 0;
        if (focus && track && preset && (focus(&ti, &item, &fx) & 1) &&
            track(ti, false) == reaper->getReaperParent(1) && preset(track(ti, false), fx, -1))
            return;
    }
    for (int i = 0; i < paramCount; ++i)
        edit(i, specs[i].initial);
}
} // namespace SideKick
BEGIN_FACTORY_DEF("Andrey Novikov", "https://github.com/andreynovikov86-cmd", "")
DEF_CLASS2(INLINE_UID_FROM_FUID(SideKick::processorID), PClassInfo::kManyInstances,
           kVstAudioEffectClass, "SideCurve (Sidechain Shaper)", Vst::kDistributable, "Fx|Dynamics",
           "0.5.5", kVstVersionString, SideKick::Processor::create)
DEF_CLASS2(INLINE_UID_FROM_FUID(SideKick::controllerID), PClassInfo::kManyInstances,
           kVstComponentControllerClass, "SideCurve Controller", 0, "", "0.5.5", kVstVersionString,
           SideKick::Controller::create)
END_FACTORY

#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace SideKick {
constexpr double pi = 3.14159265358979323846;
constexpr int paramCount = 18;
enum Param {
    Mode,
    Length,
    Mix,
    Threshold,
    HighPass,
    LowPass,
    Retrigger,
    Listen,
    Invert,
    FirstX,
    FirstY,
    AnchorAX,
    AnchorAY,
    Bend,
    AnchorBX,
    AnchorBY,
    LastX,
    LastY
};
struct Spec {
    const char *name;
    double initial, low, high;
    bool logarithmic = false;
    int steps = 0;
};
inline constexpr std::array<Spec, paramCount> specs{{{"Mode", 0, 0, 1, false, 1},
                                                     {"Length", 3, 0, 7, false, 7},
                                                     {"Mix", 100, 0, 100},
                                                     {"Threshold", -30, -60, 0},
                                                     {"High pass", 25, 10, 20000, true},
                                                     {"Low pass", 180, 10, 20000, true},
                                                     {"Retrigger", 25, 1, 250},
                                                     {"Listen", 0, 0, 1, false, 1},
                                                     {"Invert", 0, 0, 1, false, 1},
                                                     {"First point time", .02, 0, .997},
                                                     {"First point level", 0, 0, 1},
                                                     {"Second point time", .1856842105263158, 0, 1},
                                                     {"Second point level", 0, 0, 1},
                                                     {"Curvature", .15, .02, .98},
                                                     {"Fourth point time", .5157894736842106, 0, 1},
                                                     {"Fourth point level", 1, 0, 1},
                                                     {"Last point time", .97, .003, 1},
                                                     {"Last point level", 1, 0, 1}}};
inline double clamp(double x, double a, double b) {
    return std::min(b, std::max(a, x));
}
inline double fraction(double x) {
    return x - std::floor(x);
}
inline double smooth(double u) {
    u = clamp(u, 0, 1);
    return u * u * (3 - 2 * u);
}
inline double cosine(double u) {
    return .5 - .5 * std::cos(pi * clamp(u, 0, 1));
}
inline double normalize(int id, double v) {
    auto s = specs[id];
    v = clamp(v, s.low, s.high);
    return s.logarithmic ? std::log(v / s.low) / std::log(s.high / s.low)
                         : (v - s.low) / (s.high - s.low);
}
inline double plain(int id, double v) {
    auto s = specs[id];
    v = clamp(v, 0, 1);
    double x = s.logarithmic ? s.low * std::pow(s.high / s.low, v) : s.low + (s.high - s.low) * v;
    return s.steps ? std::round(x) : x;
}
struct Parameters {
    std::array<double, paramCount> v{};
    Parameters() {
        for (int i = 0; i < paramCount; ++i)
            v[i] = specs[i].initial;
    }
    double operator[](int i) const { return v[i]; }
    double &operator[](int i) { return v[i]; }
    void setNormalized(int id, double x) {
        if (id >= 0 && id < paramCount && std::isfinite(x))
            v[id] = plain(id, x);
    }
    double normalized(int id) const { return normalize(id, v[id]); }
    double start() const { return clamp(v[FirstX], 0, .997); }
    double finish() const { return clamp(v[LastX], start() + .003, 1); }
    double duration() const { return 8 / std::pow(2, clamp(std::floor(v[Length]), 0, 7)); }
    double displayEnd() const { return finish(); }
    double displayLevel(double level) const { return v[Invert] >= .5 ? 1 - level : level; }
    double curve(double position) const { return displayLevel(baseCurve(position)); }
    double baseCurve(double position) const {
        double a = clamp(v[AnchorAX], .001, .997), b = clamp(v[AnchorBX], a + .002, .999);
        double first = clamp(v[FirstY], 0, 1), last = clamp(v[LastY], 0, 1),
               av = clamp(v[AnchorAY], 0, 1), bv = clamp(v[AnchorBY], 0, 1);
        if (position < start() || position > finish()) {
            double gap = 1 - finish() + start();
            double u =
                gap > 1e-7
                    ? (position > finish() ? position - finish() : 1 - finish() + position) / gap
                    : 1;
            return last + (first - last) * cosine(u);
        }
        double p = (position - start()) / (finish() - start());
        if (p <= a)
            return first + (av - first) * smooth(p / std::max(1e-6, a));
        if (p >= b)
            return bv + (last - bv) * smooth((p - b) / std::max(1e-6, 1 - b));
        double u = smooth((p - a) / std::max(1e-6, b - a)), bend = clamp(v[Bend], .02, .98),
               k = (1 - bend) / bend;
        u = u / (k + (1 - k) * u);
        return av + (bv - av) * u;
    }
    double lastLevel() const { return displayLevel(clamp(v[LastY], 0, 1)); }
    double firstLevel() const { return displayLevel(clamp(v[FirstY], 0, 1)); }
};
struct Biquad {
    double b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
};
struct Timeline {
    double tempo = 120, beat = 0;
    bool playing = false;
    int denominator = 4;
};
struct Snapshot {
    std::array<float, 256> main{}, side{};
    float phase = 1, gain = 1;
    int active = 0, mode = 0, denominator = 4;
    std::uint64_t hits = 0;
};
class Engine {
  public:
    Parameters params;
    void prepare(double rate) {
        sampleRate = std::max(1., rate);
        gain = 1;
        audition = 0;
        phase = 1;
        active = false;
        armed = true;
        detector = 0;
        cooldown = 0;
        lastMode = -1;
        wasPlaying = false;
        cycle = 1;
        hits = 0;
        stamp.fill(0);
        wave.fill(0);
        side.fill(0);
        stateL.fill(0);
        stateR.fill(0);
        oldHP = oldLP = -1;
        oldRate = 0;
    }
    void begin(const Parameters &p, const Timeline &time, bool visible) {
        params = p;
        timeline = time;
        depth = clamp(p[Mix] * .01, 0, 1);
        threshold = std::pow(10, p[Threshold] / 20);
        if (oldHP != p[HighPass] || oldLP != p[LowPass] || oldRate != sampleRate) {
            edge(p[HighPass], true, 0);
            edge(std::max(p[HighPass] + 1, p[LowPass]), false, 4);
            oldHP = p[HighPass];
            oldLP = p[LowPass];
            oldRate = sampleRate;
        }
        release = std::exp(-1 / (sampleRate * .008));
        slew = 1 - std::exp(-1 / (sampleRate * .0007));
        auditionSlew = 1 - std::exp(-1 / (sampleRate * .005));
        join = std::max(0., 1 - p.finish() + p.start());
        step = std::max(1., time.tempo) / (60 * sampleRate * p.duration());
        seamSamples =
            std::max(1., std::floor(sampleRate * 60 * p.duration() / std::max(1., time.tempo) *
                                    (p[Mode] == 1 ? join : p.start())));
        guard = std::max(1., p[Retrigger] * sampleRate * .001);
        if (p[Mode] != lastMode || (time.playing && !wasPlaying)) {
            active = false;
            phase = 1;
            detector = 0;
            armed = true;
            cooldown = 0;
            ++cycle;
            stateL.fill(0);
            stateR.fill(0);
            startSeam();
        }
        if (p[Mode] == 0) {
            double next = fraction(time.beat / p.duration());
            if (std::abs(next - phase) > .15) {
                ++cycle;
                startSeam();
            }
            phase = next;
        }
        lastMode = int(p[Mode]);
        wasPlaying = time.playing;
        listen = p[Listen] >= .5 && visible && p[Mode] == 1;
    }
    void sample(double inL, double inR, double scL, double scR, double &outL, double &outR) {
        scL = filter(scL, stateL);
        scR = filter(scR, stateR);
        double peak = std::max(std::abs(scL), std::abs(scR));
        detector = std::max(peak, detector * release);
        cooldown = std::max(0., cooldown - 1);
        if (detector < threshold * .5)
            armed = true;
        if (params[Mode] == 1 && armed && cooldown <= 0 && peak >= threshold) {
            phase = 0;
            active = true;
            armed = false;
            cooldown = guard;
            ++hits;
            ++cycle;
            shotFrom = gain;
            seamOn = false;
        }
        double target = 1;
        if (params[Mode] == 0) {
            if (timeline.playing)
                target = 1 - depth + depth * params.curve(phase);
        } else if (active && join > 1e-7 && phase < join) {
            double firstGain = 1 - depth + depth * params.firstLevel();
            target = shotFrom + (firstGain - shotFrom) * cosine(phase / join);
        } else
            target = 1 - depth +
                     depth * (active ? params.curve(fraction(params.displayEnd() + phase))
                                     : params.lastLevel());
        if (seamOn) {
            target = seamFrom + (target - seamFrom) * cosine(seamPos / seamSamples);
            if (++seamPos > seamSamples)
                seamOn = false;
        }
        gain += slew * (target - gain);
        audition += auditionSlew * ((listen ? 1. : 0) - audition);
        double l = inL * gain, r = inR * gain;
        double wp = params[Mode] == 1
                        ? (active ? fraction(params.displayEnd() + phase) : params.displayEnd())
                        : phase;
        int idx = std::clamp(int(std::floor(wp * 256)), 0, 255);
        if (stamp[idx] != cycle) {
            wave[idx] = side[idx] = 0;
            stamp[idx] = cycle;
        }
        wave[idx] = std::max(wave[idx], float(std::max(std::abs(l), std::abs(r))));
        side[idx] = std::max(side[idx], float(peak));
        outL = l * (1 - audition) + scL * audition;
        outR = r * (1 - audition) + scR * audition;
        if (params[Mode] == 0) {
            if (timeline.playing) {
                phase += step;
                if (phase >= 1) {
                    phase = fraction(phase);
                    ++cycle;
                }
            }
        } else if (active) {
            phase += step;
            if (phase >= 1) {
                phase = 1;
                active = false;
            }
        }
    }
    Snapshot snapshot() const {
        Snapshot s;
        s.phase = float(phase);
        s.gain = float(gain);
        s.active = params[Mode] == 0 ? timeline.playing : active;
        s.mode = int(params[Mode]);
        s.denominator = timeline.denominator;
        s.hits = hits;
        for (int i = 0; i < 256; ++i)
            if (stamp[i] > 0 && stamp[i] + 1 >= cycle) {
                s.main[i] = wave[i];
                s.side[i] = side[i];
            }
        return s;
    }
    double currentGain() const { return gain; }
    double currentPhase() const { return phase; }
    std::uint64_t hitCount() const { return hits; }

  private:
    std::array<Biquad, 8> coefficients{};
    std::array<double, 16> stateL{}, stateR{};
    std::array<float, 256> wave{}, side{};
    std::array<std::uint64_t, 256> stamp{};
    Timeline timeline;
    double sampleRate = 44100, gain = 1, audition = 0, phase = 1, detector = 0, cooldown = 0;
    double oldHP = -1, oldLP = -1, oldRate = 0, depth = 1, threshold = .03, release = 0, slew = 0,
           auditionSlew = 0, join = .05, step = 0, guard = 0;
    double seamFrom = 1, seamPos = 0, seamSamples = 1, shotFrom = 1;
    bool active = false, armed = true, seamOn = false, wasPlaying = false, listen = false;
    int lastMode = -1;
    std::uint64_t cycle = 1, hits = 0;
    void startSeam() {
        seamFrom = gain;
        seamPos = 0;
        seamOn = true;
    }
    void edge(double frequency, bool high, int offset) {
        double w = 2 * pi * std::min(std::max(10., frequency), sampleRate * .45) / sampleRate,
               co = std::cos(w);
        for (int i = 0; i < 4; ++i) {
            double q = 1 / (2 * std::cos((2 * i + 1) * pi / 16)), al = std::sin(w) / (2 * q),
                   norm = 1 / (1 + al);
            auto &c = coefficients[offset + i];
            c.b0 = (high ? 1 + co : 1 - co) * .5 * norm;
            c.b1 = (high ? -(1 + co) : 1 - co) * norm;
            c.b2 = c.b0;
            c.a1 = -2 * co * norm;
            c.a2 = (1 - al) * norm;
        }
    }
    double filter(double x, std::array<double, 16> &states) {
        for (int i = 0; i < 8; ++i) {
            const auto &c = coefficients[i];
            double out = c.b0 * x + states[i * 2];
            states[i * 2] = c.b1 * x - c.a1 * out + states[i * 2 + 1];
            states[i * 2 + 1] = c.b2 * x - c.a2 * out;
            x = out;
        }
        return x;
    }
};
} // namespace SideKick

#pragma once
#include "Engine.h"

namespace SideKick {
// Pure gesture calculations shared by the editor and its interaction checks.
struct PlotPoints {
    std::array<double, 5> x{}, y{};
    explicit PlotPoints(const Parameters &p) {
        double span = p.finish() - p.start();
        x = {p.start(), p.start() + clamp(p[AnchorAX], .001, .997) * span, 0,
             p.start() + clamp(p[AnchorBX], p[AnchorAX] + .002, .999) * span, p.finish()};
        y = {p.displayLevel(p[FirstY]), p.displayLevel(p[AnchorAY]), 0, p.displayLevel(p[AnchorBY]),
             p.displayLevel(p[LastY])};
        x[2] = (x[1] + x[3]) * .5;
        y[2] = y[1] + (y[3] - y[1]) * clamp(p[Bend], .02, .98);
    }
};
inline double plotGridStep(const Parameters &p, int denominator, double width) {
    double step = (4. / std::max(1, denominator)) / p.duration();
    while (step * width > 120)
        step *= .5;
    while (step * width < 38)
        step *= 2;
    return step;
}
inline Parameters movePlotEdge(Parameters p, int index, double t) {
    PlotPoints old(p);
    double start = p.start(), finish = p.finish();
    if (index == 0)
        start = clamp(t, 0, old.x[1] - .001);
    else
        finish = clamp(t, old.x[3] + .001, 1);
    double span = std::max(.003, finish - start);
    p[FirstX] = start;
    p[LastX] = finish;
    p[AnchorAX] = clamp((old.x[1] - start) / span, .001, .997);
    p[AnchorBX] = clamp((old.x[3] - start) / span, p[AnchorAX] + .002, .999);
    return p;
}
inline Parameters movePlotPoint(Parameters p, int index, double t, double level) {
    if (index == 0 || index == 4)
        p = movePlotEdge(p, index, t);
    double v = p.displayLevel(clamp(level, 0, 1)); // inversion is its own inverse
    double norm = (t - p.start()) / (p.finish() - p.start());
    if (index == 0)
        p[FirstY] = v;
    if (index == 4)
        p[LastY] = v;
    if (index == 1) {
        p[AnchorAX] = clamp(norm, .001, p[AnchorBX] - .002);
        p[AnchorAY] = v;
    }
    if (index == 3) {
        p[AnchorBX] = clamp(norm, p[AnchorAX] + .002, .999);
        p[AnchorBY] = v;
    }
    if (index == 2 && std::abs(p[AnchorBY] - p[AnchorAY]) > .0001)
        p[Bend] = clamp((v - p[AnchorAY]) / (p[AnchorBY] - p[AnchorAY]), .02, .98);
    return p;
}
inline Parameters slidePlotTransition(Parameters p, double offset) {
    double span = p.finish() - p.start();
    double shift = clamp(offset / span, .001 - p[AnchorAX], .999 - p[AnchorBX]);
    p[AnchorAX] += shift;
    p[AnchorBX] += shift;
    return p;
}
inline Parameters liftPlotSection(Parameters p, bool tail, double offset) {
    int a = tail ? AnchorBY : FirstY, b = tail ? LastY : AnchorAY;
    double av = p.displayLevel(p[a]), bv = p.displayLevel(p[b]);
    double shift = clamp(offset, -std::min(av, bv), 1 - std::max(av, bv));
    p[a] = p.displayLevel(av + shift);
    p[b] = p.displayLevel(bv + shift);
    return p;
}
inline Parameters snapPlotTransition(Parameters p, double step) {
    PlotPoints pts(p);
    double span = p.finish() - p.start();
    int first = int(std::ceil((p.start() + .001 * span - 1e-10) / step));
    int last = int(std::floor((p.start() + .999 * span + 1e-10) / step));
    double best = 1e30;
    int left = -1, right = -1;
    // Snap only the two anchors; the curvature handle remains their free midpoint.
    for (int a = first; a <= last; ++a)
        for (int b = a + 1; b <= last; ++b) {
            double da = a * step - pts.x[1], db = b * step - pts.x[3];
            if ((b - a) * step < .002 * span)
                continue;
            double score = da * da + db * db;
            if (score < best) {
                best = score;
                left = a;
                right = b;
            }
        }
    if (left >= 0 && (right - left) * step >= .002 * span) {
        p[AnchorAX] = (left * step - p.start()) / span;
        p[AnchorBX] = (right * step - p.start()) / span;
    }
    return p;
}
inline double nearestGridTime(double t, double lo, double hi, double step) {
    double first = std::ceil((lo - 1e-10) / step), last = std::floor((hi + 1e-10) / step);
    return first <= last ? clamp(std::floor(t / step + .5), first, last) * step : t;
}
inline Parameters snapPlotPoint(Parameters p, int index, double step) {
    PlotPoints pts(p);
    double span = p.finish() - p.start(), lo = 0, hi = 1;
    if (index == 0)
        hi = pts.x[1] - .001;
    if (index == 4)
        lo = pts.x[3] + .001;
    if (index == 1) {
        lo = p.start() + .001 * span;
        hi = pts.x[3] - .002 * span;
    }
    if (index == 3) {
        lo = pts.x[1] + .002 * span;
        hi = p.start() + .999 * span;
    }
    if (index == 2) {
        return snapPlotTransition(p, step);
    }
    return movePlotPoint(p, index, nearestGridTime(pts.x[index], lo, hi, step), pts.y[index]);
}
} // namespace SideKick

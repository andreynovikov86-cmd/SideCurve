#include "Engine.h"
#include "Preferences.h"
#include "EditorModel.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>
using namespace SideKick;
int checks = 0;
void require(bool ok, const char *label) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", label);
        std::exit(1);
    }
    ++checks;
}
int main() {
    Parameters p;
    for (int id = 0; id < paramCount; ++id)
        for (double n : {0., .1, .5, .9, 1.}) {
            double v = plain(id, n), r = normalize(id, v);
            require(std::isfinite(r) && r >= 0 && r <= 1, "normalized parameter round trip");
            if (!specs[id].steps)
                require(std::abs(n - r) < 1e-10, "continuous parameter conversion");
        }
    for (int i = 0; i < 10000; ++i) {
        double t = double(i) / 9999, v = p.curve(t);
        require(std::isfinite(v) && v >= 0 && v <= 1, "bounded default shape");
        double original = v;
        auto inverse = p;
        inverse[Invert] = 1;
        require(std::abs(1 - original - inverse.curve(t)) < 1e-10, "exact vertical inversion");
    }
    require(std::abs(p.curve(0) - p.curve(1)) < 1e-12, "periodic seam");
    p[FirstX] = .07;
    p[LastX] = .91;
    p[FirstY] = .12;
    p[LastY] = .73;
    p[AnchorAX] = .23;
    p[AnchorAY] = .31;
    p[AnchorBX] = .64;
    p[AnchorBY] = .82;
    p[Bend] = .27;
    auto inverse = p;
    inverse[Invert] = 1;
    PlotPoints pts(p), inv(inverse);
    for (int i = 0; i < 5; ++i) {
        require(pts.x[i] == inv.x[i], "vertical INVERT preserves every point time");
        require(std::abs(pts.y[i] + inv.y[i] - 1) < 1e-12, "vertical INVERT reflects point levels");
    }
    require(inverse.displayEnd() == p.displayEnd(), "INVERT keeps the one-shot endpoint time");
    require(std::abs(inverse.lastLevel() - .27) < 1e-12, "INVERT holds complemented last level");
    for (int i = 0; i < 1000; ++i)
        require(std::abs(p.curve(i / 999.) + inverse.curve(i / 999.) - 1) < 1e-12,
                "custom periodic join inverts vertically too");
    auto moved = movePlotPoint(inverse, 1, .34, .76);
    PlotPoints mp(moved);
    require(std::abs(mp.x[1] - .34) < 1e-12 && std::abs(mp.y[1] - .76) < 1e-12,
            "inverted anchor follows the displayed pointer");
    moved = movePlotPoint(inverse, 2, .4, .44);
    require(std::abs(PlotPoints(moved).y[2] - .44) < 1e-12, "inverted curvature edit");
    for (double offset : {-10., -.04, .04, 10.}) {
        moved = slidePlotTransition(inverse, offset);
        PlotPoints group(moved);
        require(std::abs((group.x[3] - group.x[1]) - (pts.x[3] - pts.x[1])) < 1e-12,
                "region drag preserves transition width at boundaries");
        require(group.x[1] > group.x[0] && group.x[3] < group.x[4],
                "region drag cannot cross outer anchors");
        require(moved[AnchorAY] == inverse[AnchorAY] && moved[AnchorBY] == inverse[AnchorBY] &&
                    moved[Bend] == inverse[Bend],
                "region drag preserves levels and curvature");
    }
    for (int index = 0; index < 5; ++index) {
        moved = snapPlotPoint(inverse, index, .125);
        PlotPoints snap(moved);
        if (index == 2) {
            for (int anchor : {1, 3})
                require(std::abs(snap.x[anchor] / .125 - std::round(snap.x[anchor] / .125)) < 1e-10,
                        "middle double-click snaps only the two transition anchors");
        } else
            require(std::abs(snap.x[index] / .125 - std::round(snap.x[index] / .125)) < 1e-10,
                    "double-click snaps to a time-grid line");
        for (int i = 0; i < 5; ++i)
            require(std::abs(snap.y[i] - inv.y[i]) < 1e-12, "snapping preserves all levels");
    }
    for (bool inverted : {false, true})
        for (bool tail : {false, true}) {
            auto source = p;
            source[Invert] = inverted ? 1 : 0;
            PlotPoints before(source);
            int a = tail ? 3 : 0, b = tail ? 4 : 1;
            for (double offset : {-10., -.05, .05, 10.}) {
                auto lifted = liftPlotSection(source, tail, offset);
                PlotPoints after(lifted);
                require(std::abs((after.y[b] - after.y[a]) - (before.y[b] - before.y[a])) < 1e-12,
                        "section lift preserves the relative endpoint levels");
                require(std::abs((after.y[a] - before.y[a]) -
                                 clamp(offset, -std::min(before.y[a], before.y[b]),
                                       1 - std::max(before.y[a], before.y[b]))) < 1e-12,
                        "section lift follows display direction and clamps the whole pair");
                for (int i = 0; i < 5; ++i)
                    require(after.x[i] == before.x[i], "section lift preserves all times");
                for (int i : {0, 1, 3, 4})
                    if (i != a && i != b)
                        require(after.y[i] == before.y[i],
                                "section lift leaves opposite endpoints untouched");
                require(lifted[Bend] == source[Bend], "section lift preserves bend");
            }
        }
    auto offGrid = p;
    offGrid[FirstX] = 0;
    offGrid[LastX] = 1;
    offGrid[AnchorAX] = .13;
    offGrid[AnchorBX] = .38;
    moved = snapPlotPoint(offGrid, 2, .125);
    require(std::abs(PlotPoints(moved).x[1] - .125) < 1e-12 &&
                std::abs(PlotPoints(moved).x[3] - .375) < 1e-12,
            "each main anchor chooses its nearest grid line");
    require(std::abs(PlotPoints(moved).x[2] - .25) < 1e-12, "middle remains the derived midpoint");
    offGrid[AnchorBX] = .26;
    moved = snapPlotPoint(offGrid, 2, .125);
    require(std::abs(PlotPoints(moved).x[2] - .1875) < 1e-12,
            "middle handle is not forced onto the grid");
    moved = movePlotPoint(inverse, 0, .1, .9);
    require(std::abs(PlotPoints(moved).x[1] - pts.x[1]) < 1e-12,
            "moving outer anchor preserves inner absolute time when inverted");
    require(nearestGridTime(.43, .42, .44, .125) == .43,
            "no legal grid line leaves point unchanged");
    for (int rate : {44100, 48000, 96000}) {
        Engine e;
        e.prepare(rate);
        p = Parameters();
        p[Mode] = 1;
        Timeline time;
        time.playing = true;
        double previous = .25, maxJump = 0;
        for (int n = 0; n < rate * 2; ++n) {
            double t = double(n) / rate;
            if (n % 128 == 0) {
                time.beat = t * 2;
                e.begin(p, time, true);
            }
            double sc = ((t >= .1 && t < .15) || (t >= .35 && t < .4))
                            ? .8 * std::sin(2 * pi * 65 * t)
                            : 0,
                   l, r;
            e.sample(.25, .125, sc, -sc, l, r);
            require(std::isfinite(l) && std::isfinite(r), "finite audio");
            require(std::abs(l - r * 2) < 1e-12, "stereo linking");
            maxJump = std::max(maxJump, std::abs(l - previous));
            previous = l;
        }
        require(e.hitCount() == 2, "accepted attacks restart the one-shot");
        require(maxJump < .003, "smooth attack gain");
        require(e.currentGain() > .99999, "held final unity");
        auto s = e.snapshot();
        int populated = 0;
        for (auto value : s.side)
            if (value > .001)
                ++populated;
        require(populated > 10, "sidechain waveform occupies multiple bins");
    }
    p = Parameters();
    p[Mode] = 1;
    p[Invert] = 1;
    Engine e;
    e.prepare(48000);
    Timeline t;
    t.playing = true;
    for (int n = 0; n < 96000; ++n) {
        if (n % 128 == 0) {
            t.beat = n / 24000.;
            e.begin(p, t, true);
        }
        double time = n / 48000.,
               sc = time >= .1 && time < .15 ? .8 * std::sin(2 * pi * 65 * time) : 0, l, r;
        e.sample(.25, .125, sc, -sc, l, r);
    }
    require(e.currentGain() < 1e-8, "inverted final silence is held");
    auto dir = std::filesystem::temp_directory_path() /
               ("SideKick-test-" +
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    auto path = dir / "window.txt";
    saveWindowSize(812, 577, path);
    auto dims = loadWindowSize(path);
    require(dims.first == 812 && dims.second == 577, "new instance reads last window size");
    saveWindowSize(0, 1, path);
    dims = loadWindowSize(path);
    require(dims.first == 812 && dims.second == 577,
            "invalid size does not replace saved dimensions");
    {
        std::ofstream out(path);
        out << "invalid";
    }
    dims = loadWindowSize(path);
    require(dims.first == 698 && dims.second == 478, "invalid preferences use compact default");
    std::filesystem::remove_all(dir);
    std::printf("PASS %d audio, parameter and persistence assertions\n", checks);
}

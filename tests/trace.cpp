#include "Engine.h"
#include <cstdio>
#include <cstdlib>
using namespace SideKick;
int main(int argc, char **argv) {
    Parameters p;
    int rate = argc > 1 ? std::atoi(argv[1]) : 48000;
    double seconds = argc > 2 ? std::atof(argv[2]) : 1.;
    int signal = argc > 3 ? std::atoi(argv[3]) : 0;
    for (int i = 4; i + 1 < argc; i += 2) {
        int id = std::atoi(argv[i]);
        if (id >= 0 && id < paramCount)
            p[id] = std::atof(argv[i + 1]);
    }
    Engine e;
    e.prepare(rate);
    Timeline time;
    time.playing = true;
    for (int n = 0; n < int(seconds * rate); ++n) {
        double t = double(n) / rate;
        if (n % 128 == 0) {
            time.beat = t * 2;
            e.begin(p, time, true);
        }
        double sc = 0;
        if (signal == 1)
            sc =
                ((t >= .1 && t < .15) || (t >= .35 && t < .4)) ? .8 * std::sin(2 * pi * 65 * t) : 0;
        if (signal == 2)
            sc = .8 * std::sin(2 * pi * 65 * t);
        if (signal == 3)
            sc = .8 * std::sin(2 * pi * 8000 * t);
        double l, r;
        e.sample(.25, .125, sc, -sc, l, r);
        if (n % 128 == 0)
            std::printf("%d,%.12f,%.12f,%llu,%.12f\n", n, l, e.currentPhase(),
                        (unsigned long long)e.hitCount(), r);
    }
}

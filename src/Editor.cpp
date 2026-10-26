#include "Editor.h"
#include "Preferences.h"
#include "EditorModel.h"
#include "Branding.h"
#include "vstgui/lib/clinestyle.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/cgradient.h"
#include "vstgui/lib/cgraphicstransform.h"
#include "vstgui/lib/cfont.h"
#include "vstgui/lib/cvstguitimer.h"
#include "vstgui/lib/controls/coptionmenu.h"
#include "vstgui/lib/events.h"
#include <vector>
#include <cstdio>
using namespace VSTGUI;
namespace SideKick {
namespace {
CColor rgb(unsigned c, unsigned char alpha = 255) {
    return CColor((c >> 16) & 255, (c >> 8) & 255, c & 255, alpha);
}
// One accent hue. Only translucent overlays and selected backgrounds are tinted.
constexpr unsigned purple = 0xAF85FF, text = 0xD5D5D5, muted = 0xA0A0A0, disabled = 0x686868;
constexpr unsigned background = 0x303030, plot = 0x242424, railColor = 0x484848;
constexpr unsigned selectedBackground = 0x594770;
constexpr double buttonWidth = 88, knobX = 100, knobRadius = 48, arcRadius = 55;

const char *lengths[] = {"2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64"};
} // namespace
class Canvas : public CView {
    Controller *owner;
    SharedPointer<CFontDesc> font{owned(new CFontDesc("Arial", 15, kBoldFace))};
    SharedPointer<CFontDesc> signatureFont{owned(new CFontDesc("Arial", 12, 0))};
    double scale = 1, uw = 820, uh = 600, pl = 69, pt = 92, pw = 712, ph = 285, by = 423, fx = 242,
           fw = 362, right = 590, rail = 476, hx = 548, hy = 444;
    std::array<double, 5> xs{}, ys{};
    int drag = -1;
    double downY = 0, downValue = 0, bandX = 0, bandHP = 25, bandLP = 180;
    std::vector<int> gestures;
    bool plotHover = false;
    double regionDownX = 0, regionDownY = 0;
    Parameters regionStart;

  public:
    Canvas(const CRect &bounds, Controller *c) : CView(bounds), owner(c) { setMouseEnabled(true); }
    void layout() {
        scale = std::max(.1, std::min(getWidth() / 820, getHeight() / 600));
        uw = getWidth() / scale;
        uh = getHeight() / scale;
        pw = uw - 108;
        ph = uh - 315;
        by = uh - 177;
        right = uw - 230;
        fw = right - 24 - fx;
        rail = by + 53;
        hx = fx + fw - 18;
        hy = by + 21;
    }
    void points(const Parameters &p) {
        PlotPoints pts(p);
        xs = pts.x;
        ys = pts.y;
    }
    double knobY() const { return by + 78; }
    bool knobHit(const CPoint &q) const {
        return std::hypot(q.x - knobX, q.y - knobY()) <= arcRadius + 5;
    }
    void apply(const Parameters &p) {
        auto old = owner->values();
        for (int i = FirstX; i <= LastY; ++i)
            if (old[i] != p[i])
                change(i, p[i]);
    }
    void artwork(CDrawContext *d, const Branding::Artwork &art, double x, double y, double height,
                 unsigned color) {
        auto path = owned(d->createGraphicsPath());
        if (!path)
            return;
        for (std::size_t i = 0; i < art.count; ++i) {
            const auto &outline = art.outlines[i];
            path->beginSubpath(x + outline.points[0].x * height, y + outline.points[0].y * height);
            for (std::size_t j = 1; j < outline.count; ++j)
                path->addLine(x + outline.points[j].x * height, y + outline.points[j].y * height);
            path->closeSubpath();
        }
        d->setFillColor(rgb(color));
        d->drawGraphicsPath(path, CDrawContext::kPathFilledEvenOdd);
    }
    void rightLabel(CDrawContext *d, double x, double y, double w, double h, const char *value,
                    int size, unsigned c) {
        d->setFont(font, size);
        d->setFontColor(rgb(c));
        d->drawString(value, CRect(x, y, x + w, y + h), kRightText, true);
    }
    void box(CDrawContext *d, double x, double y, double w, double h, unsigned color,
             double radius = 6) {
        auto path = owned(d->createRoundRectGraphicsPath(CRect(x, y, x + w, y + h), radius));
        if (path) {
            d->setFillColor(rgb(color));
            d->drawGraphicsPath(path, CDrawContext::kPathFilled);
        }
    }
    void line(CDrawContext *d, double x, double y, double xx, double yy, unsigned c,
              double width = 1, int alpha = 255) {
        d->setFrameColor(rgb(c, alpha));
        d->setLineWidth(width);
        d->drawLine(CPoint(x, y), CPoint(xx, yy));
    }
    void circle(CDrawContext *d, double x, double y, double r, unsigned c) {
        d->setFillColor(rgb(c));
        d->drawEllipse(CRect(x - r, y - r, x + r, y + r), kDrawFilled);
    }
    void label(CDrawContext *d, double x, double y, double w, double h, const char *value,
               int size = 15, unsigned c = text, bool center = false) {
        d->setFont(font, size);
        d->setFontColor(rgb(c));
        d->drawString(value, CRect(x, y, x + w, y + h), center ? kCenterText : kLeftText, true);
    }
    void button(CDrawContext *d, double x, double y, double w, double h, const char *value,
                bool selected = false) {
        box(d, x, y, w, h, selected ? selectedBackground : 0x3B3B3B);
        label(d, x, y, w, h, value, 15, text, true);
    }
    void fader(CDrawContext *d, double x, double y, double w, const char *title, double value,
               double lo, double hi, bool enabled, const char *unit) {
        unsigned c = enabled ? text : disabled;
        label(d, x, y, w - 82, 24, title, 15, c);
        char buf[32];
        std::snprintf(buf, sizeof buf, "%.0f %s", value, unit);
        rightLabel(d, x + w - 76, y, 76, 24, buf, 15, c);
        double a = clamp((value - lo) / (hi - lo), 0, 1);
        box(d, x, y + 32, w, 8, railColor, 4);
        box(d, x, y + 32, std::max(8., w * a), 8, enabled ? purple : 0x4B4B4B, 4);
        circle(d, x + w * a, y + 36, 7, enabled ? purple : disabled);
    }
    void draw(CDrawContext *d) override {
        layout();
        auto p = owner->values();
        points(p);
        auto s = owner->snapshot;
        bool sc = p[Mode] >= .5;
        CDrawContext::Transform transform(*d, CGraphicsTransform().scale(scale, scale));
        d->setDrawMode(kAntiAliasing | kNonIntegralMode);
        d->setFillColor(rgb(background));
        d->drawRect(CRect(0, 0, uw, uh), kDrawFilled);
        button(d, 20, 18, buttonWidth, 30, "GRID", !sc);
        button(d, 116, 18, buttonWidth, 30, "AUDIO", sc);
        // AN remains the maker mark; the author credit uses regular upright type.
        double brandX = uw * .5 - 104.5;
        artwork(d, Branding::mark, brandX, 16, 32, text);
        label(d, brandX + 72, 12, 129, 25, "SideCurve", 22, text);
        d->setFont(signatureFont, 12);
        d->setFontColor(rgb(0xC6C6C6));
        d->drawString("by Andrey Novikov", CRect(brandX + 72, 33, brandX + 209, 49), kLeftText,
                      true);
        button(d, uw - 204, 18, buttonWidth, 30, "INVERT", p[Invert] >= .5);
        button(d, uw - 108, 18, buttonWidth, 30, "RESET");
        box(d, 20, 65, uw - 40, uh - 260, plot, 9);
        for (int i = 0; i < 5; ++i) {
            double y = pt + ph * i / 4;
            line(d, pl, y, pl + pw, y, 0x414141, 1, 204);
            char b[16];
            std::snprintf(b, sizeof b, "%d%%", 100 - i * 25);
            label(d, 26, y - 11, 43, 22, b, 14, muted);
        }
        double step = plotGridStep(p, s.denominator, pw);
        for (int i = 0; i <= 128 && i * step <= 1.000001; ++i) {
            double x = pl + std::min(1., i * step) * pw;
            line(d, x, pt, x, pt + ph, 0x414141, 1, 166);
        }
        int length = std::clamp(int(p[Length]), 0, 7);
        char half[16];
        if (length == 0)
            std::snprintf(half, sizeof half, "1/1");
        else
            std::snprintf(half, sizeof half, "1/%d", 1 << length);
        label(d, pl + pw * .5 - 50, pt + ph + 5, 100, 28, half, 15, text, true);
        label(d, pl + pw - 45, pt + ph + 5, 90, 28, lengths[length], 15, text, true);
        // Square-root peak display: reveal quiet audio without changing audio gain.
        double mid = pt + ph * .5;
        for (int i = 0; i < 256; ++i) {
            double x = pl + (i + .5) * pw / 256,
                   a = std::sqrt(std::clamp(s.main[i], 0.f, 1.f)) * ph * .45;
            line(d, x, mid - a, x, mid + a, 0xA2A2A7, 1, 112);
            if (sc) {
                a = std::sqrt(std::clamp(s.side[i], 0.f, 1.f)) * ph * .45;
                line(d, x, mid - a, x, mid + a, purple, 1, 122);
            }
        }
        if (plotHover) {
            double x1 = pl + xs[1] * pw, x2 = pl + xs[3] * pw;
            d->setFillColor(rgb(purple, 18));
            d->drawRect(CRect(x1, pt, x2, pt + ph), kDrawFilled);
            for (double x : {x1, x2})
                for (double y = pt; y < pt + ph; y += 9)
                    line(d, x, y, x, std::min(pt + ph, y + 5), purple, 1, 110);
        }
        auto path = owned(d->createGraphicsPath());
        if (path) {
            path->beginSubpath(pl, pt + ph * (1 - p.curve(0)));
            int steps = std::max(512, int(std::ceil(pw * scale)));
            for (int i = 1; i <= steps; ++i) {
                double t = double(i) / steps;
                path->addLine(pl + t * pw, pt + ph * (1 - p.curve(t)));
            }
            d->setFrameColor(rgb(purple));
            d->setLineWidth(3.2);
            d->drawGraphicsPath(path, CDrawContext::kPathStroked);
        }
        if (s.active) {
            double t = sc ? fraction(p.displayEnd() + s.phase) : s.phase;
            line(d, pl + t * pw, pt, pl + t * pw, pt + ph, 0xE1DFE6, 1, 140);
        }
        for (int i = 0; i < 5; ++i) {
            double x = pl + xs[i] * pw, y = pt + ph * (1 - ys[i]);
            circle(d, x, y, 6.4, purple);
            circle(d, x, y, 2.8, plot);
        }
        // One clean face, thick continuous accent arc, inset white dot.
        circle(d, knobX, knobY() + 1, knobRadius, 0x242424);
        auto face = owned(d->createGraphicsPath());
        if (face) {
            face->addEllipse(CRect(knobX - knobRadius, knobY() - knobRadius, knobX + knobRadius,
                                   knobY() + knobRadius));
            auto gradient = owned(face->createGradient(0, 1, rgb(0x424242), rgb(0x363636)));
            if (gradient)
                d->fillLinearGradient(face, *gradient, CPoint(knobX, knobY() - knobRadius),
                                      CPoint(knobX, knobY() + knobRadius), false);
        }
        double mix = clamp(p[Mix] * .01, 0, 1);
        auto drawArc = [&](double amount, unsigned color) {
            if (amount <= 0)
                return;
            auto arc = owned(d->createGraphicsPath());
            if (!arc)
                return;
            int steps = std::max(1, int(std::ceil(240 * amount)));
            for (int i = 0; i <= steps; ++i) {
                double a = pi * .75 + pi * 1.5 * amount * i / steps;
                double x = knobX + arcRadius * std::cos(a), y = knobY() + arcRadius * std::sin(a);
                if (!i)
                    arc->beginSubpath(x, y);
                else
                    arc->addLine(x, y);
            }
            d->setLineStyle(CLineStyle(CLineStyle::kLineCapRound, CLineStyle::kLineJoinRound));
            d->setLineWidth(6.5);
            d->setFrameColor(rgb(color));
            d->drawGraphicsPath(arc, CDrawContext::kPathStroked);
            d->setLineStyle(kLineSolid);
        };
        drawArc(1, railColor);
        drawArc(mix, purple);
        double angle = pi * .75 + pi * 1.5 * mix;
        circle(d, knobX + knobRadius * .84 * std::cos(angle),
               knobY() + knobRadius * .84 * std::sin(angle), 3.6, 0xF1EFE9);
        if (drag == Mix) {
            char value[24];
            std::snprintf(value, sizeof value, "%.0f%%", p[Mix]);
            label(d, knobX - 31, knobY() - 13, 62, 26, value, 15, text, true);
        }
        label(d, knobX - 45, by + 129, 90, 24, "MIX", 15, text, true);
        button(d, 168, by + 7, 48, 28, lengths[length]);
        box(d, hx - 18, by + 7, 36, 28,
            !sc               ? background
            : p[Listen] >= .5 ? selectedBackground
                              : 0x3B3B3B);
        unsigned icon = !sc ? disabled : p[Listen] >= .5 ? purple : text;
        for (int i = 0; i < 32; ++i) {
            double a = pi * i / 32, b = pi * (i + 1) / 32;
            line(d, hx + 7 * std::cos(a), hy - 7 * std::sin(a), hx + 7 * std::cos(b),
                 hy - 7 * std::sin(b), icon, 2);
        }
        box(d, hx - 8.5, hy - 1, 4, 8, icon, 1.5);
        box(d, hx + 4.5, hy - 1, 4, 8, icon, 1.5);
        label(d, fx, by + 9, 120, 28, "FILTER", 15, sc ? text : disabled);
        double hp = normalize(HighPass, p[HighPass]), lp = normalize(LowPass, p[LowPass]);
        box(d, fx, rail - 4, fw, 8, railColor, 4);
        box(d, fx + hp * fw, rail - 4, std::max(2., (lp - hp) * fw), 8, sc ? purple : 0x4B4B4B, 4);
        circle(d, fx + hp * fw, rail, 8.8, sc ? purple : disabled);
        circle(d, fx + lp * fw, rail, 8.8, sc ? purple : disabled);
        circle(d, fx + hp * fw, rail, 3.1, background);
        circle(d, fx + lp * fw, rail, 3.1, background);
        label(d, right, by + 9, 188, 28, "TRIGGER", 15, sc ? text : disabled);
        box(d, right, rail - 4, 188, 8, railColor, 4);
        double reduction = sc ? clamp(1 - s.gain, 0, 1) : 0;
        if (reduction > .001)
            box(d, right, rail - 4, std::max(5., 188 * reduction), 8, 0xB3A04D, 4);
        fader(d, fx, by + 93, fw, "THRESHOLD", p[Threshold], -60, 0, sc, "dB");
        fader(d, right, by + 93, 188, "RETRIGGER", p[Retrigger], 1, 250, sc, "ms");
        setDirty(false);
    }
    bool hit(const CPoint &q, double x, double y, double w, double h) {
        return q.x >= x && q.x < x + w && q.y >= y && q.y < y + h;
    }
    void beginGesture(std::initializer_list<int> ids) {
        endGesture();
        for (int i : ids) {
            gestures.push_back(i);
            owner->beginEdit(i);
        }
    }
    void endGesture() {
        for (int i : gestures)
            owner->endEdit(i);
        gestures.clear();
        drag = -1;
    }
    void change(int id, double v) { owner->edit(id, v, false); }
    void movePoint(int index, double t, double v) {
        apply(movePlotPoint(owner->values(), index, t, v));
    }
    CMouseEventResult onMouseDown(CPoint &where, const CButtonState &buttons) override {
        if (!buttons.isLeftButton())
            return kMouseEventNotHandled;
        layout();
        CPoint q(where.x / scale, where.y / scale);
        auto p = owner->values();
        points(p);
        if (hit(q, 20, 18, buttonWidth, 30)) {
            owner->edit(Mode, 0);
            owner->edit(Listen, 0);
            return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
        }
        if (hit(q, 116, 18, buttonWidth, 30)) {
            owner->edit(Mode, 1);
            return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
        }
        if (hit(q, uw - 204, 18, buttonWidth, 30)) {
            owner->edit(Invert, p[Invert] >= .5 ? 0 : 1);
            return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
        }
        if (hit(q, uw - 108, 18, buttonWidth, 30)) {
            owner->reset();
            return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
        }
        if (hit(q, 168, by + 7, 48, 28)) {
            auto menu = owned(new COptionMenu());
            for (auto value : lengths)
                menu->addEntry(value);
            menu->setValue(float(p[Length]));
            auto keep = SharedPointer<Canvas>(this);
            menu->popup(getFrame(), CPoint(168 * scale, (by + 35) * scale), [keep](COptionMenu *m) {
                int value = m->getCurrentIndex();
                if (value >= 0 && value < 8)
                    keep->owner->edit(Length, value);
            });
            return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
        }
        if (p[Mode] >= .5 && hit(q, hx - 18, by + 7, 36, 28)) {
            owner->edit(Listen, p[Listen] >= .5 ? 0 : 1);
            return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
        }
        int point = -1;
        double distance = 196;
        for (int i = 0; i < 5; ++i) {
            double dx = q.x - (pl + xs[i] * pw), dy = q.y - (pt + ph * (1 - ys[i]));
            double d = dx * dx + dy * dy;
            if (d < distance) {
                distance = d;
                point = i;
            }
        }
        if (point >= 0) {
            beginGesture(
                {FirstX, FirstY, AnchorAX, AnchorAY, Bend, AnchorBX, AnchorBY, LastX, LastY});
            if (buttons.isDoubleClick()) {
                apply(snapPlotPoint(p, point, plotGridStep(p, owner->snapshot.denominator, pw)));
                endGesture();
                return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
            }
            drag = 30 + point;
            return kMouseEventHandled;
        }
        double t = (q.x - pl) / pw;
        bool head = t > xs[0] && t<xs[1], tail = t> xs[3] && t < xs[4];
        if ((head || tail) && std::abs(q.y - (pt + ph * (1 - p.curve(t)))) <= 10) {
            if (tail)
                beginGesture({AnchorBY, LastY});
            else
                beginGesture({FirstY, AnchorAY});
            drag = tail ? 29 : 28;
            regionDownY = q.y;
            regionStart = p;
            return kMouseEventHandled;
        }
        if (hit(q, pl + xs[1] * pw, pt, (xs[3] - xs[1]) * pw, ph)) {
            beginGesture({AnchorAX, AnchorBX});
            drag = 27;
            regionDownX = q.x;
            regionStart = p;
            plotHover = true;
            invalid();
            return kMouseEventHandled;
        }
        if (knobHit(q)) {
            beginGesture({Mix});
            drag = Mix;
            if (buttons.isDoubleClick())
                change(Mix, 100);
            downY = q.y;
            downValue = owner->values()[Mix];
            invalid();
            return kMouseEventHandled;
        }
        if (p[Mode] >= .5 && hit(q, fx - 13, rail - 17, fw + 26, 34)) {
            double hp = fx + normalize(HighPass, p[HighPass]) * fw,
                   lp = fx + normalize(LowPass, p[LowPass]) * fw;
            beginGesture({HighPass, LowPass});
            if (q.x > hp + 12 && q.x < lp - 12) {
                drag = 26;
                bandX = q.x;
                bandHP = p[HighPass];
                bandLP = p[LowPass];
            } else
                drag = std::abs(q.x - hp) <= std::abs(q.x - lp) ? HighPass : LowPass;
            return kMouseEventHandled;
        }
        if (p[Mode] >= .5 && hit(q, fx - 6, by + 117, fw + 12, 24)) {
            beginGesture({Threshold});
            drag = Threshold;
            onMouseMoved(where, buttons);
            return kMouseEventHandled;
        }
        if (p[Mode] >= .5 && hit(q, right - 6, by + 117, 200, 24)) {
            beginGesture({Retrigger});
            drag = Retrigger;
            onMouseMoved(where, buttons);
            return kMouseEventHandled;
        }
        return kMouseEventNotHandled;
    }
    CMouseEventResult onMouseMoved(CPoint &where, const CButtonState &buttons) override {
        layout();
        CPoint q(where.x / scale, where.y / scale);
        auto p = owner->values();
        points(p);
        bool hover = hit(q, pl + xs[1] * pw, pt, (xs[3] - xs[1]) * pw, ph);
        if (hover != plotHover) {
            plotHover = hover;
            invalid();
        }
        if (drag < 0)
            return kMouseEventNotHandled;
        if (drag >= 30 && drag <= 34)
            movePoint(drag - 30, clamp((q.x - pl) / pw, 0, 1), clamp(1 - (q.y - pt) / ph, 0, 1));
        else if (drag == 27)
            apply(slidePlotTransition(regionStart, (q.x - regionDownX) / pw));
        else if (drag == 28 || drag == 29)
            apply(liftPlotSection(regionStart, drag == 29, (regionDownY - q.y) / ph));
        else if (drag == Mix)
            change(Mix,
                   clamp(downValue + (downY - q.y) * ((buttons & kShift) ? .0704 : .44), 0, 100));
        else if (p[Mode] >= .5) {
            if (drag == HighPass || drag == LowPass) {
                double freq = 10 * std::pow(2000, clamp((q.x - fx) / fw, 0, 1));
                if (drag == HighPass)
                    change(HighPass, std::min(p[LowPass] / 1.02, std::max(10., freq)));
                else
                    change(LowPass, std::max(p[HighPass] * 1.02, std::min(20000., freq)));
            }
            if (drag == 26) {
                double shift = clamp((q.x - bandX) / fw * std::log(2000), std::log(10 / bandHP),
                                     std::log(20000 / bandLP));
                change(HighPass, bandHP * std::exp(shift));
                change(LowPass, bandLP * std::exp(shift));
            }
            if (drag == Threshold)
                change(Threshold, -60 + 60 * clamp((q.x - fx) / fw, 0, 1));
            if (drag == Retrigger)
                change(Retrigger, 1 + 249 * clamp((q.x - right) / 188, 0, 1));
        }
        invalid();
        return kMouseEventHandled;
    }
    CMouseEventResult onMouseUp(CPoint &, const CButtonState &) override {
        endGesture();
        invalid();
        return kMouseEventHandled;
    }
    CMouseEventResult onMouseCancel() override {
        endGesture();
        invalid();
        return kMouseEventHandled;
    }
    CMouseEventResult onMouseExited(CPoint &, const CButtonState &) override {
        plotHover = false;
        invalid();
        return kMouseEventHandled;
    }
    void onMouseWheelEvent(MouseWheelEvent &event) override {
        layout();
        CPoint q(event.mousePosition.x / scale, event.mousePosition.y / scale);
        if (knobHit(q)) {
            auto p = owner->values();
            owner->edit(Mix, clamp(p[Mix] + event.deltaY * 5, 0, 100));
            event.consumed = true;
            invalid();
        }
    }
};
Editor::Editor(Controller *c) : VSTGUIEditor(c), owner(c) {
    Steinberg::ViewRect r(0, 0, c->width, c->height);
    setRect(r);
    setIdleRate(16);
}
bool PLUGIN_API Editor::open(void *parent, const PlatformType &type) {
    if (frame)
        return false;
    frame = new CFrame(CRect(0, 0, owner->width, owner->height), this);
    canvas = new Canvas(CRect(0, 0, owner->width, owner->height), owner);
    frame->addView(canvas);
    if (!frame->open(parent, type)) {
        frame->forget();
        frame = nullptr;
        canvas = nullptr;
        return false;
    }
    if (dpi != 1)
        frame->setZoom(dpi);
    owner->requestDisplay(true);
    return true;
}
void PLUGIN_API Editor::close() {
    if (!frame)
        return;
    owner->edit(Listen, 0);
    owner->requestDisplay(false);
    if (pendingSize) {
        saveWindowSize(owner->width, owner->height);
        pendingSize = false;
    }
    frame->close();
    frame = nullptr;
    canvas = nullptr;
}
Steinberg::tresult PLUGIN_API Editor::checkSizeConstraint(Steinberg::ViewRect *r) {
    if (!r)
        return Steinberg::kInvalidArgument;
    r->right = r->left + std::clamp(r->right - r->left, int(640 * dpi), int(2400 * dpi));
    r->bottom = r->top + std::clamp(r->bottom - r->top, int(460 * dpi), int(1800 * dpi));
    return Steinberg::kResultTrue;
}
Steinberg::tresult PLUGIN_API Editor::onSize(Steinberg::ViewRect *r) {
    if (!r)
        return Steinberg::kInvalidArgument;
    hostResize = true;
    auto result = VSTGUIEditor::onSize(r);
    hostResize = false;
    owner->width = int(std::round((r->right - r->left) / dpi));
    owner->height = int(std::round((r->bottom - r->top) / dpi));
    if (canvas) {
        CRect bounds(0, 0, owner->width, owner->height);
        canvas->setViewSize(bounds);
        canvas->setMouseableArea(bounds);
        canvas->invalid();
    }
    pendingSize = true;
    resized = std::chrono::steady_clock::now();
    return result;
}
Steinberg::tresult PLUGIN_API Editor::queryInterface(const Steinberg::TUID iid, void **out) {
    if (Steinberg::FUnknownPrivate::iidEqual(iid, Steinberg::IPlugViewContentScaleSupport::iid)) {
        *out = static_cast<Steinberg::IPlugViewContentScaleSupport *>(this);
        addRef();
        return Steinberg::kResultOk;
    }
    return VSTGUIEditor::queryInterface(iid, out);
}
Steinberg::tresult PLUGIN_API Editor::setContentScaleFactor(ScaleFactor factor) {
    if (!std::isfinite(factor) || factor <= 0)
        return Steinberg::kInvalidArgument;
#if SMTG_OS_MACOS
    dpi = 1; // NSView/CoreGraphics already use logical points on Retina displays.
#else
    dpi = factor;
#endif
    if (frame)
        frame->setZoom(dpi);
    else {
        Steinberg::ViewRect r(0, 0, int(owner->width * dpi), int(owner->height * dpi));
        setRect(r);
    }
    return Steinberg::kResultOk;
}
bool Editor::beforeSizeChange(const CRect &next, const CRect &) {
    if (hostResize || !plugFrame)
        return true;
    Steinberg::ViewRect r(0, 0, int(next.getWidth()), int(next.getHeight()));
    return plugFrame->resizeView(this, &r) == Steinberg::kResultTrue;
}
CMessageResult Editor::notify(CBaseObject *sender, const char *message) {
    if (message == CVSTGUITimer::kMsgTimer && frame) {
        owner->requestDisplay(true);
        if (canvas)
            canvas->invalid();
        if (pendingSize &&
            std::chrono::steady_clock::now() - resized > std::chrono::milliseconds(250)) {
            saveWindowSize(owner->width, owner->height);
            pendingSize = false;
        }
    }
    return VSTGUIEditor::notify(sender, message);
}
} // namespace SideKick

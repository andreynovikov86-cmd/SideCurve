#pragma once
#include "Plugin.h"
#include "public.sdk/source/vst/vstguieditor.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include <chrono>
namespace SideKick {
class Canvas;
class Editor : public Steinberg::Vst::VSTGUIEditor, public Steinberg::IPlugViewContentScaleSupport {
  public:
    explicit Editor(Controller *);
    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID, void **) override;
    Steinberg::uint32 PLUGIN_API addRef() override { return VSTGUIEditor::addRef(); }
    Steinberg::uint32 PLUGIN_API release() override { return VSTGUIEditor::release(); }
    Steinberg::tresult PLUGIN_API setContentScaleFactor(ScaleFactor) override;
    bool beforeSizeChange(const VSTGUI::CRect &, const VSTGUI::CRect &) override;
    bool PLUGIN_API open(void *, const VSTGUI::PlatformType &) override;
    void PLUGIN_API close() override;
    Steinberg::tresult PLUGIN_API canResize() override { return Steinberg::kResultTrue; }
    Steinberg::tresult PLUGIN_API checkSizeConstraint(Steinberg::ViewRect *) override;
    Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect *) override;
    VSTGUI::CMessageResult notify(VSTGUI::CBaseObject *, const char *) override;

  private:
    Controller *owner;
    Canvas *canvas = nullptr;
    bool pendingSize = false, hostResize = false;
    double dpi = 1;
    std::chrono::steady_clock::time_point resized{};
};
} // namespace SideKick

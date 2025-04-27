
#pragma once
#include <ul/menu/ui/ui_RawRgbaImage.hpp>
#include <ul/smi/smi_Protocol.hpp>

namespace ul::menu::ui {

    void InitializeScreenCaptures(const smi::MenuStartMode start_mode);

    ul::menu::ui::RawRgbaImage::Ref GetScreenCaptureBackground();
    bool HasScreenCaptureBackground();

    void RequestResumeScreenCaptureBackground();
    void RequestHideScreenCaptureBackground();
    void RequestHideLoseFocusScreenCaptureBackground();
    void RequestShowGainFocusScreenCaptureBackground();
    bool IsScreenCaptureBackgroundNotTransitioning();
    bool IsScreenCaptureBackgroundFocused();

    void UpdateScreenCaptureBackground(const bool after_start_stay_at_min_alpha);

}
